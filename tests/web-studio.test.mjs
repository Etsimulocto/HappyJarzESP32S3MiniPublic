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

import { rgb, rgbHex, hexRgb, brightness, capabilities, gamepadKeys, patternTheme } from '../web-controller/lights.mjs';
test('light colors reject malformed telemetry and brightness obeys the board ceiling',()=>{
  assert.deepEqual(rgb('255,0,16'),[255,0,16]);assert.equal(rgbHex([255,0,16]),'#ff0010');assert.deepEqual(hexRgb('#FF0010'),[255,0,16]);
  for(const value of ['0,,0','256,0,0','1,2','NaN,0,0',{},[0,-1,2],[0,2.5,0]])assert.equal(rgb(value),null);
  assert.throws(()=>hexRgb('#fff'));assert.equal(brightness(75),50);assert.equal(brightness(-1),0);assert.equal(brightness('24.6'),25);assert.throws(()=>brightness('broken'));
});
test('capabilities and standard gamepad mapping restrict host inputs without guessing',()=>{
  assert.deepEqual([...capabilities('up,down,left,right,a,b,unknown')],['up','down','left','right','a','b']);assert.equal(capabilities().size,0);
  const pad={mapping:'standard',buttons:Array.from({length:16},(_,i)=>({pressed:[0,5,9,12].includes(i)})),axes:[-.7,.8]};
  assert.deepEqual([...gamepadKeys(pad)].sort(),['a','down','left','r','start','up']);
  assert.equal(gamepadKeys({...pad,mapping:''}).size,0);assert.equal(gamepadKeys(null).size,0);
  assert.equal(gamepadKeys({mapping:'standard',axes:[.3,-.3]}).size,0);
});
test('pattern theme freezes a useful palette and ignores dark/off frames',()=>{
  assert.equal(patternTheme([[[0,0,0],[1,1,1]]]),null);
  const frame=[[255,40,150],[0,240,255],[255,40,150],[0,240,255]],theme=patternTheme([frame]);
  for(const value of Object.values(theme))assert.match(value,/^#[0-9a-f]{6}$/);
  const lum=color=>{const c=hexRgb(color).map(n=>{n/=255;return n<=.04045?n/12.92:((n+.055)/1.055)**2.4;});return .2126*c[0]+.7152*c[1]+.0722*c[2];};
  for(const surface of ['bg','panel'])assert.ok((lum(theme.text)+.05)/(lum(theme[surface])+.05)>=4.5);
  assert.deepEqual(patternTheme([frame,frame]),theme);
});

import {songLink,imageLink} from '../web-controller/media.mjs';
test('Monday media accepts Suno sharing links and rejects executable/foreign URLs',()=>{
 assert.equal(songLink(' https://suno.com/song/a-b '),'https://suno.com/song/a-b');assert.equal(songLink('https://suno.com/playlist/abc'),'https://suno.com/playlist/abc');assert.equal(songLink(''),'');
 for(const link of ['javascript:alert(1)','http://suno.com/song/abc','https://suno.com.evil.test/song/abc','https://user:secret@suno.com/song/abc','https://suno.com/'])assert.throws(()=>songLink(link));
 assert.equal(imageLink('./assets/monday.webp'),'./assets/monday.webp');assert.equal(imageLink('data:image/webp;base64,YWJj'),'data:image/webp;base64,YWJj');
 for(const link of ['https://evil.test/tracker.png','./assets/../secret','data:image/svg+xml;base64,YWJj'])assert.throws(()=>imageLink(link));
});


import { birthdayCountdown } from '../web-controller/profile.mjs';
test('birthday countdown uses jar calendar dates, year rollover and leap-day observance',()=>{
  assert.deepEqual(birthdayCountdown('06-11','2026-06-11T00:01:00'),{days:0,date:'2026-06-11'});
  assert.deepEqual(birthdayCountdown('01-01','2026-12-31T23:59:00'),{days:1,date:'2027-01-01'});
  assert.deepEqual(birthdayCountdown('02-29','2027-02-28T12:00:00'),{days:1,date:'2027-03-01'});
  assert.deepEqual(birthdayCountdown('02-29','2028-02-28T12:00:00'),{days:1,date:'2028-02-29'});
  assert.equal(birthdayCountdown('06-11','2026-06-12').date,'2027-06-11');
  assert.equal(birthdayCountdown('04-31','2026-01-01'),null);
  assert.equal(birthdayCountdown('06-11','2026-02-30'),null);
  assert.equal(birthdayCountdown('--','2026-01-01'),null);
});

import {STAMPS,stamp,line,shape} from '../web-controller/art-tools.mjs';
test('pixel stamps and shapes clip safely and support erase and filled geometry',()=>{
 const lit=(f,x,y)=>{const [byte,bit]=pixelIndex(x,y);return !!(f[byte]&bit);};
 assert.equal(STAMPS.length,24);
 for(const icon of STAMPS){const frame=new Uint8Array(1024);stamp(frame,icon.id,64,32,2);assert.ok(frame.some(Boolean),icon.id);stamp(frame,icon.id,64,32,2,true);assert.ok(frame.every(v=>v===0));stamp(frame,icon.id,0,0,4);assert.equal(frame.length,1024);}
 const frame=new Uint8Array(1024);line(frame,0,0,127,63);assert.ok(lit(frame,0,0)&&lit(frame,127,63));
 frame.fill(0);shape(frame,'rectangle',{x:2,y:2},{x:10,y:10});assert.ok(lit(frame,2,2));assert.equal(lit(frame,5,5),false);shape(frame,'rectangle',{x:10,y:10},{x:2,y:2},true);assert.ok(lit(frame,5,5));
 for(const kind of ['ellipse','triangle','diamond','star']){frame.fill(0);shape(frame,kind,{x:20,y:10},{x:60,y:50},true);assert.ok(lit(frame,40,30),kind);assert.equal(lit(frame,0,0),false);}
 assert.throws(()=>stamp(frame,'heart',64,32,5));
});
