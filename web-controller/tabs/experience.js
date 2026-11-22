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
 ['pet','BLOOMPETZ (on-device pet game)'],['dnd','DND (on-device dungeon game)'],
 ['hid','Native USB HID/MIDI (not installed)']
];
export const PRESETS={
 classic:['home','setup','display','profile','art','service','updates','arcade'],
 pet:['home','setup','display','profile','service','updates','pet'],
 dnd:['home','setup','display','profile','service','updates','dnd'],
 display:['home','setup','display','art','service','updates'],
 maker:FEATURES.map(x=>x[0])
};
export const ACTIONS=[
 ['NONE','Unassigned'],['SAVER NEXT','Next screensaver'],['SAVER PREV','Previous screensaver'],
 ['SAVER ENTER','Start screensaver'],['SAVER EXIT','Exit screensaver'],
 ['SAVER RESEED','New screensaver variation'],['SAVER SPEED UP','Screensaver faster'],['SAVER SPEED DOWN','Screensaver slower'],['SAVE','Save jar settings'],['SET PATTERN','LED pattern'],['SET BRIGHTNESS','Brightness percent'],
 ['KEYBOARD','Keyboard assignment (saved only)'],['MOUSE','Mouse assignment (saved only)'],
 ['GAMEPAD','Virtual gamepad assignment (saved only)'],
 ['MIDI','MIDI assignment (saved only)']
];
const SLOTS=4;
export const ARGUMENTS={
 'SET PATTERN':['SOLID','FADE','PULSE','RAINBOW','RANDOM','BREATH','AURORA','OCEAN','LAVENDER','SUNSET','GALAXY','FIRE','ICE','FOREST','NEON','ARCADE','OFF'],
 'SET BRIGHTNESS':Array.from({length:11},(_,i)=>String(i*5)),
 'KEYBOARD':['Ctrl','Shift','Alt','Space','Enter','Escape','Tab','Backspace','Arrow Up','Arrow Down','Arrow Left','Arrow Right',...'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'.split(''),'F1','F2','F3','F4','F5','F6','F7','F8','F9','F10','F11','F12'],
 'MOUSE':['Left click','Right click','Middle click','Back','Forward','Scroll up','Scroll down'],
 'GAMEPAD':['A','B','X','Y','LB','RB','LT','RT','Back','Start','Guide','D-pad Up','D-pad Down','D-pad Left','D-pad Right','Left Stick Click','Right Stick Click','Left Axis Up','Left Axis Down','Left Axis Left','Left Axis Right','Right Axis Up','Right Axis Down','Right Axis Left','Right Axis Right'],
 'MIDI':['Note C3','Note D3','Note E3','Note F3','Note G3','Note A3','Note B3','CC 1','CC 7','CC 10','CC 64','Transport Play','Transport Stop']
};
const DEFAULT_SLOT=()=>({kind:'NONE',value:''});
const DEFAULT_BINDINGS=Object.fromEntries(PADS.map(([name])=>[name,Array.from({length:SLOTS},DEFAULT_SLOT)]));
function cleanSlot(x){
 const kind=ACTION_SET.has(x?.kind)?x.kind:'NONE';
 const choices=ARGUMENTS[kind]||[];
 return {kind,value:choices.includes(x?.value)?x.value:choices[0]||''};
}
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
  const source=input?.bindings?.[name];const slots=Array.isArray(source)?source:[source||{}];
  clean.bindings[name]=Array.from({length:SLOTS},(_,i)=>cleanSlot(slots[i]||{}));
 }
 return clean;
}
export function init(ctx){
 const {$,ack,message,log}=ctx;
 const key='happyjarz-experience-v1';
 let config=DEFAULT_CONFIG,storageOK=true,lastEventAt=0,busy=false,boardId=null,firmwareVersion=null,artCount=null;
 try{config=validateConfig(JSON.parse(localStorage.getItem(key)||'null'));}catch(e){storageOK=false;config=validateConfig(null);}
 const navIds=['home','setup','display','profile','art','service','updates'];
 const page=$('experience');
 const state=$('experience-state');
 const bindingState=$('mapping-state');
 const padInputs=new Map();
 const featureInputs=new Map();
 const MENU_ITEMS=['CLOCK','LIGHTS','GAMES','SETTINGS','POWER','SYSTEM','INFO','PROFILE'];
 const MENU_NAMES=['Clock','Lights','Factory Arcade','Alarm + Timer','Power','System (always available)','Info','Profile'];
 const deviceMenuChecks=[];
 let petModuleKnown=false,petModuleVisible=false,dndModuleKnown=false,dndModuleVisible=false,moduleReport=null;
 const featureNotes=new Map();
 function firmwareAtLeast(major,minor,patch){
   const fw=firmwareVersion;
   if(!fw || !/^\d+\.\d+\.\d+$/.test(fw))return false;
   const [a,b,c]=fw.split('.').map(Number);
   return a>major || (a===major && (b>minor || (b===minor && c>=patch)));
 }
 function supportsPetModule(){return firmwareAtLeast(0,18,16);}
 function supportsDndModule(){return firmwareAtLeast(0,18,17);}
 function updateDndModule(){
   const supported=!!ctx.identity && supportsDndModule();
   for(const id of ['dnd-module-read','dnd-module-on','dnd-module-off'])
     $(id).disabled=!supported||ctx.serviceBusy||ctx.transferring||ctx.connecting||ctx.closing;
   $('dnd-module-status').textContent=!ctx.identity?'Connect a jar to check DND.'
     :!supported?'DND is not reported by this firmware; 0.18.17+ required.'
     :!dndModuleKnown?'Read DND to check the installed module and OLED menu visibility.'
     :'Installed: yes. DND OLED menu '+(dndModuleVisible?'visible':'hidden')+'. Game save is retained when hidden.';
 }
 async function dndAction(command){
   if(!supportsDndModule()||!ctx.identity)throw Error('DND requires firmware 0.18.17+.');
   await deviceMenuWork(async()=>{
     if(command!=='GET DND MODULE'){
       if(!confirm('Change only DND OLED menu visibility? Your saved dungeon remains intact.'))return;
       await ack(command,'SET DND MODULE');
     }
     const packet=await ctx.query('GET DND MODULE','DNDMODULE');
     if(String(packet.data.installed)!=='1')throw Error('DND not installed on this jar.');
     dndModuleVisible=String(packet.data.visible)==='1';
     dndModuleKnown=true;updateDndModule();updateFeatureNotes();
   });
 }
 $('dnd-module-read').onclick=()=>void dndAction('GET DND MODULE').catch(e=>message('DND module: '+e.message,'error'));
 $('dnd-module-on').onclick=()=>void dndAction('SET DND MODULE ON').catch(e=>message('DND module: '+e.message,'error'));
 $('dnd-module-off').onclick=()=>void dndAction('SET DND MODULE OFF').catch(e=>message('DND module: '+e.message,'error'));

 function updatePetModule(){
   const supported=!!ctx.identity && supportsPetModule();
   for(const id of ['pet-module-read','pet-module-on','pet-module-off'])
     $(id).disabled=!supported||ctx.serviceBusy||ctx.transferring||ctx.connecting||ctx.closing;
   $('pet-module-status').textContent=!ctx.identity?'Connect a jar to check for BLOOMPETZ.'
     :!supported?'BLOOMPETZ pet module requires candidate firmware 0.18.16+.'
     :!petModuleKnown?'Tap Read BLOOMPETZ to check on-device installation.'
     :'Installed: yes (3 pet slots, 200 stats). Menu '+(petModuleVisible?'visible':'hidden')+'. Data is retained if hidden.';
 }
 async function petAction(command){
   if(!supportsPetModule()||!ctx.identity)throw Error('Requires 0.18.16+ firmware.');
   await deviceMenuWork(async()=>{
     if(command!=='GET PET MODULE'){
       if(!confirm('Change only BLOOMPETZ OLED menu visibility? Saved pet data stays intact.'))return;
       await ack(command,'SET PET MODULE');
     }
     const packet=await ctx.query('GET PET MODULE','PETMODULE');
     if(String(packet.data.installed)!=='1')throw Error('Pet module not installed.');
     petModuleVisible=String(packet.data.visible)==='1';
     petModuleKnown=true;
     updatePetModule();updateFeatureNotes();
   });
 }
 $('pet-module-read').onclick=()=>void petAction('GET PET MODULE').catch(e=>message('Pet module: '+e.message,'error'));
 $('pet-module-on').onclick=()=>void petAction('SET PET MODULE ON').catch(e=>message('Pet module: '+e.message,'error'));
 $('pet-module-off').onclick=()=>void petAction('SET PET MODULE OFF').catch(e=>message('Pet module: '+e.message,'error'));

 let deviceMenuMask=null,deviceMenuKnown=false;
 function supportsDeviceMenu(){
   const fw=firmwareVersion;
   if(!fw || !/^\d+\.\d+\.\d+$/.test(fw))return false;
   const [a,b,c]=fw.split('.').map(Number);
   return a>0 || (a===0 && (b>18 || (b===18 && c>=15)));
 }
 function updateDeviceMenu(){
   updatePetModule();updateDndModule();updateFeatureNotes();
   const supported=!!ctx.identity && supportsDeviceMenu();
   for(let i=0;i<8;i++){
     const check=deviceMenuChecks[i];if(!check)continue;
     check.disabled=!supported||i===5;
     if(i===5)check.checked=true;
     else if(deviceMenuKnown)check.checked=!!(deviceMenuMask & (1<<i));
   }
   for(const id of ['device-menu-read','device-menu-apply','device-menu-reset'])
     $(id).disabled=!supported || ctx.serviceBusy || ctx.transferring || ctx.connecting || ctx.closing;
   $('device-menu-status').textContent=!ctx.identity?'Connect a jar to configure the device menu.'
     :!supported?'Firmware '+firmwareVersion+' does not support on-device menu settings; 0.18.15+ candidate required.'
     :deviceMenuKnown?'Jar menu mask: '+deviceMenuMask+' (SYSTEM locked on). Changes below are drafts until Apply.'
     :'Firmware supports menu settings. Read the current jar menu first.';
 }
 for(let i=0;i<8;i++){
   const label=document.createElement('label');label.className='experience-choice';
   const check=document.createElement('input');check.type='checkbox';check.checked=true;
   const text=document.createElement('span');text.textContent=MENU_NAMES[i];
   const note=document.createElement('small');note.textContent='On-jar menu item '+(i+1);
   label.append(check,text,note);$('device-menu-choices').append(label);
   deviceMenuChecks.push(check);
 }
 async function deviceMenuWork(fn){
   if(!supportsDeviceMenu()||!ctx.identity)throw Error('This jar needs firmware 0.18.15+.');
   if(ctx.serviceBusy||ctx.transferring)throw Error('Another jar operation is running.');
   ctx.serviceBusy=true;ctx.controls();
   try{await fn();}finally{ctx.serviceBusy=false;ctx.controls();updateDeviceMenu();}
 }
 $('device-menu-read').onclick=()=>void deviceMenuWork(async()=>{
   const packet=await ctx.query('GET MODULES','MODULES');
   const mask=Number(packet.data.menu_mask);
   if(!Number.isInteger(mask)||mask<0||mask>255)throw Error('Invalid mask from jar.');
   deviceMenuMask=mask|32;deviceMenuKnown=true;moduleReport=packet.data;
   if(packet.data.dnd_visible!==undefined){dndModuleVisible=String(packet.data.dnd_visible)==='1';dndModuleKnown=String(packet.data.dnd)==='1';}
   updateDeviceMenu();
 }).catch(e=>message('Could not read menu: '+e.message,'error'));
 $('device-menu-apply').onclick=()=>void deviceMenuWork(async()=>{
   if(!deviceMenuKnown)throw Error('Read jar menu before changing it.');
   const mask=deviceMenuChecks.reduce((n,c,i)=>n|(c.checked||i===5?1<<i:0),0);
   if(!confirm('Apply the selected OLED menu visibility to this jar? SYSTEM remains available. Other features and data are not removed.'))return;
   await ack('SET MODULES '+mask,'SET MODULES');
   const packet=await ctx.query('GET MODULES','MODULES');
   if(Number(packet.data.menu_mask)!==(mask|32))throw Error('Jar returned an unexpected menu mask.');
   deviceMenuMask=mask|32;deviceMenuKnown=true;moduleReport=packet.data;updateDeviceMenu();
   message('On-device menu saved and verified.');
 }).catch(e=>message('Could not apply jar menu: '+e.message,'error'));
 $('device-menu-reset').onclick=()=>void deviceMenuWork(async()=>{
   if(!confirm('Restore all factory OLED menu entries, including any installed pet and DND menus? Saved games and artwork remain intact.'))return;
   await ack('RESET MODULES');
   const packet=await ctx.query('GET MODULES','MODULES');
   if(Number(packet.data.menu_mask)!==255)throw Error('Restore verification failed.');
   deviceMenuMask=255;deviceMenuKnown=true;moduleReport=packet.data;
   if(supportsPetModule()){petModuleVisible=true;petModuleKnown=true;}
   if(supportsDndModule()){dndModuleVisible=true;dndModuleKnown=true;}
   updateDeviceMenu();
   message('Original on-device menu restored.');
 }).catch(e=>message('Could not restore jar menu: '+e.message,'error'));

 const APP_TOTAL=0x140000, ART_TOTAL=0x160000;
 // Values come from measured Arduino output, never inferred from UI checkbox states.
 const MEASURED_APPS=Object.freeze({'0.18.14':1118144,'0.18.17':1169301});
 function kib(value){return (value/1024).toLocaleString('en-US',{maximumFractionDigits:1})+' KiB';}
 function updateStorage(){
  const measured=firmwareVersion && MEASURED_APPS[firmwareVersion];
  const remaining=measured!==undefined?APP_TOTAL-measured:null;
  $('storage-app-label').textContent=measured!==undefined
   ?kib(measured)+' used / '+kib(remaining)+' remaining'
   :firmwareVersion?'Firmware '+firmwareVersion+': no measured sketch size':'Connect a jar for build-specific figures';
  $('storage-app-fill').style.width=measured!==undefined?(100*measured/APP_TOTAL).toFixed(2)+'%':'0%';
  $('storage-app-meter').setAttribute('aria-valuenow',measured!==undefined?String(Math.round(100*measured/APP_TOTAL)):'0');
  $('storage-app-detail').textContent=measured!==undefined
   ?(100*measured/APP_TOTAL).toFixed(1)+'% of the '+kib(APP_TOTAL)+' application slot (compiled sketch measurement for '+firmwareVersion+'). Includes every compiled module; hiding menu items frees 0 KiB.'
   :'No verified build-size measurement for this installed firmware. Module checkboxes never remove compiled code.';
  $('storage-art-label').textContent=kib(ART_TOTAL)+' partition'+(artCount!==null?' · '+artCount+'/50 art slots':'');
  $('storage-art-fill').style.width=artCount===null?'0%':(artCount*2)+'%';
  $('storage-art-meter').setAttribute('aria-valuenow',String(artCount??0));
  $('storage-art-detail').textContent=artCount===null
   ?'Art slot occupancy not read. Firmware does not expose exact SPIFFS free bytes.'
   :artCount+' of 50 slots occupied. Picture payload can be 1,024–2,048 bytes per slot. Actual filesystem free bytes are not reported; other files and overhead are not included.';
 }
 function updateFeatureNotes(){
   for(const [id,note] of featureNotes){
     let label='Included in Web Studio · hiding saves 0 KiB';
     if(id==='arcade')label=ctx.identity?'On-device arcade included · hiding saves 0 KiB':'On-device arcade · connect jar to verify';
     if(id==='pet')label=petModuleKnown?'Installed on jar · OLED '+(petModuleVisible?'shown':'hidden')+' · 0 KiB freed'
       :supportsPetModule()?'Firmware supports BLOOMPETZ · select Read BLOOMPETZ to verify'
       :'Requires firmware 0.18.16+ · 0 KiB freed';
     if(id==='dnd')label=dndModuleKnown?'Installed on jar · OLED '+(dndModuleVisible?'shown':'hidden')+' · 0 KiB freed'
       :supportsDndModule()?'Firmware supports DND · select Read DND to verify'
       :'Requires firmware 0.18.17+ · 0 KiB freed';
     if(id==='hid')label='Native USB HID/MIDI not implemented · browser gamepad forwarding works';
     note.textContent=label;
     note.title='These checkboxes control browser layout only. Use the separate on-device menu controls to show or hide a game on the OLED.';
   }
 }

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
   ?'Mapping ON: jar commands run on physical button presses. Keyboard, mouse, virtual gamepad and MIDI assignments are saved only and do not send OS inputs.'
    :'Mapped shortcuts OFF. Copper buttons keep their original firmware behavior.';
 }
 function render(){
  $('experience-preset').value=config.preset;
  $('experience-simple').checked=config.simple;
  $('experience-bridge').checked=config.bridge;
  $('experience-start').value=config.home;
  for(const [id,input] of featureInputs)input.checked=config.features.includes(id);
  for(const [name,rows] of padInputs)for(let i=0;i<SLOTS;i++){
   const entry=config.bindings[name][i],obj=rows[i];obj.kind.value=entry.kind;
   populateArgs(obj.value,entry.kind,entry.value);
  }
  applyVisibility();status();updateStorage();updateDeviceMenu();
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
  featureNotes.set(id,note);
  row.append(check,title,note);$('experience-features').append(row);featureInputs.set(id,check);
 }
 function populateArgs(select,kind,selected){
  select.replaceChildren();const list=ARGUMENTS[kind]||[];
  if(!list.length){select.add(new Option('— no argument —',''));select.disabled=true;return;}
  for(const value of list)select.add(new Option(value,value));
  select.value=list.includes(selected)?selected:list[0];select.disabled=false;
 }
 for(const [name,pin] of PADS){
  const group=document.createElement('div');group.className='mapping-group';
  const label=document.createElement('strong');label.textContent=name+' · GPIO'+pin;group.append(label);
  const slots=[];
  for(let i=0;i<SLOTS;i++){
   const row=document.createElement('div');row.className='mapping-row';
   const n=document.createElement('span');n.textContent='SLOT '+(i+1);
   const kind=document.createElement('select');kind.setAttribute('aria-label',name+' slot '+(i+1)+' action');
   for(const [id,title] of ACTIONS)kind.add(new Option(title,id));
   const value=document.createElement('select');value.setAttribute('aria-label',name+' slot '+(i+1)+' argument');
   const change=()=>{config.bindings[name][i]=cleanSlot({kind:kind.value,value:value.value});save();render();};
   kind.onchange=()=>{populateArgs(value,kind.value,'');change();};value.onchange=change;
   row.append(n,kind,value);group.append(row);slots.push({kind,value});
  }
  $('mapping-grid').append(group);padInputs.set(name,slots);
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
 $('experience-start').onchange=()=>{config.home=$('experience-start').value;save();render();if(!config.simple||config.features.includes(config.home))document.querySelector('[data-tab="'+config.home+'"]')?.click();};
 $('storage-refresh').onclick=async()=>{
  if(!ctx.identity){message('Connect the jar before reading art storage.','warn');return;}
  $('storage-art-detail').textContent='Reading artwork count from jar...';
  try{
    const response=await ctx.request('ART COUNT',p=>p.type==='ART'&&p.event==='ACK'&&p.data.event==='COUNT',8000);
    const count=Number(response.data.count);
    if(!Number.isInteger(count)||count<0||count>50)throw Error('Invalid artwork slot count from firmware.');
    artCount=count;updateStorage();message('Artwork slot count received: '+count+' / 50.');
  }catch(error){artCount=null;updateStorage();message('Could not read art slots: '+error.message,'error');}
 };
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
 $('mapping-restore').onclick=()=>{
  if(!confirm('Restore all 36 button mapping slots to unassigned? This leaves the Studio layout, jar menu, saved games and artwork unchanged.'))return;
  config.bindings=Object.fromEntries(PADS.map(([name])=>[name,Array.from({length:SLOTS},DEFAULT_SLOT)]));
  config.bridge=false;
  save();render();$('mapping-preset').value='custom';
  message('Button mapping defaults restored: all 36 slots unassigned, mapped commands OFF. Jar unchanged.');
 };
 const quickProfiles={
  gaming:{UP:['GAMEPAD','D-pad Up'],DOWN:['GAMEPAD','D-pad Down'],LEFT:['GAMEPAD','D-pad Left'],RIGHT:['GAMEPAD','D-pad Right'],A:['GAMEPAD','A'],B:['GAMEPAD','B'],L:['GAMEPAD','LB'],R:['GAMEPAD','RB'],START:['GAMEPAD','Start']},
  daw:{UP:['KEYBOARD','Arrow Up'],DOWN:['KEYBOARD','Arrow Down'],LEFT:['KEYBOARD','Arrow Left'],RIGHT:['KEYBOARD','Arrow Right'],A:['KEYBOARD','Space'],B:['KEYBOARD','Escape'],L:['KEYBOARD','Ctrl+Z'],R:['KEYBOARD','Ctrl+Shift+Z'],START:['MIDI','Transport Play']},
  display:{UP:['SAVER PREV',''],DOWN:['SAVER NEXT',''],LEFT:['SAVER PREV',''],RIGHT:['SAVER NEXT',''],A:['SAVER ENTER',''],B:['SAVER EXIT',''],L:['SAVER RESEED',''],R:['SAVE',''],START:['NONE','']}
 };
 $('mapping-preset').onchange=()=>{
  const value=$('mapping-preset').value;
  if(!quickProfiles[value])return;
  config.bindings=Object.fromEntries(PADS.map(([n])=>{const [kind,v]=quickProfiles[value][n];return [n,[cleanSlot({kind,value:v}),...Array.from({length:SLOTS-1},DEFAULT_SLOT)]];}));
  save();render();message('Four-slot profile saved. Jar actions work now; OS output assignments are saved only.');
 };
 render();
 if(!config.simple||config.features.includes(config.home))document.querySelector('[data-tab="'+config.home+'"]')?.click();
 const safeCommands=new Set(['SAVER NEXT','SAVER PREV','SAVER ENTER','SAVER EXIT','SAVER RESEED','SAVER SPEED UP','SAVER SPEED DOWN','SAVE','SET PATTERN','SET BRIGHTNESS']);
 const commandFor=slot=>safeCommands.has(slot.kind)?(['SET PATTERN','SET BRIGHTNESS'].includes(slot.kind)?slot.kind+' '+slot.value:slot.kind):null;
 return {
  receive({type,data}){
   if(type==='IDENTITY'){boardId=data.chip||data.serial||'V1';firmwareVersion=data.fw||null;deviceMenuKnown=false;petModuleKnown=false;dndModuleKnown=false;moduleReport=null;status();updateStorage();updateDeviceMenu();}
   if(type==='MODULES'){moduleReport=data;const n=Number(data.menu_mask);if(Number.isInteger(n)){deviceMenuMask=n|32;deviceMenuKnown=true;}if(data.dnd_visible!==undefined){dndModuleKnown=String(data.dnd)==='1';dndModuleVisible=String(data.dnd_visible)==='1';}updateDeviceMenu();}
   if(type==='PETMODULE'){petModuleKnown=String(data.installed)==='1';petModuleVisible=String(data.visible)==='1';updateDeviceMenu();}
   if(type==='DNDMODULE'){dndModuleKnown=String(data.installed)==='1';dndModuleVisible=String(data.visible)==='1';updateDeviceMenu();}
   if(type!=='EVENT'||!config.bridge||!ctx.identity||ctx.serviceBusy||ctx.transferring||ctx.connecting||ctx.closing||busy)return;
   // Never echo actions from app KEY commands back into the jar. Physical presses only.
   if(data.source==='host')return; // Native firmware physical events omit source; host KEY events say source=host.
   const pad=String(data.input||'').toUpperCase();
   const commands=(config.bindings[pad]||[]).map(commandFor).filter(Boolean);
   if(!commands.length||!ctx.deviceControls.has(pad.toLowerCase()))return;
   const now=performance.now();if(now-lastEventAt<220)return;lastEventAt=now;busy=true;
   (async()=>{for(const command of commands){
    const response=command.startsWith('SET PATTERN ')?'SET PATTERN':command.startsWith('SET BRIGHTNESS ')?'SET BRIGHTNESS':command;
    await ack(command,response);
   }bindingState.textContent='Last mapped input: '+pad+' -> '+commands.join(' → ')+' (acknowledged).';})()
    .catch(error=>log('Mapped input failed: '+error.message,'warn','EXPERIENCE')).finally(()=>{busy=false;});
  },
  reset(){boardId=null;firmwareVersion=null;artCount=null;deviceMenuKnown=false;petModuleKnown=false;dndModuleKnown=false;moduleReport=null;status();updateStorage();updateDeviceMenu();},
  onShow(){status();updateStorage();updateDeviceMenu();},
  getConfiguration(){return structuredClone(config);}
 };
}
