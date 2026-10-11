import { PAD_BINDINGS, INPUT_KEYS, rgb, rgbHex, hexRgb, brightness, capabilities, gamepadKeys, patternTheme } from '../lights.mjs';
export function init(ctx) {
  const { $, ack, query, request, action, controls, message, log } = ctx;
let brightnessTimer,themeTimer,themeFrames=[],currentScene='',themedScene='',hostPending=0;
const patterns=['SOLID','FADE','PULSE','RAINBOW','RANDOM','HUE_FADE','DUAL_HUE','BREATH','DRIFT','AURORA','OCEAN','LAVENDER','SUNSET','CHRISTMAS','HALLOWEEN','VALENTINE','EASTER','FOURTH','THANKSGIVING','CANDY','GALAXY','FIRE','ICE','FOREST','NEON','TWINKLE','SPARKLE','COLOR_SWAP','COMET','FIREFLY','BUBBLEGUM','CHASE_CW','CHASE_CCW','JAR_CHASE','SIDE_CHASE','SWEEP_LR','SWEEP_TS','DIAGONAL','PING_PONG','DUAL_CHASE','OPP_CHASE','JAR_PULSE','SIDE_ACCENT','WATERFALL','RIPPLE','TIDAL','MOONWAVE','DEEPSEA','LAGOON','ARCTIC','GLACIER','EMBER','CAMPFIRE','SUNRISE','SOLAR','INFERNO','CANDLE','AUTUMN','HARVEST','MINT','MEADOW','JUNGLE','MOSS','CLOVER','SPRING','RAINFOREST','LIMEADE','DREAM','COTTON_CANDY','MERMAID','UNICORN','VAPOR','COSMIC','STARDUST','NEBULA','ROSE_GOLD','PEACH','CORAL','CHERRY','BERRY','PLUM','GRAPE','MAGENTA','ELECTRIC','CYBER','LASER','SYNTH','ARCADE','MATRIX','PLASMA','HOLOGRAM','RAIN','THUNDER','LIGHTNING','CLOUD','SNOW','FROST','SUNBEAM','MOONLIGHT','OFF'];
for (const pattern of patterns) $('pattern').add(new Option(pattern.replaceAll('_',' '),pattern));
for (let i=1;i<=4;i++) {
  const card=document.createElement('article');card.className='light-card';
  card.innerHTML=`<div class="section-head"><h3>LIGHT ${i}</h3><span class="light-location">${['Top left jar','Top right jar','Left side accent','Right side accent'][i-1]}</span></div><div class="swatch-row"><div><span>BASE</span><i id="base${i}" class="swatch"></i><output id="base-value${i}">—</output></div><div><span>LIVE</span><i id="live${i}" class="swatch"></i><output id="live-value${i}">—</output></div></div><div class="light-actions"><label class="color-chooser">Choose color<input id="color${i}" type="color" value="#000000" data-device aria-label="Light ${i} base color"></label><button id="white${i}" data-device>White</button><button id="off${i}" data-device>Off</button></div>`;
  $('colors').append(card);
  $('color'+i).onchange=action(()=>setLight(i,hexRgb($('color'+i).value)));
  $('white'+i).onclick=action(()=>setLight(i,[255,255,255]));$('off'+i).onclick=action(()=>setLight(i,[0,0,0]));
}
async function setLight(i,color){await ack(`SET LED${i} COLOR ${color.join(' ')}`,`SET LED${i} COLOR`);await query('GET STATUS','STATUS');message(`Light ${i} base color applied. Patterns may animate the live color.`);}
async function applyPattern(){await ack('SET PATTERN '+$('pattern').value,'SET PATTERN');await query('GET STATUS','STATUS');message('Pattern applied.');}
$('apply-pattern').onclick=action(applyPattern);$('pattern').onchange=action(applyPattern);
async function applyBrightness(){await ack('SET BRIGHTNESS '+brightness($('brightness').value),'SET BRIGHTNESS');await query('GET STATUS','STATUS');}
$('brightness').oninput=()=>{$('brightness-label').textContent=brightness($('brightness').value)+'%';clearTimeout(brightnessTimer);if(ctx.identity&&!ctx.transferring&&!ctx.serviceBusy)brightnessTimer=setTimeout(()=>applyBrightness().catch(error=>message(error.message,'error')),120);};
$('apply-brightness').onclick=action(async()=>{clearTimeout(brightnessTimer);await applyBrightness();message('Brightness applied.');});
$('apply-colors').onclick=action(async()=>{for(let i=1;i<=4;i++){const color=hexRgb($('color'+i).value);await ack(`SET LED${i} COLOR ${color.join(' ')}`,`SET LED${i} COLOR`);}await query('GET STATUS','STATUS');message('Colors applied. Select SOLID to display them continuously.');});
function captureTheme(){
  clearTimeout(themeTimer);themeTimer=null;themeFrames=[];
  if(!$('pattern-theme').checked)return;
  themeTimer=setTimeout(()=>{
    themeTimer=null;const hadFrames=themeFrames.length>0,theme=patternTheme(themeFrames);themeFrames=[];if(hadFrames)themedScene=currentScene;if(!theme)return;
    for(const [name,value] of Object.entries({bg:theme.bg,card:theme.panel,ink:theme.text,line:theme.border,pink:theme.text,mint:theme.text,muted:theme.text}))document.documentElement.style.setProperty('--'+name,value);
  },650);
}
function resetTheme(){for(const name of ['bg','card','ink','line','pink','mint','muted'])document.documentElement.style.removeProperty('--'+name);}
function updateScene(data){
  if(data.pattern && data.pattern!==currentScene){currentScene=data.pattern;if(patterns.includes(data.pattern))$('pattern').value=data.pattern;captureTheme();}
  $('scene-state').textContent=`${currentScene || 'Unknown pattern'} · ${data.brightness ?? $('brightness').value}% · last update ${new Date().toLocaleTimeString()}`;
}
$('reset-theme').onclick=()=>{resetTheme();$('pattern-theme').checked=false;clearTimeout(themeTimer);themeTimer=null;themeFrames=[];};
$('pattern-theme').onchange=()=>{$('pattern-theme').checked?captureTheme():resetTheme();};
$('refresh-lights').onclick=action(async()=>{for(const [command,type] of [['GET STATUS','STATUS'],['GET CAPS','CAPS'],['GET LED FRAME','LED_FRAME'],['GET INPUT','INPUT'],['GET OLED','OLED']])await query(command,type);message('Lights and console refreshed.');});
const pulseTimers=new Map();
const dpad=document.createElement('div'),actions=document.createElement('div'),extras=document.createElement('div');
dpad.id='gamepad-dpad';actions.id='gamepad-actions';extras.id='gamepad-extras';
$('mini-buttons').append(dpad,actions,extras);
for(const key of INPUT_KEYS){
  const tile=document.createElement('div');tile.id='input-'+key;tile.className='input-tile unsupported';
  const title=document.createElement('strong'),value=document.createElement('output');title.textContent=key.toUpperCase();tile.title=PAD_BINDINGS[key]?`GPIO${PAD_BINDINGS[key].pin} · ${PAD_BINDINGS[key].role}`:'Available only if the jar advertises it';value.id='input-value-'+key;value.textContent='N/A';tile.append(title,value);$('input-tiles').append(tile);
  const button=document.createElement('button');button.id='key-'+key;button.dataset.key=key;button.title=tile.title;button.textContent=({up:'▲',down:'▼',left:'◀',right:'▶',l:'L −',r:'R +'})[key] || key.toUpperCase();button.setAttribute('aria-label',key.toUpperCase());button.onclick=action(()=>sendHostKey(key));(['up','down','left','right'].includes(key)?dpad:['a','b'].includes(key)?actions:extras).append(button);
}
function pulseKey(key){if(!INPUT_KEYS.includes(key))return;clearTimeout(pulseTimers.get(key));for(const id of ['input-','key-'])$(id+key).classList.add('active');pulseTimers.set(key,setTimeout(()=>{for(const id of ['input-','key-'])$(id+key).classList.remove('active');pulseTimers.delete(key);},120));}
async function sendHostKey(key){
  if(!ctx.identity || !ctx.deviceControls.has(key) || ctx.transferring || ctx.serviceBusy || ctx.closing || ctx.connecting || hostPending>=4)return;
  hostPending++;try{await ack('KEY '+key,'KEY');pulseKey(key);}finally{hostPending--;}
}
const keyboardKeys={ArrowUp:'up',ArrowDown:'down',ArrowLeft:'left',ArrowRight:'right',z:'a',x:'b',a:'x',s:'y',q:'l',w:'r',Enter:'start',Shift:'select'};
function hostInputAllowed(){const target=document.activeElement;return $('host-input-enabled').checked && !$('home').hidden && document.hasFocus() && !target?.matches('input,textarea,select,[contenteditable=true]');}
window.addEventListener('keydown',event=>{const key=keyboardKeys[event.key] || keyboardKeys[event.key.toLowerCase()];if(!key || event.repeat || event.ctrlKey || event.metaKey || event.altKey || !hostInputAllowed() || !ctx.identity || !ctx.deviceControls.has(key))return;event.preventDefault();void sendHostKey(key).catch(error=>message(error.message,'error'));});
let previousGamepadKeys=new Set(),lastGamepad='';
function pollGamepad(){
  let pad;try{pad=Array.from(navigator.getGamepads?.() || []).find(p=>p?.connected);}catch(error){$('gamepad-state').textContent='Gamepad access unavailable in this browser.';log(error.message,'warn','GAMEPAD');return;}
  const label=pad ? `${pad.id} · ${pad.mapping==='standard'?'standard mapping':'mapping unsupported'}` : 'No gamepad detected · press a controller button to discover it';
  if(label!==lastGamepad){$('gamepad-state').textContent=label;lastGamepad=label;}
  const keys=gamepadKeys(pad);
  if(hostInputAllowed())for(const key of keys)if(!previousGamepadKeys.has(key))void sendHostKey(key).catch(error=>message(error.message,'error'));
  previousGamepadKeys=keys;requestAnimationFrame(pollGamepad);
}
requestAnimationFrame(pollGamepad);

return { receive({type,data}) {
if(type==='IDENTITY')$('identity').textContent=`${data.serial || 'HAPPY JARZ'} · ${data.hw || '?'} · firmware ${data.fw || '?'}`;
  if (type === 'STATUS') {
    updateScene(data);
    if ([...$('pattern').options].some(o => o.value === data.pattern)) $('pattern').value = data.pattern;
    if (Number.isFinite(Number(data.brightness))) { $('brightness').value = brightness(data.brightness); $('brightness-label').textContent = `${brightness(data.brightness)}%`; }
    for (let i = 1; i <= 4; i++) {
      const rgb = data[`led${i}`]?.split(',').map(Number);
      if (rgb?.length === 3 && rgb.every(n => Number.isInteger(n) && n >= 0 && n <= 255)) {const hex=rgbHex(rgb);$('color'+i).value=hex;$('base'+i).style.backgroundColor=hex;$('base-value'+i).textContent=hex.toUpperCase();}
    }
  }
  if (type === 'POWER') $('power').textContent = Number(data.percent) >= 0 ? `Battery ${data.percent}% · ${data.voltage || '—'} V` : 'Battery reading unavailable';
  if(type==='CAPS'){
    ctx.deviceControls=capabilities(data.controls);controls();$('input-capabilities').textContent=`${data.profile || 'Device'} · ${ctx.deviceControls.size} advertised controls`;
    for(const key of INPUT_KEYS){$('input-'+key).classList.toggle('unsupported',!ctx.deviceControls.has(key));$('input-value-'+key).textContent=ctx.deviceControls.has(key)?'—':'N/A';}
  }
  if(type==='INPUT' || type==='TOUCH'){
    for(const key of INPUT_KEYS){const value=data[key] ?? (key==='a'?data.c1:key==='b'?data.c2:undefined);if(ctx.deviceControls.has(key) && value!==undefined)$('input-value-'+key).textContent=value;}
    $('input-state').textContent='Live touch readings · '+new Date().toLocaleTimeString();
  }
  if(type==='EVENT' && data.input)pulseKey(data.input.toLowerCase());
  if(type==='LED_FRAME'){
    updateScene(data);const frame=[];
    for(let i=1;i<=4;i++){const color=rgb(data['led'+i] ?? data['live_led'+i]);if(color){const hex=rgbHex(color);$('live'+i).style.backgroundColor=hex;$('live-value'+i).textContent=hex.toUpperCase();frame.push(color);}}
    if(!themeTimer && themedScene!==currentScene && $('pattern-theme').checked)captureTheme();
    if(themeTimer && frame.length===4 && themeFrames.length<64)themeFrames.push(frame);
  }
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
      $('oled-state').textContent=`LIVE · frame ${data.seq || '—'} · ${new Date().toLocaleTimeString()}`;
    } catch { log('Ignored malformed screen frame.','warn','PROTOCOL'); }
  }
}, reset() {clearTimeout(brightnessTimer);clearTimeout(themeTimer);themeTimer=null;themeFrames=[];currentScene='';themedScene='';for(const timer of pulseTimers.values())clearTimeout(timer);pulseTimers.clear();for(const key of INPUT_KEYS)for(const id of ['input-','key-'])$(id+key).classList.remove('active');$('oled-state').textContent='Disconnected · last received screen';$('input-state').textContent='Disconnected · readings paused';} };
}
