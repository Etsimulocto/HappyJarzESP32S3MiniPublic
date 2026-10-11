import {pixelIndex} from './protocol.mjs';
// Original hand-drawn monochrome icons; browser emoji are labels only.
const icons=[
 ['heart','❤️','Heart',['01100110','11111111','11111111','11111111','01111110','00111100','00011000']],
 ['star','⭐','Star',['0001000','0001000','1111111','0111110','0011100','0110110','1100011']],
 ['smile','🙂','Smile',['00111100','01000010','10100101','10000001','10100101','10011001','01000010','00111100']],
 ['wink','😉','Wink',['00111100','01000010','10100111','10000001','10100101','10011001','01000010','00111100']],
 ['cool','😎','Cool',['00111100','01000010','11111111','10100101','10000001','10011001','01000010','00111100']],
 ['ghost','👻','Ghost',['0011100','0111110','1101011','1101011','1111111','1100011','1111111','1010101']],
 ['cat','🐱','Cat',['1000001','1100011','1111111','1011101','1111111','0101010','0011100']],
 ['dog','🐶','Dog',['0111110','1111111','1101011','1101011','1111111','0110110','0011100']],
 ['paw','🐾','Paw',['0100010','1110111','0100010','0001000','0011100','0111110','0111110']],
 ['alien','👽','Alien',['00111100','01111110','11111111','10011001','11000011','01111110','00100100','00011000']],
 ['robot','🤖','Robot',['00011000','00011000','01111110','11000011','11011011','11000011','11011011','01111110']],
 ['skull','💀','Skull',['00111100','01111110','11011011','10011001','11111111','01111110','00111100','00100100']],
 ['flower','🌼','Flower',['0010100','0111110','1110111','0111110','0010100','0001000','0101000','0011000']],
 ['mushroom','🍄','Mushroom',['0011100','0111110','1101011','1111111','0001000','0011100','0011100']],
 ['butterfly','🦋','Butterfly',['1000001','1101011','1111111','0111110','0111110','1111111','1101011','1001001']],
 ['moon','🌙','Moon',['0001110','0011000','0110000','1100000','1100000','1110001','0111110','0011100']],
 ['sun','☀️','Sun',['1001001','0101010','0011100','1111111','0011100','0101010','1001001']],
 ['bolt','⚡','Lightning',['0001110','0011100','0111000','1111111','0001110','0011100','0110000']],
 ['note','🎵','Music',['0011111','0010001','0010001','0010001','0110011','1110111','1100110']],
 ['cake','🎂','Cake',['0101010','0101010','0111110','0101010','1111111','1000001','1111111']],
 ['gift','🎁','Gift',['0110110','0101010','1111111','1001001','1111111','1001001','1111111']],
 ['rocket','🚀','Rocket',['0001000','0011100','0111110','0101010','0111110','1111111','1001001','0010100']],
 ['jar','🫙','Happy jar',['0111110','0111110','0100010','1000001','1010101','1001001','0100010','0011100']],
 ['sparkle','✨','Sparkles',['0010000','0010001','1111100','0010010','0010111','0000010','0000000']],
];
export const STAMPS=icons.map(([id,emoji,name,rows])=>({id,emoji,name,rows}));
export function setPixel(frame,x,y,erase=false){x=Math.round(x);y=Math.round(y);if(x<0||y<0||x>=128||y>=64)return;const [byte,bit]=pixelIndex(x,y);erase?frame[byte]&=~bit:frame[byte]|=bit;}
export function stamp(frame,id,x,y,scale=1,erase=false){const icon=STAMPS.find(s=>s.id===id);if(!icon||!Number.isInteger(scale)||scale<1||scale>4)throw Error('Choose a stamp size from 1–4.');const width=Math.max(...icon.rows.map(r=>r.length))*scale,height=icon.rows.length*scale;const left=Math.round(x-width/2),top=Math.round(y-height/2);icon.rows.forEach((row,yy)=>[...row].forEach((bit,xx)=>{if(bit==='1')for(let dy=0;dy<scale;dy++)for(let dx=0;dx<scale;dx++)setPixel(frame,left+xx*scale+dx,top+yy*scale+dy,erase);}));}
export function line(frame,x0,y0,x1,y1,erase=false,size=1){x0=Math.round(x0);y0=Math.round(y0);x1=Math.round(x1);y1=Math.round(y1);const dx=Math.abs(x1-x0),sx=x0<x1?1:-1,dy=-Math.abs(y1-y0),sy=y0<y1?1:-1;let error=dx+dy;for(;;){for(let y=0;y<size;y++)for(let x=0;x<size;x++)setPixel(frame,x0+x-Math.floor(size/2),y0+y-Math.floor(size/2),erase);if(x0===x1&&y0===y1)break;const e2=2*error;if(e2>=dy){error+=dy;x0+=sx;}if(e2<=dx){error+=dx;y0+=sy;}}}
export function shape(frame,kind,a,b,filled=false,erase=false){const left=Math.min(a.x,b.x),right=Math.max(a.x,b.x),top=Math.min(a.y,b.y),bottom=Math.max(a.y,b.y),cx=(left+right)/2,cy=(top+bottom)/2;
 if(kind==='line'){line(frame,a.x,a.y,b.x,b.y,erase);return;}
 if(kind==='rectangle'){for(let y=top;y<=bottom;y++)for(let x=left;x<=right;x++)if(filled||x===left||x===right||y===top||y===bottom)setPixel(frame,x,y,erase);return;}
 if(kind==='ellipse'){const rx=(right-left)/2,ry=(bottom-top)/2;if(rx===0||ry===0){line(frame,left,top,right,bottom,erase);return;}if(filled){for(let y=top;y<=bottom;y++)for(let x=left;x<=right;x++)if(((x-cx)/rx)**2+((y-cy)/ry)**2<=1)setPixel(frame,x,y,erase);}else{let previous=null;for(let i=0;i<=360;i++){const point={x:Math.round(cx+rx*Math.cos(i*Math.PI/180)),y:Math.round(cy+ry*Math.sin(i*Math.PI/180))};if(previous)line(frame,previous.x,previous.y,point.x,point.y,erase);previous=point;}}return;}
 let points;
 if(kind==='triangle')points=[[cx,top],[right,bottom],[left,bottom]];
 else if(kind==='diamond')points=[[cx,top],[right,cy],[cx,bottom],[left,cy]];
 else if(kind==='star')points=Array.from({length:10},(_,i)=>{const angle=-Math.PI/2+i*Math.PI/5,r=i%2?.42:1;return[cx+Math.cos(angle)*(right-left)/2*r,cy+Math.sin(angle)*(bottom-top)/2*r];});
 else throw Error('Choose a supported shape.');
 if(filled)for(let y=top;y<=bottom;y++)for(let x=left;x<=right;x++){let inside=false;for(let i=0,j=points.length-1;i<points.length;j=i++){const [xi,yi]=points[i],[xj,yj]=points[j];if((yi>y)!==(yj>y)&&x<(xj-xi)*(y-yi)/(yj-yi)+xi)inside=!inside;}if(inside)setPixel(frame,x,y,erase);}
 points.forEach((p,i)=>{const q=points[(i+1)%points.length];line(frame,p[0],p[1],q[0],q[1],erase);});
}
