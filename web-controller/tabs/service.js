import { buildReport, safeFields } from '../diagnostics.mjs';
export function init(ctx) {
  const { $, ack, query, request, action, controls, message, log } = ctx;
const testResults=[];
function renderDiagnostics(){
  const filter=$('log-filter').value;
  const entries=ctx.diagnostics.entries.filter(e=>filter==='all' || (filter==='error' ? e.level==='error' : e.level!=='info'));
  const lines=entries.map(e=>`${e.at} ${e.level.toUpperCase()} ${e.kind} ${e.text}`);
  $('service-log').textContent=lines.slice(-250).join('\n');$('log').textContent=lines.slice(-80).join('\n');
  $('log-health').textContent=`${ctx.diagnostics.entries.length}/${ctx.diagnostics.maxEntries} events retained · ${ctx.diagnostics.dropped} older events dropped · storage: ${ctx.diagnostics.persistence}. Screen shows the latest 250 matching events; downloads include all retained events.`;
}
let inputTimer;
function updateReadouts(){
  if($('service').hidden)return;
  const root=$('service-readouts');root.replaceChildren();
  for(const type of ['IDENTITY','CAPS','POWER','STATUS','LED_FRAME','OLED','WIFI','TIME','ALARM','TIMER','DISPLAY','INPUT','TOUCH_CAL','SAVER','ACCESSORY','SLEEP','ART']){
    const card=document.createElement('section'),heading=document.createElement('h3'),body=document.createElement('pre'),stamp=document.createElement('small');
    heading.textContent=type;const reading=ctx.snapshots[type];
    body.textContent=reading ? Object.entries(safeFields(reading.fields)).map(([k,v])=>`${k}: ${v}`).join('\n') : 'No reading yet';
    stamp.textContent=reading ? `Received ${reading.at}${ctx.identity ? '' : ' · disconnected; last known reading'}` : 'Refresh to read';
    card.append(heading,body,stamp);root.append(card);
  }
}
async function serviceTest(label,command,match,timeout=10000){
  const start=performance.now();ctx.serviceBusy=true;controls();
  try {const response=await request(command,match,timeout);const detail=JSON.stringify(safeFields(response.data));
    testResults.push({at:new Date().toISOString(),board:ctx.identity?.serial,label,command,elapsedMs:Math.round(performance.now()-start),result:'response received',detail});
    log(`${label}: firmware response ${detail}. Physical result requires observation.`,'info','TEST');message(`${label} completed. Check the physical result and record your observations.`);
  }catch(error){testResults.push({at:new Date().toISOString(),board:ctx.latestIdentity?.serial,label,command,elapsedMs:Math.round(performance.now()-start),result:'failed',detail:error.message});log(`${label}: ${error.message}`,'error','TEST');throw error;}
  finally{if(testResults.length>100)testResults.shift();$('service-tests').textContent=testResults.map(t=>`${t.at} · ${t.label}: ${t.result} (${t.elapsedMs} ms) · ${t.detail}`).join('\n');ctx.serviceBusy=false;controls();}
}
$('refresh-service').onclick=action(async()=>{
  ctx.serviceBusy=true;controls();let failed=0;
  try{for(const [command,type] of [['GET CAPS','CAPS'],['GET POWER','POWER'],['GET STATUS','STATUS'],['GET LED FRAME','LED_FRAME'],['GET OLED','OLED'],['GET WIFI STATUS','WIFI'],['GET TIME STATUS','TIME'],['GET ALARM','ALARM'],['GET TIMER','TIMER'],['GET DISPLAY','DISPLAY'],['GET INPUT','INPUT'],['GET SAVER STATUS','SAVER'],['ART READY','ART']]){
    if(!ctx.identity)break;try{await query(command,type);}catch(error){failed++;log(`${command}: ${error.message}`,'error','SERVICE');}
  }message(failed?`Readouts refreshed with ${failed} failed requests. See the log.`:'Readouts refreshed.',failed?'warn':'info');}
  finally{ctx.serviceBusy=false;controls();}
});
$('service-ping').onclick=action(()=>serviceTest('Ping','PING',p=>p.type==='PONG'));
$('service-rgb').onclick=action(async()=>{if(confirm('Run a brief red/green/blue/white lamp test?'))await serviceTest('RGB routine','TEST RGB',p=>p.type==='TEST' && p.data.rgb!==undefined);});
$('service-touch').onclick=action(async()=>{if(confirm('Keep fingers off all six pads. Recalibrate touch inputs now?'))await serviceTest('Touch calibration routine','TEST TOUCH',p=>p.type==='TEST' && p.data.input!==undefined);});
$('live-input').onchange=()=>{clearInterval(inputTimer);if($('live-input').checked){inputTimer=setInterval(()=>{if(ctx.identity&&!ctx.transferring&&!ctx.serviceBusy)query('GET INPUT','INPUT').catch(error=>message(error.message,'error'));},1000);}};
function downloadFile(name,contents,type){const url=URL.createObjectURL(new Blob([contents],{type})),a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
function environment(){return ctx.environment();}
$('download-report').onclick=action(()=>{
  ctx.diagnostics.flush();const report=buildReport({logger:ctx.diagnostics,environment:environment(),identity:ctx.latestIdentity,snapshots:ctx.snapshots,tests:testResults,notes:$('support-notes').value,observations:{screen:$('observe-screen').value,lights:$('observe-lights').value,touch:$('observe-touch').value}});
  downloadFile('happyjarz-support-'+new Date().toISOString().replaceAll(':','-')+'.json',JSON.stringify(report,null,2),'application/json');message('Support report downloaded. You can review it and send it to us.');
});
$('download-log').onclick=()=>{ctx.diagnostics.flush();downloadFile('happyjarz-session-log.txt',`HAPPY JARZ Web Studio 0.5.0\nExported ${new Date().toISOString()}\nOlder events dropped: ${ctx.diagnostics.dropped}\n`+JSON.stringify(environment(),null,2)+'\n\n'+ctx.diagnostics.entries.map(e=>`${e.at} ${e.level.toUpperCase()} ${e.kind} ${e.text}`).join('\n'),'text/plain');};
$('clear-log').onclick=()=>{if(confirm('Delete retained diagnostic logs from this browser?')){ctx.diagnostics.clear();renderDiagnostics();}};
$('log-filter').onchange=renderDiagnostics;

return { renderDiagnostics, receive:updateReadouts, onShow:updateReadouts, reset(){clearInterval(inputTimer);$('live-input').checked=false;updateReadouts();} };
}
