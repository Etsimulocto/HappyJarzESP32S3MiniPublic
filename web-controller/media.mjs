export function songLink(value){
 if(!value?.trim())return '';
 const url=new URL(value.trim());
 if(url.protocol!=='https:' || !['suno.com','www.suno.com'].includes(url.hostname) || !/^\/(song|playlist|s)\/[A-Za-z0-9_-]+\/?$/.test(url.pathname) || url.username || url.password)throw Error('Paste an HTTPS Suno song, playlist or share link.');
 return url.href;
}
export function imageLink(value){
 if(!value)return '';
 if(/^data:image\/(png|jpeg|webp);base64,[A-Za-z0-9+/=]+$/.test(value))return value;
 if(value.startsWith('./assets/') && !value.includes('..'))return value;
 throw Error('Published artwork must use ./assets/ paths; local artwork must be a PNG, JPEG or WebP.');
}
