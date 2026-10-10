import { compareVersions } from '../protocol.mjs';
export function init(ctx) {
  const { $, ack, query, request, action, controls, message, log } = ctx;
let releaseCatalog;
async function checkUpdates(){
  $('release-status').textContent='Checking release information…';
  try {const response=await fetch('./releases.json',{cache:'no-store',signal:AbortSignal.timeout(10000)});if(!response.ok)throw new Error('Release information is unavailable.');const catalog=await response.json();if(catalog.schema!==1 || !Array.isArray(catalog.releases))throw new Error('Invalid release information.');releaseCatalog=catalog;updateReleaseStatus();}
  catch(error){$('release-status').textContent=error.message+' Your jar can still work offline.';log(error.message,'error','RELEASE-FETCH');}
}
function updateReleaseStatus(){if(!releaseCatalog)return;const stable=releaseCatalog.releases.filter(r=>r.channel==='stable' && r.model==='happyjarz-supermini-simple-4mb');if(!stable.length){$('release-status').textContent=`No tested firmware download has been published yet.${ctx.identity ? ' Connected firmware: '+ctx.identity.fw+'.' : ''}`;return;}try{stable.sort((a,b)=>compareVersions(b.version,a.version));$('release-status').textContent=`Latest listed release: ${stable[0].version}. Exact board compatibility must be verified before installation. Installer remains disabled.`;}catch{$('release-status').textContent='Release catalog contains an unsupported version. Installer remains disabled.';}}
$('check-updates').onclick=checkUpdates;

void checkUpdates();
return { receive({type}) {if(type==='IDENTITY')updateReleaseStatus();} };
}
