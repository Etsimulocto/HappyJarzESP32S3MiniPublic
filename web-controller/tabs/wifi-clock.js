const ZONES = ['America/Chicago','America/New_York','America/Denver','America/Los_Angeles','UTC'];
export function init(ctx) {
  const { $, ack, query, request, action, controls, message } = ctx;
  let networks = [], scanning = false;
  function zone(value) {
    if (!ZONES.includes(value)) throw new Error('Choose a time zone supported by this firmware.');
    return value;
  }
  function mirrorZone(value) { $('timezone').value=value; $('clock-timezone').value=value; }
  $('clock-timezone').value=$('timezone').value;
  $('clock-timezone').onchange=()=>mirrorZone($('clock-timezone').value);
  $('timezone').onchange=()=>mirrorZone($('timezone').value);
  async function syncClock() {
    await ack('SET TIMEZONE '+zone($('timezone').value),'SET TIMEZONE');
    await ack('SET CLOCK UNIX '+Math.floor(Date.now()/1000),'SET CLOCK UNIX');
    await ack('SAVE'); await query('GET TIME STATUS','TIME');
  }
  const sync=action(async()=>{await syncClock();message('Clock synced from this computer; time zone saved.');});
  $('sync-clock').onclick=sync; $('sync-host-clock').onclick=sync;
  $('set-timezone').onclick=action(async()=>{
    await ack('SET TIMEZONE '+zone($('clock-timezone').value),'SET TIMEZONE');
    await ack('SAVE');await query('GET TIME STATUS','TIME');message('Time zone saved.');
  });
  $('sync-internet-clock').onclick=action(async()=>{
    await ack('TIME SYNC'); await query('GET TIME STATUS','TIME');
    message('Internet time requested. Synchronization can take a moment; read the jar clock again to verify it.');
  });
  $('show-password').onchange=()=>{$('wifi-password').type=$('show-password').checked?'text':'password';};
  $('scan-wifi').onclick=action(async()=>{
    if(scanning)return;
    scanning=true;ctx.serviceBusy=true;controls();networks=[];$('wifi-networks').replaceChildren();$('wifi-scan-state').textContent='Scanning on the jar…';
    try {
      await request('SCAN WIFI',p=>p.type==='WIFI_SCAN' && p.data.count!==undefined,20000);
      message('Scan finished. Choose a network, then enter its password.');
    } catch(error) { $('wifi-scan-state').textContent='Scan failed. See Service logs; try again.';throw error; }
    finally {scanning=false;ctx.serviceBusy=false;controls();}
  });
  $('use-network').onclick=action(()=>{
    const selected=networks[Number($('wifi-networks').value)];
    if(!selected)throw new Error('Scan and select a network first.');
    $('wifi-ssid').value=selected.ssid;$('wifi-password').value='';$('wifi-password').focus();
    message(selected.security==='OPEN'?'This is an open network; the current setup requires a password-protected network.':'Network selected. Enter its password.');
  });
  async function connectWifi() {await ack('WIFI CONNECT');await query('GET WIFI STATUS','WIFI');message('Connection requested. Allow up to 20 seconds, then refresh Wi-Fi to check the result.');}
  $('connect-wifi').onclick=action(connectWifi);
  $('wifi-form').onsubmit=action(async()=>{
    const ssid=$('wifi-ssid').value,password=$('wifi-password').value;
    if(!ssid || !password || new TextEncoder().encode(ssid).length>32 || new TextEncoder().encode(password).length>63 || /[|\r\n]/.test(ssid+password) || /\s$/.test(ssid) || /\s$/.test(password))
      throw new Error('Enter SSID (up to 32 UTF-8 bytes) and password (up to 63), without |, line breaks or trailing whitespace. Empty passwords are unsupported by this firmware command.');
    // Firmware WIFI CONNECT keeps an existing connection; disconnect before replacing credentials.
    await ack('SET WIFI CLEAR');
    await ack('SET WIFI SSID '+ssid,'SET WIFI SSID');await ack('SET WIFI PASSWORD '+password,'SET WIFI PASSWORD');await ack('SAVE');
    $('wifi-password').value='';$('show-password').checked=false;$('wifi-password').type='password';await connectWifi();
  });
  $('clear-wifi').onclick=action(async()=>{
    if(!confirm('Erase saved Wi-Fi credentials on this jar and disconnect its Wi-Fi?'))return;
    await ack('SET WIFI CLEAR');await ack('SAVE');$('wifi-ssid').value='';$('wifi-password').value='';await query('GET WIFI STATUS','WIFI');message('Saved Wi-Fi cleared.');
  });
  $('set-alarm').onclick=action(async()=>{
    const time=$('alarm-time').value;
    if(!/^(?:[01]\d|2[0-3]):[0-5]\d$/.test(time))throw new Error('Choose a valid alarm time.');
    await ack('SET ALARM '+time,'SET ALARM');await ack('SET ALARM '+($('alarm-enabled').checked?'ON':'OFF'));await ack('SAVE');await query('GET ALARM','ALARM');message('Alarm saved.');
  });
  $('set-timer').onclick=action(async()=>{
    const minutes=Number($('timer-minutes').value);
    if(!Number.isInteger(minutes)||minutes<1||minutes>10080)throw new Error('Timer must be 1–10080 minutes.');
    await ack('SET TIMER MINUTES '+minutes,'SET TIMER MINUTES');await query('GET TIMER','TIMER');message('Timer started.');
  });
  $('stop-timer').onclick=action(async()=>{await ack('SET TIMER OFF');await query('GET TIMER','TIMER');message('Timer stopped.');});
  return {syncClock,reset(){
    $('wifi-password').value='';$('wifi-password').type='password';$('show-password').checked=false;
    networks=[];$('wifi-networks').replaceChildren();$('wifi-scan-state').textContent='Not scanned yet.';
    for(const id of ['wifi-state','time-state','alarm-state','timer-state'])$(id).textContent='Disconnected — refresh after connecting.';
  },receive({type,data}) {
    if(type==='WIFI_SCAN' && scanning){
      if(data.ssid){
        networks.push({...data}); const option=new Option(`${data.ssid} · ${data.rssi} dBm · CH ${data.channel} · ${data.security}`,String(networks.length-1));$('wifi-networks').add(option);
      }else if(data.count!==undefined)$('wifi-scan-state').textContent=`${networks.length} named network(s) available.`;
    }
    if(type==='WIFI')$('wifi-state').textContent=`${data.state || 'Unknown'} · ${data.ssid || 'No saved network'} · IP ${data.ip || '—'} · signal ${data.state==='CONNECTED'?data.rssi+' dBm':'—'} (last reading)`;
    if(type==='TIME'){
      $('time-state').textContent=`${data.synced==='0'?'Clock not synced':data.local || 'Clock not synced'} · ${data.timezone || '—'} (last reading)`;
      if(ZONES.includes(data.timezone))mirrorZone(data.timezone);
    }
    if(type==='ALARM'){
      $('alarm-state').textContent=`${data.enabled==='1'?'Enabled':'Disabled'} · ${data.time || '—'} local time`;
      if(/^(?:[01]\d|2[0-3]):[0-5]\d$/.test(data.time))$('alarm-time').value=data.time;
      $('alarm-enabled').checked=data.enabled==='1';
    }
    if(type==='TIMER'){
      const seconds=Number(data.remaining);
      $('timer-state').textContent=`${data.enabled==='1'?'Running':'Stopped'} · ${Number.isFinite(seconds)?Math.floor(seconds/60)+'m '+seconds%60+'s remaining':'No remaining-time reading'} (last reading)`;
      if(Number(data.minutes)>=1 && Number(data.minutes)<=10080)$('timer-minutes').value=data.minutes;
    }
    if(type==='ALARM'||type==='TIMER')$('schedule-state').textContent='Schedule read from the jar. Refresh for the latest state.';
  }};
}
