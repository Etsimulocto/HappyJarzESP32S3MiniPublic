import { pixelIndex, artChunks, importDrawing } from '../protocol.mjs';
export function init(ctx) {
  const { $, ack, query, request, action, controls, message, log } = ctx;
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
  ctx.transferring=true;controls();$('art-progress').hidden=false;$('art-progress').value=0;
  try {
    const ready=await request('ART READY',p=>p.type==='ART' && p.data.event==='READY');
    if(ready.data.ready!=='1')throw new Error('Art storage is unavailable. This draft will not format it.');
    for(const entry of commands){await request(entry.command,p=>p.type==='ART' && p.data.event===entry.event && Number(p.data.slot)===entry.slot && (entry.offset===undefined || Number(p.data.offset)===entry.offset));if(entry.offset)$('art-progress').value=entry.offset;}
    message(`Two frames saved in jar slot ${slot+1}. Choose the Art Gallery screensaver on the jar.`);
  } finally {ctx.transferring=false;controls();}
});

renderArt();
return {};
}
