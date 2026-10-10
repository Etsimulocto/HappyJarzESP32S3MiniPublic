import assert from 'node:assert/strict';
import { test } from 'node:test';
import { parseLine, profileCommand, pixelIndex, artChunks, importDrawing, compareVersions } from '../web-controller/protocol.mjs';

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
