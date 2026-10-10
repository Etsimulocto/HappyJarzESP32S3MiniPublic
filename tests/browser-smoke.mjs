import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
const { chromium } = createRequire(import.meta.url)('playwright');
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import path from 'node:path';
import assert from 'node:assert/strict';
const root=fileURLToPath(new URL('../web-controller/', import.meta.url));
const server=createServer(async(req,res)=>{try{const name=req.url.split('?')[0]==='/'?'index.html':req.url.split('?')[0].slice(1);if(name.includes('..'))throw Error();const data=await readFile(path.join(root,name));res.setHeader('Content-Type',name.endsWith('.html')?'text/html':name.endsWith('.css')?'text/css':name.endsWith('.json')?'application/json':'text/javascript');res.end(data);}catch{res.statusCode=404;res.end();}});
await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
const url=`http://127.0.0.1:${server.address().port}`;
const browser=await chromium.launch({headless:true,args:['--no-sandbox']});
try{
 const page=await browser.newPage({viewport:{width:1200,height:900}}), errors=[];
 page.on('pageerror',e=>errors.push(e.message));page.on('dialog',d=>d.accept());
 await page.addInitScript(()=>{
  let controller;window.sent=[];window.storageReady=true;window.offset=0;
  const emit=line=>{const b=new TextEncoder().encode(line+'\r\n');controller.enqueue(b.slice(0,3));controller.enqueue(b.slice(3));};
  const serial={requestPort:async()=>({
   open:async function(){this.readable=new ReadableStream({start(c){controller=c;},cancel(){}});this.writable=new WritableStream({write(bytes){
    const command=new TextDecoder().decode(bytes).trim();window.sent.push(command);
    if(command==='HELLO')emit('HJ|IDENTITY|serial=HJ-TEST|chip=TEST|hw=V1|fw=0.18.4');
    else if(command==='GET PROFILE')emit('HJ|PROFILE|owner=Quarter|birthday=08-25');
    else if(command==='GET STATUS')emit('HJ|STATUS|brightness=75|pattern=SOLID|led1=32,0,64|led2=0,32,64|led3=32,0,64|led4=0,32,64');
    else if(command==='GET POWER')emit('HJ|POWER|sensor=OK|percent=55|voltage=3.8');
    else if(command==='GET TIME STATUS')emit('HJ|TIME|synced=1|timezone=America/Chicago|local=2026-10-10T18:00:00');
    else if(command==='GET CAPS')emit('HJ|CAPS|profile=SIMPLE|leds=4');
    else if(command==='GET WIFI STATUS')emit('HJ|WIFI|state=CONNECTED|ssid=PrivateNetwork|ip=192.168.1.5|rssi=-50');
    else if(command==='GET ALARM')emit('HJ|ALARM|enabled=0|time=07:30');
    else if(command==='GET TIMER')emit('HJ|TIMER|enabled=0|remaining=0');
    else if(command==='GET DISPLAY')emit('HJ|DISPLAY|brightness=100');
    else if(command==='GET INPUT')emit('HJ|INPUT|up=100|down=105|left=101|right=102|a=104|b=106');
    else if(command==='GET SAVER STATUS')emit('HJ|SAVER|active=0|mode=ART');
    else if(command==='GET LED FRAME')emit('HJ|LED_FRAME|led1=32,0,64');
    else if(command==='GET OLED')emit('HJ|OLED|seq=2|codec=b64v1|bytes=1024|data='+btoa(String.fromCharCode(...new Uint8Array(1024))));
    else if(command==='PING')emit('HJ|PONG');
    else if(command==='TEST RGB')emit('HJ|TEST|rgb=PASS');
    else if(command==='TEST TOUCH'){emit('HJ|TOUCH_CAL|up_base=100|up_thr=120');emit('HJ|TEST|input=PASS');}
    else if(command==='ART READY')emit('HJ|ART|ACK|event=READY|ready='+(window.storageReady?1:0));
    else if(command.startsWith('ART BEGIN ')){window.offset=0;emit('HJ|ART|ACK|event=BEGIN|slot='+command.split(' ')[2]);}
    else if(command.startsWith('ART DATA ')){const [, ,slot,offset,hex]=command.split(' ');if(Number(offset)!==window.offset || hex.length>96)emit('HJ|ERR|message=BAD_DATA');else{window.offset+=hex.length/2;emit(`HJ|ART|ACK|event=DATA|slot=${slot}|offset=${window.offset}`);}}
    else if(command.startsWith('ART END ')){emit('HJ|ART|ACK|event=END|slot='+command.split(' ')[2]);}
    else {const key=command.startsWith('SET LED')?command.split(' ').slice(0,3).join(' '):command.startsWith('SET PROFILE')?'SET PROFILE':command.startsWith('SET TIMEZONE')?'SET TIMEZONE':command.startsWith('SET CLOCK UNIX')?'SET CLOCK UNIX':command.startsWith('SET PATTERN')?'SET PATTERN':command.startsWith('SET BRIGHTNESS')?'SET BRIGHTNESS':command.startsWith('KEY ')?'KEY':command;emit('HJ|ACK|command='+key);if(command==='STREAM OLED ON'){const data=new Uint8Array(1024);data[0]=1;emit('HJ|OLED|seq=1|codec=b64v1|bytes=1024|data='+btoa(String.fromCharCode(...data)));}}
   }});},setSignals:async()=>{},close:async()=>{window.closeCount=(window.closeCount||0)+1;}
  })};
  window.unplug=()=>controller.close();Object.defineProperty(navigator,'serial',{value:serial,configurable:true});
 });
 await page.goto(url);await page.locator('#release-status').filter({hasText:'No tested'}).waitFor();
 assert.equal(await page.locator('#apply-pattern').isDisabled(),true);
 await page.locator('#connect').click();await page.locator('#message').filter({hasText:'Your jar is ready'}).waitFor();
 assert.match(await page.locator('#identity').textContent(),/HJ-TEST/);
 assert.deepEqual(await page.evaluate(()=>[...document.getElementById('oled').getContext('2d').getImageData(0,0,1,1).data]),[168,231,214,255]);
 await page.locator('#apply-colors').click();await page.locator('#message').filter({hasText:'Colors applied'}).waitFor();
 await page.locator('[data-tab=profile]').click();await page.locator('#owner').fill('Cat');await page.locator('#birth-month').selectOption('06');await page.locator('#birth-day').fill('11');await page.locator('#profile-form button').click();await page.locator('#message').filter({hasText:'Profile saved'}).waitFor();
 assert.ok((await page.evaluate(()=>window.sent)).includes('SET PROFILE 06-11 Cat'));
 await page.locator('[data-tab=art]').click();await page.locator('#art-canvas').click({position:{x:40,y:40}});await page.locator('#copy-frame').click();
 await page.locator('#upload-art').click();await page.locator('#message').filter({hasText:'Two frames saved'}).waitFor();
 assert.equal(await page.evaluate(()=>window.offset),2048);assert.ok((await page.evaluate(()=>window.sent)).includes('ART END 0'));
 await page.evaluate(()=>window.storageReady=false);await page.locator('#upload-art').click();await page.locator('#message').filter({hasText:'Art storage is unavailable'}).waitFor();
 assert.equal(await page.locator('#upload-art').isDisabled(),false);
 assert.ok(!(await page.evaluate(()=>window.sent)).includes('ART FORMAT'));
 await page.locator('[data-tab=service]').click();await page.locator('#refresh-service').click();await page.locator('#message').filter({hasText:'Readouts refreshed.'}).waitFor();
 await page.locator('#service-ping').click();await page.locator('#service-tests').filter({hasText:'Ping: response received'}).waitFor();
 await page.locator('#service-rgb').click();await page.locator('#service-tests').filter({hasText:'RGB routine: response received'}).waitFor();
 await page.evaluate(()=>console.error('HJ|ERR|message=Test failure|password=supersecret'));
 await page.locator('#support-notes').fill('Screen blank after switch');
 const downloadPromise=page.waitForEvent('download');await page.locator('#download-report').click();const download=await downloadPromise;const report=JSON.parse(await readFile(await download.path(),'utf8'));
 assert.equal(report.userNotes,'Screen blank after switch');assert.equal(report.identity.serial,'HJ-TEST');assert.ok(!JSON.stringify(report).includes('supersecret'));assert.ok(!JSON.stringify(report).includes('PrivateNetwork'));assert.ok(report.entries.some(e=>e.kind==='CONSOLE' && e.level==='error'));
 await page.evaluate(()=>window.unplug());await page.locator('#connection').filter({hasText:'Jar disconnected'}).waitFor();await page.waitForFunction(()=>window.closeCount===1);
 await page.locator('#connect').click();await page.locator('#message').filter({hasText:'Your jar is ready'}).waitFor();await page.locator('#disconnect').click();await page.waitForFunction(()=>window.closeCount===2);
 await page.locator('[data-tab=home]').click();await page.screenshot({path:'/tmp/happyjarz-web-desktop.png',fullPage:true});
 await page.setViewportSize({width:390,height:844});assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);await page.screenshot({path:'/tmp/happyjarz-web-mobile.png',fullPage:true});
 assert.deepEqual(errors,[]);console.log('Browser smoke passed: USB, profile, colors, art, service readouts/tests, diagnostic export/redaction, unplug/reconnect, mobile layout.');
 const unsupported=await browser.newPage();await unsupported.addInitScript(()=>Object.defineProperty(navigator,'serial',{value:undefined,configurable:true}));
 // Removing the property tests the draft's capability fallback.
 await unsupported.addInitScript(()=>{delete Navigator.prototype.serial;delete navigator.serial;});await unsupported.goto(url);await unsupported.locator('#support').filter({hasText:'does not expose'}).waitFor();assert.equal(await unsupported.locator('#connect').isDisabled(),true);await unsupported.close();
} finally {await browser.close();server.close();}
