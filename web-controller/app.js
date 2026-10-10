import { parseLine, profileCommand, pixelIndex, artChunks, importDrawing, compareVersions } from './protocol.mjs';
import { DiagnosticLog, buildReport, safeFields } from './diagnostics.mjs';
const $ = id => document.getElementById(id);
let storage; try { storage=localStorage; } catch {}
const diagnostics=new DiagnosticLog({storage}), snapshots={}, testResults=[];
let latestIdentity, usbInfo, diagnosticFlushTimer, serviceBusy=false, inputTimer;
function renderDiagnostics(){
  const filter=$('log-filter').value;
  const entries=diagnostics.entries.filter(e=>filter==='all' || (filter==='error' ? e.level==='error' : e.level!=='info'));
  const lines=entries.map(e=>`${e.at} ${e.level.toUpperCase()} ${e.kind} ${e.text}`);
  $('service-log').textContent=lines.slice(-250).join('\n');$('log').textContent=lines.slice(-80).join('\n');
  $('log-health').textContent=`${diagnostics.entries.length}/${diagnostics.maxEntries} events retained · ${diagnostics.dropped} older events dropped · storage: ${diagnostics.persistence}. Screen shows the latest 250 matching events; downloads include all retained events.`;
}
let port, reader, writer, readTask, identity, pending, queue = Promise.resolve(), closing = false, connecting = false, transferring = false;
let pollTimer, handshakeTimer, releaseCatalog;
function message(text,level='info') { $('message').textContent = text; log(text,level,'UI'); }
function log(text,level='info',kind='RX') {
  diagnostics.add(level,kind,text);renderDiagnostics();
  if(!diagnosticFlushTimer)diagnosticFlushTimer=setTimeout(()=>{diagnosticFlushTimer=null;diagnostics.flush();renderDiagnostics();},800);
}
function controls() {
  document.querySelectorAll('[data-device]').forEach(el => el.disabled = !identity || transferring || serviceBusy);
  $('connect').disabled = connecting || closing || !('serial' in navigator);
  $('connect').hidden = Boolean(identity);
  $('disconnect').hidden = !port;
  $('disconnect').disabled = transferring || closing;
  $('connection').textContent = identity ? 'Jar connected' : connecting ? 'Connecting…' : 'Jar disconnected';
}
function rejectPending(error) { if (pending) { clearTimeout(pending.timer); const p = pending; pending = null; log(`${p.command}: ${error.message}`,'error','REQUEST');p.reject(error); } }
function receive(line) {
  const parsed = parseLine(line);
  if (!parsed) { if(line)log(line,/Guru Meditation|panic|assert failed|abort\(|Brownout|backtrace|stack smashing/i.test(line)?'error':'info','DEVICE');return; }
  const { type, data } = parsed;
  snapshots[type]={at:new Date().toISOString(),fields:{...data}};
  if(type==='OLED')snapshots[type].fields={...data,data:'[frame payload omitted]'};
  log(line,type==='ERR'?'error':'info','RX');
  updateReadouts();
  if (pending && (type === 'ERR' || pending.match(parsed))) {
    const p = pending; pending = null; clearTimeout(p.timer);
    type === 'ERR' ? p.reject(new Error(data.message || 'Jar rejected the command.')) : p.resolve(parsed);
  }
  if (type === 'IDENTITY') {
    identity = data;latestIdentity={...data}; controls(); $('identity').textContent = `${data.serial || 'HAPPY JARZ'} · ${data.hw || '?'} · firmware ${data.fw || '?'}`;
    updateReleaseStatus();
  }
  if (type === 'PROFILE') {
    $('owner').value = data.owner || '';
    const parts = /^\d{2}-\d{2}$/.test(data.birthday || '') ? data.birthday.split('-') : ['', ''];
    $('birth-month').value = parts[0]; $('birth-day').value = parts[1];
  }
  if (type === 'STATUS') {
    if ([...$('pattern').options].some(o => o.value === data.pattern)) $('pattern').value = data.pattern;
    if (Number.isFinite(Number(data.brightness))) { $('brightness').value = data.brightness; $('brightness-label').textContent = `${data.brightness}%`; }
    for (let i = 1; i <= 4; i++) {
      const rgb = data[`led${i}`]?.split(',').map(Number);
      if (rgb?.length === 3 && rgb.every(n => Number.isInteger(n) && n >= 0 && n <= 255)) $('color'+i).value = '#'+rgb.map(n=>n.toString(16).padStart(2,'0')).join('');
    }
  }
  if (type === 'POWER') $('power').textContent = Number(data.percent) >= 0 ? `Battery ${data.percent}% · ${data.voltage || '—'} V` : 'Battery reading unavailable';
  if(type==='WIFI')$('wifi-state').textContent=`${data.state} · ${data.ssid || 'No network'} · signal ${data.rssi || '—'} dBm`;
  if(type==='TIME'){$('time-state').textContent=`${data.local || 'Clock not synced'} · ${data.timezone || '—'}`;if([...$('timezone').options].some(o=>o.value===data.timezone))$('timezone').value=data.timezone;}
  if(type==='ALARM' || type==='TIMER')$('schedule-state').textContent=type+': '+Object.entries(data).map(([k,v])=>`${k}=${v}`).join(' · ');
  if(type==='SAVER')$('saver-state').textContent=Object.entries(data).map(([k,v])=>`${k}=${v}`).join(' · ');
  if (type === 'OLED' && data.codec === 'b64v1') {
    try {
      const bytes = Uint8Array.from(atob(data.data), c => c.charCodeAt(0));
      if (bytes.length !== 1024) {log('OLED frame has incorrect byte count','warn','PROTOCOL');return;}
      // U8g2 live stream uses vertical pages; art upload uses row-major XBM.
      const ctx = $('oled').getContext('2d'), image = ctx.createImageData(128,64);
      for (let y=0;y<64;y++) for (let x=0;x<128;x++) {
        const lit = bytes[(y>>3)*128+x] & (1<<(y&7));
        const i=(y*128+x)*4; image.data.set(lit ? [168,231,214,255] : [6,11,21,255],i);
      }
      ctx.putImageData(image,0,0);
    } catch { log('Ignored malformed screen frame.','warn','PROTOCOL'); }
  }
}
async function readLoop() {
  const decoder = new TextDecoder(); let buffer = '';
  try {
    while (!closing) {
      const { value, done } = await reader.read(); if (done) break;
      buffer += decoder.decode(value, {stream:true});
      let end;
      while ((end = buffer.indexOf('\n')) >= 0) { receive(buffer.slice(0,end).replace(/\r$/, '')); buffer=buffer.slice(end+1); }
      if (buffer.length > 16384) throw new Error('Serial response exceeded the limit.');
    }
  } catch (error) { if (!closing) message(`USB connection lost: ${error.message}`,'error'); }
  finally {
    reader?.releaseLock(); reader = null;
    if (!closing) {log('USB read stream ended unexpectedly.','error','USB'); rejectPending(new Error('USB connection closed.')); identity = null; controls(); void disconnect(); }
  }
}
async function write(command) {
  if (!writer || closing) throw new Error('Connect your jar first.');
  if (/[\r\n]/.test(command)) throw new Error('Command must be a single line.');
  log(command,'info','TX');
  await writer.write(new TextEncoder().encode(command+'\n'));
}
function request(command, match, timeout=5000) {
  const work = queue.then(async () => {
    if (!identity && command !== 'HELLO') throw new Error('Connect your jar first.');
    return new Promise((resolve,reject) => {
      const timer = setTimeout(()=>rejectPending(new Error('No response from jar: '+command.split(' ').slice(0,2).join(' '))),timeout);
      pending={resolve,reject,match,timer,command};
      write(command).catch(rejectPending);
    });
  });
  queue=work.catch(()=>{}); return work;
}
function ack(command, response=command) { return request(command,p=>p.type==='ACK' && p.data.command===response); }
function query(command,type) { return request(command,p=>p.type===type); }
function updateReadouts(){
  const root=$('service-readouts');root.replaceChildren();
  for(const type of ['IDENTITY','CAPS','POWER','STATUS','LED_FRAME','OLED','WIFI','TIME','ALARM','TIMER','DISPLAY','INPUT','TOUCH_CAL','SAVER','ACCESSORY','SLEEP','ART']){
    const card=document.createElement('section'),heading=document.createElement('h3'),body=document.createElement('pre'),stamp=document.createElement('small');
    heading.textContent=type;const reading=snapshots[type];
    body.textContent=reading ? Object.entries(safeFields(reading.fields)).map(([k,v])=>`${k}: ${v}`).join('\n') : 'No reading yet';
    stamp.textContent=reading ? `Received ${reading.at}${identity ? '' : ' · disconnected; last known reading'}` : 'Refresh to read';
    card.append(heading,body,stamp);root.append(card);
  }
}
async function serviceTest(label,command,match,timeout=10000){
  const start=performance.now();serviceBusy=true;controls();
  try {const response=await request(command,match,timeout);const detail=JSON.stringify(safeFields(response.data));
    testResults.push({at:new Date().toISOString(),board:identity?.serial,label,command,elapsedMs:Math.round(performance.now()-start),result:'response received',detail});
    log(`${label}: firmware response ${detail}. Physical result requires observation.`,'info','TEST');message(`${label} completed. Check the physical result and record your observations.`);
  }catch(error){testResults.push({at:new Date().toISOString(),board:latestIdentity?.serial,label,command,elapsedMs:Math.round(performance.now()-start),result:'failed',detail:error.message});log(`${label}: ${error.message}`,'error','TEST');throw error;}
  finally{if(testResults.length>100)testResults.shift();$('service-tests').textContent=testResults.map(t=>`${t.at} · ${t.label}: ${t.result} (${t.elapsedMs} ms) · ${t.detail}`).join('\n');serviceBusy=false;controls();}
}
async function connect() {
  connecting=true; controls(); message('Select your jar. Keep the Pi controller closed.');
  try {
    port=await navigator.serial.requestPort();
    for(const key of Object.keys(snapshots))delete snapshots[key];updateReadouts();
    usbInfo=port.getInfo?.() || {};log(JSON.stringify(usbInfo),'info','USB');
    await port.open({baudRate:115200});
    await port.setSignals({dataTerminalReady:false,requestToSend:false});
    writer=port.writable.getWriter(); reader=port.readable.getReader(); closing=false; readTask=readLoop();
    // Opening may reset the chip; retry HELLO through touch calibration.
    const hello=request('HELLO',p=>p.type==='IDENTITY',8000);
    handshakeTimer=setInterval(()=>write('HELLO').catch(()=>{}),300);
    try { await hello; } finally { clearInterval(handshakeTimer); }
    for (const [command,type] of [['GET PROFILE','PROFILE'],['GET STATUS','STATUS'],['GET POWER','POWER']]) await query(command,type);
    await query('GET TIME STATUS','TIME');
    await ack('STREAM OLED ON');
    pollTimer=setInterval(()=>{if(identity && !transferring && !serviceBusy) query('GET POWER','POWER').catch(error=>message(error.message,'error'));},10000);
    message('Connected. Your jar is ready.');
  } catch(error) { message(error.name==='NotFoundError' ? 'No jar selected.' : `Could not connect: ${error.message}`,error.name==='NotFoundError'?'warn':'error'); await disconnect(); }
  finally { connecting=false; controls(); }
}
async function disconnect() {
  if (closing) return;
  closing=true; clearInterval(pollTimer); clearInterval(handshakeTimer);clearInterval(inputTimer);$('live-input').checked=false;log('Disconnecting serial session.','info','USB'); rejectPending(new Error('Jar disconnected.'));
  identity=null; controls();
  try { await reader?.cancel(); await readTask; } catch(error){log(error.message,'warn','USB-CLEANUP');}
  try { writer?.releaseLock(); } catch(error){log(error.message,'warn','USB-CLEANUP');} writer=null;
  try { await port?.close(); } catch(error){log(error.message,'warn','USB-CLEANUP');} port=null; readTask=null; closing=false; controls();
}
function action(fn) { return async event => { event?.preventDefault(); try { await fn(); } catch(error) { message(error.message,'error'); } }; }
$('connect').onclick=connect; $('disconnect').onclick=()=>void disconnect();
document.querySelectorAll('[data-tab]').forEach(button=>button.onclick=()=>{
  document.querySelectorAll('.panel').forEach(p=>p.hidden=p.id!==button.dataset.tab);
  document.querySelectorAll('[data-tab]').forEach(b=>b.setAttribute('aria-selected',String(b===button)));
});
document.querySelectorAll('[data-command]').forEach(button=>button.onclick=action(async()=>{
  const command=button.dataset.command;
  if(command==='GET PROFILE') await query(command,'PROFILE'); else await ack(command,command.startsWith('KEY ') ? 'KEY' : command.startsWith('SET INPUT MODE ') ? 'SET INPUT MODE' : command);
  message('Jar acknowledged: '+command);
}));
const patterns=['SOLID','FADE','PULSE','RAINBOW','RANDOM','HUE_FADE','DUAL_HUE','BREATH','DRIFT','AURORA','OCEAN','LAVENDER','SUNSET','CHRISTMAS','HALLOWEEN','VALENTINE','EASTER','FOURTH','THANKSGIVING','CANDY','GALAXY','FIRE','ICE','FOREST','NEON','TWINKLE','SPARKLE','COLOR_SWAP','COMET','FIREFLY','BUBBLEGUM','CHASE_CW','CHASE_CCW','JAR_CHASE','SIDE_CHASE','SWEEP_LR','SWEEP_TS','DIAGONAL','PING_PONG','DUAL_CHASE','OPP_CHASE','JAR_PULSE','SIDE_ACCENT','WATERFALL','RIPPLE','TIDAL','MOONWAVE','DEEPSEA','LAGOON','ARCTIC','GLACIER','EMBER','CAMPFIRE','SUNRISE','SOLAR','INFERNO','CANDLE','AUTUMN','HARVEST','MINT','MEADOW','JUNGLE','MOSS','CLOVER','SPRING','RAINFOREST','LIMEADE','DREAM','COTTON_CANDY','MERMAID','UNICORN','VAPOR','COSMIC','STARDUST','NEBULA','ROSE_GOLD','PEACH','CORAL','CHERRY','BERRY','PLUM','GRAPE','MAGENTA','ELECTRIC','CYBER','LASER','SYNTH','ARCADE','MATRIX','PLASMA','HOLOGRAM','RAIN','THUNDER','LIGHTNING','CLOUD','SNOW','FROST','SUNBEAM','MOONLIGHT','OFF'];
for (const pattern of patterns) $('pattern').add(new Option(pattern.replaceAll('_',' '),pattern));
for (let i=1;i<=4;i++) {const label=document.createElement('label'); label.textContent='Bulb '+i; const input=document.createElement('input');input.type='color';input.id='color'+i;input.value=i%2 ? '#eaa2c8' : '#a8e7d6';label.append(input);$('colors').append(label);}
$('apply-pattern').onclick=action(async()=>{await ack('SET PATTERN '+$('pattern').value,'SET PATTERN');message('Pattern applied.');});
$('brightness').oninput=()=>{$('brightness-label').textContent=$('brightness').value+'%';};
$('apply-brightness').onclick=action(async()=>{await ack('SET BRIGHTNESS '+$('brightness').value,'SET BRIGHTNESS');message('Brightness applied.');});
$('apply-colors').onclick=action(async()=>{for(let i=1;i<=4;i++){const hex=$('color'+i).value;const rgb=[1,3,5].map(j=>parseInt(hex.slice(j,j+2),16));await ack(`SET LED${i} COLOR ${rgb.join(' ')}`,`SET LED${i} COLOR`);}message('Colors applied. Select SOLID to display them continuously.');});
for(let m=1;m<=12;m++) $('birth-month').add(new Option(new Date(2000,m-1,1).toLocaleString('en',{month:'long'}),String(m).padStart(2,'0')));
async function syncClock() {await ack('SET TIMEZONE '+$('timezone').value,'SET TIMEZONE');await ack('SET CLOCK UNIX '+Math.floor(Date.now()/1000),'SET CLOCK UNIX');await ack('SAVE');}
$('sync-clock').onclick=action(async()=>{await syncClock();message('Clock synced and time zone saved.');});
$('profile-form').onsubmit=action(async()=>{const cmd=profileCommand($('owner').value,$('birth-month').value,$('birth-day').value);await ack(cmd,'SET PROFILE');await syncClock();message('Profile saved on your jar and clock synced.');});
let frames=[new Uint8Array(1024),new Uint8Array(1024)], frame=0, previewTimer, painting=false;
function renderArt() {const ctx=$('art-canvas').getContext('2d'), image=ctx.createImageData(128,64);for(let y=0;y<64;y++)for(let x=0;x<128;x++){const [byte,bit]=pixelIndex(x,y);image.data.set(frames[frame][byte]&bit ? [168,231,214,255] : [6,11,21,255],(y*128+x)*4);}ctx.putImageData(image,0,0);$('frame0').setAttribute('aria-pressed',String(frame===0));$('frame1').setAttribute('aria-pressed',String(frame===1));}
function stopPreview(){clearInterval(previewTimer);previewTimer=null;$('preview').textContent='Play preview';}
function paint(event){const box=$('art-canvas').getBoundingClientRect();const x=Math.floor((event.clientX-box.left)/box.width*128), y=Math.floor((event.clientY-box.top)/box.height*64);if(x<0||y<0||x>=128||y>=64)return;const [byte,bit]=pixelIndex(x,y);$('erase').checked ? frames[frame][byte]&=~bit : frames[frame][byte]|=bit;renderArt();}
$('art-canvas').onpointerdown=event=>{stopPreview();painting=true;$('art-canvas').setPointerCapture(event.pointerId);paint(event);};
$('art-canvas').onpointermove=event=>{if(painting)paint(event);};
$('art-canvas').onpointerup=$('art-canvas').onpointercancel=()=>{painting=false;};
for(let i=0;i<2;i++) $('frame'+i).onclick=()=>{stopPreview();frame=i;renderArt();};
$('preview').onclick=()=>{if(previewTimer)stopPreview();else{$('preview').textContent='Stop preview';previewTimer=setInterval(()=>{frame=1-frame;renderArt();},400);}};
$('copy-frame').onclick=()=>{stopPreview();frames[1-frame]=frames[frame].slice();message('Copied to the other frame.');};
$('clear-frame').onclick=()=>{stopPreview();frames[frame].fill(0);renderArt();};
$('export-art').onclick=()=>{const blob=new Blob([JSON.stringify({format:'happyjarz-xbm-v1',frames:frames.map(f=>[...f])})],{type:'application/json'});const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download='happyjarz-drawing.json';a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);};
$('import-art').onchange=action(async()=>{const file=$('import-art').files[0];if(!file)return;if(file.size>20000)throw new Error('Drawing file is too large.');frames=importDrawing(JSON.parse(await file.text()));stopPreview();renderArt();message('Drawing loaded.');});
$('upload-art').onclick=action(async()=>{
  const slot=Number($('art-slot').value)-1, commands=artChunks(frames.map(f=>f.slice()),slot);
  if(!confirm(`Replace art slot ${slot+1} on the jar with these two frames?`)) return;
  transferring=true;controls();$('art-progress').hidden=false;$('art-progress').value=0;
  try {
    const ready=await request('ART READY',p=>p.type==='ART' && p.data.event==='READY');
    if(ready.data.ready!=='1')throw new Error('Art storage is unavailable. This draft will not format it.');
    for(const entry of commands){await request(entry.command,p=>p.type==='ART' && p.data.event===entry.event && Number(p.data.slot)===entry.slot && (entry.offset===undefined || Number(p.data.offset)===entry.offset));if(entry.offset)$('art-progress').value=entry.offset;}
    message(`Two frames saved in jar slot ${slot+1}. Choose the Art Gallery screensaver on the jar.`);
  } finally {transferring=false;controls();}
});
async function checkUpdates(){
  $('release-status').textContent='Checking release information…';
  try {const response=await fetch('./releases.json',{cache:'no-store',signal:AbortSignal.timeout(10000)});if(!response.ok)throw new Error('Release information is unavailable.');const catalog=await response.json();if(catalog.schema!==1 || !Array.isArray(catalog.releases))throw new Error('Invalid release information.');releaseCatalog=catalog;updateReleaseStatus();}
  catch(error){$('release-status').textContent=error.message+' Your jar can still work offline.';log(error.message,'error','RELEASE-FETCH');}
}
function updateReleaseStatus(){if(!releaseCatalog)return;const stable=releaseCatalog.releases.filter(r=>r.channel==='stable' && r.model==='happyjarz-supermini-simple-4mb');if(!stable.length){$('release-status').textContent=`No tested firmware download has been published yet.${identity ? ' Connected firmware: '+identity.fw+'.' : ''}`;return;}try{stable.sort((a,b)=>compareVersions(b.version,a.version));$('release-status').textContent=`Latest listed release: ${stable[0].version}. Exact board compatibility must be verified before installation. Installer remains disabled.`;}catch{$('release-status').textContent='Release catalog contains an unsupported version. Installer remains disabled.';}}
$('check-updates').onclick=checkUpdates;
$('support').textContent='serial' in navigator ? 'USB browser support detected. Click Connect jar to begin.' : 'This browser does not expose Web Serial. Try a current desktop Chrome or Edge; other browser versions vary.';
document.querySelectorAll('[data-query]').forEach(button=>button.onclick=action(async()=>{await query(button.dataset.query,button.dataset.response);message('Readout refreshed.');}));
$('wifi-form').onsubmit=action(async()=>{
  const ssid=$('wifi-ssid').value,password=$('wifi-password').value;
  if(!ssid || !password || new TextEncoder().encode(ssid).length>32 || new TextEncoder().encode(password).length>63 || /[\r\n]/.test(ssid+password))throw new Error('Enter SSID and password, check their lengths and remove line breaks. Empty passwords are not supported by the current firmware command.');
  await ack('SET WIFI SSID '+ssid,'SET WIFI SSID');
  await ack('SET WIFI PASSWORD '+password,'SET WIFI PASSWORD');
  await ack('SAVE');await ack('WIFI CONNECT','WIFI CONNECT');$('wifi-password').value='';await query('GET WIFI STATUS','WIFI');message('Wi-Fi saved; status read from jar.');
});
$('set-alarm').onclick=action(async()=>{const time=$('alarm-time').value;if(!/^\d{2}:\d{2}$/.test(time))throw new Error('Choose an alarm time.');await ack('SET ALARM '+time,'SET ALARM');const command='SET ALARM '+($('alarm-enabled').checked?'ON':'OFF');await ack(command);await query('GET ALARM','ALARM');message('Alarm applied. Use Save settings to persist it.');});
$('set-timer').onclick=action(async()=>{const minutes=Number($('timer-minutes').value);if(!Number.isInteger(minutes)||minutes<1||minutes>10080)throw new Error('Timer must be 1–10080 minutes.');await ack('SET TIMER MINUTES '+minutes,'SET TIMER MINUTES');await query('GET TIMER','TIMER');message('Timer started.');});
$('set-saver').onclick=action(async()=>{await ack('SET SAVER MODE '+$('saver-mode').value,'SET SAVER MODE');message('Screensaver applied.');});
$('save-saying').onclick=action(async()=>{const slot=Number($('saying-slot').value),text=$('saying-text').value.trim();if(!Number.isInteger(slot)||slot<1||slot>50||!text||/[|\r\n]/.test(text)||new TextEncoder().encode(text).length>96)throw new Error('Choose slot 1–50 and a single line of up to 96 UTF-8 bytes without |.');await ack(`SET CUSTOM SAYING ${slot} ${text}`,'SET CUSTOM SAYING');message('Marquee saying saved.');});
$('set-saying-source').onclick=action(async()=>{await ack('SET SAYING SOURCE '+$('saying-source').value,'SET SAYING SOURCE');message('Saying source applied.');});
$('refresh-service').onclick=action(async()=>{
  serviceBusy=true;controls();let failed=0;
  try{for(const [command,type] of [['GET CAPS','CAPS'],['GET POWER','POWER'],['GET STATUS','STATUS'],['GET LED FRAME','LED_FRAME'],['GET OLED','OLED'],['GET WIFI STATUS','WIFI'],['GET TIME STATUS','TIME'],['GET ALARM','ALARM'],['GET TIMER','TIMER'],['GET DISPLAY','DISPLAY'],['GET INPUT','INPUT'],['GET SAVER STATUS','SAVER'],['ART READY','ART']]){
    if(!identity)break;try{await query(command,type);}catch(error){failed++;log(`${command}: ${error.message}`,'error','SERVICE');}
  }message(failed?`Readouts refreshed with ${failed} failed requests. See the log.`:'Readouts refreshed.',failed?'warn':'info');}
  finally{serviceBusy=false;controls();}
});
$('service-ping').onclick=action(()=>serviceTest('Ping','PING',p=>p.type==='PONG'));
$('service-rgb').onclick=action(async()=>{if(confirm('Run a brief red/green/blue/white lamp test?'))await serviceTest('RGB routine','TEST RGB',p=>p.type==='TEST' && p.data.rgb!==undefined);});
$('service-touch').onclick=action(async()=>{if(confirm('Keep fingers off all six pads. Recalibrate touch inputs now?'))await serviceTest('Touch calibration routine','TEST TOUCH',p=>p.type==='TEST' && p.data.input!==undefined);});
$('live-input').onchange=()=>{clearInterval(inputTimer);if($('live-input').checked){inputTimer=setInterval(()=>{if(identity&&!transferring&&!serviceBusy)query('GET INPUT','INPUT').catch(error=>message(error.message,'error'));},1000);}};
function downloadFile(name,contents,type){const url=URL.createObjectURL(new Blob([contents],{type})),a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
function environment(){return {userAgent:navigator.userAgent,language:navigator.language,languages:navigator.languages,timezone:Intl.DateTimeFormat().resolvedOptions().timeZone,timezoneOffsetMinutes:new Date().getTimezoneOffset(),secureContext:isSecureContext,webSerialAvailable:'serial' in navigator,online:navigator.onLine,origin:location.origin,viewport:{width:innerWidth,height:innerHeight},usb:usbInfo||null};}
$('download-report').onclick=action(()=>{
  diagnostics.flush();const report=buildReport({logger:diagnostics,environment:environment(),identity:latestIdentity,snapshots,tests:testResults,notes:$('support-notes').value,observations:{screen:$('observe-screen').value,lights:$('observe-lights').value,touch:$('observe-touch').value}});
  downloadFile('happyjarz-support-'+new Date().toISOString().replaceAll(':','-')+'.json',JSON.stringify(report,null,2),'application/json');message('Support report downloaded. You can review it and send it to us.');
});
$('download-log').onclick=()=>{diagnostics.flush();downloadFile('happyjarz-session-log.txt',`HAPPY JARZ Web Studio 0.2.0\nExported ${new Date().toISOString()}\nOlder events dropped: ${diagnostics.dropped}\n`+JSON.stringify(environment(),null,2)+'\n\n'+diagnostics.entries.map(e=>`${e.at} ${e.level.toUpperCase()} ${e.kind} ${e.text}`).join('\n'),'text/plain');};
$('clear-log').onclick=()=>{if(confirm('Delete retained diagnostic logs from this browser?')){diagnostics.clear();renderDiagnostics();}};
$('log-filter').onchange=renderDiagnostics;
window.addEventListener('error',event=>{const target=event.target;if(target!==window)log('Resource failed: '+target?.tagName,'error','RESOURCE');else log(`${event.message}\n${event.error?.stack||''}`,'error','JAVASCRIPT');},true);
window.addEventListener('unhandledrejection',event=>log(event.reason?.stack||event.reason?.message||String(event.reason),'error','UNHANDLED'));
window.addEventListener('pagehide',()=>diagnostics.flush());
for(const name of ['error','warn']){const original=console[name].bind(console);console[name]=(...args)=>{const text=args.map(a=>{if(typeof a==='string')return a;if(a?.stack)return a.stack;try{return JSON.stringify(a);}catch{return '[unserializable console value]';}}).join(' ');log(text,name==='error'?'error':'warn','CONSOLE');original(...args);};}
log(JSON.stringify(environment()),'info','SESSION');controls();renderArt();updateReadouts();void checkUpdates();
