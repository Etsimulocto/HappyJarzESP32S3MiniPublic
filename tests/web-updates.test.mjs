import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { validRelease } from '../web-controller/tabs/updates.js';

const normal = {
  channel:'stable', model:'happyjarz-supermini-simple-4mb',
  hw:'V1', chip:'ESP32-S3',
  layout:'arduino-4mb-default-app-0x10000-0x140000',
  flashSize:4194304, version:'0.18.13',
  images:[{url:'./releases/happyjarz-0.18.13-app.bin',
    address:65536, size:1114000, sha256:'a'.repeat(64)}]
};

test('firmware manifest permits only reviewed application partition metadata', () => {
  assert.equal(validRelease(normal),true);
  for (const wrong of [
    {...normal, chip:'ESP32'},
    {...normal, model:'another-board'},
    {...normal, hw:'V2'},
    {...normal, layout:'other-layout'},
    {...normal, flashSize:8388608},
    {...normal, version:'HEAD'},
    {...normal, images:[{...normal.images[0],address:0}]},
    {...normal, images:[{...normal.images[0],size:1310721}]},
    {...normal, images:[{...normal.images[0],size:0}]},
    {...normal, images:[{...normal.images[0],sha256:'bad'}]},
    {...normal, images:[...normal.images,...normal.images]}
  ]) assert.equal(validRelease(wrong),false,JSON.stringify(wrong));
});

test('unpublished releases.json never includes installable images', async () => {
  const json = JSON.parse(await readFile(new URL('../web-controller/releases.json',import.meta.url),'utf8'));
  assert.equal(json.schema,1);
  assert.ok(Array.isArray(json.releases));
  assert.equal(json.releases.length,0,'Only publish after real Board #4 browser flashing tests');
});
