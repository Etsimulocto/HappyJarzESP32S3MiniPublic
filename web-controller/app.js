import { parseLine } from './protocol.mjs';
import { DiagnosticLog } from './diagnostics.mjs';
const $ = id => document.getElementById(id);
let storage; try { storage=localStorage; } catch {}
const diagnostics=new DiagnosticLog({storage}), snapshots={};
const panels={};
let latestIdentity, usbInfo, diagnosticFlushTimer, serviceBusy=false;
let port, reader, writer, readTask, identity, pending, queue = Promise.resolve(), closing = false, connecting = false, transferring = false;
let pollTimer, handshakeTimer;
let deviceControls=new Set(), connectionGeneration=0, booting=true;
const telemetryLogTimes={};
function message(text,level='info') { $('message').textContent = text; log(text,level,'UI'); }
function log(text,level='info',kind='RX') {
  diagnostics.add(level,kind,text);panels.service?.renderDiagnostics();
  if(!diagnosticFlushTimer)diagnosticFlushTimer=setTimeout(()=>{diagnosticFlushTimer=null;diagnostics.flush();panels.service?.renderDiagnostics();},800);
}
function controls() {
  document.querySelectorAll('[data-device]').forEach(el => el.disabled = !identity || connecting || closing || transferring || serviceBusy);
  document.querySelectorAll('[data-key]').forEach(el=>el.disabled=!identity || connecting || closing || transferring || serviceBusy || !deviceControls.has(el.dataset.key));
  $('connect').disabled = booting || connecting || closing || !navigator.serial?.requestPort;
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
  const highRate=['OLED','LED_FRAME','INPUT','TOUCH'].includes(type),now=performance.now();
  if(!highRate || now-(telemetryLogTimes[type]??-Infinity)>1000){log(line,type==='ERR'?'error':'info','RX');telemetryLogTimes[type]=now;}

  if (pending && (type === 'ERR' || pending.match(parsed))) {
    const p = pending; pending = null; clearTimeout(p.timer);
    type === 'ERR' ? p.reject(new Error(data.message || 'Jar rejected the command.')) : p.resolve(parsed);
  }
  if (type === 'IDENTITY') {
    identity = data;latestIdentity={...data}; controls();
  }
  notifyPanels('receive',parsed);
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
  const generation=connectionGeneration;
  const work = queue.then(async () => {
    if(generation!==connectionGeneration)throw new Error('Request belongs to a previous USB connection.');
    if (!identity && command !== 'HELLO') throw new Error('Connect your jar first.');
    return new Promise((resolve,reject) => {
      const current={resolve,reject,match,command};
      current.timer=setTimeout(()=>{if(pending===current)rejectPending(new Error('No response from jar: '+command.split(' ').slice(0,2).join(' ')));},timeout);
      pending=current;
      write(command).catch(error=>{if(pending===current)rejectPending(error);});
    });
  });
  queue=work.catch(()=>{}); return work;
}
function ack(command, response=command) { return request(command,p=>p.type==='ACK' && p.data.command===response); }
function query(command,type) { return request(command,p=>p.type===type); }
async function connect() {
  connectionGeneration++;deviceControls=new Set();notifyPanels('reset');
  connecting=true; controls(); message('Select your jar. Keep the Pi controller closed.');
  try {
    port=await navigator.serial.requestPort();
    for(const key of Object.keys(snapshots))delete snapshots[key];
    usbInfo=port.getInfo?.() || {};log(JSON.stringify(usbInfo),'info','USB');
    await port.open({baudRate:115200});
    await port.setSignals({dataTerminalReady:false,requestToSend:false});
    writer=port.writable.getWriter(); reader=port.readable.getReader(); closing=false; readTask=readLoop();
    // Opening may reset the chip; retry HELLO through touch calibration.
    const hello=request('HELLO',p=>p.type==='IDENTITY',8000);
    handshakeTimer=setInterval(()=>write('HELLO').catch(()=>{}),300);
    try { await hello; } finally { clearInterval(handshakeTimer); }
    for (const [command,type] of [['GET PROFILE','PROFILE'],['GET STATUS','STATUS'],['GET POWER','POWER']]) await query(command,type);
    await query('GET CAPS','CAPS');
    await query('GET TIME STATUS','TIME');
    await ack('STREAM OLED ON');
    await ack('STREAM LED ON');await ack('STREAM TOUCH ON');
    pollTimer=setInterval(()=>{if(identity && !transferring && !serviceBusy) query('GET POWER','POWER').catch(error=>message(error.message,'error'));},10000);
    message('Connected. Your jar is ready.');
  } catch(error) { message(error.name==='NotFoundError' ? 'No jar selected.' : `Could not connect: ${error.message}`,error.name==='NotFoundError'?'warn':'error'); await disconnect(); }
  finally { connecting=false; controls(); }
}
async function disconnect() {
  if (closing) return;
  closing=true; clearInterval(pollTimer); clearInterval(handshakeTimer);log('Disconnecting serial session.','info','USB'); rejectPending(new Error('Jar disconnected.'));
  connectionGeneration++;deviceControls=new Set();
  identity=null; controls();notifyPanels('reset');
  try{if(writer && reader){const command='STREAM OLED OFF\nSTREAM LED OFF\nSTREAM TOUCH OFF\n';log('Stopping OLED, LED and touch streams.','info','USB');await writer.write(new TextEncoder().encode(command));}}catch(error){log(error.message,'warn','USB-CLEANUP');}
  try { await reader?.cancel(); await readTask; } catch(error){log(error.message,'warn','USB-CLEANUP');}
  try { writer?.releaseLock(); } catch(error){log(error.message,'warn','USB-CLEANUP');} writer=null;
  try { await port?.close(); } catch(error){log(error.message,'warn','USB-CLEANUP');} port=null; readTask=null; closing=false; controls();
}
function action(fn) { return async event => { event?.preventDefault(); try { await fn(); } catch(error) { message(error.message,'error'); } }; }
$('connect').onclick=connect; $('disconnect').onclick=()=>void disconnect();
document.querySelectorAll('[data-tab]').forEach(button=>button.onclick=()=>{
  document.querySelectorAll('.panel').forEach(p=>p.hidden=p.id!==button.dataset.tab);
  document.querySelectorAll('[data-tab]').forEach(b=>b.setAttribute('aria-selected',String(b===button)));
  panels[button.dataset.tab]?.onShow?.();
});
document.querySelectorAll('[data-command]').forEach(button=>button.onclick=action(async()=>{
  const command=button.dataset.command;
  if(command==='GET PROFILE') await query(command,'PROFILE'); else await ack(command,command.startsWith('KEY ') ? 'KEY' : command.startsWith('SET INPUT MODE ') ? 'SET INPUT MODE' : command);
  message('Jar acknowledged: '+command);
}));
$('support').textContent=Boolean(navigator.serial?.requestPort) ? 'USB browser support detected. Click Connect jar to begin.' : 'This browser does not expose Web Serial. Try a current desktop Chrome or Edge; other browser versions vary.';
document.querySelectorAll('[data-query]').forEach(button=>button.onclick=action(async()=>{await query(button.dataset.query,button.dataset.response);message('Readout refreshed.');}));
window.addEventListener('error',event=>{const target=event.target;if(target!==window)log('Resource failed: '+target?.tagName,'error','RESOURCE');else log(`${event.message}\n${event.error?.stack||''}`,'error','JAVASCRIPT');},true);
window.addEventListener('unhandledrejection',event=>log(event.reason?.stack||event.reason?.message||String(event.reason),'error','UNHANDLED'));
window.addEventListener('pagehide',()=>diagnostics.flush());
for(const name of ['error','warn']){const original=console[name].bind(console);console[name]=(...args)=>{const text=args.map(a=>{if(typeof a==='string')return a;if(a?.stack)return a.stack;try{return JSON.stringify(a);}catch{return '[unserializable console value]';}}).join(' ');log(text,name==='error'?'error':'warn','CONSOLE');original(...args);};}
const ctx={ $, ack, query, request, action, controls, message, log, diagnostics, snapshots, environment,
  get identity(){return identity;},get latestIdentity(){return latestIdentity;},get transferring(){return transferring;},set transferring(value){transferring=value;},
  get serviceBusy(){return serviceBusy;},set serviceBusy(value){serviceBusy=value;},get connecting(){return connecting;},get closing(){return closing;},
  get deviceControls(){return deviceControls;},set deviceControls(value){deviceControls=value;},syncClock:()=>panels.setup.syncClock()
};
controls();
for(const [id,file] of [['home','lights-control'],['setup','wifi-clock'],['display','display-input'],['profile','profile'],['art','art'],['service','service'],['updates','updates'],['bloopetz','bloopetz'],['dnd','dnd']]){
  try{const {init}=await import(`./tabs/${file}.js`);panels[id]=init(ctx);}catch(error){log(`Panel ${id} failed to initialize: ${error.stack || error.message}`,'error','PANEL');document.querySelector(`[data-tab="${id}"]`).disabled=true;message(`The ${id} panel could not load. See the Service log.`,'error');}
}
booting=false;
log(JSON.stringify(environment()),'info','SESSION');controls();

function notifyPanels(method,payload){for(const [id,panel] of Object.entries(panels)){try{panel[method]?.(payload);}catch(error){log(`Panel ${id}: ${error.stack || error.message}`,'error','PANEL');}}}
function environment(){return {userAgent:navigator.userAgent,language:navigator.language,languages:navigator.languages,timezone:Intl.DateTimeFormat().resolvedOptions().timeZone,timezoneOffsetMinutes:new Date().getTimezoneOffset(),secureContext:isSecureContext,webSerialAvailable:Boolean(navigator.serial?.requestPort),online:navigator.onLine,origin:location.origin,viewport:{width:innerWidth,height:innerHeight},usb:usbInfo||null};}
