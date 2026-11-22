// HAPPY JARZ Experience Manager v0.1
// Browser-side configuration only. Never confuses hidden UI with removed firmware.
// Physical board events can optionally invoke *safe, already-supported* jar commands.
// Native keyboard/gamepad/MIDI output requires future ESP32 USB HID/MIDI firmware.
export const PADS=[
 ['UP',4],['DOWN',5],['LEFT',9],['RIGHT',10],
 ['A',1],['B',2],['L',11],['R',12],['START',13]
];
export const FEATURES=[
 ['home','Lights + Controls'],['setup','Wi-Fi + Clock'],['display','OLED + Screensavers'],
 ['profile','Owner Profile'],['art','Art Studio'],['service','Service + Diagnostics'],
 ['updates','Firmware Updates'],['arcade','Factory Arcade (on jar)'],
 ['pet','BLOOMPETZ (planned install)'],['dnd','DND (planned install)'],
 ['hid','USB Macros / Gamepad (planned firmware)']
];
export const PRESETS={
 classic:['home','setup','display','profile','art','service','updates','arcade'],
 pet:['home','setup','display','profile','service','updates','pet'],
 dnd:['home','setup','display','profile','service','updates','dnd','hid'],
 display:['home','setup','display','art','service','updates'],
 maker:FEATURES.map(x=>x[0])
};
export const ACTIONS=[
 ['NONE','Unassigned'],['SAVER NEXT','Next screensaver'],['SAVER PREV','Previous screensaver'],
 ['SAVER ENTER','Start screensaver'],['SAVER EXIT','Exit screensaver'],
 ['SAVER RESEED','New screensaver variation'],['SAVE','Save jar settings'],
 ['KEYBOARD','Keyboard key / shortcut (future USB HID)'],
 ['GAMEPAD','Gamepad button (future USB HID)'],
 ['MIDI','MIDI note / control (future USB MIDI)']
];
const DEFAULT_BINDINGS=Object.fromEntries(PADS.map(([name])=>[name,{kind:'NONE',value:''}]));
export const DEFAULT_CONFIG={
 preset:'classic',features:[...PRESETS.classic],home:'home',simple:false,
 bridge:false,bindings:DEFAULT_BINDINGS
};
const ACTION_SET=new Set(ACTIONS.map(x=>x[0]));
const FEATURE_SET=new Set(FEATURES.map(x=>x[0]));
export function validateConfig(input){
 const clean={...DEFAULT_CONFIG};
 clean.preset=Object.hasOwn(PRESETS,input?.preset)?input.preset:'custom';
 clean.features=Array.isArray(input?.features)?
  [...new Set(input.features.filter(x=>FEATURE_SET.has(x)))]:[...DEFAULT_CONFIG.features];
 // Keep help, updater and configuration recovery reachable from Web Studio.
 clean.simple=input?.simple===true;
 clean.bridge=input?.bridge===true;
 clean.home=['home','display','art','setup','profile'].includes(input?.home)?input.home:'home';
 clean.bindings={};
 for(const [name] of PADS){
  const b=input?.bindings?.[name]||{};
  const kind=ACTION_SET.has(b.kind)?b.kind:'NONE';
  const value=typeof b.value==='string'?b.value.trim().slice(0,64):'';
  clean.bindings[name]={kind,value};
 }
 return clean;
}
export function init(ctx){
 const {$,ack,message,log}=ctx;
 const key='happyjarz-experience-v1';
 let config=DEFAULT_CONFIG,storageOK=true,lastEventAt=0,busy=false,boardId=null;
 try{config=validateConfig(JSON.parse(localStorage.getItem(key)||'null'));}catch(e){storageOK=false;config=validateConfig(null);}
 const navIds=['home','setup','display','profile','art','service','updates'];
 const page=$('experience');
 const state=$('experience-state');
 const bindingState=$('mapping-state');
 const padInputs=new Map();
 const featureInputs=new Map();
 function save(){
  try{localStorage.setItem(key,JSON.stringify(config));storageOK=true;}
  catch(e){storageOK=false;log('Experience settings could not be stored: '+e.message,'warn','EXPERIENCE');}
 }
 function applyVisibility(){
  // Visible sections are a browser UI preference, never a firmware uninstall.
  for(const id of navIds){
    const tab=document.querySelector('[data-tab="'+id+'"]');
    if(tab)tab.hidden= config.simple && !config.features.includes(id);
  }
  // Always preserve access to the manager (and its full UI restore).
  const active=document.querySelector('nav [aria-selected="true"]');
  if(active?.hidden){
    const target=navIds.find(id=>config.features.includes(id))||'home';
    document.querySelector('[data-tab="'+target+'"]')?.click();
  }
  document.body.classList.toggle('jar-simple-ui',config.simple);
 }
 function status(){
  state.textContent=(storageOK?'Saved in this browser.':'Browser storage unavailable; changes last only this session.')+
   ' '+(boardId?'Connected jar: '+boardId+'.':'Not connected.')+
   ' These preferences do not remove firmware modules or change the jar OLED menu.';
  bindingState.textContent=config.bridge
   ?'Live jar shortcuts ON (requires connected Web Studio). Keyboard/gamepad/MIDI assignments are saved plans, not active OS outputs.'
   :'Live jar shortcuts OFF. Nothing is sent from physical touch events by this mapper.';
 }
 function render(){
  $('experience-preset').value=config.preset;
  $('experience-simple').checked=config.simple;
  $('experience-bridge').checked=config.bridge;
  $('experience-start').value=config.home;
  for(const [id,input] of featureInputs)input.checked=config.features.includes(id);
  for(const [name,obj] of padInputs){obj.kind.value=config.bindings[name].kind;obj.value.value=config.bindings[name].value;
   obj.value.disabled=!['KEYBOARD','GAMEPAD','MIDI'].includes(obj.kind.value);
  }
  applyVisibility();status();
 }
 for(const [id,label] of FEATURES){
  const row=document.createElement('label');row.className='experience-choice';
  const check=document.createElement('input');check.type='checkbox';check.checked=config.features.includes(id);
  check.addEventListener('change',()=>{
   config={...config,preset:'custom',features:check.checked?[...config.features,id]:config.features.filter(x=>x!==id)};
   save();render();
  });
  const title=document.createElement('span');title.textContent=label;
  const note=document.createElement('small');
  note.textContent=['pet','dnd','hid'].includes(id)?'Planned hardware module':'Visible in experience preview';
  row.append(check,title,note);$('experience-features').append(row);featureInputs.set(id,check);
 }
 for(const [name,pin] of PADS){
  const row=document.createElement('div');row.className='mapping-row';
  const tag=document.createElement('strong');tag.textContent=name+' · GPIO'+pin;
  const select=document.createElement('select');select.setAttribute('aria-label',name+' action');
  for(const [id,title] of ACTIONS)select.add(new Option(title,id));
  const input=document.createElement('input');input.maxLength=64;input.placeholder='e.g. Ctrl+Z, Space, CC 7';input.setAttribute('aria-label',name+' key or MIDI mapping');
  const update=()=>{
   config.bindings[name]={kind:select.value,value:input.value};
   save();render();
  };
  select.onchange=update;input.onchange=update;
  row.append(tag,select,input);$('mapping-grid').append(row);padInputs.set(name,{kind:select,value:input});
 }
 $('experience-preset').add(new Option('Custom','custom'));
 for(const id of Object.keys(PRESETS))$('experience-preset').add(new Option(id==='display'?'Business display':id==='pet'?'Pet only':id==='dnd'?'DND player':id==='maker'?'Everything / Maker':'Factory Classic',id));
 $('experience-preset').onchange=()=>{
  const id=$('experience-preset').value;
  config={...config,preset:id,features:id==='custom'?config.features:[...PRESETS[id]],simple:['pet','dnd','display'].includes(id)};
  save();render();
 };
 $('experience-simple').onchange=()=>{config.simple=$('experience-simple').checked;save();render();};
 $('experience-bridge').onchange=()=>{config.bridge=$('experience-bridge').checked;save();render();};
 $('experience-start').onchange=()=>{config.home=$('experience-start').value;save();render();};
 $('experience-reset').onclick=()=>{
  if(!confirm('Restore the factory Web Studio layout and clear this browser\'s control mappings? Jar data is unaffected.'))return;
  config=validateConfig(DEFAULT_CONFIG);save();render();message('Factory Web Studio layout restored. Jar unchanged.');
 };
 $('experience-export').onclick=()=>{
  const blob=new Blob([JSON.stringify(config,null,2)+'\n'],{type:'application/json'});
  const url=URL.createObjectURL(blob),a=document.createElement('a');
  a.href=url;a.download='happyjarz-experience.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
 };
 $('experience-import').onchange=async event=>{
  const file=event.target.files?.[0];if(!file)return;
  try{
   if(file.size>20000)throw Error('Profile too large (20 KB maximum).');
   const data=JSON.parse(await file.text());
   if(!data||!Array.isArray(data.features)||!data.bindings)throw Error('Invalid experience profile.');
   config=validateConfig(data);save();render();message('Browser experience profile imported. Jar unchanged.');
  }catch(error){message('Import rejected: '+error.message,'error');}
  finally{event.target.value='';}
 };
 const quickProfiles={
  gaming:{UP:['GAMEPAD','D-pad Up'],DOWN:['GAMEPAD','D-pad Down'],LEFT:['GAMEPAD','D-pad Left'],RIGHT:['GAMEPAD','D-pad Right'],A:['GAMEPAD','A'],B:['GAMEPAD','B'],L:['GAMEPAD','LB'],R:['GAMEPAD','RB'],START:['GAMEPAD','Start']},
  daw:{UP:['KEYBOARD','Arrow Up'],DOWN:['KEYBOARD','Arrow Down'],LEFT:['KEYBOARD','Arrow Left'],RIGHT:['KEYBOARD','Arrow Right'],A:['KEYBOARD','Space'],B:['KEYBOARD','Escape'],L:['KEYBOARD','Ctrl+Z'],R:['KEYBOARD','Ctrl+Shift+Z'],START:['MIDI','Transport Play']},
  display:{UP:['SAVER PREV',''],DOWN:['SAVER NEXT',''],LEFT:['SAVER PREV',''],RIGHT:['SAVER NEXT',''],A:['SAVER ENTER',''],B:['SAVER EXIT',''],L:['SAVER RESEED',''],R:['SAVE',''],START:['NONE','']}
 };
 $('mapping-preset').onchange=()=>{
  const value=$('mapping-preset').value;
  if(!quickProfiles[value])return;
  config.bindings=Object.fromEntries(PADS.map(([n])=>{const [kind,v]=quickProfiles[value][n];return [n,{kind,value:v}];}));
  save();render();message('Mapping preset saved in browser. Future HID/MIDI mappings are not active yet.');
 };
 const safeCommands=new Set(['SAVER NEXT','SAVER PREV','SAVER ENTER','SAVER EXIT','SAVER RESEED','SAVE']);
 return {
  receive({type,data}){
   if(type==='IDENTITY'){boardId=data.chip||data.serial||'V1';status();}
   if(type!=='EVENT'||!config.bridge||!ctx.identity||ctx.serviceBusy||ctx.transferring||ctx.connecting||ctx.closing||busy)return;
   // Never echo actions from app KEY commands back into the jar. Physical presses only.
   if(data.source!=='physical' && data.source!=='touch')return;
   const pad=String(data.input||'').toUpperCase();
   const command=config.bindings[pad]?.kind;
   if(!safeCommands.has(command)||!ctx.deviceControls.has(pad.toLowerCase()))return;
   const now=performance.now();if(now-lastEventAt<220)return;lastEventAt=now;busy=true;
   ack(command).then(()=>{bindingState.textContent='Last live action: '+pad+' -> '+command+' (jar acknowledged).';})
    .catch(error=>{log('Mapped input failed: '+error.message,'warn','EXPERIENCE');})
    .finally(()=>{busy=false;});
  },
  reset(){boardId=null;status();},
  onShow(){status();},
  getConfiguration(){return structuredClone(config);}
 };
}
