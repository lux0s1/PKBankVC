#pragma once

#include "font.h"
#include "save.h"

#define POKEDEX_BIN_PATH "/pk/bank/pokedex.bin"
#define POKEDEX_BIN_MAGIC (0x58444B50) // "PKDX"

typedef enum
{
	ITEM_POCKET_NONE = 0,
	ITEM_POCKET_ITEMS = 1,
	ITEM_POCKET_BALLS = 2,
	ITEM_POCKET_KEY = 3,
	ITEM_POCKET_TMHM = 4,
	ITEM_POCKET_PC = 5,
} ItemPocket;

typedef struct
{
	char8_t name[13];
	uint8_t pocket;
	uint8_t gen1Id;
} ItemInfo;

typedef struct
{
	uint32_t magic;
	uint32_t version;
	uint16_t itemCounts[256];
	uint8_t dexSeen[32];
	uint8_t dexOwned[32];
	uint8_t dexOrigin[252];
	uint8_t mysteryGiftClaimed;
	uint8_t reserved[3];
} BankStoreData;

extern const ItemInfo items_table[256];
extern const uint8_t item_g1_to_g2[256];
extern BankStoreData bankStore;

void bankStoreLoad(void);
void bankStoreSave(void);
void bankStoreScanAll(void);
void bankStoreRecordSpecies(uint8_t nationalDex, SAV_GameVersion originVersion);
bool bankStoreIsOwned(uint8_t nationalDex);
bool bankStoreIsSeen(uint8_t nationalDex);
uint16_t bankStoreCountKanto(void);
uint16_t bankStoreCountNational(void);
bool bankStoreHas150Kanto(void);
bool bankStoreIsMewClaimed(void);
int16_t bankStoreClaimMewGF(void);

const char8_t* itemGetName(uint8_t itemId, bool isGen1);
uint8_t itemToGen2(uint8_t itemId, bool isGen1);
uint8_t itemToGen1(uint8_t gen2Id);
bool itemCanStoreInBank(uint8_t itemId, bool isGen1);
bool itemCanGiveToGame(uint8_t gen2Id, bool targetIsGen1);

// Game Save Item Accessors
uint8_t gameItemGetCount(ItemPocket pocket);
void gameItemGetSlot(ItemPocket pocket, uint8_t slot, uint8_t* outId, uint8_t* outQty);
bool gameItemAdd(ItemPocket pocket, uint8_t gen2Id, uint8_t qty);
bool gameItemRemove(ItemPocket pocket, uint8_t slot, uint8_t qty);
void gameItemSort(ItemPocket pocket, uint8_t sortMode);
