import { parseLine, profileCommand, pixelIndex, artChunks, importDrawing, compareVersions } from './protocol.mjs';
const $ = id => document.getElementById(id);
let port, reader, writer, readTask, identity, pending, queue = Promise.resolve(), closing = false, connecting = false, transferring = false;
let pollTimer, handshakeTimer, releaseCatalog;
function message(text) { $('message').textContent = text; }
function log(text) { const el = $('log'); el.textContent = (el.textContent + text + '\n').split('\n').slice(-80).join('\n'); }
function controls() {
  document.querySelectorAll('[data-device]').forEach(el => el.disabled = !identity || transferring);
  $('connect').disabled = connecting || closing || !('serial' in navigator);
  $('connect').hidden = Boolean(identity);
  $('disconnect').hidden = !port;
  $('disconnect').disabled = transferring || closing;
  $('connection').textContent = identity ? 'Jar connected' : connecting ? 'Connecting…' : 'Jar disconnected';
}
function rejectPending(error) { if (pending) { clearTimeout(pending.timer); const p = pending; pending = null; p.reject(error); } }
function receive(line) {
  const parsed = parseLine(line);
  if (!parsed) return;
  const { type, data } = parsed;
  if (pending && (type === 'ERR' || pending.match(parsed))) {
    const p = pending; pending = null; clearTimeout(p.timer);
    type === 'ERR' ? p.reject(new Error(data.message || 'Jar rejected the command.')) : p.resolve(parsed);
  }
  if (type !== 'OLED' && !(type === 'ART' && data.event === 'DATA')) log(line);
  if (type === 'IDENTITY') {
    identity = data; controls(); $('identity').textContent = `${data.serial || 'HAPPY JARZ'} · ${data.hw || '?'} · firmware ${data.fw || '?'}`;
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
  if (type === 'OLED' && data.codec === 'b64v1') {
    try {
      const bytes = Uint8Array.from(atob(data.data), c => c.charCodeAt(0));
      if (bytes.length !== 1024) return;
      // U8g2 live stream uses vertical pages; art upload uses row-major XBM.
      const ctx = $('oled').getContext('2d'), image = ctx.createImageData(128,64);
      for (let y=0;y<64;y++) for (let x=0;x<128;x++) {
        const lit = bytes[(y>>3)*128+x] & (1<<(y&7));
        const i=(y*128+x)*4; image.data.set(lit ? [168,231,214,255] : [6,11,21,255],i);
      }
      ctx.putImageData(image,0,0);
    } catch { log('Ignored malformed screen frame.'); }
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
  } catch (error) { if (!closing) message(`USB connection lost: ${error.message}`); }
  finally {
    reader?.releaseLock(); reader = null;
    if (!closing) { rejectPending(new Error('USB connection closed.')); identity = null; controls(); void disconnect(); }
  }
}
async function write(command) {
  if (!writer || closing) throw new Error('Connect your jar first.');
  if (/[\r\n]/.test(command)) throw new Error('Command must be a single line.');
  await writer.write(new TextEncoder().encode(command+'\n'));
}
function request(command, match, timeout=5000) {
  const work = queue.then(async () => {
    if (!identity && command !== 'HELLO') throw new Error('Connect your jar first.');
    return new Promise((resolve,reject) => {
      const timer = setTimeout(()=>rejectPending(new Error('No response from jar: '+command.split(' ').slice(0,2).join(' '))),timeout);
      pending={resolve,reject,match,timer};
      write(command).catch(rejectPending);
    });
  });
  queue=work.catch(()=>{}); return work;
}
function ack(command, response=command) { return request(command,p=>p.type==='ACK' && p.data.command===response); }
function query(command,type) { return request(command,p=>p.type===type); }
async function connect() {
  connecting=true; controls(); message('Select your jar. Keep the Pi controller closed.');
  try {
    port=await navigator.serial.requestPort();
    await port.open({baudRate:115200});
    await port.setSignals({dataTerminalReady:false,requestToSend:false});
    writer=port.writable.getWriter(); reader=port.readable.getReader(); closing=false; readTask=readLoop();
    // Opening may reset the chip; retry HELLO through touch calibration.
    const hello=request('HELLO',p=>p.type==='IDENTITY',8000);
    handshakeTimer=setInterval(()=>write('HELLO').catch(()=>{}),300);
    try { await hello; } finally { clearInterval(handshakeTimer); }
    for (const [command,type] of [['GET PROFILE','PROFILE'],['GET STATUS','STATUS'],['GET POWER','POWER']]) await query(command,type);
    await ack('STREAM OLED ON');
    pollTimer=setInterval(()=>{if(identity && !transferring) query('GET POWER','POWER').catch(error=>message(error.message));},10000);
    message('Connected. Your jar is ready.');
  } catch(error) { message(error.name==='NotFoundError' ? 'No jar selected.' : `Could not connect: ${error.message}`); await disconnect(); }
  finally { connecting=false; controls(); }
}
async function disconnect() {
  if (closing) return;
  closing=true; clearInterval(pollTimer); clearInterval(handshakeTimer); rejectPending(new Error('Jar disconnected.'));
  identity=null; controls();
  try { await reader?.cancel(); await readTask; } catch {}
  try { writer?.releaseLock(); } catch {} writer=null;
  try { await port?.close(); } catch {} port=null; readTask=null; closing=false; controls();
}
function action(fn) { return async event => { event?.preventDefault(); try { await fn(); } catch(error) { message(error.message); } }; }
$('connect').onclick=connect; $('disconnect').onclick=()=>void disconnect();
document.querySelectorAll('[data-tab]').forEach(button=>button.onclick=()=>{
  document.querySelectorAll('.panel').forEach(p=>p.hidden=p.id!==button.dataset.tab);
  document.querySelectorAll('[data-tab]').forEach(b=>b.setAttribute('aria-selected',String(b===button)));
});
document.querySelectorAll('[data-command]').forEach(button=>button.onclick=action(async()=>{
  const command=button.dataset.command;
  if(command==='GET PROFILE') await query(command,'PROFILE'); else await ack(command,command.startsWith('KEY ') ? 'KEY' : command);
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
  catch(error){$('release-status').textContent=error.message+' Your jar can still work offline.';}
}
function updateReleaseStatus(){if(!releaseCatalog)return;const stable=releaseCatalog.releases.filter(r=>r.channel==='stable' && r.model==='happyjarz-supermini-simple-4mb');if(!stable.length){$('release-status').textContent=`No tested firmware download has been published yet.${identity ? ' Connected firmware: '+identity.fw+'.' : ''}`;return;}try{stable.sort((a,b)=>compareVersions(b.version,a.version));$('release-status').textContent=`Latest listed release: ${stable[0].version}. Exact board compatibility must be verified before installation. Installer remains disabled.`;}catch{$('release-status').textContent='Release catalog contains an unsupported version. Installer remains disabled.';}}
$('check-updates').onclick=checkUpdates;
$('support').textContent='serial' in navigator ? 'USB browser support detected. Click Connect jar to begin.' : 'This browser does not expose Web Serial. Try a current desktop Chrome or Edge; other browser versions vary.';
controls();renderArt();void checkUpdates();
