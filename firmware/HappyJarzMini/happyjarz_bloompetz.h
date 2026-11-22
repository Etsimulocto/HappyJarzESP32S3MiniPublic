// HAPPY JARZ optional BloomPetz - first hardware-test slice.
// Adapted from BloomCircuit/BloomPetz/firmware/bloompetz_v0_1.ino.
// Pet game data isolated in namespace and NVS "hjpetz"; NO changes to
// Happy Jarz settings, ART SPIFFS, or BloomCircuit "bloomsys" saves.
// DND, full editor and unified BLOOM SYSTEM migration are NOT in this slice.
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

namespace HJPet {
static constexpr uint8_t SLOTS=3, ACTIONS=8, ROLLS=25;
static constexpr uint16_t STATS=200;
static constexpr uint32_t MAGIC=0x42505A31;
static const char *const names[ACTIONS]={"CHECK","CLEAN","PET","PLAY","REST","SCRATCH","SOCIALIZE","TRAIN"};
static const uint8_t favored[ACTIONS][4]={{2,3,9,16},{9,11,14,12},{0,12,13,15},{7,1,4,15},{8,9,12,14},{0,12,15,13},{0,12,13,4},{1,2,5,16}};
// Preserve the original v0.1 field order/layout and checksum algorithm.
struct PetSave {
  uint32_t magic;
  uint16_t version, size;
  uint32_t checksum;
  bool occupied;
  char name[13], type[13], art[17];
  uint32_t petId, createdDay, lastSeenDay, lifetimeActions, lifetimeTouches, lifetimeTreats;
  float foodEnergy;
  uint8_t treatsToday, dailyActionMask, reserved[2];
  float stats[STATS];
};
static PetSave pets[SLOTS];
static uint8_t slot=0,action=0,menu=0,statIndex=0,seq[3]={},seqIndex=0;
static bool active=false, loaded=false;
enum Mode:uint8_t {HOME,MENU,STATS_VIEW,WATCH,INPUT,RESULT,CONFIRM_CREATE};
static Mode mode=HOME;
static String lines[4];
static uint8_t watchIndex=0;
static unsigned long watchAt=0, responseStart=0;
static String resultMessage;
static bool resultSuccess=false;
static uint32_t fnv(const uint8_t *data,size_t n){
  uint32_t h=2166136261UL;
  for(size_t i=0;i<n;i++){h^=data[i];h*=16777619UL;}
  return h;
}
static uint32_t checksum(PetSave p){p.checksum=0;return fnv((const uint8_t*)&p,sizeof(p));}
static void blank(PetSave &p){memset(&p,0,sizeof(p));p.magic=MAGIC;p.version=1;p.size=sizeof(p);p.foodEnergy=100.0f;}
static bool valid(const PetSave &p){return p.magic==MAGIC&&p.version==1&&p.size==sizeof(p)&&p.checksum==checksum(p);}
static String key(uint8_t i){return String("pet")+i;}
static void save(){
  PetSave &p=pets[slot];p.checksum=checksum(p);
  Preferences nv;
  if(nv.begin("hjpetz",false)){
    nv.putBytes(key(slot).c_str(),&p,sizeof(p));
    nv.putUChar("active",slot);
    nv.end();
  }
}
static void load(){
  if(loaded)return;
  Preferences nv;
  for(uint8_t i=0;i<SLOTS;i++)blank(pets[i]);
  if(nv.begin("hjpetz",true)){
    slot=nv.getUChar("active",0);
    if(slot>=SLOTS)slot=0;
    for(uint8_t i=0;i<SLOTS;i++){
      PetSave candidate;
      if(nv.getBytes(key(i).c_str(),&candidate,sizeof(candidate))==sizeof(candidate)&&valid(candidate))
        pets[i]=candidate;
    }
    nv.end();
  }
  loaded=true;
}
static uint32_t dayKey(){
  time_t now=time(nullptr);
  if(now<1700000000)return 0;
  tm t={};localtime_r(&now,&t);
  return uint32_t(t.tm_year+1900)*10000UL+uint32_t(t.tm_mon+1)*100UL+t.tm_mday;
}
static void daily(){
  PetSave &p=pets[slot];if(!p.occupied)return;
  uint32_t today=dayKey();if(!today)return;
  if(p.lastSeenDay!=today){
    p.lastSeenDay=today;
    if(!p.createdDay)p.createdDay=today;
    p.treatsToday=0;p.dailyActionMask=0;save();
  }
}
static void line(uint8_t i,String s){if(s.length()>21)s=s.substring(0,21);lines[i]=s;}
static void draw(){
  if(!active)return;
  PetSave &p=pets[slot];
  if(mode==CONFIRM_CREATE){
    line(0,String(pets[slot].occupied?"REPLACE PET? SLOT ":"CREATE PET - SLOT ")+String(slot+1));
    line(1,"Default: SPARK");
    line(2,"A CONFIRM B CANCEL");
    line(3,"3 slots / 200 stats");return;
  }
  if(!p.occupied){
    line(0,"BLOOMPETZ - SLOT "+String(slot+1));
    line(1,"EMPTY PET SLOT");
    line(2,"A CREATE  LT/RT SLOT");
    line(3,"B EXIT");return;
  }
  if(mode==HOME){
    line(0,String(p.art));
    line(1,String(p.name)+"  E:"+String((int)p.foodEnergy));
    line(2,String((p.dailyActionMask&(1U<<action))?"* ":"> ")+names[action]);
    line(3,"A PLAY  B MENU");
  }else if(mode==MENU){
    static const char* labels[]={"FEED","TREAT","STATS","NEXT SLOT","CREATE/RESET","EXIT"};
    line(0,String(p.name)+" MENU");
    line(1,String("> ")+labels[menu]);
    line(2,"UP/DN SELECT");
    line(3,"A OK  B BACK");
  }else if(mode==STATS_VIEW){
    line(0,String(p.name)+" STATS");
    line(1,"STAT "+String(statIndex+1)+"/200");
    line(2,String(p.stats[statIndex]*100.0f,4)+"%");
    line(3,"LT/RT NEXT B BACK");
  }else if(mode==WATCH){
    static const char* dirs[]={"UP","DOWN","LEFT","RIGHT"};
    line(0,String(p.name)+" "+names[action]);
    line(1,"WATCH THE PATTERN");
    line(2,watchIndex<3?String(dirs[seq[watchIndex]])+" "+String(watchIndex+1)+"/3":"READY");
    line(3,"B CANCEL");
  }else if(mode==INPUT){
    line(0,String(p.name)+" "+names[action]);
    line(1,"YOUR TURN");
    line(2,String(seqIndex)+"/3 MATCHED");
    line(3,"ARROWS  B CANCEL");
  }else if(mode==RESULT){
    line(0,String(p.name)+" "+names[action]);
    line(1,resultMessage);
    line(2,"A RETRY/OK");
    line(3,"B BACK");
  }
}
static void create(){
  PetSave &p=pets[slot];blank(p);p.occupied=true;
  strncpy(p.name,"SPARK",sizeof(p.name)-1);
  strncpy(p.type,"BLOOMPET",sizeof(p.type)-1);
  strncpy(p.art,"(^_^)",sizeof(p.art)-1);
  p.petId=esp_random();p.createdDay=dayKey();p.lastSeenDay=p.createdDay;
  save();mode=HOME;draw();
}
static void enter(){load();active=true;mode=HOME;daily();draw();}
static void leave(){active=false;mode=HOME;}
static bool running(){return active;}
static uint16_t chooseStat(){
  if(random(100)<80){uint8_t cat=favored[action][random(4)];return uint16_t(cat)*10+random(10);}
  return random(STATS);
}
static void grow(float base){
  PetSave &p=pets[slot];bool chosen[STATS]={};
  uint8_t n=0;uint16_t attempts=0;
  while(n<ROLLS&&attempts++<200){
    uint16_t id=chooseStat();if(chosen[id])continue;chosen[id]=true;
    float wobble=random(850,1151)/1000.0f;
    float gain=constrain(base*wobble,0.0001f,0.0050f);
    p.stats[id]=min(1.0f,p.stats[id]+gain);n++;
  }
}
static void start(){
  PetSave &p=pets[slot];
  if(!p.occupied){mode=CONFIRM_CREATE;draw();return;}
  daily();
  if(p.foodEnergy<12.5f){mode=RESULT;resultMessage="LOW ENERGY";resultSuccess=false;draw();return;}
  for(uint8_t i=0;i<3;i++)seq[i]=random(4);
  watchIndex=0;watchAt=millis();mode=WATCH;draw();
}
static void finish(){
  PetSave &p=pets[slot];
  bool first=!(p.dailyActionMask&(1U<<action));
  float duration=(float)(millis()-responseStart);
  float gain=0.0001f+(1.0f-constrain((duration-600.0f)/5400.0f,0.0f,1.0f))*0.0049f;
  p.lifetimeActions++;
  if(first){
    grow(gain);p.dailyActionMask|=(1U<<action);
    p.foodEnergy=max(0.0f,p.foodEnergy-12.5f);
  }
  save();mode=RESULT;resultSuccess=true;
  resultMessage=first?"DAILY GROWTH!":"FUN ONLY TODAY";draw();
}
static void service(){
  if(!active)return;
  static unsigned long lastDay=0;
  if(millis()-lastDay>30000){lastDay=millis();daily();}
  if(mode==WATCH&&millis()-watchAt>=600){
    watchAt=millis();watchIndex++;
    if(watchIndex>=3){mode=INPUT;seqIndex=0;responseStart=millis();}
    draw();
  }
}
static void button(uint8_t b){// 0 up, 1 down, 2 left, 3 right, 4 A, 5 B
  if(!active)return;
  PetSave &p=pets[slot];
  if(mode==CONFIRM_CREATE){
    if(b==4)create();else if(b==5){mode=HOME;draw();}return;
  }
  if(!p.occupied){
    if(b==4){mode=CONFIRM_CREATE;draw();}
    else if(b==2){slot=(slot+SLOTS-1)%SLOTS;draw();}
    else if(b==3){slot=(slot+1)%SLOTS;draw();}
    else if(b==5){leave();return;}
    return;
  }
  p.lifetimeTouches++;
  if(mode==WATCH){if(b==5){mode=HOME;draw();}return;}
  if(mode==INPUT){
    if(b==5){mode=HOME;draw();return;}
    if(b>=4)return;
    if(b!=seq[seqIndex]){
      mode=RESULT;resultSuccess=false;resultMessage="WRONG - RETRY";draw();return;
    }
    seqIndex++;if(seqIndex>=3)finish();else draw();return;
  }
  if(mode==RESULT){
    if(b==4){if(resultSuccess){mode=HOME;draw();}else start();}
    else if(b==5){mode=HOME;draw();}return;
  }
  if(mode==STATS_VIEW){
    if(b==2)statIndex=(statIndex+199)%200;
    else if(b==3)statIndex=(statIndex+1)%200;
    else if(b==5)mode=MENU;
    draw();return;
  }
  if(mode==HOME){
    if(b==0)action=(action+7)%8;
    else if(b==1)action=(action+1)%8;
    else if(b==4){start();return;}
    else if(b==5){mode=MENU;menu=0;}
    draw();return;
  }
  if(mode==MENU){
    if(b==0)menu=(menu+5)%6;
    else if(b==1)menu=(menu+1)%6;
    else if(b==5)mode=HOME;
    else if(b==4){
      if(menu==0){if(p.foodEnergy<=0.01f){p.foodEnergy=100.0f;save();resultMessage="FED - FULL";}else resultMessage="STILL HAS FOOD";mode=RESULT;resultSuccess=true;}
      else if(menu==1){daily();if(p.treatsToday<3){uint16_t id=random(STATS);p.stats[id]=min(1.0f,p.stats[id]+random(1,51)/10000.0f);p.treatsToday++;p.lifetimeTreats++;save();resultMessage="TREAT +STAT";}else resultMessage="NO TREATS LEFT";mode=RESULT;resultSuccess=true;}
      else if(menu==2){mode=STATS_VIEW;statIndex=0;}
      else if(menu==3){slot=(slot+1)%SLOTS;save();mode=HOME;}
      else if(menu==4){mode=CONFIRM_CREATE;}
      else if(menu==5){leave();return;}
    }
    draw();return;
  }
}
} // namespace HJPet
