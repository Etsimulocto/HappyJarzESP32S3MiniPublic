
export function init(ctx) {
  const { $, ack, query, request, action, controls, message, log } = ctx;
async function syncClock() {await ack('SET TIMEZONE '+$('timezone').value,'SET TIMEZONE');await ack('SET CLOCK UNIX '+Math.floor(Date.now()/1000),'SET CLOCK UNIX');await ack('SAVE');}
$('sync-clock').onclick=action(async()=>{await syncClock();message('Clock synced and time zone saved.');});
$('wifi-form').onsubmit=action(async()=>{
  const ssid=$('wifi-ssid').value,password=$('wifi-password').value;
  if(!ssid || !password || new TextEncoder().encode(ssid).length>32 || new TextEncoder().encode(password).length>63 || /[\r\n]/.test(ssid+password))throw new Error('Enter SSID and password, check their lengths and remove line breaks. Empty passwords are not supported by the current firmware command.');
  await ack('SET WIFI SSID '+ssid,'SET WIFI SSID');
  await ack('SET WIFI PASSWORD '+password,'SET WIFI PASSWORD');
  await ack('SAVE');await ack('WIFI CONNECT','WIFI CONNECT');$('wifi-password').value='';await query('GET WIFI STATUS','WIFI');message('Wi-Fi saved; status read from jar.');
});
$('set-alarm').onclick=action(async()=>{const time=$('alarm-time').value;if(!/^\d{2}:\d{2}$/.test(time))throw new Error('Choose an alarm time.');await ack('SET ALARM '+time,'SET ALARM');const command='SET ALARM '+($('alarm-enabled').checked?'ON':'OFF');await ack(command);await query('GET ALARM','ALARM');message('Alarm applied. Use Save settings to persist it.');});
$('set-timer').onclick=action(async()=>{const minutes=Number($('timer-minutes').value);if(!Number.isInteger(minutes)||minutes<1||minutes>10080)throw new Error('Timer must be 1–10080 minutes.');await ack('SET TIMER MINUTES '+minutes,'SET TIMER MINUTES');await query('GET TIMER','TIMER');message('Timer started.');});

return { syncClock, receive({type,data}) {
  if(type==='WIFI')$('wifi-state').textContent=`${data.state} · ${data.ssid || 'No network'} · signal ${data.rssi || '—'} dBm`;
  if(type==='TIME'){$('time-state').textContent=`${data.local || 'Clock not synced'} · ${data.timezone || '—'}`;if([...$('timezone').options].some(o=>o.value===data.timezone))$('timezone').value=data.timezone;}
  if(type==='ALARM' || type==='TIMER')$('schedule-state').textContent=type+': '+Object.entries(data).map(([k,v])=>`${k}=${v}`).join(' · ');

} };
}
