import {rgb,rgbHex} from './lights.mjs';
export function init(ctx){
 const {$,log}=ctx,frame=$('bloom-background'),reduce=matchMedia('(prefers-reduced-motion: reduce)');
 let paused=reduce.matches,editing=false,ready=false,lastPattern='',pendingPalette;
 try{const value=localStorage.getItem('happyjarz.background.paused');if(value!==null)paused=value==='1';}catch{}
 function send(){if(ready)frame.contentWindow.postMessage({type:'studio-background',paused:paused||document.hidden,editing},location.origin);$('background-pause').textContent=paused?'Resume glitter':'Pause glitter';}
 function close(){editing=false;frame.classList.remove('editing');frame.tabIndex=-1;frame.setAttribute('aria-hidden','true');$('background-close').hidden=true;send();$('background-edit').focus();}
 $('background-edit').onclick=()=>{editing=true;frame.classList.add('editing');frame.tabIndex=0;frame.setAttribute('aria-hidden','false');$('background-close').hidden=false;send();frame.focus();};
 $('background-close').onclick=close;
 $('background-pause').onclick=()=>{paused=!paused;try{localStorage.setItem('happyjarz.background.paused',paused?'1':'0');}catch{}send();};
 window.addEventListener('keydown',event=>{if(editing&&event.key==='Escape')close();});
 document.addEventListener('visibilitychange',send);
 window.addEventListener('message',event=>{
  if(event.origin!==location.origin||event.source!==frame.contentWindow)return;
  if(event.data?.type==='bloomsaver-ready'){ready=true;send();if(pendingPalette)frame.contentWindow.postMessage({type:'happyjarz-palette',palette:pendingPalette},location.origin);}
  if(event.data?.type==='bloomsaver-error')log(event.data.message,'error','BACKGROUND');
  if(event.data?.type==='bloomsaver-close')close();
 });
 frame.addEventListener('load',()=>{ready=true;send();if(pendingPalette)frame.contentWindow.postMessage({type:'happyjarz-palette',palette:pendingPalette},location.origin);});
 try{ready=frame.contentDocument?.readyState==='complete';}catch{}
 frame.addEventListener('error',()=>log('BloomSaver frame could not load.','error','BACKGROUND'));send();
 return {receive({type,data}){
  if(type!=='LED_FRAME' || data.pattern===lastPattern)return;
  const colors=[1,2,3,4].map(i=>rgb(data['led'+i]??data['live_led'+i])).filter(c=>c?.some(n=>n>0)).map(rgbHex);if(!colors.length)return;
  lastPattern=data.pattern;pendingPalette={name:data.pattern,colors};if(ready)frame.contentWindow.postMessage({type:'happyjarz-palette',palette:pendingPalette},location.origin);
 },reset(){lastPattern='';}};
}
