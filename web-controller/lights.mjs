export const INPUT_KEYS=['up','down','left','right','a','b','x','y','l','r','start','select'];
export function rgb(value){const out=Array.isArray(value)?value:typeof value==='string' && /^\d{1,3},\d{1,3},\d{1,3}$/.test(value)?value.split(',').map(Number):null;return out?.length===3 && out.every(n=>Number.isInteger(n)&&n>=0&&n<=255)?out:null;}
export function rgbHex(value){const color=rgb(value);if(!color)throw new Error('Invalid RGB color.');return '#'+color.map(n=>n.toString(16).padStart(2,'0')).join('');}
export function hexRgb(hex){if(!/^#[0-9a-f]{6}$/i.test(hex))throw new Error('Invalid color.');return [1,3,5].map(i=>parseInt(hex.slice(i,i+2),16));}
export function brightness(value){const n=Number(value);if(!Number.isFinite(n))throw new Error('Invalid brightness.');return Math.max(0,Math.min(50,Math.round(n)));}
export function capabilities(raw){return new Set(String(raw||'').toLowerCase().split(',').map(s=>s.trim()).filter(k=>INPUT_KEYS.includes(k)));}
export function gamepadKeys(pad){
  const keys=new Set();if(!pad || pad.mapping!=='standard')return keys;
  const buttons={0:'a',1:'b',2:'x',3:'y',4:'l',5:'r',8:'select',9:'start',12:'up',13:'down',14:'left',15:'right'};
  for(const [i,key] of Object.entries(buttons))if(pad.buttons?.[i]?.pressed)keys.add(key);
  if(pad.axes?.[0]<-.4)keys.add('left');if(pad.axes?.[0]>.4)keys.add('right');
  if(pad.axes?.[1]<-.4)keys.add('up');if(pad.axes?.[1]>.4)keys.add('down');return keys;
}
const luma=c=>.2126*c[0]+.7152*c[1]+.0722*c[2];
const distance=(a,b)=>Math.hypot(...a.map((n,i)=>n-b[i]));
const scale=(c,n)=>c.map(v=>Math.max(0,Math.min(255,Math.round(v*n))));
export function patternTheme(frames){
  const samples=frames.flat().filter(c=>rgb(c)&&luma(c)>=18).sort((a,b)=>luma(b)-luma(a));
  const unique=[];for(const c of samples)if(unique.every(old=>distance(c,old)>=45))unique.push(c);
  if(!unique.length)return null;
  const primary=unique[0],secondary=unique.length>1 ? unique.slice(1).sort((a,b)=>distance(primary,b)-distance(primary,a))[0] : [primary[1],primary[2],primary[0]];
  let text=secondary;if(luma(text)<115)text=scale(text,Math.min(2.8,115/Math.max(1,luma(text))));
  // Preserve Pi's pattern palette, while keeping browser text contrast readable.
  const bg=scale(primary,.22),panel=scale(secondary,.30);
  const linear=n=>{n/=255;return n<=.04045?n/12.92:((n+.055)/1.055)**2.4;};
  const luminance=c=>.2126*linear(c[0])+.7152*linear(c[1])+.0722*linear(c[2]);
  if((luminance(text)+.05)/(Math.max(luminance(bg),luminance(panel))+.05)<4.5)text=scale(text.map(v=>v*.25+191),1);
  return {bg:rgbHex(bg),panel:rgbHex(panel),border:rgbHex(primary),text:rgbHex(text),accent:rgbHex(primary),secondary:rgbHex(secondary)};
}
