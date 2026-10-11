import { profileCommand } from '../protocol.mjs';
import { birthdayCountdown } from '../profile.mjs';
export function init(ctx) {
  const {$,ack,query,action,controls,message}=ctx;
  let saved=null,clock=null,dirty=false,acceptProfile=true;
  for(let m=1;m<=12;m++)$('birth-month').add(new Option(new Date(2000,m-1,1).toLocaleString('en',{month:'long'}),String(m).padStart(2,'0')));
  function render(){
    $('profile-chip').textContent=ctx.identity ? 'Jar '+(ctx.identity.chip || ctx.identity.serial || '—')+' · firmware '+(ctx.identity.fw || '—') : 'Connect a jar to identify its profile.';
    $('profile-saved').textContent=saved ? (saved.owner || 'No owner saved')+' · birthday '+(saved.birthday==='--'?'not set':saved.birthday || 'not set') : 'No profile reading yet.';
    $('profile-clock').textContent=clock ? `${clock.local || 'Clock not synced'} · ${clock.timezone || '—'} (last jar reading)` : 'Read the jar clock to check its date.';
    const countdown=saved && clock?.synced!=='0'?birthdayCountdown(saved.birthday,clock?.local):null;
    $('profile-countdown').textContent=!saved?.birthday || saved.birthday==='--'?'Add a birthday to start the countdown.':!countdown?'Sync or refresh the jar clock to see the countdown.':countdown.days===0?`HAPPY BIRTHDAY${saved.owner?', '+saved.owner:''}!`:`${countdown.days} day${countdown.days===1?'':'s'} until ${saved.owner || 'your'}${saved.owner?"’s":''} birthday.`;
    $('profile-countdown-detail').textContent=countdown ? `Next birthday: ${countdown.date} · calculated from the jar’s last reported local date.` : 'Countdown uses the saved profile and the jar clock, not your unsaved edits.';
    try {const command=profileCommand($('owner').value,$('birth-month').value,$('birth-day').value);const birthday=command.split(' ')[2];$('profile-draft').textContent=`${dirty?'Unsaved draft':'Editor'} · ${new TextEncoder().encode($('owner').value.trim()).length}/32 name bytes · birthday ${birthday==='--'?'not set':birthday}`;}catch(error){$('profile-draft').textContent=error.message;}
  }
  for(const id of ['owner','birth-month','birth-day'])$(id).oninput=()=>{dirty=true;render();};
  $('remove-birthday').onclick=()=>{$('birth-month').value='';$('birth-day').value='';dirty=true;render();message('Birthday removed from the draft. Save to apply it to the jar.');};
  async function refreshProfile(){acceptProfile=true;try{await query('GET PROFILE','PROFILE');await query('GET TIME STATUS','TIME');}finally{acceptProfile=false;}}
  async function exclusive(fn){if(ctx.serviceBusy||ctx.transferring)throw new Error('Wait for the current jar operation.');ctx.serviceBusy=true;controls();try{await fn();}finally{ctx.serviceBusy=false;controls();}}
  async function save(sync){
    const command=profileCommand($('owner').value,$('birth-month').value,$('birth-day').value);
    await exclusive(async()=>{
      $('profile-state').textContent='Saving profile on the jar…';acceptProfile=true;let confirmed=false;
      try{
        await ack(command,'SET PROFILE');await query('GET PROFILE','PROFILE');dirty=false;confirmed=true;
        $('profile-state').textContent='Profile saved on the jar.';
        if(sync)try{await ctx.syncClock();}catch(error){$('profile-state').textContent='Profile saved, but clock sync failed. Retry clock sync in Wi-Fi + Clock.';throw error;}
        render();message(sync?'Profile saved and clock synced.':'Profile saved on the jar.');
      }catch(error){if(!confirmed)$('profile-state').textContent='Save could not be confirmed. Load from the jar to check.';throw error;}
      finally{acceptProfile=false;}
    });
  }
  $('profile-form').onsubmit=action(()=>save(true));$('save-profile-only').onclick=action(()=>save(false));
  $('load-profile').onclick=action(async()=>{if(dirty&&!confirm('Replace unsaved profile edits with the jar’s saved profile?'))return;await exclusive(refreshProfile);$('profile-state').textContent='Profile loaded from the jar.';message('Profile and jar clock refreshed.');});
  $('clear-profile').onclick=action(async()=>{if(!confirm('Erase this jar’s saved name and birthday?'))return;await exclusive(async()=>{acceptProfile=true;try{await ack('CLEAR PROFILE');await query('GET PROFILE','PROFILE');}finally{acceptProfile=false;}});$('profile-state').textContent='Name and birthday cleared on the jar.';message('Profile cleared.');});
  $('refresh-profile-clock').onclick=action(async()=>{await query('GET TIME STATUS','TIME');message('Birthday countdown refreshed from the jar clock.');});
  render();
  return {onShow:render,receive({type,data}){
    if(type==='IDENTITY')render();
    if(type==='TIME'){clock={...data};render();}
    if(type==='PROFILE'){
      saved={...data};
      if(acceptProfile||!dirty){$('owner').value=data.owner || '';const parts=/^\d{2}-\d{2}$/.test(data.birthday || '')?data.birthday.split('-'):['',''];$('birth-month').value=parts[0];$('birth-day').value=parts[1];dirty=false;}
      acceptProfile=false;render();
    }
  },reset(){saved=null;clock=null;dirty=false;acceptProfile=true;$('owner').value='';$('birth-month').value='';$('birth-day').value='';$('profile-state').textContent='Disconnected. Load the next jar’s profile after connecting.';render();}};
}
