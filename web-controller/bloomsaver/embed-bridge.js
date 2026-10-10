(() => {
 if(!new URLSearchParams(location.search).has('embed'))return;
 const send=(type,message)=>parent.postMessage({type,message},location.origin);
 addEventListener('error',event=>send('bloomsaver-error',String(event.message||'Background resource failed')),true);
 addEventListener('unhandledrejection',event=>send('bloomsaver-error',String(event.reason?.message||event.reason)));
 addEventListener('keydown',event=>{if(event.key==='Escape')send('bloomsaver-close');});
})();
