
export function init(ctx) {
  const { $, ack, query, request, action, controls, message, log } = ctx;
$('set-saver').onclick=action(async()=>{await ack('SET SAVER MODE '+$('saver-mode').value,'SET SAVER MODE');message('Screensaver applied.');});
$('save-saying').onclick=action(async()=>{const slot=Number($('saying-slot').value),text=$('saying-text').value.trim();if(!Number.isInteger(slot)||slot<1||slot>50||!text||/[|\r\n]/.test(text)||new TextEncoder().encode(text).length>96)throw new Error('Choose slot 1–50 and a single line of up to 96 UTF-8 bytes without |.');await ack(`SET CUSTOM SAYING ${slot} ${text}`,'SET CUSTOM SAYING');message('Marquee saying saved.');});
$('set-saying-source').onclick=action(async()=>{await ack('SET SAYING SOURCE '+$('saying-source').value,'SET SAYING SOURCE');message('Saying source applied.');});

return { receive({type,data}) {
  if(type==='SAVER')$('saver-state').textContent=Object.entries(data).map(([k,v])=>`${k}=${v}`).join(' · ');

} };
}
