import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import vm from 'node:vm';
test('ported BloomSaver runs its original controls and honors background pause/edit messages',async()=>{
 const source=await readFile(new URL('../web-controller/bloomsaver/app.js',import.meta.url),'utf8'),nodes=new Map(),events={},frames=[],stored=new Map();let paints=0,time=0;
 const ctx=new Proxy({createLinearGradient:()=>({addColorStop(){}}),fillRect(){paints++;}}, {get:(o,k)=>k in o?o[k]:()=>{},set:(o,k,v)=>{o[k]=v;return true;}});
 function node(id){if(nodes.has(id))return nodes.get(id);const n={id,value:'',textContent:'',style:{},dataset:{},children:[],classes:new Set(),listeners:{},classList:{add(){},remove(){},contains(){return false;},toggle(){}},addEventListener(k,fn){this.listeners[k]=fn;},appendChild(child){this.children.push(child);},getContext:()=>ctx,querySelector(tag){return this[tag] ||=node(id+'-'+tag);}};nodes.set(id,n);return n;}
 const document={hidden:false,body:node('body'),documentElement:node('root'),activeElement:{tagName:'BODY'},getElementById:node,createElement:tag=>node('new-'+tag+'-'+nodes.size),querySelectorAll:()=>[]};
 const parent={postMessage(){}},sandbox={document,parent,location:{search:'?embed=1',origin:'http://localhost:8080'},innerWidth:1400,innerHeight:900,devicePixelRatio:1,performance:{now:()=>time},URLSearchParams,Math,Number,Object,JSON,localStorage:{getItem:k=>stored.get(k)||null,setItem:(k,v)=>stored.set(k,v)},requestAnimationFrame:fn=>frames.push(fn),setTimeout,clearTimeout,addEventListener:(k,fn)=>{(events[k] ||= []).push(fn);}};
 vm.runInNewContext(source,sandbox);assert.equal(node('controls').children.length,22);assert.equal(frames.length,1);
 time=100;frames.shift()(time);assert.ok(paints>1,'engine renders frames');const before=paints;
 for(const fn of events.message)fn({source:parent,origin:'http://localhost:8080',data:{type:'studio-background',paused:true,editing:true}});
 time=200;frames.shift()(time);assert.equal(paints,before,'paused background performs no painting');
 for(const fn of events.message)fn({source:{},origin:'http://evil.test',data:{type:'studio-background',paused:false}});
 time=300;frames.shift()(time);assert.equal(paints,before,'foreign window cannot resume background');
 for(const fn of events.message)fn({source:parent,origin:'http://localhost:8080',data:{type:'studio-background',paused:false,editing:false}});
 node('mutate').onclick();time=400;frames.shift()(time);assert.ok(paints>before);
 node('panel').listeners.click({target:{id:'mutate'}});const state=JSON.parse(stored.get('happyjarz.bloomsaver.current'));assert.equal(state.version,4);assert.ok(state.settings.count>=40 && state.settings.count<=1200);
});
