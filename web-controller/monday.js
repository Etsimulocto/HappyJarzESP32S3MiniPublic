import {songLink,imageLink} from './media.mjs';
// Public display only. Publishing is controlled by repository write permissions.
export function init(ctx){
 const {$,log}=ctx;
 let media={song:'',title:'Monday’s HAPPY JARZ mix',images:[]};
 function render(){
  $('monday-song-title').textContent=media.title;
  $('monday-listen').hidden=!media.song;$('monday-listen').href=media.song||'#';$('monday-music-state').textContent=media.song?'Click to listen on Suno. No autoplay.':'Monday’s next track is on its way.';
  $('monday-gallery').replaceChildren();
  for(const [i,source] of media.images.entries()){
   const figure=document.createElement('figure'),image=document.createElement('img'),caption=document.createElement('figcaption');image.src=source;image.alt=`HAPPY JARZ gallery artwork ${i+1}`;image.loading='lazy';caption.textContent=i===0?'HAPPY JARZ · GLASS FULL OF STARS':`ART ${String(i+1).padStart(2,'0')}`;figure.append(image,caption);$('monday-gallery').append(figure);
  }
  $('monday-gallery-state').textContent=media.images.length?'A little gallery from the Architect.':'New artwork is on its way.';
 }
 async function load(){
  try{
   const response=await fetch('./monday-media.json',{cache:'no-store',signal:AbortSignal.timeout(10000)});if(!response.ok)throw Error('Monday’s media could not load.');
   const saved=await response.json();
   media={song:songLink(saved.song||''),title:String(saved.title||media.title).slice(0,100),images:(Array.isArray(saved.images)?saved.images:[]).slice(0,6).map(imageLink)};
  }catch(error){log(error.message,'warn','MEDIA');}render();
 }
 render();const ready=load();return {ready};
}
