import { pixelIndex, artChunks, importDrawing } from '../protocol.mjs';
import { STAMPS, stamp, line, shape } from '../art-tools.mjs';
export function init(ctx) {
  const { $, ack, query, request, action, controls, message, log } = ctx;
let frames=[new Uint8Array(1024),new Uint8Array(1024)], frame=0, previewTimer, painting=false;
// Keep drafts only on this browser; never transmit them without a user action.
const ART_STORAGE_KEY='happyjarz-art-v1';
let saveTimer, storageAvailable=true, restoreWarning='';
let stored=null;
try {
  stored=localStorage.getItem(ART_STORAGE_KEY);
}catch(error){
  storageAvailable=false;
  console.warn('Artwork storage cannot be read:',error.message);
}
if(stored!==null){
  try {
    frames=importDrawing(JSON.parse(stored));
  }catch(error){
    // Invalid saved art is not a storage failure. Back it up before allowing
    // future drawings to replace the unusable value.
    try {
      const backupKey=ART_STORAGE_KEY+'-corrupt';
      if(localStorage.getItem(backupKey)===null){
        localStorage.setItem(backupKey,stored);
        restoreWarning='Previous artwork could not be opened. A recovery copy was kept in this browser; new drawings will save normally.';
      }else{
        restoreWarning='Previous artwork could not be opened. An older recovery copy already exists; download a backup of new drawings.';
      }
    }catch(backupError){
      restoreWarning='Previous artwork could not be opened or backed up. Download new drawings for safekeeping.';
      console.warn('Artwork recovery copy unavailable:',backupError.message);
    }
    console.warn('Saved artwork is invalid:',error.message);
  }
}
function saveArtwork(){
  clearTimeout(saveTimer);
  if(!storageAvailable)return;
  try{
    localStorage.setItem(ART_STORAGE_KEY,JSON.stringify({format:'happyjarz-xbm-v1',frames:frames.map(f=>[...f])}));
    const state=document.getElementById('art-save-state');
    if(state)state.textContent='Drawing saved automatically in this browser.';
  }catch(error){
    storageAvailable=false;
    const state=document.getElementById('art-save-state');
    if(state)state.textContent='Browser storage unavailable. Download your drawing for safekeeping.';
    console.warn('Art save unavailable:',error.message);
  }
}
function scheduleArtworkSave(){clearTimeout(saveTimer);saveTimer=setTimeout(saveArtwork,350);}
window.addEventListener('pagehide',()=>{if(saveTimer)saveArtwork();});

function renderArt() {const ctx=$('art-canvas').getContext('2d'), image=ctx.createImageData(128,64);for(let y=0;y<64;y++)for(let x=0;x<128;x++){const [byte,bit]=pixelIndex(x,y);image.data.set(frames[frame][byte]&bit ? [168,231,214,255] : [6,11,21,255],(y*128+x)*4);}ctx.putImageData(image,0,0);$('frame0').setAttribute('aria-pressed',String(frame===0));$('frame1').setAttribute('aria-pressed',String(frame===1));}
function stopPreview(){clearInterval(previewTimer);previewTimer=null;$('preview').textContent='Play preview';}
const histories=[[],[]];let gesture=null,selectedStamp=STAMPS[0].id;
function remember(){histories[frame].push(frames[frame].slice());if(histories[frame].length>40)histories[frame].shift();}
function cancelGesture(){if(gesture){frames[frame]=gesture.base;gesture=null;painting=false;renderArt();}}
function point(event){const box=$('art-canvas').getBoundingClientRect();return {x:Math.max(0,Math.min(127,Math.floor((event.clientX-box.left)/box.width*128))),y:Math.max(0,Math.min(63,Math.floor((event.clientY-box.top)/box.height*64)))};}
function toolState(){const name=$('art-tool').value; $('art-tool-state').textContent=name==='stamp'?`Stamp: ${STAMPS.find(s=>s.id===selectedStamp).name}. Tap the canvas to place it.`:name==='pen'?'Draw freely. Eraser removes pixels.':'Drag on the canvas to draw a '+name+'.';}
for(const icon of STAMPS){const button=document.createElement('button');button.id='stamp-'+icon.id;button.type='button';button.textContent=icon.emoji;button.title=icon.name;button.setAttribute('aria-label',icon.name+' stamp');button.setAttribute('aria-pressed',String(icon.id===selectedStamp));button.onclick=()=>{cancelGesture();selectedStamp=icon.id;$('art-tool').value='stamp';for(const s of STAMPS)$('stamp-'+s.id).setAttribute('aria-pressed',String(s.id===selectedStamp));toolState();};$('art-stamps').append(button);}
function filterStamps(){const search=$('stamp-search').value.trim().toLowerCase(),category=$('stamp-category').value;let count=0;for(const icon of STAMPS){const show=(!category||category==='all'||icon.category===category)&&(!search||[icon.name,icon.id,icon.category,icon.emoji].join(' ').toLowerCase().includes(search));$('stamp-'+icon.id).hidden=!show;if(show)count++;}$('stamp-count').textContent=count+' of '+STAMPS.length+' stamps';}
$('stamp-search').oninput=filterStamps;$('stamp-category').onchange=filterStamps;$('stamp-category').value='all';filterStamps();
$('art-tool').onchange=()=>{cancelGesture();toolState();};
function drawGesture(p){
 const g=gesture;if(!g)return;
 if(g.tool==='pen'){line(frames[frame],g.last.x,g.last.y,p.x,p.y,g.erase,g.size);g.last=p;}
 else {frames[frame]=g.base.slice();if(g.tool==='stamp')stamp(frames[frame],g.stamp,p.x,p.y,g.scale,g.erase);else shape(frames[frame],g.tool,g.start,p,g.filled,g.erase);}
 renderArt();scheduleArtworkSave();
}
$('art-canvas').onpointerdown=event=>{
 if(gesture || (event.button!==undefined && event.button!==0))return;stopPreview();const p=point(event);painting=true;
 gesture={pointer:event.pointerId,base:frames[frame].slice(),start:p,last:p,tool:$('art-tool').value,erase:$('erase').checked,filled:$('shape-filled').checked,stamp:selectedStamp,scale:Math.max(1,Math.min(4,Number($('stamp-size').value)||1)),size:Math.max(1,Math.min(6,Number($('brush-size').value)||1))};
 $('art-canvas').setPointerCapture(event.pointerId);drawGesture(p);
};
$('art-canvas').onpointermove=event=>{if(gesture?.pointer===event.pointerId)drawGesture(point(event));};
$('art-canvas').onpointerup=event=>{if(gesture?.pointer!==event.pointerId)return;drawGesture(point(event));histories[frame].push(gesture.base);if(histories[frame].length>40)histories[frame].shift();gesture=null;painting=false;scheduleArtworkSave();};
$('art-canvas').onpointercancel=cancelGesture;
$('undo-art').onclick=()=>{cancelGesture();stopPreview();if(histories[frame].length){frames[frame]=histories[frame].pop();renderArt();scheduleArtworkSave();}};
$('art-tool').value='pen';toolState();
for(let i=0;i<2;i++) $('frame'+i).onclick=()=>{cancelGesture();stopPreview();frame=i;renderArt();};
$('preview').onclick=()=>{cancelGesture();if(previewTimer)stopPreview();else{$('preview').textContent='Stop preview';previewTimer=setInterval(()=>{frame=1-frame;renderArt();},400);}};
$('copy-frame').onclick=()=>{cancelGesture();stopPreview();histories[1-frame].push(frames[1-frame].slice());if(histories[1-frame].length>40)histories[1-frame].shift();frames[1-frame]=frames[frame].slice();scheduleArtworkSave();message('Copied to the other frame.');};
$('clear-frame').onclick=()=>{cancelGesture();stopPreview();remember();frames[frame].fill(0);renderArt();scheduleArtworkSave();};
$('export-art').onclick=()=>{const blob=new Blob([JSON.stringify({format:'happyjarz-xbm-v1',frames:frames.map(f=>[...f])})],{type:'application/json'});const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download='happyjarz-drawing.json';a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);};
$('import-art').onchange=action(async()=>{const file=$('import-art').files[0];if(!file)return;if(file.size>20000)throw new Error('Drawing file is too large.');const imported=importDrawing(JSON.parse(await file.text()));cancelGesture();for(let i=0;i<2;i++){histories[i].push(frames[i].slice());if(histories[i].length>40)histories[i].shift();}frames=imported;stopPreview();renderArt();saveArtwork();message('Drawing loaded.');});
$('upload-art').onclick=action(async()=>{
  const slot=Number($('art-slot').value)-1, commands=artChunks(frames.map(f=>f.slice()),slot);
  if(!confirm(`Replace art slot ${slot+1} on the jar with these two frames?`)) return;
  ctx.transferring=true;controls();$('art-progress').hidden=false;$('art-progress').value=0;
  try {
    const ready=await request('ART READY',p=>p.type==='ART' && p.data.event==='READY');
    if(ready.data.ready!=='1')throw new Error('Art storage is unavailable. The studio will not format it.');
    for(const entry of commands){await request(entry.command,p=>p.type==='ART' && p.data.event===entry.event && Number(p.data.slot)===entry.slot && (entry.offset===undefined || Number(p.data.offset)===entry.offset));if(entry.offset)$('art-progress').value=entry.offset;}
    message(`Two frames saved in jar slot ${slot+1}. Choose the Art Gallery screensaver on the jar.`);
  } finally {ctx.transferring=false;controls();}
});

renderArt();
const artSaveState=document.getElementById('art-save-state');
if(artSaveState){
  if(!storageAvailable)artSaveState.textContent='Browser storage unavailable. Download your drawing for safekeeping.';
  else if(restoreWarning)artSaveState.textContent=restoreWarning;
}
return {reset(){cancelGesture();stopPreview();}};
}
