import {songLink,imageLink} from './media.mjs';
export function init(ctx){
 const {$,action,message,log}=ctx,key='happyjarz.monday.media.v1';
 let media={song:'',title:'Monday’s HAPPY JARZ mix',images:[]},storage,revision=0;
 try{storage=localStorage;}catch{}
 function render(){
  $('monday-song-title').textContent=media.title;$('monday-song-url').value=media.song;$('monday-song-name').value=media.title;
  $('monday-listen').hidden=!media.song;$('monday-listen').href=media.song||'#';$('monday-music-state').textContent=media.song?'Click to listen on Suno. No autoplay.':'Add Monday’s song or playlist link below.';
  $('monday-gallery').replaceChildren();
  for(const [i,source] of media.images.entries()){
   const figure=document.createElement('figure'),image=document.createElement('img'),caption=document.createElement('figcaption');image.src=source;image.alt=`Monday’s gallery artwork ${i+1}`;image.loading='lazy';caption.textContent=`ART ${String(i+1).padStart(2,'0')}`;figure.append(image,caption);$('monday-gallery').append(figure);
  }
  $('monday-gallery-state').textContent=media.images.length?`${media.images.length}/6 images · saved on this browser when storage permits`:'Your Monday artwork goes here. Add up to six images.';
 }
 function persist(){revision++;try{storage?.setItem(key,JSON.stringify(media));if(!storage)throw Error();$('monday-media-state').textContent='Saved on this browser. Published defaults are in monday-media.json.';}catch{$('monday-media-state').textContent='Browser storage unavailable/full. Changes work for this session; keep your original files.';log('Monday media persistence unavailable.','warn','MEDIA');}}
 $('monday-song-form').onsubmit=action(()=>{media.song=songLink($('monday-song-url').value);media.title=$('monday-song-name').value.trim().slice(0,100)||'Monday’s HAPPY JARZ mix';persist();render();message('Monday’s music link updated.');});
 $('monday-images').onchange=action(async()=>{
  const files=Array.from($('monday-images').files||[]);if(files.length+media.images.length>6)throw Error('The gallery holds six images. Clear it before replacing the set.');
  const added=[];
  for(const file of files){
   if(!['image/png','image/jpeg','image/webp'].includes(file.type)||file.size>12000000)throw Error('Use PNG/JPEG/WebP images under 12 MB.');
   const bitmap=await createImageBitmap(file);try{
    const canvas=document.createElement('canvas'),scale=Math.min(1,1000/Math.max(bitmap.width,bitmap.height));canvas.width=Math.max(1,Math.round(bitmap.width*scale));canvas.height=Math.max(1,Math.round(bitmap.height*scale));canvas.getContext('2d').drawImage(bitmap,0,0,canvas.width,canvas.height);
    const data=canvas.toDataURL('image/webp',.8);if(data.length>450000)throw Error('That image is too detailed for local gallery storage. Try a smaller copy.');added.push(imageLink(data));
   }finally{bitmap.close();}
  }
  media.images.push(...added);persist();render();$('monday-images').value='';message('Artwork added to Monday’s gallery.');
 });
 $('monday-clear-images').onclick=()=>{if(confirm('Clear this browser’s Monday gallery? Your original files are kept.')){media.images=[];persist();render();}};
 async function load(){
  const initialRevision=revision;
  try{
   let saved;try{saved=JSON.parse(storage?.getItem(key)||'null');}catch{}
   if(!saved){const response=await fetch('./monday-media.json',{cache:'no-store'});if(!response.ok)throw Error('Monday media defaults unavailable.');saved=await response.json();}
   if(revision!==initialRevision)return;
   media={song:songLink(saved.song||''),title:String(saved.title||media.title).slice(0,100),images:(Array.isArray(saved.images)?saved.images:[]).slice(0,6).map(imageLink)};
  }catch(error){log(error.message,'warn','MEDIA');}render();
 }
 render();void load();return {};
}
