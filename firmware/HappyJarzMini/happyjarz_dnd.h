// HAPPY JARZ DND hardware test port: BloomCircuit 2026-10-05 original.
// Gameplay/catalog/room generation from the original 4,321-line Pi firmware.
// Only the display, input, lifecycle and persistence adapters differ.
// NVS namespace 'hjdnd' is isolated from 'hjpetz', 'bloomsys', and 'happyjarz'.
#pragma once
#include <Arduino.h>
#include <Preferences.h>
namespace HJDnd {
enum Input : uint8_t {UP=0,DOWN,LEFT,RIGHT,AKEY,BKEY};
static bool active=false,dirty=false;
static String lines[4];
static void leave();
static void saveBloomSystemState();
static bool loadBloomSystemState();
// BLOOM_DND_PLAYABLE_V0_1
// BLOOM_DND_INFINITE_ROOMS_DEPTH_V1
static constexpr uint8_t DND_VIEW_W = 14;
static constexpr uint8_t DND_VIEW_H = 4;
static constexpr uint8_t DND_HUD_W = 7;
static constexpr uint8_t DND_ROOM_W = 30;
static constexpr uint8_t DND_ROOM_H = 16;
static constexpr uint8_t DND_ENEMY_COUNT = 8;
static constexpr uint8_t DND_EVENT_CAP = 10;
static constexpr uint8_t DND_MENU_COUNT = 4;
static constexpr uint8_t DND_SYMBOL_COUNT = 15;

enum DndScreen : uint8_t { DND_PLAY=0, DND_MENU=1, DND_SYMBOLS=2, DND_INFO=3, DND_INVENTORY=4, DND_SHOP=5, DND_EQUIP_CATEGORIES=6, DND_EQUIP_LIST=7, DND_CHARACTER_STATS=8};

struct DndEnemy {
  char glyph;
  const char *name;
  int8_t x;
  int8_t y;
  int16_t str;
  int16_t dex;
  int16_t con;
  int16_t intel;
  int16_t wis;
  int16_t cha;
  int16_t hp;
  int16_t maxHp;
  int16_t ac;
  int16_t hitBonus;
  int16_t damageMin;
  int16_t damageMax;
  uint16_t xpReward;
  uint16_t goldReward;
  uint16_t moveMinMs;
  uint16_t moveMaxMs;
  uint32_t nextMoveMs;
  bool alive;
};


// BLOOM_DND_CONTENT_EXPANSION_V1
// 48 species; only the existing small actor array is instantiated at once.
struct DndEnemyDef {
  char glyph;
  const char *name;
  uint8_t tier;
  int16_t hp;
  int16_t ac;
  int16_t dmgMin;
  int16_t dmgMax;
  uint16_t moveMin;
  uint16_t moveMax;
};

static const DndEnemyDef DND_ENEMY_CATALOG[48] = {
  {'r',"RAT",0,5,9,1,4,520,900},
  {'b',"BAT",0,4,10,1,3,360,650},
  {'s',"SLIME",0,7,8,1,3,850,1350},
  {'g',"GOBLIN",0,9,11,2,6,820,1350},
  {'k',"KOBOLD",0,8,11,2,5,720,1180},
  {'w',"WOLF",0,10,11,2,6,520,900},
  {'p',"SPIDER",0,7,12,1,5,430,800},
  {'z',"ZOMBIE",0,12,9,2,6,1050,1750},

  {'s',"SKELETON",1,12,12,3,7,1050,1700},
  {'o',"ORC",1,16,12,3,8,900,1450},
  {'h',"HOBGOBLIN",1,15,13,3,7,760,1260},
  {'c',"CULTIST",1,13,12,2,8,820,1380},
  {'m',"MIMIC",1,18,11,3,9,1100,1750},
  {'v',"VIPER",1,11,13,2,7,430,760},
  {'f',"FUNGUS",1,17,10,2,7,1200,1900},
  {'d',"DARKLING",1,14,14,3,8,650,1080},

  {'t',"TROLL",2,26,13,4,11,1150,1800},
  {'O',"OGRE",2,30,12,5,13,1250,1950},
  {'G',"GHOUL",2,22,14,4,10,720,1200},
  {'W',"WRAITH",2,20,15,4,11,560,980},
  {'e',"ELEMENTAL",2,24,15,5,12,760,1250},
  {'n',"NAGA",2,25,14,4,12,690,1160},
  {'B',"BUGBEAR",2,27,14,5,12,860,1420},
  {'a',"ASSASSIN",2,19,16,5,12,390,720},

  {'M',"MINOTAUR",3,38,15,6,15,880,1450},
  {'C',"CHIMERA",3,36,16,6,16,640,1080},
  {'L',"LICH",3,34,17,7,17,820,1380},
  {'D',"DEMON",3,40,17,7,18,650,1100},
  {'V',"VAMPIRE",3,35,18,6,17,500,880},
  {'Y',"WYVERN",3,42,16,7,18,580,1000},
  {'K',"DEATH KNIGHT",3,45,18,7,19,760,1260},
  {'H',"HYDRA",3,50,16,8,20,900,1450},

  {'R',"STONE GOLEM",4,62,19,9,22,1300,2050},
  {'F',"FROST GIANT",4,68,18,10,24,1150,1850},
  {'I',"INFERNAL",4,60,20,10,25,760,1260},
  {'P',"PHANTOM",4,52,21,9,23,430,790},
  {'Q',"QUEEN SPIDER",4,58,20,9,24,520,900},
  {'X',"VOID HORROR",4,70,21,11,27,650,1100},
  {'A',"ABOMINATION",4,76,19,11,28,980,1580},
  {'N',"NIGHTMARE",4,64,22,10,27,390,720},

  {'J',"JUGGERNAUT",5,88,21,12,31,950,1520},
  {'S',"STORM LORD",5,82,23,12,32,560,960},
  {'E',"ELDER LICH",5,78,24,13,34,720,1200},
  {'U',"UNDERKING",5,94,23,13,35,820,1340},
  {'Z',"VOID TITAN",5,110,24,14,38,960,1560},
  {'T',"ANCIENT DRAGON",5,120,25,15,40,660,1120},
  {'!',"CHAOS BEAST",5,105,25,15,39,520,920},
  {'*',"BLOOM WARDEN",5,128,26,16,42,620,1040}
};
static const uint8_t DND_ENEMY_SPECIES_COUNT = 48;

struct DndWeapon {
  String name;
  int16_t damageMin;
  int16_t damageMax;
  int16_t hitBonus;
  uint16_t foundRoom;
};

// BLOOM_DND_INVENTORY_SHOP_V1
// BLOOM_DND_EQUIPMENT_CATALOG_V2
struct DndItemDef {
  const char *name;
  char type;           // W weapon, A armor, D shield, R ring, C charm/relic, H consumable
  uint16_t basePrice;
  int16_t damageMin;
  int16_t damageMax;
  int16_t hitBonus;
  int16_t strBonus;
  int16_t dexBonus;
  int16_t acBonus;
  int16_t hpBonus;
  int16_t healAmount;
  uint8_t rarity;      // 0 common .. 4 legendary
};

static const DndItemDef DND_ITEM_CATALOG[] = {
  {"RUSTY KNIFE",'W',12,1,4,-1,0,0,0,0,0,0},
  {"DAGGER",'W',19,1,4,0,0,0,0,0,0,0},
  {"IRON SWORD",'W',26,1,4,1,0,1,0,0,0,0},
  {"SHORT SWORD",'W',33,1,4,2,0,1,0,0,0,0},
  {"HAND AXE",'W',40,1,4,3,0,1,0,0,0,0},
  {"MACE",'W',47,1,5,-1,0,0,0,0,0,0},
  {"SPEAR",'W',54,1,5,0,0,0,0,0,0,0},
  {"WAR AXE",'W',61,2,6,1,0,0,0,0,0,0},
  {"LONG SWORD",'W',68,2,6,2,0,0,0,0,0,0},
  {"CROSSBOW",'W',75,2,6,3,0,0,0,0,0,0},
  {"BONE BLADE",'W',110,3,9,0,0,0,0,0,0,1},
  {"SILVER SWORD",'W',117,3,9,1,0,1,0,0,0,1},
  {"FROST BLADE",'W',124,3,9,2,0,1,0,0,0,1},
  {"FLAME SWORD",'W',131,3,9,3,0,1,0,0,0,1},
  {"VOID AXE",'W',138,4,10,4,0,0,0,0,0,1},
  {"DRAGON BLADE",'W',145,4,11,0,0,0,0,0,0,1},
  {"BRONZE SABER",'W',152,4,11,1,1,0,0,0,0,1},
  {"HUNTER BOW",'W',159,4,11,2,1,0,0,0,0,1},
  {"WAR HAMMER",'W',166,4,11,3,1,0,0,0,0,1},
  {"TRIDENT",'W',173,4,11,4,1,0,0,0,0,1},
  {"SCIMITAR",'W',208,5,14,1,1,1,0,0,0,2},
  {"RAPIER",'W',215,6,15,2,1,1,0,0,0,2},
  {"BATTLE STAFF",'W',222,6,15,3,1,1,0,0,0,2},
  {"CLEAVER",'W',229,6,15,4,1,0,0,0,0,2},
  {"MOON BLADE",'W',236,6,15,5,1,0,0,0,0,2},
  {"SUN SPEAR",'W',243,6,16,1,1,0,0,0,0,2},
  {"THORN WHIP",'W',250,6,16,2,1,0,0,0,0,2},
  {"CRYSTAL MACE",'W',257,6,16,3,1,0,0,0,0,2},
  {"STORM BOW",'W',264,7,17,4,1,0,0,0,0,2},
  {"EMBER AXE",'W',271,7,17,5,1,1,0,0,0,2},
  {"ICE SPEAR",'W',306,8,20,2,1,1,0,0,0,3},
  {"SHADOW DAGGER",'W',313,8,20,3,1,1,0,0,0,3},
  {"ROYAL SWORD",'W',320,8,20,4,2,0,0,0,0,3},
  {"GIANT HAMMER",'W',327,8,20,5,2,0,0,0,0,3},
  {"PHANTOM BLADE",'W',334,8,20,6,2,0,0,0,0,3},
  {"RUNE STAFF",'W',341,9,22,2,2,0,0,0,0,3},
  {"BLOOD AXE",'W',348,9,22,3,2,0,0,0,0,3},
  {"STAR PIKE",'W',355,9,22,4,2,0,0,0,0,3},
  {"CHAOS BOW",'W',362,9,22,5,2,1,0,0,0,3},
  {"WARDEN MACE",'W',369,9,22,6,2,1,0,0,0,3},
  {"TITAN SWORD",'W',404,10,25,3,2,1,0,0,0,4},
  {"NIGHT REAPER",'W',411,10,25,4,2,0,0,0,0,4},
  {"CELESTIAL SPEAR",'W',418,11,26,5,2,0,0,0,0,4},
  {"INFERNO BLADE",'W',425,11,26,6,2,0,0,0,0,4},
  {"ABYSS HAMMER",'W',432,11,26,7,2,0,0,0,0,4},
  {"WORLD AXE",'W',439,11,27,3,2,0,0,0,0,4},
  {"BLOOM EDGE",'W',446,11,27,4,2,0,0,0,0,4},
  {"CROWN BLADE",'W',453,11,27,5,2,1,0,0,0,4},
  {"LEATHER CAP",'A',14,0,0,0,0,0,1,0,0,0},
  {"LEATHER ARMOR",'A',22,0,0,0,0,0,1,0,0,0},
  {"IRON HELM",'A',30,0,0,0,0,0,1,0,0,0},
  {"CHAIN MAIL",'A',38,0,0,0,0,0,1,0,0,0},
  {"IRON ARMOR",'A',46,0,0,0,0,0,1,0,0,0},
  {"STEEL ARMOR",'A',54,0,0,0,0,0,1,0,0,0},
  {"DRAGON SCALE",'A',62,0,0,0,0,0,1,0,0,0},
  {"PADDED COAT",'A',70,0,0,0,0,0,1,0,0,0},
  {"HIDE ARMOR",'A',78,0,0,0,0,0,2,0,0,0},
  {"BRONZE MAIL",'A',86,0,0,0,0,-1,2,0,0,0},
  {"SCALE MAIL",'A',126,0,0,0,0,-1,3,1,0,1},
  {"RANGER COAT",'A',134,0,0,0,0,-1,3,1,0,1},
  {"SOLDIER MAIL",'A',142,0,0,0,0,0,3,3,0,1},
  {"KNIGHT PLATE",'A',150,0,0,0,0,0,3,3,0,1},
  {"MITHRIL MAIL",'A',158,0,0,0,0,0,3,3,0,1},
  {"SILVER PLATE",'A',166,0,0,0,0,0,3,3,0,1},
  {"FROST MAIL",'A',174,0,0,0,0,0,4,3,0,1},
  {"FLAME MAIL",'A',182,0,0,0,0,0,4,3,0,1},
  {"THORN MAIL",'A',190,0,0,0,0,0,4,3,0,1},
  {"BONE ARMOR",'A',198,0,0,0,0,0,4,3,0,1},
  {"ROYAL PLATE",'A',238,0,0,0,0,0,5,4,0,2},
  {"RUNE ROBE",'A',246,0,0,0,0,-1,5,4,0,2},
  {"SHADOW COAT",'A',254,0,0,0,0,-1,5,4,0,2},
  {"CRYSTAL MAIL",'A',262,0,0,0,0,-1,5,4,0,2},
  {"STORM PLATE",'A',270,0,0,0,0,0,6,6,0,2},
  {"EMBER MAIL",'A',278,0,0,0,0,0,6,6,0,2},
  {"MOON ROBE",'A',286,0,0,0,0,0,6,6,0,2},
  {"SUN PLATE",'A',294,0,0,0,0,0,6,6,0,2},
  {"VOID MAIL",'A',302,0,0,0,0,0,6,6,0,2},
  {"BLOOD PLATE",'A',310,0,0,0,0,0,6,6,0,2},
  {"WARDEN ARMOR",'A',350,0,0,0,0,0,7,7,0,3},
  {"PHANTOM MAIL",'A',358,0,0,0,0,0,7,7,0,3},
  {"GIANT PLATE",'A',366,0,0,0,0,0,8,7,0,3},
  {"DRAGON PLATE",'A',374,0,0,0,0,-1,8,7,0,3},
  {"DEMON MAIL",'A',382,0,0,0,0,-1,8,7,0,3},
  {"ANGEL ROBE",'A',390,0,0,0,0,-1,8,7,0,3},
  {"CHAOS PLATE",'A',398,0,0,0,1,0,8,9,0,3},
  {"STAR ARMOR",'A',406,0,0,0,0,0,8,9,0,3},
  {"TITAN MAIL",'A',414,0,0,0,0,0,8,9,0,3},
  {"NIGHT PLATE",'A',422,0,0,0,1,0,8,9,0,3},
  {"ABYSS ARMOR",'A',462,0,0,0,0,0,10,10,0,4},
  {"CELESTIAL PLATE",'A',470,0,0,0,0,0,10,10,0,4},
  {"INFERNO MAIL",'A',478,0,0,0,1,0,10,10,0,4},
  {"WORLD PLATE",'A',486,0,0,0,0,0,10,10,0,4},
  {"BLOOM MAIL",'A',494,0,0,0,0,0,10,10,0,4},
  {"ANCIENT PLATE",'A',502,0,0,0,1,-1,10,10,0,4},
  {"CROWN ARMOR",'A',510,0,0,0,0,-1,10,10,0,4},
  {"WARDEN PLATE",'A',518,0,0,0,0,-1,10,10,0,4},
  {"WOOD SHIELD",'D',10,0,0,0,0,0,1,0,0,0},
  {"IRON SHIELD",'D',17,0,0,0,0,0,1,0,0,0},
  {"TOWER SHIELD",'D',24,0,0,0,0,0,1,0,0,0},
  {"BUCKLER",'D',31,0,0,0,0,1,1,0,0,0},
  {"BRONZE SHIELD",'D',38,0,0,0,0,1,1,0,0,0},
  {"BONE SHIELD",'D',45,0,0,0,0,0,1,0,0,0},
  {"ROUND SHIELD",'D',52,0,0,0,0,0,1,0,0,0},
  {"KITE SHIELD",'D',59,0,0,0,0,0,1,0,0,0},
  {"STEEL SHIELD",'D',66,0,0,0,0,0,1,0,0,0},
  {"SPIKE SHIELD",'D',73,0,0,0,0,0,1,0,0,0},
  {"RANGER SHIELD",'D',110,0,0,-1,0,-1,3,1,0,1},
  {"SILVER SHIELD",'D',117,0,0,-1,0,-1,3,1,0,1},
  {"FROST SHIELD",'D',124,0,0,0,0,0,3,1,0,1},
  {"FLAME SHIELD",'D',131,0,0,0,0,0,3,1,0,1},
  {"THORN SHIELD",'D',138,0,0,0,0,0,3,1,0,1},
  {"RUNE SHIELD",'D',145,0,0,0,0,1,3,1,0,1},
  {"MOON SHIELD",'D',152,0,0,0,0,1,3,2,0,1},
  {"SUN SHIELD",'D',159,0,0,0,0,0,3,2,0,1},
  {"CRYSTAL SHIELD",'D',166,0,0,0,0,0,3,2,0,1},
  {"STORM SHIELD",'D',173,0,0,0,0,0,3,2,0,1},
  {"EMBER SHIELD",'D',210,0,0,0,0,0,5,3,0,2},
  {"SHADOW SHIELD",'D',217,0,0,0,0,0,5,3,0,2},
  {"ROYAL SHIELD",'D',224,0,0,-1,0,-1,5,3,0,2},
  {"DRAGON SHIELD",'D',231,0,0,-1,0,-1,5,3,0,2},
  {"PHANTOM SHIELD",'D',238,0,0,0,0,0,5,3,0,2},
  {"BLOOD SHIELD",'D',245,0,0,0,0,0,5,3,0,2},
  {"VOID SHIELD",'D',252,0,0,0,0,0,5,3,0,2},
  {"WARDEN SHIELD",'D',259,0,0,0,0,1,5,3,0,2},
  {"GIANT SHIELD",'D',266,0,0,0,0,1,5,3,0,2},
  {"DEMON SHIELD",'D',273,0,0,0,0,0,5,3,0,2},
  {"ANGEL SHIELD",'D',310,0,0,0,0,0,7,4,0,3},
  {"CHAOS SHIELD",'D',317,0,0,0,0,0,7,4,0,3},
  {"STAR SHIELD",'D',324,0,0,0,0,0,7,5,0,3},
  {"TITAN SHIELD",'D',331,0,0,0,0,0,7,5,0,3},
  {"NIGHT SHIELD",'D',338,0,0,-1,0,-1,7,5,0,3},
  {"ABYSS SHIELD",'D',345,0,0,-1,1,-1,7,5,0,3},
  {"CELESTIAL SHIELD",'D',352,0,0,0,0,0,7,5,0,3},
  {"INFERNO SHIELD",'D',359,0,0,0,0,0,7,5,0,3},
  {"WORLD SHIELD",'D',366,0,0,0,0,0,7,5,0,3},
  {"BLOOM SHIELD",'D',373,0,0,0,0,1,7,5,0,3},
  {"ANCIENT SHIELD",'D',410,0,0,0,1,1,9,6,0,4},
  {"CROWN SHIELD",'D',417,0,0,0,0,0,9,6,0,4},
  {"MIRROR SHIELD",'D',424,0,0,0,0,0,9,6,0,4},
  {"OBSIDIAN SHIELD",'D',431,0,0,0,0,0,9,6,0,4},
  {"AETHER SHIELD",'D',438,0,0,0,0,0,9,6,0,4},
  {"NOVA SHIELD",'D',445,0,0,0,1,0,9,6,0,4},
  {"ETERNAL SHIELD",'D',452,0,0,-1,0,-1,9,6,0,4},
  {"FINAL SHIELD",'D',459,0,0,-1,0,-1,9,6,0,4},
  {"COPPER RING",'R',18,0,0,0,1,0,0,0,0,0},
  {"LUCKY RING",'R',27,0,0,0,0,1,0,0,0,0},
  {"RUBY RING",'R',36,0,0,0,0,0,1,0,0,0},
  {"EMERALD RING",'R',45,0,0,0,0,0,0,2,0,0},
  {"SAPPHIRE RING",'R',54,0,0,0,1,0,0,0,0,0},
  {"SKULL RING",'R',63,0,0,0,0,1,0,0,0,0},
  {"ROYAL SEAL",'R',72,0,0,0,0,0,1,0,0,0},
  {"IRON RING",'R',81,0,0,0,0,0,0,2,0,0},
  {"SILVER RING",'R',90,0,0,0,1,0,0,0,0,0},
  {"GOLD RING",'R',99,0,0,0,0,1,0,0,0,0},
  {"AGATE RING",'R',146,0,0,0,0,0,1,1,0,1},
  {"ONYX RING",'R',155,0,0,0,0,0,0,4,0,1},
  {"MOON RING",'R',164,0,0,0,1,0,0,1,0,1},
  {"SUN RING",'R',173,0,0,0,0,1,0,1,0,1},
  {"FROST RING",'R',182,0,0,0,0,0,1,1,0,1},
  {"FLAME RING",'R',191,0,0,0,0,0,0,4,0,1},
  {"THORN RING",'R',200,0,0,0,1,0,0,1,0,1},
  {"BONE RING",'R',209,0,0,0,0,1,0,1,0,1},
  {"RUNE RING",'R',218,0,0,0,0,0,1,1,0,1},
  {"CRYSTAL RING",'R',227,0,0,0,0,0,0,4,0,1},
  {"STORM RING",'R',274,0,0,0,1,0,0,2,0,2},
  {"EMBER RING",'R',283,0,0,0,0,1,0,2,0,2},
  {"SHADOW RING",'R',292,0,0,0,0,0,1,2,0,2},
  {"DRAGON RING",'R',301,0,0,0,0,0,0,6,0,2},
  {"PHANTOM RING",'R',310,0,0,0,1,0,0,2,0,2},
  {"BLOOD RING",'R',319,0,0,0,0,1,0,2,0,2},
  {"VOID RING",'R',328,0,0,0,0,0,1,2,0,2},
  {"WARDEN RING",'R',337,0,0,0,0,0,0,6,0,2},
  {"GIANT RING",'R',346,0,0,0,1,0,0,2,0,2},
  {"DEMON RING",'R',355,0,0,0,0,1,0,2,0,2},
  {"ANGEL RING",'R',402,0,0,0,0,0,1,3,0,3},
  {"CHAOS RING",'R',411,0,0,0,0,0,0,8,0,3},
  {"STAR RING",'R',420,0,0,0,2,0,0,3,0,3},
  {"TITAN RING",'R',429,0,0,0,0,2,0,3,0,3},
  {"NIGHT RING",'R',438,0,0,0,0,0,1,3,0,3},
  {"ABYSS RING",'R',447,0,0,0,0,0,0,8,0,3},
  {"CELESTIAL RING",'R',456,0,0,0,1,0,0,3,0,3},
  {"INFERNO RING",'R',465,0,0,0,0,1,0,3,0,3},
  {"WORLD RING",'R',474,0,0,0,0,0,1,3,0,3},
  {"BLOOM RING",'R',483,0,0,0,0,0,0,8,0,3},
  {"ANCIENT RING",'R',530,0,0,0,2,0,1,4,0,4},
  {"CROWN RING",'R',539,0,0,0,0,2,1,4,0,4},
  {"MIRROR RING",'R',548,0,0,0,0,0,1,4,0,4},
  {"AETHER RING",'R',557,0,0,0,0,0,1,10,0,4},
  {"NOVA RING",'R',566,0,0,0,1,0,1,4,0,4},
  {"ETERNAL RING",'R',575,0,0,0,0,1,1,4,0,4},
  {"FATE RING",'R',584,0,0,0,0,0,1,4,0,4},
  {"FINAL RING",'R',593,0,0,0,0,0,1,10,0,4},
  {"GOBLIN TOOTH",'C',22,0,0,0,1,0,0,2,0,0},
  {"BONE CHARM",'C',32,0,0,0,0,1,0,2,0,0},
  {"MOON CHARM",'C',42,0,0,0,0,0,1,2,0,0},
  {"SUN CHARM",'C',52,0,0,0,0,0,0,4,0,0},
  {"ANCIENT IDOL",'C',62,0,0,0,0,0,0,2,0,0},
  {"BLACK PEARL",'C',72,0,0,0,1,0,0,2,0,0},
  {"STAR RELIC",'C',82,0,0,0,0,1,0,2,0,0},
  {"VOID HEART",'C',92,0,0,0,0,0,1,2,0,0},
  {"CROWN OF BLOOM",'C',102,0,0,0,0,0,0,4,0,0},
  {"RAT TAIL",'C',112,0,0,0,0,0,0,2,0,0},
  {"WOLF FANG",'C',164,0,0,0,2,1,0,5,0,1},
  {"SPIDER EYE",'C',174,0,0,0,1,2,0,5,0,1},
  {"ORC TUSK",'C',184,0,0,0,1,1,1,5,0,1},
  {"TROLL STONE",'C',194,0,0,0,1,1,0,7,0,1},
  {"WYVERN SCALE",'C',204,0,0,0,1,1,0,5,0,1},
  {"DRAGON EYE",'C',214,0,0,0,2,1,0,5,0,1},
  {"FROST RELIC",'C',224,0,0,0,1,2,0,5,0,1},
  {"FLAME RELIC",'C',234,0,0,0,1,1,1,5,0,1},
  {"THORN RELIC",'C',244,0,0,0,1,1,0,7,0,1},
  {"RUNE RELIC",'C',254,0,0,0,1,1,0,5,0,1},
  {"CRYSTAL RELIC",'C',306,0,0,0,3,2,1,8,0,2},
  {"STORM RELIC",'C',316,0,0,0,2,3,1,8,0,2},
  {"EMBER RELIC",'C',326,0,0,0,2,2,2,8,0,2},
  {"SHADOW RELIC",'C',336,0,0,0,2,2,1,10,0,2},
  {"ROYAL RELIC",'C',346,0,0,0,2,2,1,8,0,2},
  {"PHANTOM RELIC",'C',356,0,0,0,3,2,1,8,0,2},
  {"BLOOD RELIC",'C',366,0,0,0,2,3,1,8,0,2},
  {"WARDEN RELIC",'C',376,0,0,0,2,2,2,8,0,2},
  {"GIANT RELIC",'C',386,0,0,0,2,2,1,10,0,2},
  {"DEMON RELIC",'C',396,0,0,0,2,2,1,8,0,2},
  {"ANGEL RELIC",'C',448,0,0,0,4,3,1,11,0,3},
  {"CHAOS RELIC",'C',458,0,0,0,3,4,1,11,0,3},
  {"TITAN RELIC",'C',468,0,0,0,3,3,2,11,0,3},
  {"NIGHT RELIC",'C',478,0,0,0,3,3,1,13,0,3},
  {"ABYSS RELIC",'C',488,0,0,0,3,3,1,11,0,3},
  {"CELESTIAL RELIC",'C',498,0,0,0,4,3,1,11,0,3},
  {"INFERNO RELIC",'C',508,0,0,0,3,4,1,11,0,3},
  {"WORLD RELIC",'C',518,0,0,0,3,3,2,11,0,3},
  {"BLOOM RELIC",'C',528,0,0,0,3,3,1,13,0,3},
  {"ANCIENT HEART",'C',538,0,0,0,3,3,1,11,0,3},
  {"CROWN RELIC",'C',590,0,0,0,5,4,2,14,0,4},
  {"MIRROR RELIC",'C',600,0,0,0,4,5,2,14,0,4},
  {"OBSIDIAN IDOL",'C',610,0,0,0,4,4,3,14,0,4},
  {"AETHER HEART",'C',620,0,0,0,4,4,2,16,0,4},
  {"NOVA HEART",'C',630,0,0,0,4,4,2,14,0,4},
  {"ETERNAL IDOL",'C',640,0,0,0,5,4,2,14,0,4},
  {"FATE RELIC",'C',650,0,0,0,4,5,2,14,0,4},
  {"FINAL RELIC",'C',660,0,0,0,4,4,3,14,0,4},
  {"BANDAGE",'H',8,0,0,0,0,0,0,0,5,0},
  {"RED POTION",'H',13,0,0,0,0,0,0,0,7,0},
  {"BIG RED POTION",'H',18,0,0,0,0,0,0,0,9,0},
  {"ELIXIR",'H',23,0,0,0,0,0,0,0,11,0},
  {"PHOENIX TONIC",'H',28,0,0,0,0,0,0,0,13,0},
  {"FULL RESTORE",'H',33,0,0,0,0,0,0,0,9999,0},
  {"HERB",'H',38,0,0,0,0,0,0,0,17,0},
  {"HEALING HERB",'H',43,0,0,0,0,0,0,0,19,0},
  {"MOSS TONIC",'H',48,0,0,0,0,0,0,0,21,0},
  {"HONEY DRAUGHT",'H',53,0,0,0,0,0,0,0,23,0},
  {"IRON TONIC",'H',78,0,0,0,0,0,0,0,33,1},
  {"SILVER TONIC",'H',83,0,0,0,0,0,0,0,35,1},
  {"FROST TONIC",'H',88,0,0,0,0,0,0,0,37,1},
  {"FLAME TONIC",'H',93,0,0,0,0,0,0,0,39,1},
  {"THORN TONIC",'H',98,0,0,0,0,0,0,0,41,1},
  {"RUNE TONIC",'H',103,0,0,0,0,0,0,0,43,1},
  {"MOON TONIC",'H',108,0,0,0,0,0,0,0,45,1},
  {"SUN TONIC",'H',113,0,0,0,0,0,0,0,47,1},
  {"CRYSTAL TONIC",'H',118,0,0,0,0,0,0,0,49,1},
  {"STORM TONIC",'H',123,0,0,0,0,0,0,0,51,1},
  {"EMBER TONIC",'H',148,0,0,0,0,0,0,0,61,2},
  {"SHADOW TONIC",'H',153,0,0,0,0,0,0,0,63,2},
  {"ROYAL TONIC",'H',158,0,0,0,0,0,0,0,65,2},
  {"DRAGON TONIC",'H',163,0,0,0,0,0,0,0,67,2},
  {"PHANTOM TONIC",'H',168,0,0,0,0,0,0,0,69,2},
  {"BLOOD TONIC",'H',173,0,0,0,0,0,0,0,71,2},
  {"VOID TONIC",'H',178,0,0,0,0,0,0,0,73,2},
  {"WARDEN TONIC",'H',183,0,0,0,0,0,0,0,75,2},
  {"GIANT TONIC",'H',188,0,0,0,0,0,0,0,77,2},
  {"DEMON TONIC",'H',193,0,0,0,0,0,0,0,79,2},
  {"ANGEL TONIC",'H',218,0,0,0,0,0,0,0,89,3},
  {"CHAOS TONIC",'H',223,0,0,0,0,0,0,0,91,3},
  {"STAR TONIC",'H',228,0,0,0,0,0,0,0,93,3},
  {"TITAN TONIC",'H',233,0,0,0,0,0,0,0,95,3},
  {"NIGHT TONIC",'H',238,0,0,0,0,0,0,0,97,3},
  {"ABYSS TONIC",'H',243,0,0,0,0,0,0,0,99,3},
  {"CELESTIAL TONIC",'H',248,0,0,0,0,0,0,0,101,3},
  {"INFERNO TONIC",'H',253,0,0,0,0,0,0,0,103,3},
  {"WORLD TONIC",'H',258,0,0,0,0,0,0,0,105,3},
  {"BLOOM TONIC",'H',263,0,0,0,0,0,0,0,107,3},
  {"ANCIENT TONIC",'H',288,0,0,0,0,0,0,0,117,4},
  {"CROWN TONIC",'H',293,0,0,0,0,0,0,0,119,4},
  {"MIRROR TONIC",'H',298,0,0,0,0,0,0,0,121,4},
  {"AETHER TONIC",'H',303,0,0,0,0,0,0,0,123,4},
  {"NOVA TONIC",'H',308,0,0,0,0,0,0,0,125,4},
  {"ETERNAL TONIC",'H',313,0,0,0,0,0,0,0,127,4},
  {"FATE TONIC",'H',318,0,0,0,0,0,0,0,129,4},
  {"FINAL TONIC",'H',323,0,0,0,0,0,0,0,131,4}
};
static const uint16_t DND_ITEM_COUNT = sizeof(DND_ITEM_CATALOG)/sizeof(DND_ITEM_CATALOG[0]);
static const uint16_t DND_INV_CAP = 288;
static const uint8_t DND_SHOP_COUNT = 4;
static const uint16_t DND_NO_ITEM = 0xFFFFU;

static uint16_t dndShop[DND_SHOP_COUNT] = {0,1,2,3};
static uint8_t dndShopIndex = 0;
static uint16_t dndShopRoom = 0;
static uint16_t dndInventory[DND_INV_CAP];
static uint16_t dndInventoryCount = 0;
static uint16_t dndInventoryIndex = 0;

static uint16_t dndEquipWeapon = DND_NO_ITEM;
static uint16_t dndEquipArmor = DND_NO_ITEM;
static uint16_t dndEquipShield = DND_NO_ITEM;
static uint16_t dndEquipRing1 = DND_NO_ITEM;
static uint16_t dndEquipRing2 = DND_NO_ITEM;
static uint16_t dndEquipCharm = DND_NO_ITEM;
static uint8_t dndEquipCategory = 0;
static uint16_t dndEquipOwnedIndex = 0;
// BLOOM_DND_CHARACTER_STATS_V1
static const uint8_t DND_EQUIP_CATEGORY_COUNT = 8;
static const char* const DND_EQUIP_CATEGORY_NAMES[DND_EQUIP_CATEGORY_COUNT] = {
  "STATS","WEAPON","ARMOR","SHIELD","RING 1","RING 2","CHARM","CONSUMABLES"
};

static DndScreen dndScreen = DND_PLAY;
static char dndMap[DND_ROOM_H][DND_ROOM_W + 1];
static int8_t dndPlayerX = 2;
static int8_t dndPlayerY = 2;
static int8_t dndCamX = 0;
static int8_t dndCamY = 0;

static int16_t dndHp = 18;
static int16_t dndMaxHp = 18;
static int16_t dndStrength = 3;
static int16_t dndDexterity = 2;
static int16_t dndArmorClass = 12;
static uint32_t dndXp = 0;
static uint32_t dndGold = 0;
static uint16_t dndLevel = 1;
static uint16_t dndKeys = 0;
static uint16_t dndRoomNumber = 1;
static uint16_t dndDeepestRoom = 1;
// BLOOM_DND_ROOM_ARCHETYPES_V1
enum DndRoomStyle : uint8_t {
  DND_ROOM_RUINS=0,
  DND_ROOM_HALL,
  DND_ROOM_CORRIDORS,
  DND_ROOM_CHAMBERS,
  DND_ROOM_TRAP_RUN,
  DND_ROOM_TREASURE,
  DND_ROOM_DEN,
  DND_ROOM_VOID
};
static DndRoomStyle dndRoomStyle = DND_ROOM_RUINS;
static const char* const DND_ROOM_STYLE_NAMES[8] = {
  "RUINS", "HALL", "CORRIDORS", "CHAMBERS",
  "TRAP RUN", "TREASURE", "DEN", "VOID"
};

static const char* const DND_ROOM_THEME_NAMES[48] = {
  "CRUMBLING RUINS","TORCH HALL","BROKEN CORRIDORS","DUST CHAMBERS","SPIKE RUN","COIN VAULT","RAT DEN","BLACK VOID",
  "MOSS RUINS","BANNER HALL","BONE CORRIDORS","CRYPT CHAMBERS","BLADE RUN","JEWEL VAULT","GOBLIN DEN","ECHO VOID",
  "FROST RUINS","IRON HALL","WEB CORRIDORS","MUMMY CHAMBERS","FIRE RUN","RELIC VAULT","WOLF DEN","STARLESS VOID",
  "SUNKEN RUINS","ROYAL HALL","ROOT CORRIDORS","RUNE CHAMBERS","POISON RUN","DRAGON VAULT","TROLL DEN","DEEP VOID",
  "ASH RUINS","MIRROR HALL","SHADOW CORRIDORS","BLOOD CHAMBERS","CHAOS RUN","ANCIENT VAULT","DEMON DEN","DREAM VOID",
  "BLOOM RUINS","CROWN HALL","CRYSTAL CORRIDORS","WARDEN CHAMBERS","VOID RUN","FINAL VAULT","TITAN DEN","BLOOM VOID"
};
static uint8_t dndRoomThemeIndex = 0;
static DndWeapon dndWeapon = {"IRON SWORD", 2, 6, 0, 1};

static uint32_t dndAttackArmedUntilMs = 0;
static uint8_t dndMenuIndex = 0;
static uint8_t dndSymbolIndex = 0;
static uint16_t dndMenuMarquee = 0;
static uint32_t dndMenuMarqueeMs = 0;
static String dndInfoTitle;
static String dndInfoBody;
static DndEnemy dndEnemies[DND_ENEMY_COUNT];

static String dndEvents[DND_EVENT_CAP];
static uint8_t dndEventHead = 0;
static uint8_t dndEventTail = 0;
static String dndCurrentEvent = "EXPLORE";
static uint16_t dndEventMarquee = 0;
static uint32_t dndEventMarqueeMs = 0;
static uint32_t dndEventUntilMs = 0;
static uint8_t dndStatusFirst = 0;
static uint16_t dndStatusMarquee[3] = {0,0,0};
static uint32_t dndStatusMarqueeMs[3] = {0,0,0};
static uint32_t dndStatusRotateMs = 0;
static uint32_t dndLastRenderMs = 0;

static const char* const DND_MENU_ITEMS[DND_MENU_COUNT] = {
  "CHARACTER", "INVENTORY", "SYMBOLS", "SYSTEM MENU"
};
static const uint8_t DND_SYMBOL_COUNT_EXPANDED = 48;
static const char DND_SYMBOL_GLYPHS[DND_SYMBOL_COUNT_EXPANDED] = {
  '@','#','.','+','/','^','c','$','!','w','k','?','~','*','x','r',
  'b','s','g','K','W','p','z','o','h','C','m','v','f','d','t','O',
  'G','R','e','n','B','a','M','L','D','V','Y','H','X','T','Z','&'
};
static const char* const DND_SYMBOL_NAMES[DND_SYMBOL_COUNT_EXPANDED] = {
  "PLAYER","WALL","FLOOR","NEXT ROOM DOOR","OPEN DOOR","TRAP","CHEST","GOLD",
  "POTION","WEAPON","KEY","MYSTERY","WATER OR HAZARD","MAGIC OR RELIC","HIT MARK","RAT",
  "BAT","SLIME","GOBLIN","KOBOLD","WOLF","SPIDER","ZOMBIE","ORC",
  "HOBGOBLIN","CULTIST","MIMIC","VIPER","FUNGUS","DARKLING","TROLL","OGRE",
  "GHOUL","STONE GOLEM","ELEMENTAL","NAGA","BUGBEAR","ASSASSIN","MINOTAUR","LICH",
  "DEMON","VAMPIRE","WYVERN","HYDRA","VOID HORROR","ANCIENT DRAGON","VOID TITAN","SPECIAL ENTITY"
};

static int dndDepthForRoll() {
  // ROOM is the scaling source. Cap only prevents absurd integer ranges far beyond
  // normal play; ROOM/deepest counters themselves remain unbounded uint16 values.
  return dndRoomNumber > 120 ? 120 : (int)dndRoomNumber;
}

static int dndSignedRoomMod() {
  int depth = dndDepthForRoll();
  return (int)random(-depth, depth + 1);
}

static int dndScaled(int baseValue, int minimumValue) {
  long v = (long)baseValue + (long)dndSignedRoomMod();
  if (v < minimumValue) v = minimumValue;
  if (v > 30000) v = 30000;
  return (int)v;
}

static int dndEnemyScaled(int baseValue, int minimumValue) {
  // Independent +/-ROOM volatility plus a gentle average upward pressure. ROOM 30
  // can still roll weak individual stats, but the population trends stronger.
  int depth = dndDepthForRoll();
  long v = (long)baseValue + (long)(depth / 3) + (long)dndSignedRoomMod();
  if (v < minimumValue) v = minimumValue;
  if (v > 30000) v = 30000;
  return (int)v;
}

static String dndWindow(const String &src, uint16_t off, uint8_t width) {
  if (src.length() <= width) {
    String out = src;
    while (out.length() < width) out += ' ';
    return out;
  }
  String looped = src + "    " + src + "    ";
  uint16_t cycle = (uint16_t)(src.length() + 4U);
  uint16_t start = cycle ? (off % cycle) : 0;
  return looped.substring(start, start + width);
}

static void dndQueue(const String &msg) {
  uint8_t next = (uint8_t)((dndEventTail + 1U) % DND_EVENT_CAP);
  if (next == dndEventHead) dndEventHead = (uint8_t)((dndEventHead + 1U) % DND_EVENT_CAP);
  dndEvents[dndEventTail] = msg;
  dndEventTail = next;
}

static void dndServiceEventQueue() {
  uint32_t now = millis();
  if ((int32_t)(now - dndEventUntilMs) >= 0 && dndEventHead != dndEventTail) {
    dndCurrentEvent = dndEvents[dndEventHead];
    dndEventHead = (uint8_t)((dndEventHead + 1U) % DND_EVENT_CAP);
    dndEventMarquee = 0;
    dndEventMarqueeMs = now;
    dndEventUntilMs = now + 1050UL;
  }
  if (dndCurrentEvent.length() > DND_HUD_W && now - dndEventMarqueeMs >= 260UL) {
    dndEventMarqueeMs = now;
    dndEventMarquee++;
  }
}

static uint16_t dndLevelFromXp(uint32_t xp) {
  // Smooth tiny-RPG curve: level 2 at 40 XP, then increasingly expensive.
  uint16_t level = 1;
  uint32_t need = 40;
  uint32_t spent = 0;
  while (level < 99 && xp >= spent + need) {
    spent += need;
    level++;
    need += 20U + (uint32_t)level * 8U;
  }
  return level;
}

static void dndCheckLevelUp() {
  uint16_t target = dndLevelFromXp(dndXp);
  while (dndLevel < target) {
    dndLevel++;
    int hpGain = 2 + (int)(dndLevel % 3U);
    dndMaxHp += hpGain;
    dndHp += hpGain;
    if ((dndLevel % 2U) == 0U) dndStrength++;
    if ((dndLevel % 3U) == 0U) dndDexterity++;
    if ((dndLevel % 4U) == 0U) dndArmorClass++;
    dndQueue("LEVEL " + String(dndLevel));
    dndQueue("MAX HP +" + String(hpGain));
  }
}

static String dndStatusText(uint8_t idx) {
  switch (idx % 9U) {
    case 0: return "ROOM " + String(dndRoomNumber);
    case 1: return "HEALTH " + String(dndHp) + "/" + String(dndEffectiveMaxHp());
    case 2: return "ARMOR CLASS " + String(dndEffectiveArmorClass());
    case 3: return "LEVEL " + String(dndLevel);
    case 4: return "EXPERIENCE " + String(dndXp);
    case 5: return "GOLD " + String(dndGold);
    case 6: return dndWeapon.name + " " + String(dndWeapon.damageMin) + "-" + String(dndWeapon.damageMax);
    case 7: return "DEEPEST " + String(dndDeepestRoom);
    default: return "KEYS " + String(dndKeys);
  }
}

static void dndServiceHudScroll() {
  uint32_t now = millis();
  if (now - dndStatusRotateMs >= 1900UL) {
    dndStatusRotateMs = now;
    dndStatusFirst = (uint8_t)((dndStatusFirst + 1U) % 9U);
    for (uint8_t i=0;i<3;++i) dndStatusMarquee[i] = 0;
  }
  for (uint8_t row=0; row<3; ++row) {
    String msg = dndStatusText((uint8_t)(dndStatusFirst + row));
    if (msg.length() > DND_HUD_W && now - dndStatusMarqueeMs[row] >= 330UL) {
      dndStatusMarqueeMs[row] = now;
      dndStatusMarquee[row]++;
    }
  }
}

static int8_t dndEnemyAt(int8_t x, int8_t y) {
  for (uint8_t i=0; i<DND_ENEMY_COUNT; ++i) {
    if (dndEnemies[i].alive && dndEnemies[i].x == x && dndEnemies[i].y == y) return (int8_t)i;
  }
  return -1;
}

static bool dndInside(int x, int y) {
  return x >= 0 && x < DND_ROOM_W && y >= 0 && y < DND_ROOM_H;
}

static bool dndWalkable(int8_t x, int8_t y) {
  if (!dndInside(x, y)) return false;
  return dndMap[(uint8_t)y][(uint8_t)x] != '#';
}

static void dndUpdateCamera() {
  int cx = dndPlayerX - (DND_VIEW_W / 2);
  int cy = dndPlayerY - (DND_VIEW_H / 2);
  int maxX = DND_ROOM_W - DND_VIEW_W;
  int maxY = DND_ROOM_H - DND_VIEW_H;
  if (cx < 0) cx = 0;
  if (cy < 0) cy = 0;
  if (cx > maxX) cx = maxX;
  if (cy > maxY) cy = maxY;
  dndCamX = (int8_t)cx;
  dndCamY = (int8_t)cy;
}

static void dndRenderFull(const String &r0, const String &r1, const String &r2, const String &r3) {
  lines[0]=r0.substring(0,21); lines[1]=r1.substring(0,21);
  lines[2]=r2.substring(0,21); lines[3]=r3.substring(0,21);
  dirty=true;
}

static void dndDrawPlay() {
  dndUpdateCamera();
  String rows[DND_VIEW_H];
  for (uint8_t vy=0; vy<DND_VIEW_H; ++vy) {
    int ry = dndCamY + vy;
    String mapPart;
    for (uint8_t vx=0; vx<DND_VIEW_W; ++vx) {
      int rx = dndCamX + vx;
      char c = dndMap[ry][rx];
      int8_t ei = dndEnemyAt((int8_t)rx, (int8_t)ry);
      if (dndPlayerX == rx && dndPlayerY == ry) c = '@';
      else if (ei >= 0) c = dndEnemies[(uint8_t)ei].glyph;
      mapPart += c;
    }
    String hud;
    if (vy == 0) hud = dndWindow(dndCurrentEvent, dndEventMarquee, DND_HUD_W);
    else hud = dndWindow(dndStatusText((uint8_t)(dndStatusFirst + vy - 1)), dndStatusMarquee[vy-1], DND_HUD_W);
    rows[vy] = hud + mapPart;
  }
  dndRenderFull(rows[0], rows[1], rows[2], rows[3]);
}

static void dndClearEnemies() {
  for (uint8_t i=0;i<DND_ENEMY_COUNT;++i) dndEnemies[i].alive = false;
}

static bool dndTileFreeForSpawn(int x, int y) {
  if (x <= 0 || x >= DND_ROOM_W-1 || y <= 0 || y >= DND_ROOM_H-1) return false;
  if (dndMap[y][x] != '.') return false;
  if (x == dndPlayerX && y == dndPlayerY) return false;
  if (dndEnemyAt((int8_t)x,(int8_t)y) >= 0) return false;
  return true;
}

static bool dndFindFreeTile(int8_t &x, int8_t &y) {
  for (uint16_t tries=0; tries<250; ++tries) {
    int tx = (int)random(1, DND_ROOM_W-1);
    int ty = (int)random(1, DND_ROOM_H-1);
    if (dndTileFreeForSpawn(tx, ty)) {
      x = (int8_t)tx; y = (int8_t)ty; return true;
    }
  }
  return false;
}

static void dndPlaceTile(char glyph, uint8_t count) {
  for (uint8_t i=0;i<count;++i) {
    int8_t x=0,y=0;
    if (dndFindFreeTile(x,y)) dndMap[(uint8_t)y][(uint8_t)x] = glyph;
  }
}

static void dndBuildEnemy(uint8_t idx, char glyph, const char *name,
                          int baseHp, int baseAc, int baseDamageMin, int baseDamageMax,
                          uint16_t moveMin, uint16_t moveMax) {
  if (idx >= DND_ENEMY_COUNT) return;
  int8_t x=0,y=0;
  if (!dndFindFreeTile(x,y)) return;
  DndEnemy &e = dndEnemies[idx];
  e.glyph = glyph; e.name = name; e.x = x; e.y = y;
  e.str   = dndEnemyScaled(3, 1);
  e.dex   = dndEnemyScaled(3, 1);
  e.con   = dndEnemyScaled(3, 1);
  e.intel = dndEnemyScaled(2, 1);
  e.wis   = dndEnemyScaled(2, 1);
  e.cha   = dndEnemyScaled(2, 1);
  e.maxHp = dndEnemyScaled(baseHp, 1);
  e.hp = e.maxHp;
  e.ac = dndEnemyScaled(baseAc, 1);
  e.hitBonus = dndEnemyScaled(1, -20);
  e.damageMin = dndEnemyScaled(baseDamageMin, 1);
  e.damageMax = dndEnemyScaled(baseDamageMax, e.damageMin);
  e.xpReward = (uint16_t)dndEnemyScaled(baseHp + baseAc, 1);
  e.goldReward = (uint16_t)dndEnemyScaled(baseDamageMax + 2, 0);
  int speedSwing = dndSignedRoomMod() * 8;
  int mn = (int)moveMin - speedSwing;
  int mx = (int)moveMax + speedSwing;
  if (mn < 220) mn = 220;
  if (mx < mn + 100) mx = mn + 100;
  if (mx > 5000) mx = 5000;
  e.moveMinMs = (uint16_t)mn;
  e.moveMaxMs = (uint16_t)mx;
  e.nextMoveMs = millis() + (uint32_t)random(e.moveMinMs, e.moveMaxMs + 1U);
  e.alive = true;
}

static void dndGenerateRoom(bool announce) {
  dndClearEnemies();
  // Theme is content flavor; geometry still uses the proven 8-style engine.
  dndRoomThemeIndex = (uint8_t)((dndRoomNumber - 1U + (uint16_t)random(48)) % 48U);
  for (uint8_t y=0; y<DND_ROOM_H; ++y) {
    for (uint8_t x=0; x<DND_ROOM_W; ++x) {
      dndMap[y][x] = (x==0 || y==0 || x==DND_ROOM_W-1 || y==DND_ROOM_H-1) ? '#' : '.';
    }
    dndMap[y][DND_ROOM_W] = '\0';
  }

  // Fresh entry point every room. Old geometry is intentionally discarded.
  dndPlayerX = (int8_t)random(2, DND_ROOM_W-2);
  dndPlayerY = (int8_t)random(2, DND_ROOM_H-2);

  // BLOOM_DND_ROOM_ARCHETYPES_V1
  // Weighted room personality. Geometry changes; the existing door generator,
  // depth scaling, enemy spawning, loot generation, and camera continue below.
  uint8_t styleRoll = (uint8_t)random(100);
  if      (styleRoll < 24) dndRoomStyle = DND_ROOM_RUINS;
  else if (styleRoll < 40) dndRoomStyle = DND_ROOM_HALL;
  else if (styleRoll < 55) dndRoomStyle = DND_ROOM_CORRIDORS;
  else if (styleRoll < 69) dndRoomStyle = DND_ROOM_CHAMBERS;
  else if (styleRoll < 80) dndRoomStyle = DND_ROOM_TRAP_RUN;
  else if (styleRoll < 89) dndRoomStyle = DND_ROOM_TREASURE;
  else if (styleRoll < 96) dndRoomStyle = DND_ROOM_DEN;
  else                     dndRoomStyle = DND_ROOM_VOID;

  if (dndRoomStyle == DND_ROOM_RUINS) {
    // Broken short wall scars, close to the original generator but less uniform.
    uint8_t scars = (uint8_t)(8 + random(8));
    for (uint8_t i=0; i<scars; ++i) {
      int x = (int)random(2, DND_ROOM_W-2);
      int y = (int)random(2, DND_ROOM_H-2);
      bool horiz = random(2) == 0;
      int len = (int)random(2, 6);
      for (int j=0; j<len; ++j) {
        int xx = x + (horiz ? j : 0);
        int yy = y + (horiz ? 0 : j);
        if (xx>0 && xx<DND_ROOM_W-1 && yy>0 && yy<DND_ROOM_H-1) dndMap[yy][xx] = '#';
      }
    }
  }
  else if (dndRoomStyle == DND_ROOM_HALL) {
    // Broad open hall with deliberate pillars.
    for (int y=3; y<DND_ROOM_H-2; y+=4) {
      for (int x=4; x<DND_ROOM_W-3; x+=6) {
        if (random(100) < 78) dndMap[y][x] = '#';
      }
    }
  }
  else if (dndRoomStyle == DND_ROOM_CORRIDORS) {
    // Three long broken walls create lanes without becoming a hard maze.
    const int xs[3] = {7, 15, 23};
    for (uint8_t k=0; k<3; ++k) {
      int x = xs[k];
      int gap1 = 3 + (int)random(3);
      int gap2 = 9 + (int)random(4);
      for (int y=1; y<DND_ROOM_H-1; ++y) {
        if (y != gap1 && y != gap1+1 && y != gap2 && y != gap2+1) dndMap[y][x] = '#';
      }
    }
  }
  else if (dndRoomStyle == DND_ROOM_CHAMBERS) {
    // A cross divides the floor into four chambers; several doorways keep it connected.
    int mx = DND_ROOM_W/2;
    int my = DND_ROOM_H/2;
    for (int y=1; y<DND_ROOM_H-1; ++y) dndMap[y][mx] = '#';
    for (int x=1; x<DND_ROOM_W-1; ++x) dndMap[my][x] = '#';
    dndMap[3][mx]='.'; dndMap[4][mx]='.';
    dndMap[DND_ROOM_H-4][mx]='.'; dndMap[DND_ROOM_H-5][mx]='.';
    dndMap[my][6]='.'; dndMap[my][7]='.';
    dndMap[my][DND_ROOM_W-7]='.'; dndMap[my][DND_ROOM_W-8]='.';
  }
  else if (dndRoomStyle == DND_ROOM_TRAP_RUN) {
    // Open lanes with alternating barricades and extra depth-scaled trap tiles.
    for (int y=3; y<DND_ROOM_H-2; y+=3) {
      bool fromLeft = ((y/3) & 1) == 0;
      if (fromLeft) {
        for (int x=2; x<DND_ROOM_W-8; ++x) dndMap[y][x] = '#';
      } else {
        for (int x=8; x<DND_ROOM_W-2; ++x) dndMap[y][x] = '#';
      }
    }
    for (uint8_t i=0; i<10; ++i) {
      int tx=(int)random(2,DND_ROOM_W-2), ty=(int)random(2,DND_ROOM_H-2);
      if (dndMap[ty][tx]=='.') dndMap[ty][tx]='^';
    }
  }
  else if (dndRoomStyle == DND_ROOM_TREASURE) {
    // Central vault with four entrances and guaranteed visible treasure flavor.
    int x0=9, x1=20, y0=4, y1=11;
    for (int x=x0; x<=x1; ++x) { dndMap[y0][x]='#'; dndMap[y1][x]='#'; }
    for (int y=y0; y<=y1; ++y) { dndMap[y][x0]='#'; dndMap[y][x1]='#'; }
    dndMap[y0][14]='.'; dndMap[y1][15]='.'; dndMap[7][x0]='.'; dndMap[8][x1]='.';
    dndMap[6][13]='c'; dndMap[9][16]='c'; dndMap[7][16]='$'; dndMap[9][13]='w';
  }
  else if (dndRoomStyle == DND_ROOM_DEN) {
    // Arena/ring room; good for enemy-heavy encounters from the existing spawner.
    int x0=6, x1=23, y0=3, y1=12;
    for (int x=x0; x<=x1; ++x) { dndMap[y0][x]='#'; dndMap[y1][x]='#'; }
    for (int y=y0; y<=y1; ++y) { dndMap[y][x0]='#'; dndMap[y][x1]='#'; }
    dndMap[y0][14]='.'; dndMap[y1][15]='.'; dndMap[7][x0]='.'; dndMap[8][x1]='.';
    for (uint8_t i=0; i<4; ++i) {
      int tx=(int)random(8,22), ty=(int)random(5,11);
      if (dndMap[ty][tx]=='.') dndMap[ty][tx]='^';
    }
  }
  else {
    // VOID: eerie mostly-open floor with isolated 2x2 islands.
    for (int y=3; y<DND_ROOM_H-3; y+=5) {
      for (int x=4; x<DND_ROOM_W-4; x+=7) {
        if (random(100) < 70) {
          dndMap[y][x]='#'; dndMap[y][x+1]='#';
          dndMap[y+1][x]='#'; dndMap[y+1][x+1]='#';
        }
      }
    }
  }

  // Never let personality geometry spawn the player inside a wall/object.
  // Keep a small local breathing zone; the guaranteed-exit patch below will then
  // carve at least one complete route from this zone to a one-way + door.
  for (int dy=-1; dy<=1; ++dy) {
    for (int dx=-1; dx<=1; ++dx) {
      int xx=dndPlayerX+dx, yy=dndPlayerY+dy;
      if (xx>0 && xx<DND_ROOM_W-1 && yy>0 && yy<DND_ROOM_H-1) dndMap[yy][xx]='.';
    }
  }
  dndQueue(String("ROOM ") + String(dndRoomNumber) + " " + DND_ROOM_STYLE_NAMES[(uint8_t)dndRoomStyle]);

  // BLOOM_DND_GUARANTEED_EXIT_V1
  // Always create one guaranteed reachable one-way exit first. Random wall scars
  // may partition the room, so carve a Manhattan corridor from the player to the
  // cell directly inside the chosen border door. This prevents unwinnable rooms.
  uint8_t guaranteedSide = (uint8_t)random(4);
  int8_t doorX = dndPlayerX;
  int8_t doorY = dndPlayerY;
  int8_t goalX = dndPlayerX;
  int8_t goalY = dndPlayerY;

  if (guaranteedSide == 0) {          // top
    doorX = (int8_t)random(2, DND_ROOM_W-2);
    doorY = 0;
    goalX = doorX;
    goalY = 1;
  } else if (guaranteedSide == 1) {   // bottom
    doorX = (int8_t)random(2, DND_ROOM_W-2);
    doorY = DND_ROOM_H-1;
    goalX = doorX;
    goalY = DND_ROOM_H-2;
  } else if (guaranteedSide == 2) {   // left
    doorX = 0;
    doorY = (int8_t)random(2, DND_ROOM_H-2);
    goalX = 1;
    goalY = doorY;
  } else {                            // right
    doorX = DND_ROOM_W-1;
    doorY = (int8_t)random(2, DND_ROOM_H-2);
    goalX = DND_ROOM_W-2;
    goalY = doorY;
  }

  int8_t carveX = dndPlayerX;
  int8_t carveY = dndPlayerY;
  dndMap[(uint8_t)carveY][(uint8_t)carveX] = '.';
  while (carveX != goalX) {
    carveX += (goalX > carveX) ? 1 : -1;
    dndMap[(uint8_t)carveY][(uint8_t)carveX] = '.';
  }
  while (carveY != goalY) {
    carveY += (goalY > carveY) ? 1 : -1;
    dndMap[(uint8_t)carveY][(uint8_t)carveX] = '.';
  }
  dndMap[(uint8_t)doorY][(uint8_t)doorX] = '+';

  // Add 1-3 extra random one-way exits for variety. They do not need individual
  // connectivity guarantees because the room already has one proven route out.
  uint8_t extraDoors = (uint8_t)random(1,4);
  for (uint8_t i=0;i<extraDoors;++i) {
    uint8_t side = (uint8_t)random(4);
    if (side == 0) dndMap[0][random(2,DND_ROOM_W-2)] = '+';
    else if (side == 1) dndMap[DND_ROOM_H-1][random(2,DND_ROOM_W-2)] = '+';
    else if (side == 2) dndMap[random(2,DND_ROOM_H-2)][0] = '+';
    else dndMap[random(2,DND_ROOM_H-2)][DND_ROOM_W-1] = '+';
  }

  // Interactive content scales in both frequency and stats with depth.
  dndPlaceTile('c', (uint8_t)random(1,4));
  dndPlaceTile('^', (uint8_t)random(1,5));
  dndPlaceTile('$', (uint8_t)random(0,3));
  dndPlaceTile('!', (uint8_t)random(0,3));
  if (random(100) < 35) dndPlaceTile('w', 1);

  uint8_t enemyCount = (uint8_t)(3 + (dndRoomNumber / 8));
  if (enemyCount > DND_ENEMY_COUNT) enemyCount = DND_ENEMY_COUNT;
  for (uint8_t i=0;i<enemyCount;++i) {
    // 48-species content pool. Deeper rooms unlock higher tiers, while a
    // little under-tier randomness keeps old creatures appearing later.
    uint8_t maxTier = (uint8_t)(dndRoomNumber / 8U);
    if (maxTier > 5U) maxTier = 5U;
    uint8_t wantedTier = maxTier;
    if (maxTier > 0U && random(100) < 28) wantedTier = (uint8_t)random(maxTier + 1U);
    uint8_t choices[48];
    uint8_t choiceCount = 0;
    for (uint8_t eidx=0; eidx<DND_ENEMY_SPECIES_COUNT; ++eidx) {
      if (DND_ENEMY_CATALOG[eidx].tier == wantedTier) choices[choiceCount++] = eidx;
    }
    uint8_t pick = choices[choiceCount ? (uint8_t)random(choiceCount) : 0];
    const DndEnemyDef &def = DND_ENEMY_CATALOG[pick];
    dndBuildEnemy(i, def.glyph, def.name, def.hp, def.ac,
                  def.dmgMin, def.dmgMax, def.moveMin, def.moveMax);
  }

  dndUpdateCamera();
  if (announce) {
    dndQueue("ROOM " + String(dndRoomNumber));
    dndQueue(String(DND_ROOM_THEME_NAMES[dndRoomThemeIndex]));
    dndQueue("DEPTH MOD +/-" + String(dndRoomNumber));
  }
}

static uint16_t dndItemPrice(uint16_t id) {
  if (id >= DND_ITEM_COUNT) return 65000U;
  const DndItemDef &it = DND_ITEM_CATALOG[id];
  uint32_t price = (uint32_t)it.basePrice + (uint32_t)dndRoomNumber * (uint32_t)(1U + it.rarity);
  if (price > 65000U) price = 65000U;
  return (uint16_t)price;
}

static uint16_t dndFindItemByName(const String &name, char type=0) {
  for (uint16_t i=0; i<DND_ITEM_COUNT; ++i) {
    if ((!type || DND_ITEM_CATALOG[i].type == type) && name == DND_ITEM_CATALOG[i].name) return i;
  }
  return DND_NO_ITEM;
}

static uint16_t dndRollCatalogIndex() {
  uint8_t maxRarity = (uint8_t)(dndRoomNumber / 8U);
  if (maxRarity > 4U) maxRarity = 4U;
  uint8_t wanted = maxRarity;
  if (maxRarity > 0U && random(100) < 35) wanted = (uint8_t)random(maxRarity + 1U);
  for (uint16_t tries=0; tries<250; ++tries) {
    uint16_t id = (uint16_t)random((long)DND_ITEM_COUNT);
    if (DND_ITEM_CATALOG[id].rarity == wanted) return id;
  }
  return (uint16_t)random((long)DND_ITEM_COUNT);
}

static void dndRollShop(bool force=false) {
  if (!force && dndShopRoom == dndRoomNumber) return;
  dndShopRoom = dndRoomNumber;
  for (uint8_t i=0; i<DND_SHOP_COUNT; ++i) {
    uint16_t id;
    bool duplicate;
    do {
      id = dndRollCatalogIndex();
      duplicate = false;
      for (uint8_t j=0; j<i; ++j) if (dndShop[j] == id) duplicate = true;
    } while (duplicate);
    dndShop[i] = id;
  }
  dndShopIndex = 0;
}

static uint16_t dndInventoryCountId(uint16_t id) {
  uint16_t count = 0;
  for (uint16_t i=0; i<dndInventoryCount; ++i) if (dndInventory[i] == id) count++;
  return count;
}

static bool dndAddInventory(uint16_t id) {
  if (id >= DND_ITEM_COUNT || dndInventoryCount >= DND_INV_CAP) return false;
  dndInventory[dndInventoryCount++] = id;
  return true;
}

static void dndRemoveInventoryAt(uint16_t slot) {
  if (slot >= dndInventoryCount) return;
  for (uint16_t i=slot+1; i<dndInventoryCount; ++i) dndInventory[i-1] = dndInventory[i];
  dndInventoryCount--;
  if (dndInventoryCount == 0) dndInventoryIndex = 0;
  else if (dndInventoryIndex >= dndInventoryCount) dndInventoryIndex = dndInventoryCount - 1U;
}

static uint16_t dndOwnedCountType(char type) {
  uint16_t count = 0;
  for (uint16_t i=0; i<dndInventoryCount; ++i) {
    uint16_t id = dndInventory[i];
    if (id < DND_ITEM_COUNT && DND_ITEM_CATALOG[id].type == type) count++;
  }
  return count;
}

static uint16_t dndNthOwnedSlot(char type, uint16_t nth) {
  uint16_t seen = 0;
  for (uint16_t i=0; i<dndInventoryCount; ++i) {
    uint16_t id = dndInventory[i];
    if (id < DND_ITEM_COUNT && DND_ITEM_CATALOG[id].type == type) {
      if (seen == nth) return i;
      seen++;
    }
  }
  return DND_NO_ITEM;
}

static uint16_t dndNthOwnedId(char type, uint16_t nth) {
  uint16_t slot = dndNthOwnedSlot(type, nth);
  return slot == DND_NO_ITEM ? DND_NO_ITEM : dndInventory[slot];
}

static int dndGearBonus(char field) {
  uint16_t ids[6] = {dndEquipWeapon,dndEquipArmor,dndEquipShield,dndEquipRing1,dndEquipRing2,dndEquipCharm};
  int total = 0;
  for (uint8_t i=0; i<6; ++i) {
    uint16_t id = ids[i];
    if (id >= DND_ITEM_COUNT) continue;
    const DndItemDef &it = DND_ITEM_CATALOG[id];
    if (field=='S') total += it.strBonus;
    else if (field=='D') total += it.dexBonus;
    else if (field=='A') total += it.acBonus;
    else if (field=='H') total += it.hpBonus;
  }
  return total;
}

static int dndEffectiveStrength() { return dndStrength + dndGearBonus('S'); }
static int dndEffectiveDexterity() { return dndDexterity + dndGearBonus('D'); }
static int dndEffectiveArmorClass() { return dndArmorClass + dndGearBonus('A'); }
static int dndEffectiveMaxHp() { return dndMaxHp + dndGearBonus('H'); }

static void dndSyncWeaponFromEquipped() {
  uint16_t id = dndEquipWeapon;
  if (id >= DND_ITEM_COUNT || DND_ITEM_CATALOG[id].type != 'W') {
    id = dndFindItemByName("IRON SWORD", 'W');
    dndEquipWeapon = id;
  }
  if (id < DND_ITEM_COUNT) {
    const DndItemDef &it = DND_ITEM_CATALOG[id];
    dndWeapon.name = String(it.name);
    dndWeapon.damageMin = it.damageMin;
    dndWeapon.damageMax = it.damageMax;
    dndWeapon.hitBonus = it.hitBonus;
    dndWeapon.foundRoom = dndRoomNumber;
  }
}

static String dndItemTypeName(char type) {
  if (type=='W') return "WEAPON";
  if (type=='A') return "ARMOR";
  if (type=='D') return "SHIELD";
  if (type=='R') return "RING";
  if (type=='C') return "CHARM";
  return "CONSUMABLE";
}

static String dndItemStatsLine(uint16_t id) {
  if (id >= DND_ITEM_COUNT) return "NO ITEM";
  const DndItemDef &it = DND_ITEM_CATALOG[id];
  if (it.type=='W') return "DMG "+String(it.damageMin)+"-"+String(it.damageMax)+" HIT "+String(it.hitBonus)+" S"+String(it.strBonus)+" D"+String(it.dexBonus);
  if (it.type=='H') return it.healAmount >= 9999 ? "FULL RESTORE" : "HEAL "+String(it.healAmount);
  String out;
  if (it.strBonus) out += "STR "+String(it.strBonus)+" ";
  if (it.dexBonus) out += "DEX "+String(it.dexBonus)+" ";
  if (it.acBonus) out += "AC "+String(it.acBonus)+" ";
  if (it.hpBonus) out += "HP "+String(it.hpBonus)+" ";
  if (!out.length()) out = "NO BONUS";
  return out;
}

static void dndClampHpForGear() {
  int cap = dndEffectiveMaxHp();
  if (cap < 1) cap = 1;
  if (dndHp > cap) dndHp = cap;
}

static bool dndEquipOwnedItem(uint16_t id, uint8_t category) {
  if (id >= DND_ITEM_COUNT) return false;
  char type = DND_ITEM_CATALOG[id].type;
  if (category==1 && type=='W') dndEquipWeapon=id;
  else if (category==2 && type=='A') dndEquipArmor=id;
  else if (category==3 && type=='D') dndEquipShield=id;
  else if (category==5 && type=='R') {
    if (dndEquipRing2==id && dndInventoryCountId(id)<2) { dndQueue("NEED SECOND RING"); return false; }
    dndEquipRing1=id;
  }
  else if (category==4 && type=='R') {
    if (dndEquipRing1==id && dndInventoryCountId(id)<2) { dndQueue("NEED SECOND RING"); return false; }
    dndEquipRing2=id;
  }
  else if (category==6 && type=='C') dndEquipCharm=id;
  else return false;
  dndSyncWeaponFromEquipped();
  dndClampHpForGear();
  saveBloomSystemState();
  dndQueue("EQUIPPED "+String(DND_ITEM_CATALOG[id].name));
  return true;
}

static bool dndUseConsumable(uint16_t inventorySlot) {
  if (inventorySlot >= dndInventoryCount) return false;
  uint16_t id = dndInventory[inventorySlot];
  if (id >= DND_ITEM_COUNT || DND_ITEM_CATALOG[id].type != 'H') return false;
  int heal = DND_ITEM_CATALOG[id].healAmount;
  int cap = dndEffectiveMaxHp();
  if (heal >= 9999) dndHp = cap;
  else { dndHp += heal; if (dndHp > cap) dndHp = cap; }
  dndQueue("USED "+String(DND_ITEM_CATALOG[id].name));
  dndQueue("HP "+String(dndHp)+"/"+String(cap));
  dndRemoveInventoryAt(inventorySlot);
  saveBloomSystemState();
  return true;
}

static String dndShopLine(uint8_t slot) {
  uint16_t id = dndShop[slot];
  if (id >= DND_ITEM_COUNT) return "?";
  String line = String(slot==dndShopIndex?">":" ")+DND_ITEM_CATALOG[id].name+" "+String(dndItemPrice(id))+"G";
  return dndWindow(line, slot==dndShopIndex?dndMenuMarquee:0, 21);
}

static void dndDrawShop() {
  dndRollShop(false);
  uint8_t first=dndShopIndex>1?dndShopIndex-1:0;
  if (first>DND_SHOP_COUNT-3) first=DND_SHOP_COUNT-3;
  dndRenderFull("SHOP GOLD "+String(dndGold),dndShopLine(first),dndShopLine(first+1),dndShopLine(first+2));
}

static void dndDrawInventory() {
  if (!dndInventoryCount) {
    dndRenderFull("INVENTORY 0/288","NO ITEMS","GOLD "+String(dndGold),"A SHOP B BACK");
    return;
  }
  if (dndInventoryIndex >= dndInventoryCount) dndInventoryIndex = dndInventoryCount-1U;
  uint16_t id=dndInventory[dndInventoryIndex];
  String title="ITEM "+String(dndInventoryIndex+1)+"/"+String(dndInventoryCount);
  String name=id<DND_ITEM_COUNT?String(DND_ITEM_CATALOG[id].name):"?";
  dndRenderFull(title,dndWindow(name,dndMenuMarquee,21),dndWindow(dndItemStatsLine(id),dndMenuMarquee,21),"A SHOP B BACK");
}

static void dndBuySelectedShopItem() {
  uint16_t id=dndShop[dndShopIndex];
  if (id>=DND_ITEM_COUNT) return;
  uint16_t price=dndItemPrice(id);
  if (dndGold<price) { dndQueue("NEED "+String(price)+" GOLD"); return; }
  if (!dndAddInventory(id)) { dndQueue("INVENTORY FULL"); return; }
  dndGold-=price;
  dndQueue("BOUGHT "+String(DND_ITEM_CATALOG[id].name));
  dndQueue("-"+String(price)+" GOLD");
  saveBloomSystemState();
}

static char dndCategoryType(uint8_t category) {
  if (category==1) return 'W';
  if (category==2) return 'A';
  if (category==3) return 'D';
  if (category==4 || category==5) return 'R';
  if (category==6) return 'C';
  if (category==7) return 'H';
  return 0;
}

static uint16_t dndEquippedForCategory(uint8_t category) {
  if (category==1) return dndEquipWeapon;
  if (category==2) return dndEquipArmor;
  if (category==3) return dndEquipShield;
  if (category==4) return dndEquipRing1;
  if (category==5) return dndEquipRing2;
  if (category==6) return dndEquipCharm;
  return DND_NO_ITEM;
}

static void dndDrawCharacterStats() {
  String row0 = "LV "+String(dndLevel)+" XP "+String(dndXp)+" RM "+String(dndRoomNumber);
  String row1 = "HP "+String(dndHp)+"/"+String(dndEffectiveMaxHp())+" AC "+String(dndEffectiveArmorClass());
  String row2 = "STR "+String(dndEffectiveStrength())+" DEX "+String(dndEffectiveDexterity())+" G "+String(dndGold);
  String row3 = dndWeapon.name+" "+String(dndWeapon.damageMin)+"-"+String(dndWeapon.damageMax)+" H"+String(dndWeapon.hitBonus)+" B BACK";
  dndRenderFull(dndWindow(row0,dndMenuMarquee,21),
                dndWindow(row1,dndMenuMarquee,21),
                dndWindow(row2,dndMenuMarquee,21),
                dndWindow(row3,dndMenuMarquee,21));
}

static String dndCharacterCategoryLine(uint8_t idx) {
  String line=String(idx==dndEquipCategory?">":" ")+DND_EQUIP_CATEGORY_NAMES[idx];
  uint16_t eq=dndEquippedForCategory(idx);
  if (idx>0 && idx<7 && eq<DND_ITEM_COUNT) line += " "+String(DND_ITEM_CATALOG[eq].name);
  else if (idx>0 && idx<7) line += " -";
  return dndWindow(line,idx==dndEquipCategory?dndMenuMarquee:0,21);
}

static void dndDrawCharacterCategories() {
  uint8_t first=dndEquipCategory>1?dndEquipCategory-1:0;
  if (first>DND_EQUIP_CATEGORY_COUNT-3) first=DND_EQUIP_CATEGORY_COUNT-3;
  dndRenderFull(dndCharacterCategoryLine(first),dndCharacterCategoryLine(first+1),dndCharacterCategoryLine(first+2),"A SELECT B BACK");
}

static void dndDrawEquipList() {
  char type=dndCategoryType(dndEquipCategory);
  uint16_t count=dndOwnedCountType(type);
  if (!count) {
    dndRenderFull(DND_EQUIP_CATEGORY_NAMES[dndEquipCategory],"NO OWNED ITEMS","","B BACK");
    return;
  }
  if (dndEquipOwnedIndex>=count) dndEquipOwnedIndex=count-1U;
  uint16_t id=dndNthOwnedId(type,dndEquipOwnedIndex);
  String title=String(DND_EQUIP_CATEGORY_NAMES[dndEquipCategory])+" "+String(dndEquipOwnedIndex+1)+"/"+String(count);
  String marker=(dndEquipCategory>0 && dndEquipCategory<7 && dndEquippedForCategory(dndEquipCategory)==id)?"* ":"> ";
  String footer=type=='H'?"A USE B BACK":"A EQUIP B BACK";
  dndRenderFull(dndWindow(title,0,21),dndWindow(marker+String(DND_ITEM_CATALOG[id].name),dndMenuMarquee,21),dndWindow(dndItemStatsLine(id),dndMenuMarquee,21),footer);
}

static void dndNextRoom() {
  if (dndRoomNumber < 65535) dndRoomNumber++;
  if (dndRoomNumber > dndDeepestRoom) dndDeepestRoom = dndRoomNumber;
  dndGenerateRoom(true);
  dndRollShop(true);
}

static void dndGenerateWeapon() {
  uint16_t id=DND_NO_ITEM;
  for (uint16_t tries=0; tries<250; ++tries) {
    uint16_t pick=dndRollCatalogIndex();
    if (pick<DND_ITEM_COUNT && DND_ITEM_CATALOG[pick].type=='W') { id=pick; break; }
  }
  if (id==DND_NO_ITEM) id=dndFindItemByName("IRON SWORD",'W');
  if (id<DND_ITEM_COUNT && dndAddInventory(id)) {
    dndQueue("FOUND "+String(DND_ITEM_CATALOG[id].name));
    dndQueue(dndItemStatsLine(id));
    saveBloomSystemState();
  } else dndQueue("PACK FULL");
}

static void dndDamagePlayer(int amount, const String &source) {
  if (amount < 0) amount = 0;
  dndHp -= amount;
  if (dndHp < 0) dndHp = 0;
  dndQueue(source + " HIT");
  dndQueue("DAMAGE " + String(amount));
  if (dndHp <= 0) {
    dndQueue("YOU FALL");
    dndGold /= 2U;
    dndRoomNumber = 1;
    dndHp = dndEffectiveMaxHp();
    dndGenerateRoom(false);
    dndQueue("BACK TO ROOM 1");
  }
}

static void dndAttackEnemy(uint8_t idx, const String &verb) {
  if (idx >= DND_ENEMY_COUNT || !dndEnemies[idx].alive) return;
  DndEnemy &e = dndEnemies[idx];
  int roll = (int)random(1,21);
  int total = roll + dndEffectiveStrength() + dndWeapon.hitBonus;
  dndQueue(verb + " " + String(e.name));
  dndQueue("ROLL " + String(roll));
  dndQueue("TOTAL " + String(total));
  if (roll == 1 || total < e.ac) { dndQueue("MISS AC " + String(e.ac)); return; }
  int dmg = (int)random(dndWeapon.damageMin, dndWeapon.damageMax + 1) + (dndEffectiveStrength() / 2);
  if (roll == 20) { dmg *= 2; dndQueue("CRITICAL"); }
  e.hp -= dmg;
  dndQueue("DAMAGE " + String(dmg));
  if (e.hp <= 0) {
    e.alive = false;
    dndXp += e.xpReward;
    dndGold += e.goldReward;
    dndCheckLevelUp();
    dndQueue(String(e.name) + " DOWN");
    dndQueue("+" + String(e.xpReward) + " XP");
    dndQueue("+" + String(e.goldReward) + " GOLD");
  }
}

static void dndAttackDirection(int8_t dx, int8_t dy) {
  dndAttackArmedUntilMs = 0;
  int8_t tx = dndPlayerX + dx;
  int8_t ty = dndPlayerY + dy;
  dndQueue("ATTACK");
  int8_t ei = dndEnemyAt(tx,ty);
  if (ei >= 0) dndAttackEnemy((uint8_t)ei,"ATTACK");
  else dndQueue("SWING AIR");
}

static void dndInteractTile(int8_t x, int8_t y) {
  int8_t ei = dndEnemyAt(x,y);
  if (ei >= 0) { dndAttackEnemy((uint8_t)ei,"BUMP"); return; }
  char &tile = dndMap[(uint8_t)y][(uint8_t)x];

  if (tile == '+') {
    dndQueue("THROUGH DOOR");
    dndNextRoom();
    return;
  }
  if (tile == 'c') {
    int difficulty = dndScaled(10 + (dndRoomNumber/3), 2);
    int roll = (int)random(1,21) + dndEffectiveDexterity();
    dndQueue("OPEN CHEST"); dndQueue("ROLL " + String(roll));
    if (roll >= difficulty) {
      int loot = (int)random(100);
      tile = loot < 35 ? 'w' : (loot < 72 ? '$' : '!');
      dndQueue("CHEST OPEN");
    } else dndQueue("LOCK " + String(difficulty));
    return;
  }
  if (tile == '^') {
    int difficulty = dndScaled(12 + (dndRoomNumber/4), 3);
    int total = (int)random(1,21) + dndEffectiveDexterity();
    int damage = dndScaled(4 + (dndRoomNumber/4), 1);
    tile = '.';
    dndQueue("TRAP " + String(difficulty));
    if (total < difficulty) dndDamagePlayer(damage,"TRAP");
    else dndQueue("DODGED");
    return;
  }
  if (tile == '$') {
    int g = dndScaled(8 + (dndRoomNumber/2), 1);
    dndGold += (uint32_t)g;
    tile='.'; dndPlayerX=x; dndPlayerY=y;
    dndQueue("+" + String(g) + " GOLD");
    return;
  }
  if (tile == '!') {
    int heal = dndScaled(6 + (dndRoomNumber/3), 1);
    dndHp += heal; if (dndHp > dndMaxHp) dndHp = dndEffectiveMaxHp();
    tile='.'; dndPlayerX=x; dndPlayerY=y;
    dndQueue("HEAL " + String(heal));
    return;
  }
  if (tile == 'w') {
    tile='.'; dndPlayerX=x; dndPlayerY=y;
    dndGenerateWeapon();
    return;
  }
  dndPlayerX=x; dndPlayerY=y;
}

static void dndMovePlayer(int8_t dx, int8_t dy) {
  int8_t nx = dndPlayerX + dx;
  int8_t ny = dndPlayerY + dy;
  if (!dndWalkable(nx,ny)) { dndQueue("WALL"); return; }
  dndInteractTile(nx,ny);
}

static void dndServiceEnemies() {
  if (dndScreen != DND_PLAY) return;
  uint32_t now = millis();
  for (uint8_t i=0;i<DND_ENEMY_COUNT;++i) {
    DndEnemy &e=dndEnemies[i];
    if (!e.alive || (int32_t)(now-e.nextMoveMs)<0) continue;
    e.nextMoveMs = now + (uint32_t)random(e.moveMinMs,e.moveMaxMs+1U);
    int dxp=dndPlayerX-e.x, dyp=dndPlayerY-e.y;
    if (abs(dxp)+abs(dyp)==1) {
      int raw=(int)random(1,21);
      int total=raw+e.hitBonus+(e.dex/3);
      dndQueue(String(e.name)+" ATTACK");
      dndQueue("ROLL "+String(total));
      if (total >= dndEffectiveArmorClass()) {
        int dmg=(int)random(e.damageMin,e.damageMax+1);
        dndDamagePlayer(dmg,String(e.name));
      } else dndQueue("MISS");
      continue;
    }
    int8_t dx=0,dy=0;
    bool chase=abs(dxp)+abs(dyp)<=10;
    if (chase) {
      if (abs(dxp)>=abs(dyp) && dxp!=0) dx=dxp>0?1:-1;
      else if (dyp!=0) dy=dyp>0?1:-1;
    } else {
      uint8_t r=(uint8_t)random(4);
      dx=r==0?1:(r==1?-1:0); dy=r==2?1:(r==3?-1:0);
    }
    int8_t nx=e.x+dx, ny=e.y+dy;
    if (!dndWalkable(nx,ny)) continue;
    if (nx==dndPlayerX && ny==dndPlayerY) continue;
    if (dndEnemyAt(nx,ny)>=0) continue;
    char t=dndMap[(uint8_t)ny][(uint8_t)nx];
    if (t=='c'||t=='^'||t=='$'||t=='!'||t=='w'||t=='+') continue;
    e.x=nx; e.y=ny;
  }
}

static String dndMenuWindow(uint8_t idx) {
  String s=String(idx==dndMenuIndex?">":" ")+DND_MENU_ITEMS[idx];
  uint16_t off=(idx==dndMenuIndex)?dndMenuMarquee:0;
  return dndWindow(s,off,21);
}

static void dndDrawMenu() {
  uint8_t first=dndMenuIndex>1?dndMenuIndex-1:0;
  if (first>DND_MENU_COUNT-3) first=DND_MENU_COUNT-3;
  dndRenderFull(dndMenuWindow(first),dndMenuWindow(first+1),dndMenuWindow(first+2),"A ENTER B BACK");
}

static void dndDrawSymbols() {
  String title=String(DND_SYMBOL_GLYPHS[dndSymbolIndex])+" "+DND_SYMBOL_NAMES[dndSymbolIndex];
  dndRenderFull("SYMBOLS",dndWindow(title,dndMenuMarquee,21),
                String(dndSymbolIndex+1)+"/"+String(DND_SYMBOL_COUNT_EXPANDED),"UD MOVE B BACK");
}

static void dndDrawInfo() {
  dndRenderFull(dndWindow(dndInfoTitle,dndMenuMarquee,21),
                dndWindow(dndInfoBody,dndMenuMarquee,21),"","B BACK");
}

static void dndOpenMenu() {
  dndScreen=DND_MENU; dndMenuIndex=0; dndMenuMarquee=0; dndMenuMarqueeMs=millis();
  dndDrawMenu();
}

// Safe HAPPY JARZ DND-only save. Never read/write BloomCircuit bloomsys/state,
// Happy Jarz happyjarz/*, artwork SPIFFS, or the hjpetz pet save.
struct BloomDndDurableV2 {
  uint8_t valid;
  int16_t hp; int16_t maxHp; int16_t strength; int16_t dexterity; int16_t armorClass;
  uint32_t xp; uint32_t gold;
  uint16_t level; uint16_t keys; uint16_t roomNumber; uint16_t deepestRoom;
  uint16_t inventoryCount;
  uint16_t inventory[DND_INV_CAP];
  uint16_t shop[4];
  uint8_t shopIndex; uint16_t shopRoom; uint8_t roomThemeIndex;
  uint16_t equipWeapon; uint16_t equipArmor; uint16_t equipShield;
  uint16_t equipRing1; uint16_t equipRing2; uint16_t equipCharm;
};
struct BloomSystemSaveV2 {
struct HJDndSave {
 uint32_t magic;
 uint16_t version, size;
 BloomDndDurableV2 dnd;
 uint32_t checksum;
};
static constexpr uint32_t HJDND_MAGIC=0x484A4431UL; // HJD1
static bool bloomDndHasState=false;
static uint32_t bloomUnifiedLastAutosaveMs=0;
static uint32_t hjDndHash(const uint8_t *d,size_t n) {
 uint32_t h=2166136261UL;
 for(size_t i=0;i<n;++i){ h^=d[i];h*=16777619UL; }
 return h;
}
static void saveBloomSystemState(){
 if(!bloomDndHasState)return;
 HJDndSave st={};
 st.magic=HJDND_MAGIC;st.version=1;st.size=sizeof(st);
 BloomDndDurableV2 &d=st.dnd;
 d.valid=1;
 d.hp=dndHp;d.maxHp=dndMaxHp;d.strength=dndStrength;d.dexterity=dndDexterity;d.armorClass=dndArmorClass;
 d.xp=dndXp;d.gold=dndGold;d.level=dndLevel;d.keys=dndKeys;
 d.roomNumber=dndRoomNumber;d.deepestRoom=dndDeepestRoom;
 d.inventoryCount=min((uint16_t)DND_INV_CAP,dndInventoryCount);
 for(uint16_t i=0;i<d.inventoryCount;i++)d.inventory[i]=dndInventory[i];
 for(uint8_t i=0;i<4;i++)d.shop[i]=dndShop[i];
 d.shopIndex=dndShopIndex;d.shopRoom=dndShopRoom;d.roomThemeIndex=dndRoomThemeIndex;
 d.equipWeapon=dndEquipWeapon;d.equipArmor=dndEquipArmor;d.equipShield=dndEquipShield;
 d.equipRing1=dndEquipRing1;d.equipRing2=dndEquipRing2;d.equipCharm=dndEquipCharm;
 st.checksum=hjDndHash((const uint8_t*)&st,sizeof(st)-sizeof(st.checksum));
 Preferences nv;
 if(nv.begin("hjdnd",false)){nv.putBytes("state",&st,sizeof(st));nv.end();}
}
static bool loadBloomSystemState(){
 Preferences nv;
 if(!nv.begin("hjdnd",true))return false;
 if(nv.getBytesLength("state")!=sizeof(HJDndSave)){nv.end();return false;}
 HJDndSave st={};
 const size_t got=nv.getBytes("state",&st,sizeof(st));nv.end();
 if(got!=sizeof(st)||st.magic!=HJDND_MAGIC||st.version!=1||st.size!=sizeof(st)||
    st.checksum!=hjDndHash((const uint8_t*)&st,sizeof(st)-sizeof(st.checksum))||!st.dnd.valid)return false;
 const BloomDndDurableV2 &d=st.dnd;
 dndHp=d.hp;dndMaxHp=max((int16_t)1,d.maxHp);dndStrength=d.strength;dndDexterity=d.dexterity;dndArmorClass=d.armorClass;
 dndXp=d.xp;dndGold=d.gold;dndLevel=max((uint16_t)1,d.level);dndKeys=d.keys;
 dndRoomNumber=max((uint16_t)1,d.roomNumber);dndDeepestRoom=max(dndRoomNumber,d.deepestRoom);
 dndInventoryCount=min((uint16_t)DND_INV_CAP,d.inventoryCount);dndInventoryIndex=0;
 for(uint16_t i=0;i<dndInventoryCount;i++)dndInventory[i]=d.inventory[i]<DND_ITEM_COUNT?d.inventory[i]:DND_NO_ITEM;
 for(uint8_t i=0;i<4;i++)dndShop[i]=d.shop[i]<DND_ITEM_COUNT?d.shop[i]:0;
 dndShopIndex=d.shopIndex%4;dndShopRoom=d.shopRoom;dndRoomThemeIndex=d.roomThemeIndex%48;
 dndEquipWeapon=d.equipWeapon<DND_ITEM_COUNT?d.equipWeapon:DND_NO_ITEM;
 dndEquipArmor=d.equipArmor<DND_ITEM_COUNT?d.equipArmor:DND_NO_ITEM;
 dndEquipShield=d.equipShield<DND_ITEM_COUNT?d.equipShield:DND_NO_ITEM;
 dndEquipRing1=d.equipRing1<DND_ITEM_COUNT?d.equipRing1:DND_NO_ITEM;
 dndEquipRing2=d.equipRing2<DND_ITEM_COUNT?d.equipRing2:DND_NO_ITEM;
 dndEquipCharm=d.equipCharm<DND_ITEM_COUNT?d.equipCharm:DND_NO_ITEM;
 dndSyncWeaponFromEquipped();dndClampHpForGear();
 bloomDndHasState=true;
 return true;
}
static void leave(){
 if(bloomDndHasState)saveBloomSystemState();
 active=false;
}
static void resumeBloomDndFromUnifiedSave() {
  dndScreen=DND_PLAY; dndEventHead=dndEventTail=0; dndCurrentEvent="RESUME";
  dndEventUntilMs=millis()+900UL; dndEventMarquee=0; dndStatusFirst=0; dndStatusRotateMs=millis(); dndAttackArmedUntilMs=0;
  for (uint8_t i=0;i<3;++i) { dndStatusMarquee[i]=0; dndStatusMarqueeMs[i]=millis(); }
  dndSyncWeaponFromEquipped(); dndClampHpForGear(); dndGenerateRoom(false); dndDrawPlay();
}

static void dndStartGame() {
  // 18 HP is the level-1 starting point, not a permanent cap.
  dndHp=dndMaxHp=18;
  dndStrength=3; dndDexterity=2; dndArmorClass=12;
  dndXp=0; dndGold=0; dndLevel=1; dndKeys=0;
  dndRoomNumber=1; dndDeepestRoom=1;
  dndWeapon={"IRON SWORD",2,6,0,1};
  dndInventoryCount=0; dndInventoryIndex=0; dndShopRoom=0; dndShopIndex=0;
  dndEquipWeapon=dndFindItemByName("IRON SWORD",'W');
  dndEquipArmor=dndEquipShield=dndEquipRing1=dndEquipRing2=dndEquipCharm=DND_NO_ITEM;
  if (dndEquipWeapon<DND_ITEM_COUNT) dndAddInventory(dndEquipWeapon);
  dndSyncWeaponFromEquipped();
  dndRollShop(true);
  dndScreen=DND_PLAY;
  dndEventHead=dndEventTail=0;
  dndCurrentEvent="EXPLORE";
  dndEventUntilMs=millis()+700UL;
  dndEventMarquee=0; dndStatusFirst=0; dndStatusRotateMs=millis();
  dndAttackArmedUntilMs=0;
  for (uint8_t i=0;i<3;++i) { dndStatusMarquee[i]=0; dndStatusMarqueeMs[i]=millis(); }
  dndGenerateRoom(false);
  // BLOOM_DND_ENGINE_OWNERSHIP_PROBE_V1
  {
    int8_t probeX = (dndPlayerX <= (DND_ROOM_W - 5)) ? (dndPlayerX + 3) : (dndPlayerX - 3);
    int8_t step = (probeX > dndPlayerX) ? 1 : -1;
    for (int8_t x = dndPlayerX; x != probeX; x += step) {
      dndMap[(uint8_t)dndPlayerY][(uint8_t)x] = '.';
    }
    dndMap[(uint8_t)dndPlayerY][(uint8_t)probeX] = '+';
  }
  dndQueue("GEN30 LIVE");
  dndQueue("ROOM 1 MOD +/-1");
  
  bloomDndHasState = true;
  saveBloomSystemState();
  dndDrawPlay();
}

static void drawDndPlaceholder() {
  if (bloomDndHasState) resumeBloomDndFromUnifiedSave();
  else dndStartGame();
}

static void dndHandleInput(Input in) {
  if (dndScreen==DND_PLAY) {
    if (in==BKEY) { dndOpenMenu(); return; }
    if (in==AKEY) {
      dndAttackArmedUntilMs=millis()+700UL;
      dndCurrentEvent="AIM"; dndEventMarquee=0; dndEventUntilMs=millis()+700UL;
      dndDrawPlay(); return;
    }
    bool armed=(int32_t)(dndAttackArmedUntilMs-millis())>0;
    if (in==UP) { if(armed)dndAttackDirection(0,-1); else dndMovePlayer(0,-1); }
    else if (in==DOWN) { if(armed)dndAttackDirection(0,1); else dndMovePlayer(0,1); }
    else if (in==LEFT) { if(armed)dndAttackDirection(-1,0); else dndMovePlayer(-1,0); }
    else if (in==RIGHT) { if(armed)dndAttackDirection(1,0); else dndMovePlayer(1,0); }
    dndDrawPlay(); return;
  }

  if (dndScreen==DND_MENU) {
    if (in==BKEY) { dndScreen=DND_PLAY; dndDrawPlay(); return; }
    if (in==UP) dndMenuIndex=(uint8_t)((dndMenuIndex+DND_MENU_COUNT-1U)%DND_MENU_COUNT);
    else if (in==DOWN) dndMenuIndex=(uint8_t)((dndMenuIndex+1U)%DND_MENU_COUNT);
    else if (in==AKEY) {
      if (dndMenuIndex==0) {
        dndScreen=DND_EQUIP_CATEGORIES; dndEquipCategory=0; dndEquipOwnedIndex=0;
        dndMenuMarquee=0; dndMenuMarqueeMs=millis(); dndDrawCharacterCategories(); return;
      }
      if (dndMenuIndex==1) {
        dndScreen=DND_INVENTORY; dndInventoryIndex=0; dndMenuMarquee=0; dndDrawInventory(); return;
      }
      if (dndMenuIndex==2) {
        dndScreen=DND_SYMBOLS; dndSymbolIndex=0; dndMenuMarquee=0; dndDrawSymbols(); return;
      }
      if (dndMenuIndex==3) { leave(); return; }
    }
    dndMenuMarquee=0; dndMenuMarqueeMs=millis(); dndDrawMenu(); return;
  }

  if (dndScreen==DND_EQUIP_CATEGORIES) {
    if (in==BKEY) { dndScreen=DND_MENU; dndMenuMarquee=0; dndDrawMenu(); return; }
    if (in==UP) dndEquipCategory=(uint8_t)((dndEquipCategory+DND_EQUIP_CATEGORY_COUNT-1U)%DND_EQUIP_CATEGORY_COUNT);
    else if (in==DOWN) dndEquipCategory=(uint8_t)((dndEquipCategory+1U)%DND_EQUIP_CATEGORY_COUNT);
    else if (in==AKEY) {
      if (dndEquipCategory==0) {
        dndScreen=DND_CHARACTER_STATS; dndMenuMarquee=0; dndMenuMarqueeMs=millis();
        dndDrawCharacterStats(); return;
      }
      dndEquipOwnedIndex=0; dndScreen=DND_EQUIP_LIST; dndMenuMarquee=0; dndDrawEquipList(); return;
    }
    dndMenuMarquee=0; dndMenuMarqueeMs=millis(); dndDrawCharacterCategories(); return;
  }

  if (dndScreen==DND_CHARACTER_STATS) {
    if (in==BKEY || in==AKEY) {
      dndScreen=DND_EQUIP_CATEGORIES; dndMenuMarquee=0; dndMenuMarqueeMs=millis();
      dndDrawCharacterCategories();
    }
    return;
  }

  if (dndScreen==DND_EQUIP_LIST) {
    if (in==BKEY) { dndScreen=DND_EQUIP_CATEGORIES; dndMenuMarquee=0; dndDrawCharacterCategories(); return; }
    char type=dndCategoryType(dndEquipCategory);
    uint16_t count=dndOwnedCountType(type);
    if (count) {
      if (in==UP) dndEquipOwnedIndex=(dndEquipOwnedIndex+count-1U)%count;
      else if (in==DOWN) dndEquipOwnedIndex=(dndEquipOwnedIndex+1U)%count;
      else if (in==AKEY) {
        uint16_t slot=dndNthOwnedSlot(type,dndEquipOwnedIndex);
        uint16_t id=slot==DND_NO_ITEM?DND_NO_ITEM:dndInventory[slot];
        if (type=='H') { if (slot!=DND_NO_ITEM) dndUseConsumable(slot); }
        else dndEquipOwnedItem(id,dndEquipCategory);
        count=dndOwnedCountType(type);
        if (!count) dndEquipOwnedIndex=0;
        else if (dndEquipOwnedIndex>=count) dndEquipOwnedIndex=count-1U;
      }
    }
    dndMenuMarquee=0; dndMenuMarqueeMs=millis(); dndDrawEquipList(); return;
  }

  if (dndScreen==DND_INVENTORY) {
    if (in==BKEY) { dndScreen=DND_MENU; dndMenuMarquee=0; dndDrawMenu(); return; }
    if (in==AKEY) { dndScreen=DND_SHOP; dndShopIndex=0; dndMenuMarquee=0; dndRollShop(false); dndDrawShop(); return; }
    if (dndInventoryCount) {
      if (in==UP) dndInventoryIndex=(dndInventoryIndex+dndInventoryCount-1U)%dndInventoryCount;
      else if (in==DOWN) dndInventoryIndex=(dndInventoryIndex+1U)%dndInventoryCount;
    }
    dndMenuMarquee=0; dndMenuMarqueeMs=millis(); dndDrawInventory(); return;
  }

  if (dndScreen==DND_SHOP) {
    if (in==BKEY) { dndScreen=DND_INVENTORY; dndMenuMarquee=0; dndDrawInventory(); return; }
    if (in==UP) dndShopIndex=(uint8_t)((dndShopIndex+DND_SHOP_COUNT-1U)%DND_SHOP_COUNT);
    else if (in==DOWN) dndShopIndex=(uint8_t)((dndShopIndex+1U)%DND_SHOP_COUNT);
    else if (in==AKEY) dndBuySelectedShopItem();
    dndMenuMarquee=0; dndMenuMarqueeMs=millis(); dndDrawShop(); return;
  }

  if (dndScreen==DND_SYMBOLS) {
    if (in==BKEY) { dndScreen=DND_MENU; dndMenuMarquee=0; dndDrawMenu(); return; }
    if (in==UP) dndSymbolIndex=(uint8_t)((dndSymbolIndex+DND_SYMBOL_COUNT_EXPANDED-1U)%DND_SYMBOL_COUNT_EXPANDED);
    else if (in==DOWN) dndSymbolIndex=(uint8_t)((dndSymbolIndex+1U)%DND_SYMBOL_COUNT_EXPANDED);
    dndMenuMarquee=0; dndMenuMarqueeMs=millis(); dndDrawSymbols(); return;
  }

  if (dndScreen==DND_INFO) {
    if (in==BKEY || in==AKEY) { dndScreen=DND_MENU; dndMenuMarquee=0; dndDrawMenu(); }
    return;
  }
}

static void dndServiceGame() {
  if (active && bloomDndHasState) {
    uint32_t nowSave = millis();
    if (bloomUnifiedLastAutosaveMs == 0 || nowSave - bloomUnifiedLastAutosaveMs >= 30000UL) {
      bloomUnifiedLastAutosaveMs = nowSave;
      saveBloomSystemState();
    }
  }
  if (!active) return;
  if (dndScreen==DND_PLAY) {
    dndServiceEventQueue(); dndServiceHudScroll(); dndServiceEnemies();
    uint32_t now=millis();
    if (now-dndLastRenderMs>=120UL) { dndLastRenderMs=now; dndDrawPlay(); }
    return;
  }
  uint32_t now=millis();
  if (now-dndMenuMarqueeMs>=320UL) {
    dndMenuMarqueeMs=now; dndMenuMarquee++;
    if (dndScreen==DND_MENU) dndDrawMenu();
    else if (dndScreen==DND_SYMBOLS) dndDrawSymbols();
    else if (dndScreen==DND_INFO) dndDrawInfo();
    else if (dndScreen==DND_INVENTORY) dndDrawInventory();
    else if (dndScreen==DND_SHOP) dndDrawShop();
    else if (dndScreen==DND_EQUIP_CATEGORIES) dndDrawCharacterCategories();
    else if (dndScreen==DND_EQUIP_LIST) dndDrawEquipList();
    else if (dndScreen==DND_CHARACTER_STATS) dndDrawCharacterStats();
  }
}
static void enter(){
 if(active)return;
 active=true;
 if(!bloomDndHasState) loadBloomSystemState();
 if(bloomDndHasState)resumeBloomDndFromUnifiedSave();
 else dndStartGame();
 dirty=true;
}
static bool running(){return active;}
static void button(uint8_t idx){
 if(!active||idx>5)return;
 dndHandleInput(static_cast<Input>(idx));
 dirty=true;
}
static void service(){if(active)dndServiceGame();}
} // namespace HJDnd
