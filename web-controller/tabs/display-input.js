const MODES=['SAYINGS','SPIRAL','TRIPPY','PARTICLES','BLOOM','BREATHE','GLITTER','ART'];
const SOURCES=['BUILTIN','CUSTOM','MIXED'];
const PADS=['up','down','left','right','a','b'];
export function init(ctx) {
  const {$,ack,query,request,action,controls,message,log}=ctx;
  let loading=false,received=new Map();
  for(const pad of PADS){const tile=document.createElement('div'),name=document.createElement('strong'),value=document.createElement('output');tile.className='input-tile';tile.id='display-pad-tile-'+pad;name.textContent=pad.toUpperCase();value.id='display-pad-'+pad;value.textContent='—';tile.append(name,value);$('display-pad-readouts').append(tile);}
  function source(){const value=$('saying-source').value;if(!SOURCES.includes(value))throw new Error('Choose a supported saying source.');return value;}
  function validText(text){if(/[|\r\n]/.test(text)||new TextEncoder().encode(text).length>96)throw new Error('Each message must fit 96 UTF-8 bytes, with no | or line breaks.');return text;}
  async function exclusive(fn){if(ctx.serviceBusy||ctx.transferring)throw new Error('Wait for the current jar operation to finish.');ctx.serviceBusy=true;controls();try{return await fn();}finally{ctx.serviceBusy=false;controls();}}
  async function loadSayings(){
    loading=true;received=new Map();$('sayings-state').textContent='Loading all 50 slots…';
    try{await request('GET CUSTOM SAYINGS',p=>p.type==='CUSTOM_SAYINGS'&&p.event==='END',10000);
      if(received.size!==50)throw new Error('Incomplete marquee response. Editor left unchanged; try loading again.');
      const lines=Array.from({length:50},(_,i)=>received.get(i+1));while(lines.length&&!lines.at(-1))lines.pop();$('sayings-list').value=lines.join('\n');
      $('sayings-state').textContent=`Loaded ${lines.filter(Boolean).length} messages from 50 slots.`;
    }catch(error){$('sayings-state').textContent=error.message;throw error;}finally{loading=false;}
  }
  $('load-sayings').onclick=action(()=>exclusive(loadSayings));
  $('set-saver').onclick=action(async()=>{const mode=$('saver-mode').value;if(!MODES.includes(mode))throw new Error('Choose a supported screensaver.');await ack('SET SAVER MODE '+mode,'SET SAVER MODE');await query('GET SAVER STATUS','SAVER');message('Screensaver applied.');});
  $('save-saying').onclick=action(async()=>{const slot=Number($('saying-slot').value),text=validText($('saying-text').value.trim());if(!Number.isInteger(slot)||slot<1||slot>50||!text)throw new Error('Choose slot 1–50 and enter a message.');await exclusive(async()=>{await ack(`SET CUSTOM SAYING ${slot} ${text}`,'SET CUSTOM SAYING');await loadSayings();});message('Marquee slot saved.');});
  $('set-saying-source').onclick=action(async()=>{await ack('SET SAYING SOURCE '+source(),'SET SAYING SOURCE');message('Saying source saved on the jar.');});
  $('save-sayings').onclick=action(async()=>{
    const lines=$('sayings-list').value.replace(/\r\n/g,'\n').split('\n');while(lines.length&&!lines.at(-1).trim())lines.pop();
    if(lines.length>50)throw new Error('Use at most 50 lines, one per slot.');
    const messages=lines.map(line=>validText(line.trim())),selected=source();
    if(!confirm('Replace all 50 custom-message slots on this jar?'))return;
    await exclusive(async()=>{
      $('sayings-state').textContent='Saving… keep the jar connected.';
      try{await ack('CLEAR CUSTOM SAYINGS');for(let i=0;i<messages.length;i++)if(messages[i])await ack(`SET CUSTOM SAYING ${i+1} ${messages[i]}`,'SET CUSTOM SAYING');await ack('SET SAYING SOURCE '+selected,'SET SAYING SOURCE');await loadSayings();}
      catch(error){$('sayings-state').textContent='Save interrupted; the jar may have a partial list. Your draft stays in the editor. Load to check before retrying.';throw error;}
    });message('Custom messages saved.');
  });
  $('clear-sayings').onclick=action(async()=>{if(!confirm('Erase all custom messages and return to built-in sayings?'))return;await exclusive(async()=>{await ack('CLEAR CUSTOM SAYINGS');await ack('SET SAYING SOURCE BUILTIN','SET SAYING SOURCE');$('saying-source').value='BUILTIN';await loadSayings();});message('Custom messages cleared.');});
  // Route recalibration through Service so it shares the report's test history.
  $('display-test-input').onclick=()=>$('service-touch').click();
  for(const mode of ['JAR','MENU','GAME'])$('display-mode-'+mode.toLowerCase()).onclick=action(async()=>{await ack('SET INPUT MODE '+mode,'SET INPUT MODE');$('display-input-mode').textContent=mode+' mode · acknowledged by jar';message(mode+' input mode applied.');});
  return {receive({type,data,event}){
    if(type==='CUSTOM_SAYING'&&loading){const slot=Number(data.slot);if(Number.isInteger(slot)&&slot>=1&&slot<=50)received.set(slot,data.text||'');}
    if(type==='CUSTOM_SAYINGS'&&loading&&event==='END'&&SOURCES.includes(data.source))$('saying-source').value=data.source;
    if(type==='SAVER'){
      if(MODES.includes(data.mode))$('saver-mode').value=data.mode;
      const speed=data.mode==='SPIRAL'?data.spiral_speed:data.mode==='TRIPPY'?data.trippy_speed:data.mode==='PARTICLES'?data.particle_speed:['BLOOM','BREATHE','GLITTER'].includes(data.mode)?data.visual_speed:undefined;
      $('saver-state').textContent=`${data.mode || 'Unknown'} · ${data.active==='1'?'Active':'Idle'}${speed?' · speed '+speed+'/8':''}${data.idle_ms?' · auto-start after '+Number(data.idle_ms)/1000+'s':''}`;
      if(data.mode==='PARTICLES')$('saver-state').textContent+=` · ${data.particles ?? '—'} particles · ${data.attractors ?? '—'} wells · links ${data.links ?? '—'} · wrap ${data.wrap ?? '—'} · repel ${data.repel ?? '—'}`;
    }
    if(type==='DISPLAY')$('display-state').textContent=`Reported brightness ${data.brightness ?? '—'}% · current firmware fixes brightness at 100%.`;
    if(type==='ACK'&&data.command==='SET INPUT MODE')$('display-input-mode').textContent='Input mode change acknowledged by jar.';
    if(type==='INPUT'||type==='TOUCH'){for(const pad of PADS)$('display-pad-'+pad).textContent=ctx.deviceControls.has(pad)?data[pad] ?? '—':'N/A';$('display-input-state').textContent='Live raw values · '+new Date().toLocaleTimeString();}
    if(type==='TOUCH_CAL')$('display-calibration').textContent=PADS.map(p=>`${p.toUpperCase()}: baseline ${data[p+'_base'] ?? '—'} · threshold ${data[p+'_thr'] ?? '—'}`).join('\n');
    if(type==='OLED'&&data.codec==='b64v1')try{
      const bytes=Uint8Array.from(atob(data.data),c=>c.charCodeAt(0));if(bytes.length!==1024)throw new Error('Invalid screen byte count');
      const canvas=$('display-oled').getContext('2d'),image=canvas.createImageData(128,64);
      for(let y=0;y<64;y++)for(let x=0;x<128;x++)image.data.set(bytes[(y>>3)*128+x]&(1<<(y&7))?[168,231,214,255]:[6,11,21,255],(y*128+x)*4);
      canvas.putImageData(image,0,0);$('display-oled-state').textContent='Received screen · frame '+(data.seq || '—')+' · '+new Date().toLocaleTimeString();
    }catch(error){log(error.message,'warn','DISPLAY');}
  },reset(){loading=false;received.clear();for(const id of ['display-oled-state','display-input-state','saver-state','display-state','display-input-mode'])$(id).textContent='Disconnected · last readings paused.';for(const pad of PADS)$('display-pad-'+pad).textContent='—';$('display-calibration').textContent='No calibration for current connection.';}};
}
