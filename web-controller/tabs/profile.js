import { profileCommand } from '../protocol.mjs';
export function init(ctx) {
  const { $, ack, query, request, action, controls, message, log } = ctx;
for(let m=1;m<=12;m++) $('birth-month').add(new Option(new Date(2000,m-1,1).toLocaleString('en',{month:'long'}),String(m).padStart(2,'0')));
$('profile-form').onsubmit=action(async()=>{const cmd=profileCommand($('owner').value,$('birth-month').value,$('birth-day').value);await ack(cmd,'SET PROFILE');await ctx.syncClock();message('Profile saved on your jar and clock synced.');});

return { receive({type,data}) {
  if (type === 'PROFILE') {
    $('owner').value = data.owner || '';
    const parts = /^\d{2}-\d{2}$/.test(data.birthday || '') ? data.birthday.split('-') : ['', ''];
    $('birth-month').value = parts[0]; $('birth-day').value = parts[1];
  }

} };
}
