import assert from 'node:assert/strict';
import { test } from 'node:test';
import { parseLine, profileCommand, pixelIndex, artChunks, importDrawing, compareVersions } from '../web-controller/protocol.mjs';
import { DiagnosticLog, redact, buildReport } from '../web-controller/diagnostics.mjs';

test('profile validation blocks command injection and impossible birthdays',()=>{
  assert.equal(profileCommand(' Quarter ','02','29'),'SET PROFILE 02-29 Quarter');
  assert.equal(profileCommand('Cat','',''),'SET PROFILE -- Cat');
  for(const name of ['','hello\nSAVE','a|b','é'.repeat(17)]) assert.throws(()=>profileCommand(name,'',''));
  assert.throws(()=>profileCommand('Quarter','04','31'));
  assert.throws(()=>profileCommand('Quarter','', '1'));
});
test('art packing covers every pixel with firmware-sized chunks and exact ACK offsets',()=>{
  const frames=[new Uint8Array(1024),new Uint8Array(1024)];
  const [byte,bit]=pixelIndex(127,63); assert.equal(byte,1023);assert.equal(bit,128);frames[0][byte]|=bit;
  const chunks=artChunks(frames,49), data=chunks.filter(c=>c.event==='DATA');
  assert.equal(chunks[0].command,'ART BEGIN 49 2');assert.equal(chunks.at(-1).command,'ART END 49');
  const recovered=[];
  for(const c of data){const [, ,slot,offset,hex]=c.command.split(' ');assert.equal(slot,'49');assert.equal(Number(offset),recovered.length);assert.ok(hex.length<=96);recovered.push(...Buffer.from(hex,'hex'));assert.equal(c.offset,recovered.length);}
  assert.equal(recovered.length,2048);assert.equal(recovered[1023],128);assert.deepEqual(recovered.slice(1024),[...frames[1]]);
  assert.throws(()=>artChunks(frames,50));assert.throws(()=>artChunks(frames,-1));
});
test('drawing import rejects malformed payloads; protocol parsing retains equals',()=>{
  const value={format:'happyjarz-xbm-v1',frames:[Array(1024).fill(0),Array(1024).fill(255)]};
  assert.equal(importDrawing(value)[1][0],255);value.frames[0][3]=256;assert.throws(()=>importDrawing(value));
  const p=parseLine('HJ|ERR|message=a=b');assert.equal(p.data.message,'a=b');assert.equal(parseLine('boot noise'),null);
  assert.equal(compareVersions('0.18.10','0.18.4'),1);assert.throws(()=>compareVersions('latest','0.18.4'));
});
test('diagnostics redact credentials, profiles, network and frame payloads before storage',()=>{
  const samples=['SET WIFI PASSWORD secretword','SET WIFI SSID secretword','SET PROFILE 08-25 secretword','HJ|PROFILE|owner=secretword|birthday=secretword','HJ|WIFI|ssid=secretword|ip=secretword','{"password":"secretword","owner":"secretword"}','ART DATA 0 0 secretword','HJ|OLED|data=secretword'];
  for(const sample of samples)assert.ok(!redact(sample).includes('secretword'),sample);
  const values=new Map(),storage={getItem:k=>values.get(k),setItem:(k,v)=>values.set(k,v)};
  const log=new DiagnosticLog({storage});log.add('error','TX',samples[0]);log.flush();
  assert.ok(![...values.values()][0].includes('secretword'));
  const restored=new DiagnosticLog({storage});assert.equal(restored.entries.length,1);assert.equal(restored.entries[0].level,'error');
});
test('bounded log explicitly counts dropped events and tolerates blocked/full storage',()=>{
  const log=new DiagnosticLog({maxEntries:2});for(let i=0;i<4;i++)log.add('info','TEST',String(i));
  assert.equal(log.entries.length,2);assert.equal(log.dropped,2);
  const blocked=new DiagnosticLog({storage:{getItem(){throw Error('blocked');},setItem(){throw Error('full');}}});blocked.add('error','USB','Port lost');blocked.flush();assert.equal(blocked.persistence,'unavailable');assert.equal(blocked.entries.length,1);
  blocked.clear();assert.equal(blocked.entries.length,0);
});
test('support reports retain hardware evidence and observations without private profile fields',()=>{
  const logger=new DiagnosticLog();logger.add('error','USB','No response');
  const report=buildReport({logger,environment:{language:'de-DE',timezone:'Europe/Berlin'},identity:{serial:'HJ-123',fw:'0.18.4'},snapshots:{PROFILE:{at:'2026-10-10T00:00:00Z',fields:{owner:'private-name',birthday:'08-25'}},POWER:{at:'2026-10-10T00:00:01Z',fields:{voltage:'3.8',percent:'55'}}},tests:[{command:'TEST RGB',result:'response received',detail:'rgb=PASS'}],notes:'Screen blank after switch',observations:{screen:'Blank'}});
  assert.equal(report.identity.serial,'HJ-123');assert.equal(report.readouts.POWER.fields.voltage,'3.8');assert.equal(report.observations.screen,'Blank');assert.ok(!JSON.stringify(report).includes('private-name'));assert.equal(report.entries[0].level,'error');
});
