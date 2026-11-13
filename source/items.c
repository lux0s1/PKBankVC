#include <stdlib.h>
#include "items.h"
#include "pkdir.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

BankStoreData bankStore;

static const char8_t gen1_key_mapa[]    = { 0x8C, 0x80, 0x8F, 0x80, 0x50 }; // MAPA
static const char8_t gen1_key_pokedex[] = { 0x8F, 0x8E, 0x8A, 0xBA, 0x83, 0x84, 0x97, 0x50 }; // POKéDEX
static const char8_t gen1_key_surf[]    = { 0x93, 0x80, 0x81, 0x8B, 0x80, 0x7F, 0x92, 0x94, 0x91, 0x85, 0x50 }; // TABLA SURF
static const char8_t gen1_key_fosil_d[] = { 0x85, 0xCC, 0x92, 0x88, 0x8B, 0x7F, 0x83, 0x8E, 0x8C, 0x8E, 0x50 }; // FÓSIL DOMO
static const char8_t gen1_key_fosil_h[] = { 0x85, 0xCC, 0x92, 0x88, 0x8B, 0x7F, 0x87, 0xC7, 0x8B, 0x88, 0x97, 0x50 }; // FÓSIL HÉLIX
static const char8_t gen1_key_secreta[] = { 0x8B, 0x8B, 0xE8, 0x7F, 0x92, 0x84, 0x82, 0x91, 0x84, 0x93, 0x80, 0x50 }; // LL. SECRETA
static const char8_t gen1_key_bono[]    = { 0x81, 0x8E, 0x8D, 0x8E, 0x7F, 0x81, 0x88, 0x82, 0x88, 0x50 }; // BONO BICI
static const char8_t gen1_key_ambar[]   = { 0xBF, 0x8C, 0x81, 0x80, 0x91, 0x7F, 0x95, 0x88, 0x84, 0x89, 0x8E, 0x50 }; // ÁMBAR VIEJO
static const char8_t gen1_key_oro[]     = { 0x83, 0x88, 0x84, 0x8D, 0x93, 0x84, 0x92, 0x7F, 0x8E, 0x91, 0x8E, 0x50 }; // DIENTES ORO
static const char8_t gen1_key_oak[]     = { 0x82, 0x8E, 0x91, 0x91, 0x84, 0x8E, 0x7F, 0x8E, 0x80, 0x8A, 0x50 }; // CORREO OAK
static const char8_t gen1_key_flauta[]  = { 0x8F, 0x8E, 0x8A, 0xBA, 0x7F, 0x85, 0x8B, 0x80, 0x94, 0x93, 0x80, 0x50 }; // POKé FLAUTA
static const char8_t gen1_key_ascens[]  = { 0x8B, 0x8B, 0xE8, 0x7F, 0x80, 0x92, 0x82, 0x84, 0x8D, 0x92, 0xE8, 0x50 }; // LL. ASCENS.
static const char8_t gen1_key_silph[]   = { 0x92, 0x82, 0x8E, 0x8F, 0x84, 0x7F, 0x92, 0x88, 0x8B, 0x8F, 0x87, 0x50 }; // SCOPE SILPH
static const char8_t gen1_key_medalla[] = { 0x8C, 0x84, 0x83, 0x80, 0x8B, 0x8B, 0x80, 0x50 }; // MEDALLA

void bankStoreRecordSpecies(uint8_t nationalDex, SAV_GameVersion originVersion)
{
	if (nationalDex == 0 || nationalDex > 251) return;
	uint8_t bit = nationalDex - 1;
	bankStore.dexSeen[bit / 8] |= (1 << (bit % 8));
	bankStore.dexOwned[bit / 8] |= (1 << (bit % 8));
	if (originVersion != NOT_POKEMON)
	{
		bankStore.dexOrigin[nationalDex] |= (1 << originVersion);
	}
}

bool bankStoreIsOwned(uint8_t nationalDex)
{
	if (nationalDex == 0 || nationalDex > 251) return false;
	uint8_t bit = nationalDex - 1;
	return (bankStore.dexOwned[bit / 8] & (1 << (bit % 8))) != 0;
}

bool bankStoreIsSeen(uint8_t nationalDex)
{
	if (nationalDex == 0 || nationalDex > 251) return false;
	uint8_t bit = nationalDex - 1;
	return (bankStore.dexSeen[bit / 8] & (1 << (bit % 8))) != 0;
}

uint16_t bankStoreCountKanto(void)
{
	uint16_t count = 0;
	for (uint8_t i = 1; i <= 151; i++)
	{
		if (bankStoreIsOwned(i)) count++;
	}
	return count;
}

uint16_t bankStoreCountNational(void)
{
	uint16_t count = 0;
	for (uint16_t i = 1; i <= 251; i++)
	{
		if (bankStoreIsOwned((uint8_t)i)) count++;
	}
	return count;
}

void bankStoreScanAll(void)
{
	// Scan active game's Pokédex Seen/Owned bitmasks
	// Scan all 20 Bank boxes
	for (uint8_t b = 0; b < sbank.boxCount; b++)
	{
		for (uint8_t s = 0; s < sbank.boxes[b].capacity; s++)
		{
			const SAV_Pokemon* pkm = &sbank.boxes[b].slots[s];
			if (!saveIsPkmEmpty(pkm) && !pkm->isEgg)
			{
				bankStoreRecordSpecies(pkm->nationalDex, NOT_POKEMON);
			}
		}
	}

	// Clamp any stored item count to 99 max
	for (uint16_t i = 0; i < 256; i++)
	{
		if (bankStore.itemCounts[i] > 99)
			bankStore.itemCounts[i] = 99;
	}
}

void bankStoreLoad(void)
{
	memset(&bankStore, 0, sizeof(BankStoreData));
	bankStore.magic = POKEDEX_BIN_MAGIC;
	bankStore.version = 2;

	FILE* fp = fopen(POKEDEX_BIN_PATH, "rb");
	if (fp)
	{
		BankStoreData tmp;
		size_t r = fread(&tmp, 1, sizeof(BankStoreData), fp);
		fclose(fp);
		if (r == sizeof(BankStoreData) && tmp.magic == POKEDEX_BIN_MAGIC)
		{
			memcpy(bankStore.itemCounts, tmp.itemCounts, sizeof(bankStore.itemCounts));
			if (tmp.version >= 2)
			{
				memcpy(bankStore.dexSeen, tmp.dexSeen, sizeof(bankStore.dexSeen));
				memcpy(bankStore.dexOwned, tmp.dexOwned, sizeof(bankStore.dexOwned));
				memcpy(bankStore.dexOrigin, tmp.dexOrigin, sizeof(bankStore.dexOrigin));
			}
		}
	}

	bankStoreScanAll();
}

void bankStoreSave(void)
{
	bankStoreScanAll();
	mkdir(BASE_FOLDER, 0700);
	mkdir(BANK_FOLDER, 0700);

	FILE* fp = fopen(POKEDEX_BIN_PATH, "wb");
	if (fp)
	{
		fwrite(&bankStore, 1, sizeof(BankStoreData), fp);
		fclose(fp);
	}
}

uint8_t itemToGen2(uint8_t itemId, bool isGen1)
{
	if (!isGen1) return itemId;
	return item_g1_to_g2[itemId];
}

uint8_t itemToGen1(uint8_t gen2Id)
{
	return items_table[gen2Id].gen1Id;
}

const char8_t* itemGetName(uint8_t itemId, bool isGen1)
{
	if (!isGen1)
	{
		return items_table[itemId].name;
	}
	uint8_t g2 = item_g1_to_g2[itemId];
	if (g2 != 0)
	{
		return items_table[g2].name;
	}
	switch (itemId)
	{
		case 0x05: return gen1_key_mapa;
		case 0x07: return gen1_key_surf;
		case 0x09: return gen1_key_pokedex;
		case 0x29: return gen1_key_fosil_d;
		case 0x2A: return gen1_key_fosil_h;
		case 0x2B: return gen1_key_secreta;
		case 0x2D: return gen1_key_bono;
		case 0x40: return gen1_key_oro;
		case 0x46: return gen1_key_oak;
		case 0x49: return gen1_key_flauta;
		case 0x4A: return gen1_key_ascens;
		case 0x48: return gen1_key_silph;
		case 0x1F: return gen1_key_ambar;
		default:
			if (itemId >= 0x15 && itemId <= 0x1C) return gen1_key_medalla;
			return items_table[0].name;
	}
}

bool itemCanStoreInBank(uint8_t itemId, bool isGen1)
{
	uint8_t g2 = itemToGen2(itemId, isGen1);
	if (g2 == 0) return false;
	uint8_t p = items_table[g2].pocket;
	// Allow Items (1), Balls (2), TMs (4); block Key Items & HMs (3) and Invalid (0)
	return (p == ITEM_POCKET_ITEMS || p == ITEM_POCKET_BALLS || p == ITEM_POCKET_TMHM);
}

bool itemCanGiveToGame(uint8_t gen2Id, bool targetIsGen1)
{
	if (!itemCanStoreInBank(gen2Id, false)) return false;
	if (targetIsGen1)
	{
		return (items_table[gen2Id].gen1Id != 0);
	}
	return true;
}

static uint16_t getGen2PocketOffset(ItemPocket pocket)
{
	bool isCrystal = (vgame == POKEMON_CRYSTAL);
	bool isJP = (lgame == POKEMON_JP);
	if (isJP)
	{
		// Japanese Gen 2 offsets
		switch (pocket)
		{
			case ITEM_POCKET_TMHM:  return isCrystal ? 0x23D9 : 0x23D8;
			case ITEM_POCKET_ITEMS: return isCrystal ? 0x2412 : 0x2411;
			case ITEM_POCKET_KEY:   return isCrystal ? 0x243C : 0x243B;
			case ITEM_POCKET_BALLS: return isCrystal ? 0x2457 : 0x2456;
			case ITEM_POCKET_PC:    return isCrystal ? 0x2471 : 0x2470;
			default: return 0;
		}
	}
	else
	{
		// International Gen 2 offsets (verified against user saves)
		switch (pocket)
		{
			case ITEM_POCKET_TMHM:  return isCrystal ? 0x23E7 : 0x23E6;
			case ITEM_POCKET_ITEMS: return isCrystal ? 0x2420 : 0x241F;
			case ITEM_POCKET_KEY:   return isCrystal ? 0x244A : 0x2449;
			case ITEM_POCKET_BALLS: return isCrystal ? 0x2465 : 0x2464;
			case ITEM_POCKET_PC:    return isCrystal ? 0x247F : 0x247E;
			default: return 0;
		}
	}
}


uint8_t gameItemGetCount(ItemPocket pocket)
{
	if (vgame <= POKEMON_YELLOW)
	{
		if (pocket == ITEM_POCKET_PC)
		{
			uint16_t off = (lgame == POKEMON_JP ? 0x27DC : 0x27E6);
			return (save[off] <= 50 ? save[off] : 0);
		}
		else
		{
			// All items, balls, and key items in Gen 1 are stored in the single general bag at 0x25C9 / 0x25C4
			uint16_t off = (lgame == POKEMON_JP ? 0x25C4 : 0x25C9);
			return (save[off] <= 20 ? save[off] : 0);
		}
	}
	else
	{
		uint16_t off = getGen2PocketOffset(pocket);
		if (!off) return 0;
		if (pocket == ITEM_POCKET_TMHM)
		{
			uint8_t cnt = 0;
			for (uint8_t i = 0; i < 57; i++)
			{
				if (save[off + i] > 0 && save[off + i] <= 99) cnt++;
			}
			return cnt;
		}
		uint8_t maxCap = (pocket == ITEM_POCKET_ITEMS ? 20 : (pocket == ITEM_POCKET_KEY ? 26 : (pocket == ITEM_POCKET_BALLS ? 12 : 50)));
		return (save[off] <= maxCap ? save[off] : 0);
	}
}

void gameItemGetSlot(ItemPocket pocket, uint8_t slot, uint8_t* outId, uint8_t* outQty)
{
	*outId = 0;
	*outQty = 0;

	if (vgame <= POKEMON_YELLOW)
	{
		uint16_t off = (pocket == ITEM_POCKET_PC ? (lgame == POKEMON_JP ? 0x27DC : 0x27E6) : (lgame == POKEMON_JP ? 0x25C4 : 0x25C9));
		uint8_t count = gameItemGetCount(pocket);
		if (slot >= count) return;
		*outId = save[off + 1 + slot * 2];
		*outQty = save[off + 2 + slot * 2];
	}
	else
	{
		uint16_t off = getGen2PocketOffset(pocket);
		if (!off) return;
		if (pocket == ITEM_POCKET_TMHM)
		{
			uint8_t idx = 0;
			for (uint8_t i = 0; i < 57; i++)
			{
				if (save[off + i] > 0 && save[off + i] <= 99)
				{
					if (idx == slot)
					{
						// Map TM index (0..49) or HM index (50..56) to Gen 2 item ID
						uint8_t g2Id;
						if (i < 4) g2Id = 190 + i;
						else if (i < 28) g2Id = 191 + i;
						else if (i < 50) g2Id = 192 + i;
						else g2Id = 242 + (i - 50);
						*outId = g2Id;
						*outQty = save[off + i];
						return;
					}
					idx++;
				}
			}
			return;
		}
		else if (pocket == ITEM_POCKET_KEY)
		{
			if (slot >= gameItemGetCount(pocket)) return;
			*outId = save[off + 1 + slot];
			*outQty = 1;
		}
		else
		{
			if (slot >= gameItemGetCount(pocket)) return;
			*outId = save[off + 1 + slot * 2];
			*outQty = save[off + 2 + slot * 2];
		}
	}
}

bool gameItemAdd(ItemPocket pocket, uint8_t gen2Id, uint8_t qty)
{
	if (qty == 0) return false;
	bool isGen1 = (vgame <= POKEMON_YELLOW);
	if (!itemCanGiveToGame(gen2Id, isGen1)) return false;

	if (isGen1)
	{
		uint8_t g1Id = itemToGen1(gen2Id);
		if (!g1Id) return false;
		bool isPC = (pocket == ITEM_POCKET_PC);
		uint16_t off = isPC ? (lgame == POKEMON_JP ? 0x27DC : 0x27E6) : (lgame == POKEMON_JP ? 0x25C4 : 0x25C9);
		uint8_t maxCap = isPC ? 50 : 20;
		uint8_t count = (save[off] <= maxCap ? save[off] : 0);
		for (uint8_t i = 0; i < count; i++)
		{
			if (save[off + 1 + i * 2] == g1Id && save[off + 2 + i * 2] + qty <= 99)
			{
				save[off + 2 + i * 2] += qty;
				return true;
			}
		}
		if (count >= maxCap) return false;
		save[off + 1 + count * 2] = g1Id;
		save[off + 2 + count * 2] = qty;
		save[off] = count + 1;
		save[off + 1 + (count + 1) * 2] = 0xFF;
		return true;
	}
	else
	{
		uint8_t p = items_table[gen2Id].pocket;
		if (p == ITEM_POCKET_TMHM)
		{
			uint16_t off = getGen2PocketOffset(ITEM_POCKET_TMHM);
			uint8_t tmIdx = 0xFF;
			if (gen2Id >= 190 && gen2Id <= 193) tmIdx = gen2Id - 190;
			else if (gen2Id >= 195 && gen2Id <= 218) tmIdx = gen2Id - 191;
			else if (gen2Id >= 220 && gen2Id <= 241) tmIdx = gen2Id - 192;
			if (tmIdx >= 50) return false;
			if (save[off + tmIdx] + qty > 99) return false;
			save[off + tmIdx] += qty;
			return true;
		}
		ItemPocket targetPocket = (p == ITEM_POCKET_BALLS ? ITEM_POCKET_BALLS : ITEM_POCKET_ITEMS);
		uint8_t maxCap = (targetPocket == ITEM_POCKET_BALLS ? 12 : 20);
		uint16_t off = getGen2PocketOffset(targetPocket);
		uint8_t count = gameItemGetCount(targetPocket);
		for (uint8_t i = 0; i < count; i++)
		{
			if (save[off + 1 + i * 2] == gen2Id && save[off + 2 + i * 2] + qty <= 99)
			{
				save[off + 2 + i * 2] += qty;
				return true;
			}
		}
		if (count >= maxCap) return false;
		save[off + 1 + count * 2] = gen2Id;
		save[off + 2 + count * 2] = qty;
		save[off] = count + 1;
		save[off + 1 + (count + 1) * 2] = 0xFF;
		return true;
	}
}

bool gameItemRemove(ItemPocket pocket, uint8_t slot, uint8_t qty)
{
	if (pocket == ITEM_POCKET_KEY) return false;
	uint8_t count = gameItemGetCount(pocket);
	if (slot >= count) return false;

	if (vgame <= POKEMON_YELLOW)
	{
		uint16_t off = (pocket == ITEM_POCKET_PC ? (lgame == POKEMON_JP ? 0x27DC : 0x27E6) : (lgame == POKEMON_JP ? 0x25C4 : 0x25C9));
		uint8_t maxCap = (pocket == ITEM_POCKET_PC ? 50 : 20);
		uint8_t rawCount = (save[off] <= maxCap ? save[off] : 0);
		if (slot >= rawCount) return false;

		uint8_t curQty = save[off + 2 + slot * 2];
		if (qty < curQty)
		{
			save[off + 2 + slot * 2] -= qty;
			return true;
		}
		for (uint8_t i = slot; i < rawCount - 1; i++)
		{
			save[off + 1 + i * 2] = save[off + 1 + (i + 1) * 2];
			save[off + 2 + i * 2] = save[off + 2 + (i + 1) * 2];
		}
		save[off] = rawCount - 1;
		save[off + 1 + (rawCount - 1) * 2] = 0xFF;
		return true;
	}
	else
	{
		uint16_t off = getGen2PocketOffset(pocket);
		if (!off) return false;
		if (pocket == ITEM_POCKET_TMHM)
		{
			uint8_t idx = 0;
			for (uint8_t i = 0; i < 57; i++)
			{
				if (save[off + i] > 0 && save[off + i] <= 99)
				{
					if (idx == slot)
					{
						if (i >= 50) return false; // Block removing HMs
						if (qty < save[off + i]) save[off + i] -= qty;
						else save[off + i] = 0;
						return true;
					}
					idx++;
				}
			}
			return false;
		}
		uint8_t curQty = save[off + 2 + slot * 2];
		if (qty < curQty)
		{
			save[off + 2 + slot * 2] -= qty;
			return true;
		}
		for (uint8_t i = slot; i < count - 1; i++)
		{
			save[off + 1 + i * 2] = save[off + 1 + (i + 1) * 2];
			save[off + 2 + i * 2] = save[off + 2 + (i + 1) * 2];
		}
		save[off] = count - 1;
		save[off + 1 + (count - 1) * 2] = 0xFF;
		return true;
	}
}

typedef struct {
	uint8_t id;
	uint8_t qty;
	uint8_t g2Id;
	const char8_t* name;
} GameSortSlot;

static int compareGameItemsAlpha(const void* a, const void* b)
{
	const GameSortSlot* slotA = (const GameSortSlot*)a;
	const GameSortSlot* slotB = (const GameSortSlot*)b;
	const char8_t* nameA = slotA->name;
	const char8_t* nameB = slotB->name;
	for (int i = 0; i < 12; i++)
	{
		if (nameA[i] == 0x50 && nameB[i] == 0x50) return 0;
		if (nameA[i] == 0x50) return -1;
		if (nameB[i] == 0x50) return 1;
		if (nameA[i] != nameB[i]) return (int)nameA[i] - (int)nameB[i];
	}
	return 0;
}

static int compareGameItemsID(const void* a, const void* b)
{
	const GameSortSlot* slotA = (const GameSortSlot*)a;
	const GameSortSlot* slotB = (const GameSortSlot*)b;
	if (slotA->g2Id != slotB->g2Id) return (int)slotA->g2Id - (int)slotB->g2Id;
	return (int)slotA->id - (int)slotB->id;
}

static int compareGameItemsType(const void* a, const void* b)
{
	const GameSortSlot* slotA = (const GameSortSlot*)a;
	const GameSortSlot* slotB = (const GameSortSlot*)b;
	uint8_t pA = items_table[slotA->g2Id].pocket;
	uint8_t pB = items_table[slotB->g2Id].pocket;
	if (pA != pB) return (int)pA - (int)pB;
	return compareGameItemsAlpha(a, b);
}

void gameItemSort(ItemPocket pocket, uint8_t sortMode)
{
	if (pocket == ITEM_POCKET_TMHM) return;
	bool isGen1 = (vgame <= POKEMON_YELLOW);
	uint8_t count = gameItemGetCount(pocket);
	if (count <= 1) return;

	GameSortSlot slots[50];
	for (uint8_t i = 0; i < count; i++)
	{
		uint8_t id = 0, qty = 0;
		gameItemGetSlot(pocket, i, &id, &qty);
		slots[i].id = id;
		slots[i].qty = qty;
		slots[i].g2Id = itemToGen2(id, isGen1);
		slots[i].name = itemGetName(id, isGen1);
	}

	if (sortMode == 0)
		qsort(slots, count, sizeof(GameSortSlot), compareGameItemsID);
	else if (sortMode == 1)
		qsort(slots, count, sizeof(GameSortSlot), compareGameItemsAlpha);
	else if (sortMode == 2)
		qsort(slots, count, sizeof(GameSortSlot), compareGameItemsType);

	if (isGen1)
	{
		bool isPC = (pocket == ITEM_POCKET_PC);
		uint16_t off = isPC ? (lgame == POKEMON_JP ? 0x27DC : 0x27E6) : (lgame == POKEMON_JP ? 0x25C4 : 0x25C9);
		for (uint8_t i = 0; i < count; i++)
		{
			save[off + 1 + i * 2] = slots[i].id;
			save[off + 2 + i * 2] = slots[i].qty;
		}
		save[off + 1 + count * 2] = 0xFF;
	}
	else
	{
		uint16_t off = getGen2PocketOffset(pocket);
		if (!off) return;
		if (pocket == ITEM_POCKET_KEY)
		{
			for (uint8_t i = 0; i < count; i++)
				save[off + 1 + i] = slots[i].id;
			save[off + 1 + count] = 0xFF;
		}
		else
		{
			for (uint8_t i = 0; i < count; i++)
			{
				save[off + 1 + i * 2] = slots[i].id;
				save[off + 2 + i * 2] = slots[i].qty;
			}
			save[off + 1 + count * 2] = 0xFF;
		}
	}
}
