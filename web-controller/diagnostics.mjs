// Local-only, bounded diagnostics. Secrets and personal fields are never persisted.
const PRIVATE_FIELDS = /^(owner|birthday|ssid|ip|password|passphrase|text)$/i;
export function redact(text) {
  return String(text)
    .replace(/("(?:password|passphrase|token|secret|owner|birthday|ssid|ip|text)"\s*:\s*")([^"\\]|\\.)*(")/gi,'$1[redacted]$3')
    .replace(/(SET WIFI (?:PASSWORD|SSID) )[^\r\n]*/gi,'$1[redacted]')
    .replace(/(SET PROFILE )[^\r\n]*/gi,'$1[redacted]')
    .replace(/(SET CUSTOM SAYING \d+ )[^\r\n]*/gi,'$1[redacted]')
    .replace(/(ART DATA \d+ \d+ )[^\r\n]*/g,'$1[pixel payload omitted]')
    .replace(/(\|(?:owner|birthday|ssid|ip|password|passphrase|text)=)[^|\r\n]*/gi,'$1[redacted]')
    .replace(/(\|data=)[^|\r\n]*/g,'$1[frame payload omitted]')
    .replace(/((?:password|passphrase|token|secret)\s*[:=]\s*)[^\s|,;]+/gi,'$1[redacted]');
}
export function safeFields(fields) {
  return Object.fromEntries(Object.entries(fields).map(([key,value])=>[key, PRIVATE_FIELDS.test(key) ? '[redacted]' : redact(value)]));
}
export class DiagnosticLog {
  constructor({storage, key='happyjarz-diagnostics-v1', maxEntries=2000, maxChars=700000, now=()=>new Date().toISOString()}={}) {
    this.storage=storage;this.key=key;this.maxEntries=maxEntries;this.maxChars=maxChars;this.now=now;
    this.entries=[];this.dropped=0;this.persistence='memory';this.session=now();
    try {
      const old=JSON.parse(storage?.getItem(key)||'null');
      if(old?.schema===1 && Array.isArray(old.entries)){
        this.entries=old.entries.filter(e=>typeof e.text==='string' && typeof e.at==='string').map(e=>({at:e.at,session:String(e.session||''),level:['info','warn','error'].includes(e.level)?e.level:'info',kind:String(e.kind||'previous').slice(0,40),text:redact(e.text).slice(0,4000)}));
        this.dropped=Number.isSafeInteger(old.dropped) && old.dropped>=0 ? old.dropped : 0;
      }
      if(storage)this.persistence='local';
    }catch{this.persistence='unavailable';}
    this.trim();
  }
  trim(){let chars=this.entries.reduce((sum,e)=>sum+JSON.stringify(e).length,0);while(this.entries.length>this.maxEntries || chars>this.maxChars){chars-=JSON.stringify(this.entries.shift()).length;this.dropped++;}}
  add(level,kind,text){const entry={at:this.now(),session:this.session,level,kind,text:redact(text).slice(0,4000)};this.entries.push(entry);this.trim();return entry;}
  flush(){try{this.storage?.setItem(this.key,JSON.stringify({schema:1,entries:this.entries,dropped:this.dropped}));}catch{this.persistence='unavailable';}}
  clear(){this.entries=[];this.dropped=0;this.flush();}
  snapshot(){return {entries:this.entries.map(e=>({...e})),dropped:this.dropped,persistence:this.persistence};}
}
export function buildReport({logger,environment,identity,snapshots,tests,notes,observations}) {
  return {schema:'happyjarz-support-v1',createdUtc:new Date().toISOString(),appVersion:'0.9.0',environment,
    identity:identity ? safeFields(identity) : null,
    readouts:Object.fromEntries(Object.entries(snapshots).map(([type,entry])=>[type,{...entry,fields:safeFields(entry.fields)}])),
    tests:tests.map(t=>({...t,command:redact(t.command),detail:redact(t.detail||'')})),
    userNotes:redact(notes).slice(0,4000),observations,
    privacy:'Local report: names, birthdays, Wi-Fi network/IP/passwords and pixel payloads redacted. Board ID and user-written notes included. No automatic upload.',
    ...logger.snapshot()};
}
