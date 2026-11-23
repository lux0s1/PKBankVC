#include "box_viewer.h"

#include "key.h"
#include "gfx.h"
#include "font.h"
#include "data.h"
#include "save.h"
#include "items.h"
#include "personal.h"
#include "render.h"
#include "events.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BOX_ROW_PKM_COUNT (8)
#define BOX_COL_PKM_COUNT (4)
#define BOX_PKM_COUNT (BOX_ROW_PKM_COUNT*BOX_COL_PKM_COUNT)
#define BOX_HEADER_SELECTED (0)

typedef enum
{
	VIEW_MODE_BOXES = 0,
	VIEW_MODE_BAG = 1,
	VIEW_MODE_POKEDEX = 2,
	VIEW_MODE_DIPLOMA = 3,
	VIEW_MODE_PKM_EDIT = 4,
	VIEW_MODE_EVENTS = 5,
} ViewerMode;

typedef struct
{
	uint32_t down;
	uint32_t held;
	uint32_t up;
	touchPosition touch;
} KeyState;

typedef struct 
{
	bool inBank;
	int16_t box;
	int8_t row;
	int8_t col;
} CursorSlot;

typedef struct
{
	bool inBank;
	int16_t box;
	int8_t row;
	int8_t col;
	int8_t slot;
	SAV_PokemonList* vBox;
} CursorInbox;

typedef struct
{
	CursorSlot sSlot;
	CursorInbox pc;
	CursorInbox bk;
	CursorInbox* box;
	SAV_Pokemon* vPkm;
	SAV_Pokemon* sPkm;
	bool isPkmDragged : 1;
	unsigned:7;
} CursorBox;

static CursorBox cursor;
static KeyState ks;
static ViewerMode currentMode = VIEW_MODE_BOXES;

// Bag & Item Store state
static uint8_t bagPocket = 0; // 0: Objetos, 1: Balls, 2: Obj.Clave, 3: MT/MO, 4: PC
static bool bagInStore = false;
static int16_t bagGameCursor = 0;
static int16_t bagGameScroll = 0;
static int16_t bagStoreCursor = 0;
static int16_t bagStoreScroll = 0;
static bool bagCatalogMode = false;
static bool bagDeleteConfirm = false;
static uint8_t bagDeleteTargetG2 = 0;
static uint8_t bagCatalogItem = 1;
static uint8_t bagSortMode = 0; // 0: ID, 1: Nombre (A-Z), 2: Tipo/Bolsillo
static uint8_t bagGameSortMode = 0; // 0: ID, 1: Nombre (A-Z), 2: Tipo/Bolsillo
static char8_t bagStatusMsg[32];
static uint64_t bagStatusTime = 0;

// Pokédex & Diploma state
static int16_t dexCursor = 1; // 1..251
static int16_t dexScroll = 0;
static bool dexDiplomaIsGen2 = false;

// Historical Mystery Gifts state
static EventCategory evCategory = EVENT_CAT_GEN1;
static int16_t evCursor = 0;
static int16_t evScroll = 0;
static uint8_t evOtIdx = 0;
static bool evForceShiny = false;
static EventInjectDest evDest = EVENT_DEST_PC_BOX;
static uint16_t evFilteredIndices[128];
static uint16_t evFilteredCount = 0;

static bool boxExitRequested = false;
bool boxViewerWantsExit(void) { return boxExitRequested; }

static char8_t boxAlert1[36];
static char8_t boxAlert2[36];
static bool boxAlertActive = false;

static void showBoxAlert(const char* l1, const char* l2)
{
	memset(boxAlert1, 0x50, sizeof(boxAlert1));
	memset(boxAlert2, 0x50, sizeof(boxAlert2));
	fontConvertString(boxAlert1, l1);
	fontConvertString(boxAlert2, l2);
	boxAlertActive = true;
}

static uint64_t touchHoldStartTime = 0;
static bool touchHoldActive = false;
static bool touchHoldCloned = false;
static SAV_Pokemon* touchHoldSourcePkm = NULL;
static bool touchHoldSourceBank = false;
static int16_t touchHoldSourceRow = -1;
static int16_t touchHoldSourceCol = -1;

static uint8_t editorRow = 0;
static uint8_t editorCol = 0;
static uint8_t editorPage = 0; // 0: Stats/Item/Trade, 1: Moves
static uint8_t editorMoveSlot = 0;

static bool batchMode = false;
static bool batchSelected[POKEMON_LIST_MAX_COUNT];
static uint8_t batchSourceBox = 0;
static bool batchSourceInBank = false;

static uint8_t getBatchSelectedCount(void)
{
	uint8_t count = 0;
	for (uint8_t i = 0; i < POKEMON_LIST_MAX_COUNT; i++)
		if (batchSelected[i]) count++;
	return count;
}

static void clearBatchSelection(void)
{
	batchMode = false;
	memset(batchSelected, 0, sizeof(batchSelected));
}

static void executeBatchMove(void)
{
	uint8_t selCount = getBatchSelectedCount();
	if (selCount == 0) return;

	if (cursor.box->inBank == batchSourceInBank && cursor.box->box == batchSourceBox)
	{
		showBoxAlert("DESTINO INVALIDO", "EL DESTINO ES LA MISMA CAJA");
		return;
	}

	uint8_t freeSlots = 0;
	for (uint8_t i = 0; i < cursor.box->vBox->capacity; i++)
	{
		if (saveIsPkmEmpty(saveGetPkm(cursor.box->box, i, cursor.box->inBank)))
			freeSlots++;
	}

	if (freeSlots < selCount)
	{
		showBoxAlert("ESPACIO INSUFICIENTE", "LA CAJA DESTINO NO TIENE HUECOS");
		return;
	}

	// Legality validation before moving any Pokémon
	for (uint8_t i = 0; i < POKEMON_LIST_MAX_COUNT; i++)
	{
		if (!batchSelected[i]) continue;
		SAV_Pokemon* p = saveGetPkm(batchSourceBox, i, batchSourceInBank);
		if (!p || saveIsPkmEmpty(p)) continue;

		if (cursor.box->inBank && p->isEgg)
		{
			showBoxAlert("ACCION BLOQUEADA", "NO SE GUARDAN HUEVOS EN BANCO");
			return;
		}
		if (!saveCanPlacePokemon(p, cursor.box->inBank))
		{
			showBoxAlert("ACCION BLOQUEADA",
				(!cursor.box->inBank && vgame <= POKEMON_YELLOW) ? "HAY POKEMON INCOMPATIBLES CON GEN 1" : "POKEMON NO COMPATIBLE CON DESTINO");
			return;
		}
		if (!cursor.box->inBank && vgame <= POKEMON_YELLOW)
		{
			for (uint8_t m = 0; m < 4; m++)
			{
				if (p->moves[m] > 165)
				{
					showBoxAlert("ACCION BLOQUEADA", "MOVIMIENTO INCOMPATIBLE CON GEN 1");
					return;
				}
			}
		}
	}

	uint8_t destSlot = 0;
	for (uint8_t i = 0; i < POKEMON_LIST_MAX_COUNT; i++)
	{
		if (!batchSelected[i]) continue;
		SAV_Pokemon* srcPkm = saveGetPkm(batchSourceBox, i, batchSourceInBank);
		if (!srcPkm || saveIsPkmEmpty(srcPkm)) continue;

		while (destSlot < cursor.box->vBox->capacity && !saveIsPkmEmpty(saveGetPkm(cursor.box->box, destSlot, cursor.box->inBank)))
			destSlot++;
		if (destSlot >= cursor.box->vBox->capacity) break;

		SAV_Pokemon* dstPkm = saveGetPkm(cursor.box->box, destSlot, cursor.box->inBank);
		if (!saveMovePkm(dstPkm, srcPkm, batchSourceInBank, cursor.box->inBank))
		{
			showBoxAlert("ACCION BLOQUEADA", "MOVIMIENTO MULTI INTERRUMPIDO");
			return;
		}

		if (cursor.box->inBank)
		{
			if (!dstPkm->isEgg)
				bankStoreRecordSpecies(dstPkm->nationalDex, vgame);
		}
		else
		{
			if (!dstPkm->isEgg)
			{
				pokedexAddOwned(dstPkm->nationalDex);
				pokedexAddSeen(dstPkm->nationalDex);
			}
		}
		destSlot++;
	}

	clearBatchSelection();
	showBoxAlert("MOVIMIENTO EN LOTE", "TRANSFERENCIA COMPLETADA");
}

static void boxViewerDrawAlert(void);
static void pkmEditorUpdate(void);
static void pkmEditorDrawBottom(void);

static uint32_t getGameThemeBgColor(void)
{
	switch (vgame)
	{
		case POKEMON_RED:     return RENDER_RGBA(0x98, 0x28, 0x28, 0xFF);
		case POKEMON_BLUE:    return RENDER_RGBA(0x28, 0x48, 0x98, 0xFF);
		case POKEMON_GREEN:   return RENDER_RGBA(0x28, 0x80, 0x38, 0xFF);
		case POKEMON_YELLOW:  return RENDER_RGBA(0xB8, 0x90, 0x18, 0xFF);
		case POKEMON_GOLD:    return RENDER_RGBA(0x98, 0x78, 0x20, 0xFF);
		case POKEMON_SILVER:  return RENDER_RGBA(0x60, 0x68, 0x78, 0xFF);
		case POKEMON_CRYSTAL: return RENDER_RGBA(0x30, 0x78, 0xA8, 0xFF);
		default:              return RENDER_RGBA(0x38, 0x40, 0x50, 0xFF);
	}
}

static void setBagMessage(const char* str)
{
	memset(bagStatusMsg, 0x50, sizeof(bagStatusMsg));
	fontConvertString(bagStatusMsg, str);
	bagStatusTime = osGetTime();
}

static void computeSlot(CursorInbox* cursorin)
{
	cursorin->slot = cursorin->row * BOX_COL_PKM_COUNT + cursorin->col;
}

static void extractSlot(CursorInbox* cursorin, CursorSlot* cursorot)
{
	cursorot->inBank = cursorin->inBank;
	cursorot->box = cursorin->box;
	cursorot->row = cursorin->row;
	cursorot->col = cursorin->col;
}

static inline bool touchWithin(s16 px, s16 py, s16 x, s16 y, s16 w, s16 h)
{
	return (px >= x && py >= y) && (px <= x + w && py <= y + h);
}

static void drawRetroButton(int16_t x, int16_t y, int16_t w, int16_t h, u32 fillCol, u32 borderCol)
{
	renderDrawRect(x, y, w, h, fillCol);
	renderDrawRect(x, y, w, 1, borderCol);
	renderDrawRect(x, y + h - 1, w, 1, borderCol);
	renderDrawRect(x, y, 1, h, borderCol);
	renderDrawRect(x + w - 1, y, 1, h, borderCol);
}

static void convertSafeAscii(char8_t* out, const char* in, int maxLen)
{
	char tmp[64];
	int n = 0;
	if (maxLen > (int)sizeof(tmp) - 1) maxLen = (int)sizeof(tmp) - 1;
	if (in)
	{
		while (in[n] != '\0' && n < maxLen)
		{
			char c = in[n];
			if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
				c == ' ' || c == '-' || c == '?' || c == '!' || c == '.' || c == '/' ||
				c == ',' || c == ':' || c == '(' || c == '[' || c == ')' || c == ']' ||
				c == '+' || c == '>' || c == '<')
			{
				tmp[n] = c;
			}
			else if (c == ';') tmp[n] = ':';
			else if (c == '=') tmp[n] = '-';
			else if (c == '&') tmp[n] = '+';
			else tmp[n] = ' ';
			n++;
		}
	}
	tmp[n] = '\0';
	fontConvertString(out, tmp);
}

static void drawRetroButtonCentered(int16_t x, int16_t y, int16_t w, int16_t h, u32 fillCol, u32 borderCol, const char* label, bool withArrow)
{
	drawRetroButton(x, y, w, h, fillCol, borderCol);
	char8_t buf[48];
	int maxChars = (w - (withArrow ? 14 : 4)) / 8;
	if (maxChars < 1) maxChars = 1;
	if (maxChars > 46) maxChars = 46;
	convertSafeAscii(buf, label, maxChars);

	int len = 0;
	while (len < maxChars && buf[len] != 0x50) len++;
	int16_t contentW = (withArrow ? 10 : 0) + len * 8;
	int16_t tx = x + (w - contentW) / 2;
	if (tx < x + 2) tx = x + 2;
	int16_t ty = y + (h - 8) / 2;

	if (withArrow)
	{
		fontDrawChar8(tx, ty, 0xED); // ►
		fontDrawString8(tx + 10, ty, buf);
	}
	else
	{
		fontDrawString8(tx, ty, buf);
	}
}

static void boxViewerDrawTop(void);
static void boxViewerDrawBottom(void);

static void boxDrawBox(CursorInbox* cursor, int16_t x, int16_t y);
static void boxDrawPokemon(SAV_Pokemon* pkm, int16_t x, int16_t y);
static void boxSelectViewBox(void);
static void boxSelectViewPkm(void);
static void boxSelectMovePokemon(void);
static void boxCancelMovePokemon(void);
static void boxSwapBox(void);
static uint8_t boxGetHeight(void);
static uint8_t boxGetWidth(uint8_t row);
static void boxViewerDrawTop(void);
static void boxViewerDrawBottom(void);

static void bagViewerUpdate(void);
static void bagViewerDrawTop(void);
static void bagViewerDrawBottom(void);

static void dexViewerUpdate(void);
static void dexViewerDrawTop(void);
static void dexViewerDrawBottom(void);

static void eventsRefreshCategory(bool resetCursor);
static void eventsViewerUpdate(void);
static void eventsViewerDrawTop(void);
static void eventsViewerDrawBottom(void);

void boxViewerInitialize(void)
{
	currentMode = VIEW_MODE_BOXES;
	boxExitRequested = false;
	touchHoldActive = false;
	touchHoldCloned = false;
	touchHoldStartTime = 0;
	touchHoldSourcePkm = NULL;
	editorRow = 0;
	editorCol = 0;
	editorPage = 0;
	clearBatchSelection();
	boxAlertActive = false;
	bagPocket = 0;
	bagInStore = false;
	bagGameCursor = 0;
	bagGameScroll = 0;
	bagStoreCursor = 0;
	bagStoreScroll = 0;
	bagCatalogMode = false;
	bagDeleteConfirm = false;
	bagStatusMsg[0] = 0x50;
	dexCursor = 1;
	dexScroll = 0;
	evCategory = (vgame >= POKEMON_GOLD) ? EVENT_CAT_GEN2 : EVENT_CAT_GEN1;
	evCursor = 0;
	evScroll = 0;
	evOtIdx = 0;
	evForceShiny = false;
	evDest = EVENT_DEST_PC_BOX;
	eventsRefreshCategory(true);

	boxExitRequested = false;

	cursor.pc.inBank = false;
	cursor.pc.box = saveGetCurrentBox(save);
	cursor.pc.row = 0;
	cursor.pc.col = 0;
	cursor.pc.slot = 0;
	cursor.bk.inBank = true;
	cursor.bk.box = 0;
	cursor.bk.row = 0;
	cursor.bk.col = 0;
	cursor.bk.slot = 0;

	cursor.sPkm = NULL;

	cursor.box = &cursor.bk; boxSelectViewBox();
	cursor.box = &cursor.pc; boxSelectViewBox();
	boxSelectViewPkm();

	bankStoreLoad();

	renderSetClearColor(RENDER_RGBA(0xF8,0xF8,0xF8,0xFF));
}

static ItemPocket getActiveGamePocket(void)
{
	if (vgame <= POKEMON_YELLOW)
	{
		// Gen 1: 0 = Mochila de Juego, 1 = PC del Jugador
		return (bagPocket == 1 ? ITEM_POCKET_PC : ITEM_POCKET_ITEMS);
	}

	switch (bagPocket)
	{
		case 0: return ITEM_POCKET_ITEMS;
		case 1: return ITEM_POCKET_BALLS;
		case 2: return ITEM_POCKET_KEY;
		case 3: return ITEM_POCKET_TMHM;
		default: return ITEM_POCKET_PC;
	}
}

static int compareItemsAlpha(const void* a, const void* b)
{
	uint8_t idA = *(const uint8_t*)a;
	uint8_t idB = *(const uint8_t*)b;
	const char8_t* nameA = items_table[idA].name;
	const char8_t* nameB = items_table[idB].name;
	for (int i = 0; i < 12; i++)
	{
		if (nameA[i] == 0x50 && nameB[i] == 0x50) return 0;
		if (nameA[i] == 0x50) return -1;
		if (nameB[i] == 0x50) return 1;
		if (nameA[i] != nameB[i]) return (int)nameA[i] - (int)nameB[i];
	}
	return 0;
}

static int compareItemsType(const void* a, const void* b)
{
	uint8_t idA = *(const uint8_t*)a;
	uint8_t idB = *(const uint8_t*)b;
	uint8_t pA = items_table[idA].pocket;
	uint8_t pB = items_table[idB].pocket;
	if (pA != pB) return (int)pA - (int)pB;
	return compareItemsAlpha(a, b);
}

static uint16_t buildStoreList(uint8_t* outIds)
{
	uint16_t count = 0;
	for (uint16_t i = 1; i <= 255; i++)
	{
		if (bankStore.itemCounts[i] > 0)
			outIds[count++] = (uint8_t)i;
	}

	if (bagSortMode == 1 && count > 1)
	{
		qsort(outIds, count, sizeof(uint8_t), compareItemsAlpha);
	}
	else if (bagSortMode == 2 && count > 1)
	{
		qsort(outIds, count, sizeof(uint8_t), compareItemsType);
	}

	return count;
}

static void bagViewerUpdate(void)
{
	// Handle Delete Confirmation Popup first
	if (bagDeleteConfirm)
	{
		if (ks.down & KEY_A)
		{
			bankStore.itemCounts[bagDeleteTargetG2] = 0;
			bagDeleteConfirm = false;
			setBagMessage("OBJETO ELIMINADO");
			uint8_t storeIds[256];
			uint16_t sCount = buildStoreList(storeIds);
			if (bagStoreCursor > sCount) bagStoreCursor = sCount;
		}
		else if (ks.down & (KEY_B | KEY_X))
		{
			bagDeleteConfirm = false;
			setBagMessage("CANCELADO");
		}
		return;
	}

	if (bagCatalogMode)
	{
		if (ks.down & (KEY_B | KEY_X))
		{
			bagCatalogMode = false;
			return;
		}

		if (ks.down & KEY_UP)
		{
			do {
				bagCatalogItem = (bagCatalogItem > 1 ? bagCatalogItem - 1 : 255);
			} while (!itemCanStoreInBank(bagCatalogItem, false));
		}
		else if (ks.down & KEY_DOWN)
		{
			do {
				bagCatalogItem = (bagCatalogItem < 255 ? bagCatalogItem + 1 : 1);
			} while (!itemCanStoreInBank(bagCatalogItem, false));
		}
		else if (ks.down & (KEY_L | KEY_ZL))
		{
			for (int j = 0; j < 10; j++)
			{
				do {
					bagCatalogItem = (bagCatalogItem > 1 ? bagCatalogItem - 1 : 255);
				} while (!itemCanStoreInBank(bagCatalogItem, false));
			}
		}
		else if (ks.down & (KEY_R | KEY_ZR))
		{
			for (int j = 0; j < 10; j++)
			{
				do {
					bagCatalogItem = (bagCatalogItem < 255 ? bagCatalogItem + 1 : 1);
				} while (!itemCanStoreInBank(bagCatalogItem, false));
			}
		}

		if (ks.down & (KEY_A | KEY_RIGHT))
		{
			if (bankStore.itemCounts[bagCatalogItem] < 99)
			{
				bankStore.itemCounts[bagCatalogItem]++;
				setBagMessage("AGREGADO AL ALMACEN +1");
			}
			else
			{
				setBagMessage("MAXIMO 99 ALCANZADO");
			}
		}
		else if (ks.down & KEY_LEFT)
		{
			if (bankStore.itemCounts[bagCatalogItem] > 0)
			{
				bankStore.itemCounts[bagCatalogItem]--;
				setBagMessage("CANTIDAD -1");
			}
		}
		return;
	}

	if (ks.down & KEY_B)
	{
		bagCatalogMode = false;
		bagDeleteConfirm = false;
		currentMode = VIEW_MODE_BOXES;
		return;
	}

	bool isGen1 = (vgame <= POKEMON_YELLOW);
	uint8_t maxPockets = (isGen1 ? 2 : 5);

	// L/R: Switch storage/pocket
	if (ks.down & KEY_L)
	{
		bagPocket = (bagPocket == 0 ? maxPockets - 1 : bagPocket - 1);
		bagGameCursor = 0;
		bagGameScroll = 0;
	}
	else if (ks.down & KEY_R)
	{
		bagPocket = (bagPocket + 1) % maxPockets;
		bagGameCursor = 0;
		bagGameScroll = 0;
	}

	// ZL / ZR: Switch column between Game (ZL) and Storage (ZR)
	if (ks.down & KEY_ZL)
	{
		bagInStore = false;
	}
	else if (ks.down & KEY_ZR)
	{
		bagInStore = true;
	}

	ItemPocket gPocket = getActiveGamePocket();
	uint8_t gCount = gameItemGetCount(gPocket);
	uint8_t storeIds[256];
	uint16_t sCount = buildStoreList(storeIds);

	if (!bagInStore)
	{
		if (gCount > 0)
		{
			if (ks.down & KEY_UP) bagGameCursor = (bagGameCursor > 0 ? bagGameCursor - 1 : gCount - 1);
			else if (ks.down & KEY_DOWN) bagGameCursor = (bagGameCursor + 1) % gCount;
			if (bagGameCursor < bagGameScroll) bagGameScroll = bagGameCursor;
			if (bagGameCursor >= bagGameScroll + 8) bagGameScroll = bagGameCursor - 7;
		}
		else
		{
			bagGameCursor = 0;
			bagGameScroll = 0;
		}

		// C-Stick / C-Pad to sort game bag / PC items in real time
		if (ks.down & (KEY_CSTICK_RIGHT | KEY_CPAD_RIGHT))
		{
			bagGameSortMode = (bagGameSortMode + 1) % 3;
			gameItemSort(gPocket, bagGameSortMode);
			bagGameCursor = 0;
			bagGameScroll = 0;
			const char* tPrefix = (gPocket == ITEM_POCKET_PC) ? "PC: " : "MOCHILA: ";
			char msg[32];
			if (bagGameSortMode == 0) snprintf(msg, sizeof(msg), "%sORDEN ID", tPrefix);
			else if (bagGameSortMode == 1) snprintf(msg, sizeof(msg), "%sORDEN A-Z", tPrefix);
			else snprintf(msg, sizeof(msg), "%sORDEN TIPO", tPrefix);
			setBagMessage(msg);
		}
		else if (ks.down & (KEY_CSTICK_LEFT | KEY_CPAD_LEFT))
		{
			bagGameSortMode = (bagGameSortMode == 0 ? 2 : bagGameSortMode - 1);
			gameItemSort(gPocket, bagGameSortMode);
			bagGameCursor = 0;
			bagGameScroll = 0;
			const char* tPrefix = (gPocket == ITEM_POCKET_PC) ? "PC: " : "MOCHILA: ";
			char msg[32];
			if (bagGameSortMode == 0) snprintf(msg, sizeof(msg), "%sORDEN ID", tPrefix);
			else if (bagGameSortMode == 1) snprintf(msg, sizeof(msg), "%sORDEN A-Z", tPrefix);
			else snprintf(msg, sizeof(msg), "%sORDEN TIPO", tPrefix);
			setBagMessage(msg);
		}

		// A: Deposit 1 item from Game to Bank Store (capped at 99)
		if ((ks.down & KEY_A) && gCount > 0)
		{
			uint8_t id = 0, qty = 0;
			gameItemGetSlot(gPocket, bagGameCursor, &id, &qty);
			if (!itemCanStoreInBank(id, isGen1) || (!isGen1 && gPocket == ITEM_POCKET_KEY))
			{
				setBagMessage("SOLO LECTURA (OBJ. CLAVE)");
			}
			else
			{
				uint8_t g2 = itemToGen2(id, isGen1);
				if (bankStore.itemCounts[g2] >= 99)
				{
					setBagMessage("ALMACEN LLENO (MAX 99)");
				}
				else if (gameItemRemove(gPocket, bagGameCursor, 1))
				{
					bankStore.itemCounts[g2]++;
					setBagMessage("GUARDADO EN ALMACEN +1");
					uint8_t newCount = gameItemGetCount(gPocket);
					if (bagGameCursor >= newCount && newCount > 0) bagGameCursor = newCount - 1;
				}
			}
		}
	}
	else
	{
		uint16_t totalRows = sCount + 1;
		if (ks.down & KEY_UP) bagStoreCursor = (bagStoreCursor > 0 ? bagStoreCursor - 1 : totalRows - 1);
		else if (ks.down & KEY_DOWN) bagStoreCursor = (bagStoreCursor + 1) % totalRows;

		if (bagStoreCursor == 0)
		{
			// Row 0 is the anchored 'X: AGREGAR OBJ.' row
		}
		else
		{
			uint16_t itemIdx = bagStoreCursor - 1;
			if (itemIdx < bagStoreScroll) bagStoreScroll = itemIdx;
			if (itemIdx >= bagStoreScroll + 7) bagStoreScroll = itemIdx - 6;
		}

		// C-Stick, Circle Pad, or ZL/ZR to change sort order: ID -> ALFABETICO -> TIPO
		if (ks.down & (KEY_CSTICK_RIGHT | KEY_CPAD_RIGHT))
		{
			bagSortMode = (bagSortMode + 1) % 3;
			bagStoreCursor = 0;
			bagStoreScroll = 0;
			if (bagSortMode == 0) setBagMessage("ORDEN: NUMERICO (ID)");
			else if (bagSortMode == 1) setBagMessage("ORDEN: ALFABETICO (A-Z)");
			else setBagMessage("ORDEN: TIPO / BOLSILLO");
		}
		else if (ks.down & (KEY_CSTICK_LEFT | KEY_CPAD_LEFT))
		{
			bagSortMode = (bagSortMode == 0 ? 2 : bagSortMode - 1);
			bagStoreCursor = 0;
			bagStoreScroll = 0;
			if (bagSortMode == 0) setBagMessage("ORDEN: NUMERICO (ID)");
			else if (bagSortMode == 1) setBagMessage("ORDEN: ALFABETICO (A-Z)");
			else setBagMessage("ORDEN: TIPO / BOLSILLO");
		}

		// X: Directly open Catalog to add items to storage
		if (ks.down & KEY_X)
		{
			bagCatalogMode = true;
			if (!itemCanStoreInBank(bagCatalogItem, false)) bagCatalogItem = 1;
		}

		// A: Transfer 1 item to game bag, or open Catalog if on row 0 "X: AGREGAR OBJ."
		if (ks.down & KEY_A)
		{
			if (bagStoreCursor == 0)
			{
				bagCatalogMode = true;
				if (!itemCanStoreInBank(bagCatalogItem, false)) bagCatalogItem = 1;
			}
			else if (sCount > 0 && bagStoreCursor <= sCount)
			{
				uint8_t g2 = storeIds[bagStoreCursor - 1];
				if (!itemCanGiveToGame(g2, isGen1))
				{
					setBagMessage("INCOMPATIBLE CON GEN 1");
				}
				else
				{
					if (gameItemAdd(gPocket, g2, 1))
					{
						bankStore.itemCounts[g2]--;
						if (!isGen1)
						{
							uint8_t targetP = items_table[g2].pocket;
							if (targetP == ITEM_POCKET_BALLS) bagPocket = 1;
							else if (targetP == ITEM_POCKET_TMHM) bagPocket = 3;
							else bagPocket = 0;
							setBagMessage("ENVIADO A LA MOCHILA");
						}
						else
						{
							setBagMessage(bagPocket == 1 ? "ENVIADO AL PC DEL JUGADOR" : "ENVIADO A LA MOCHILA");
						}
					}
					else
					{
						if (isGen1)
							setBagMessage(bagPocket == 1 ? "PC DEL JUGADOR LLENO (MAX 50)" : "MOCHILA LLENA (MAX 20)");
						else
							setBagMessage("BOLSILLO LLENO");
					}
				}
			}
		}
		// D-Pad RIGHT: +10 quantity
		else if ((ks.down & KEY_RIGHT) && bagStoreCursor > 0 && (bagStoreCursor - 1) < sCount)
		{
			uint8_t g2 = storeIds[bagStoreCursor - 1];
			if (bankStore.itemCounts[g2] < 99)
			{
				if (bankStore.itemCounts[g2] + 10 <= 99)
					bankStore.itemCounts[g2] += 10;
				else
					bankStore.itemCounts[g2] = 99;
				setBagMessage("CANTIDAD +10");
			}
			else
			{
				setBagMessage("MAXIMO 99 ALCANZADO");
			}
		}
		// D-Pad LEFT: -10 quantity
		else if ((ks.down & KEY_LEFT) && bagStoreCursor > 0 && (bagStoreCursor - 1) < sCount)
		{
			uint8_t g2 = storeIds[bagStoreCursor - 1];
			if (bankStore.itemCounts[g2] > 10)
			{
				bankStore.itemCounts[g2] -= 10;
				setBagMessage("CANTIDAD -10");
			}
			else if (bankStore.itemCounts[g2] > 1)
			{
				bankStore.itemCounts[g2] = 1;
				setBagMessage("CANTIDAD -10 (MIN 1)");
			}
			else if (bankStore.itemCounts[g2] == 1)
			{
				bagDeleteConfirm = true;
				bagDeleteTargetG2 = g2;
			}
		}
	}
}

static void bagViewerDrawTop(void)
{
	gfxDrawFrame(0, 0);
	renderDrawRect(48, 40, 304, 160, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(52, 44, 35, 17);

	char8_t buf[48];

	// Centered title at y=54 inside panel (center x=200 relative to 400px top screen frame)
	fontConvertString(buf, "MOCHILA Y ALMACENAMIENTO");
	fontDrawString8(104, 54, buf);

	bool isFemale = (vgame == POKEMON_CRYSTAL && lgame != POKEMON_JP && (save[0x3E3D] & 1));
	bool isGen1 = (vgame <= POKEMON_YELLOW);

	// On Gen 1, keep bag icon static on 0 (do not toggle pocket animations)
	uint8_t iconPocket = isGen1 ? 0 : (bagPocket < 4 ? bagPocket : 0);
	gfxDrawBagUI(64, 72, iconPocket, isFemale);

	if (isGen1)
	{
		fontConvertString(buf, bagPocket == 1 ? "PC DEL JUGADOR" : "MOCHILA DE JUEGO");
		fontDrawString8(116, 72, buf);
		fontConvertString(buf, "L/R: CAMBIAR ALMACEN");
		fontDrawString8(116, 86, buf);
	}
	else
	{
		const char* pNames[5] = { "BOLSILLO OBJETOS", "BOLSILLO BALLS", "OBJETOS CLAVE", "BOLSILLO MT/MO", "PC DE OBJETOS" };
		const char* curPName = pNames[bagPocket % 5];
		fontConvertString(buf, curPName);
		fontDrawString8(116, 72, buf);
		fontConvertString(buf, "L/R: CAMBIAR BOLSILLO");
		fontDrawString8(116, 86, buf);
	}

	if (bagDeleteConfirm)
	{
		renderDrawRect(88, 98, 224, 68, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		gfxDrawPanel(88, 98, 26, 6);
		fontConvertString(buf, "ELIMINAR EL OBJETO?");
		fontDrawString8(120, 110, buf);
		fontDrawString8(136, 123, itemGetName(bagDeleteTargetG2, false));
		drawRetroButton(108, 138, 84, 16, RENDER_RGBA(0xEC, 0xDC, 0xDC, 0xFF), RENDER_RGBA(0x98, 0x78, 0x78, 0xFF));
		fontConvertString(buf, "A: SI");
		fontDrawString8(130, 142, buf);
		drawRetroButton(208, 138, 84, 16, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF), RENDER_RGBA(0x78, 0x82, 0x96, 0xFF));
		fontConvertString(buf, "B: NO");
		fontDrawString8(230, 142, buf);
		return;
	}

	if (bagCatalogMode)
	{
		renderDrawRect(104, 98, 240, 68, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		gfxDrawPanel(104, 98, 28, 6);
		fontConvertString(buf, "AGREGAR AL ALMACEN:");
		fontDrawString8(120, 108, buf);
		drawRetroButton(116, 120, 216, 16, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF), RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
		gfxDrawFullArrow(120, 124);
		fontDrawString8(130, 124, itemGetName(bagCatalogItem, false));
		fontDrawChar8(260, 124, 0xF1);
		fontDrawFromRightUInt32(284, 124, bankStore.itemCounts[bagCatalogItem]);
		drawRetroButton(116, 139, 96, 16, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF), RENDER_RGBA(0x78, 0x82, 0x96, 0xFF));
		fontConvertString(buf, "A: +1");
		fontDrawString8(144, 143, buf);
		drawRetroButton(220, 139, 112, 16, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF), RENDER_RGBA(0x78, 0x82, 0x96, 0xFF));
		fontConvertString(buf, "B: CERRAR");
		fontDrawString8(240, 143, buf);
		return;
	}
	else
	{
		// Controls list shifted to the right (x=116) to avoid overlapping with pocket tag (x=64..104)
		fontConvertString(buf, bagInStore ? "A: ENVIAR AL JUEGO" : "A: GUARDAR EN ALMACEN");
		fontDrawString8(116, 104, buf);

		fontConvertString(buf, "DPAD IZQ/DER: -10 / +10");
		fontDrawString8(116, 118, buf);

		fontConvertString(buf, "ZL/ZR: COLUMNA  C-PAD: ORDEN");
		fontDrawString8(116, 132, buf);

		fontConvertString(buf, "X: AGREGAR OBJ. AL ALMACEN");
		fontDrawString8(116, 146, buf);

		// Compatibility banner anchored at bottom of panel (y=164), centered across frame (x=200)
		uint8_t storeIds[256];
		uint16_t sCount = buildStoreList(storeIds);
		uint8_t g2Item = 0;
		if (bagInStore)
		{
			if (bagStoreCursor > 0 && (bagStoreCursor - 1) < sCount)
				g2Item = storeIds[bagStoreCursor - 1];
		}
		else
		{
			ItemPocket gPocket = getActiveGamePocket();
			uint8_t gCount = gameItemGetCount(gPocket);
			if (gCount > 0 && bagGameCursor < gCount)
			{
				uint8_t id = 0, qty = 0;
				gameItemGetSlot(gPocket, bagGameCursor, &id, &qty);
				g2Item = itemToGen2(id, isGen1);
			}
		}

		if (g2Item != 0)
		{
			bool okG1 = itemCanGiveToGame(g2Item, true);
			fontConvertString(buf, okG1 ? "COMPATIBLE: GEN 1 Y GEN 2" : "EXCLUSIVO DE 2A GENERACION");
			int16_t lenC = 0; while (buf[lenC] != 0x50) lenC++;
			fontDrawString8(200 - (lenC * 8) / 2, 164, buf);
		}
	}
	if (bagStatusMsg[0] != 0x50 && (osGetTime() - bagStatusTime) < 2500)
	{
		renderDrawRect(64, 160, 272, 24, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		gfxDrawPanel(64, 160, 32, 1);
		fontDrawString8(80, 168, bagStatusMsg);
	}
}

static void bagViewerDrawBottom(void)
{
	char8_t buf[32];
	renderDrawRect(0, 0, 320, 240, getGameThemeBgColor());

	// Left Column: Game Pocket (x=0, w=160; inner x=8..151)
	renderDrawRect(0, 0, 160, 32, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(0, 0, 18, 2);
	renderDrawRect(0, 32, 160, 208, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(0, 32, 18, 24);
	bool isGen1 = (vgame <= POKEMON_YELLOW);
	if (isGen1)
	{
		fontConvertString(buf, bagPocket == 1 ? "PC DEL JUGADOR" : "MOCHILA DE JUEGO");
	}
	else
	{
		fontConvertString(buf, bagPocket == 4 ? "PC DE OBJETOS" : "MOCHILA DE JUEGO");
	}
	fontDrawString8(12, 12, buf);

	ItemPocket gPocket = getActiveGamePocket();
	uint8_t gCount = gameItemGetCount(gPocket);

	for (uint8_t i = 0; i < 8; i++)
	{
		uint8_t idx = bagGameScroll + i;
		if (idx >= gCount) break;
		uint8_t id = 0, qty = 0;
		gameItemGetSlot(gPocket, idx, &id, &qty);
		int16_t ry = 46 + i * 22;
		if (!bagInStore && idx == bagGameCursor)
			gfxDrawFullArrow(8, ry);
		fontDrawString8(16, ry, itemGetName(id, isGen1));
		if (gPocket != ITEM_POCKET_KEY && itemCanStoreInBank(id, isGen1))
		{
			fontDrawChar8(122, ry, 0xF1); // ×
			fontDrawFromRightUInt32(138, ry, qty);
		}
	}

	// Right Column: Bank Store (x=160, w=160; inner x=168..311)
	renderDrawRect(160, 0, 160, 32, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(160, 0, 18, 2);
	renderDrawRect(160, 32, 160, 208, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(160, 32, 18, 24);
	fontConvertString(buf, "ALMACENAMIENTO");
	fontDrawString8(172, 12, buf);

	uint8_t storeIds[256];
	uint16_t sCount = buildStoreList(storeIds);

	// Row 0 is ALWAYS anchored at top as a retro action button: "X: AGREGAR OBJ."
	bool addSel = (bagInStore && bagStoreCursor == 0);
	drawRetroButton(168, 42, 144, 16,
		addSel ? RENDER_RGBA(0xFF, 0xFF, 0xFF, 0xFF) : RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF),
		addSel ? RENDER_RGBA(0x28, 0x30, 0x40, 0xFF) : RENDER_RGBA(0x78, 0x82, 0x96, 0xFF));
	if (addSel)
		gfxDrawFullArrow(171, 46);
	fontConvertString(buf, "X: AGREGAR OBJ.");
	fontDrawString8(181, 46, buf);

	// Rows 1..7 (up to 7 items visible) scroll smoothly underneath
	for (uint8_t i = 0; i < 7; i++)
	{
		uint16_t itemIdx = bagStoreScroll + i;
		if (itemIdx >= sCount) break;
		int16_t ry = 46 + (i + 1) * 22;
		if (bagInStore && (itemIdx + 1) == bagStoreCursor)
			gfxDrawFullArrow(168, ry);

		uint8_t g2 = storeIds[itemIdx];
		fontDrawString8(176, ry, itemGetName(g2, false));
		fontDrawChar8(282, ry, 0xF1); // ×
		fontDrawFromRightUInt32(298, ry, bankStore.itemCounts[g2]);
	}
}


// Habitat coordinates (mx, my) on the 160x144 Johto & Kanto maps
// Mapped precisely to the authentic Game Boy town and route grid tiles!
static const struct { uint8_t mx, my; const char* area; } kantoSpots[16] = {
	{  51, 107, "PUEBLO PALETA / RUTA 1" },
	{  51,  75, "CIUDAD VERDE / RUTA 2" },
	{  51,  51, "CIUDAD PLATEADA / RUTA 3" },
	{  75,  51, "MT. MOON / RUTA 4" },
	{  99,  51, "CIUDAD CELESTE / RUTA 24" },
	{  99,  67, "CIUDAD AZAFRAN / RUTA 6" },
	{  75,  67, "CIUDAD AZULONA / RUTA 7" },
	{ 131,  67, "PUEBLO LAVANDA / TORRE" },
	{  99,  83, "CIUDAD CARMIN / RUTA 11" },
	{ 131,  91, "RUTA 12 / PUENTE SILENCIO" },
	{  91, 115, "CIUDAD FUCSIA / ZONA SAFARI" },
	{  51, 131, "ISLA CANELA / MANSION" },
	{  75, 131, "ISLAS ESPUMA" },
	{ 131,  51, "CENTRAL ENERGIA / RUTA 10" },
	{  27,  35, "CALLE VICTORIA / MESETA" },
	{ 115,  51, "CUEVA CELESTE / RUTA 9" }
};

static const struct { uint8_t mx, my; const char* area; } johtoSpots[16] = {
	{ 140, 100, "P. PRIMAVERA / RUTA 29" },
	{ 100, 100, "C. CEREZO / RUTA 30" },
	{  84,  60, "C. MALVA / TORRE BELLSPROUT" },
	{  92, 116, "CUEVA UNION / RUTA 33" },
	{  84, 124, "POZO SLOWPOKE" },
	{  68, 124, "PUEBLO AZALEA / ENCINAR" },
	{  52,  92, "CIUDAD TRIGAL / RUTA 35" },
	{  68,  44, "CIUDAD IRIS / TORRES" },
	{  36,  60, "CIUDAD OLIVO / FARO" },
	{  20, 100, "CIUDAD ORQUIDEA / RUTA 41" },
	{ 108,  44, "PUEBLO CAOBA / RUTA 42" },
	{ 108,  28, "LAGO DE LA FURIA" },
	{ 132,  44, "CIUDAD ENDRINO / CUEVA" },
	{  28,  92, "ISLAS REMOLINO" },
	{ 148,  68, "MT. PLATEADO" },
	{  76,  76, "RUINAS ALFA" }
};

static uint8_t getSpeciesSpotIndex(uint8_t sp, bool* outIsKanto)
{
	*outIsKanto = (sp <= 151);
	if (sp >= 1 && sp <= 9) return 0;
	if (sp == 144) return 11;
	if (sp == 145) return 12;
	if (sp == 146) return 13;
	if (sp == 150 || sp == 151) return 14;
	if (sp >= 152 && sp <= 160) return 0;
	if (sp == 201) return 15;
	if (sp == 249) return 12;
	if (sp == 250) return 13;
	if (sp == 251) return 4;
	if (sp == 246 || sp == 247 || sp == 248) return 14;
	return (sp * 7 + 3) % 15;
}

static void dexViewerUpdate(void)
{
	if (currentMode == VIEW_MODE_DIPLOMA)
	{
		if (ks.down & (KEY_B | KEY_A | KEY_START))
		{
			currentMode = VIEW_MODE_POKEDEX;
		}
		return;
	}

	if (bankStoreCanClaimMewGF())
	{
		bool claimPressed = (ks.down & KEY_START);
		if (ks.down & KEY_TOUCH)
		{
			hidTouchRead(&ks.touch);
			if (touchWithin(ks.touch.px, ks.touch.py, 24, 196, 272, 28))
				claimPressed = true;
		}

		if (claimPressed)
		{
			int16_t boxNum = bankStoreClaimMewGF();
			if (boxNum > 0)
			{
				char alertBuf[36];
				snprintf(alertBuf, sizeof(alertBuf), "MEW DE EVENTO GF EN CAJA %d", (int)boxNum);
				showBoxAlert("REGALO MISTERIOSO!", alertBuf);
			}
			else
			{
				showBoxAlert("BANCO LLENO", "LIBERA ESPACIO PARA MEW");
			}
			return;
		}
	}

	if (ks.down & KEY_B)
	{
		currentMode = VIEW_MODE_BOXES;
		return;
	}
	if (ks.down & KEY_X)
	{
		currentMode = VIEW_MODE_BAG;
		return;
	}
	if (ks.down & KEY_A)
	{
		if (!saveIsGamePokedexComplete())
		{
			showBoxAlert("DIPLOMA BLOQUEADO", "COMPLETA LA POKEDEX DEL JUEGO");
			return;
		}
		// Use Gen 1 diploma for Gen 1 games (Red, Blue, Yellow), and Gold/Silver diploma for Gen 2 (Gold, Silver, Crystal)
		dexDiplomaIsGen2 = (vgame >= POKEMON_GOLD);
		currentMode = VIEW_MODE_DIPLOMA;
		return;
	}

	if (ks.down & KEY_UP)
	{
		dexCursor = (dexCursor > 1 ? dexCursor - 1 : 251);
	}
	else if (ks.down & KEY_DOWN)
	{
		dexCursor = (dexCursor < 251 ? dexCursor + 1 : 1);
	}
	else if (ks.down & (KEY_LEFT | KEY_L))
	{
		dexCursor -= 7;
		if (dexCursor < 1) dexCursor = 1;
	}
	else if (ks.down & (KEY_RIGHT | KEY_R))
	{
		dexCursor += 7;
		if (dexCursor > 251) dexCursor = 251;
	}

	if (dexCursor - 1 < dexScroll) dexScroll = dexCursor - 1;
	if (dexCursor - 1 >= dexScroll + 7) dexScroll = dexCursor - 7;
}

static void dexViewerDrawTop(void)
{
	gfxDrawFrame(0, 0);
	renderDrawRect(48, 40, 304, 160, RENDER_RGBA(0x20, 0x20, 0x28, 0xFF));

	if (currentMode == VIEW_MODE_DIPLOMA)
	{
		bool isGen2 = (vgame >= POKEMON_GOLD);
		gfxDrawDiplomaBg(52, 48, isGen2);
		const char8_t* tName = saveGetTrainerName();
		if (isGen2)
			fontDrawString8(52 + 88, 48 + 40, tName);
		else
			fontDrawString8(52 + 88, 48 + 32, tName);

		renderDrawRect(216, 48, 128, 144, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		gfxDrawPanel(216, 48, 14, 16);

		char8_t buf[24];
		fontConvertString(buf, isGen2 ? "DIPLOMA JOHTO" : "DIPLOMA KANTO");
		fontDrawString8(226, 60, buf);

		uint16_t gCnt = saveGetGamePokedexOwnedCount();

		if (isGen2)
		{
			fontConvertString(buf, "NACIONAL:");
			fontDrawString8(226, 84, buf);
			fontDrawPaddingUInt8(234, 98, gCnt);
			fontDrawChar8(258, 98, CHAR8_SLASH);
			fontDrawPaddingUInt8(266, 98, 251);
		}
		else
		{
			fontConvertString(buf, "POKEDEX:");
			fontDrawString8(226, 84, buf);
			fontDrawPaddingUInt8(234, 98, gCnt);
			fontDrawChar8(258, 98, CHAR8_SLASH);
			fontDrawPaddingUInt8(266, 98, 151);
		}

		fontConvertString(buf, "B: VOLVER");
		fontDrawString8(226, 166, buf);
		return;
	}

	// Draw authentic Crystal/GS Pokédex UI (160x144) at left of top screen (52, 48)
	int16_t dx = 52, dy = 48;
	gfxDrawPokedexBg(dx, dy);

	bool owned = bankStoreIsOwned(dexCursor);
	bool seen  = bankStoreIsSeen(dexCursor) || owned;

	if (seen)
	{
		// Draw the full 56x56 animated Pokémon Crystal front sprite centered at (dx+3, dy+8) inside the preview box!
		GFX_Frame fr = ((osGetTime() / 260) % 2 == 0) ? GFX_FRAME_0 : GFX_FRAME_1;
		gfxDrawCrystalSprite(dx + 3, dy + 8, dexCursor, fr);
	}

	// Draw VIST / TIEN labels and numbers in WHITE font on the black Pokédex background!
	uint16_t totalSeen = 0, totalOwn = bankStoreCountNational();
	for (uint16_t i = 1; i <= 251; i++)
	{
		if (bankStoreIsSeen((uint8_t)i) || bankStoreIsOwned((uint8_t)i)) totalSeen++;
	}

	char8_t buf[24];
	fontSetWhite(true);

	fontConvertString(buf, "VIST");
	fontDrawString8(dx + 4, dy + 84, buf);
	fontDrawPaddingUInt8(dx + 28, dy + 94, totalSeen);

	fontConvertString(buf, "TIEN");
	fontDrawString8(dx + 4, dy + 108, buf);
	fontDrawPaddingUInt8(dx + 28, dy + 118, totalOwn);

	// Draw 7 visible species entries in the Pokédex list (dx + 68, dy + 9) in WHITE font!
	for (int16_t i = 0; i < 7; i++)
	{
		uint8_t sp = dexScroll + i + 1;
		if (sp > 251) break;
		int16_t ry = dy + 9 + i * 16;

		fontDrawPaddingUInt8(dx + 68, ry, sp);
		if (bankStoreIsOwned(sp))
		{
			fontDrawChar8(dx + 68, ry + 8, CHAR8_POKEBALL_SEEN);
		}
		else if (bankStoreIsSeen(sp))
		{
			fontDrawChar8(dx + 68, ry + 8, CHAR8_DEX_SEEN);
		}
		if (bankStoreIsSeen(sp) || bankStoreIsOwned(sp))
		{
			fontDrawString8(dx + 76, ry + 8, dataSpecies(sp));
		}
		else
		{
			static const char8_t unk[] = { 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x50 };
			fontDrawString8(dx + 76, ry + 8, unk);
		}

		// Authentic Gold/Silver hollow green selector frame around the selected species!
		if (sp == dexCursor)
		{
			int16_t fx = dx + 63;
			int16_t fy = ry - 3;
			uint32_t cLight = RENDER_RGBA(0x58, 0xB8, 0x00, 0xFF);
			uint32_t cDark  = RENDER_RGBA(0x38, 0x88, 0x00, 0xFF);

			// Top left & right bracket arms (with center gap matching GS Pokédex asset)
			renderDrawRect(fx + 2,  fy,     37, 1, cLight);
			renderDrawRect(fx + 56, fy,     37, 1, cLight);
			renderDrawRect(fx + 1,  fy + 1, 38, 1, cDark);
			renderDrawRect(fx + 56, fy + 1, 38, 1, cDark);
			renderDrawRect(fx + 1,  fy + 2, 38, 1, cLight);
			renderDrawRect(fx + 56, fy + 2, 38, 1, cLight);

			// Bottom left & right bracket arms
			renderDrawRect(fx + 1,  fy + 18, 38, 1, cLight);
			renderDrawRect(fx + 56, fy + 18, 38, 1, cLight);
			renderDrawRect(fx + 1,  fy + 19, 38, 1, cDark);
			renderDrawRect(fx + 56, fy + 19, 38, 1, cDark);
			renderDrawRect(fx + 2,  fy + 20, 37, 1, cLight);
			renderDrawRect(fx + 56, fy + 20, 37, 1, cLight);

			// Upper vertical side brackets
			renderDrawRect(fx,      fy + 2, 3, 5, cLight);
			renderDrawRect(fx + 1,  fy + 2, 1, 4, cDark);
			renderDrawRect(fx + 92, fy + 2, 3, 5, cLight);
			renderDrawRect(fx + 93, fy + 2, 1, 4, cDark);

			// Lower vertical side brackets
			renderDrawRect(fx,      fy + 14, 3, 5, cLight);
			renderDrawRect(fx + 1,  fy + 15, 1, 4, cDark);
			renderDrawRect(fx + 92, fy + 14, 3, 5, cLight);
			renderDrawRect(fx + 93, fy + 15, 1, 4, cDark);
		}
	}

	fontSetWhite(false);

	// Right Panel (212, 48, 136x144; inner x=220..339): Detailed Species Info, Types, Progress & Retro Buttons
	renderDrawRect(212, 48, 136, 144, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(212, 48, 15, 16);

	static const char8_t unkName[] = { 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x50 };
	static const char8_t unkType[] = { 0xE3, 0xE3, 0xE3, 0x50 };

	if (owned)
	{
		const PersonalInfo* pInfo = Personal(dexCursor);
		fontDrawString8(222, 58, dataSpecies(dexCursor));

		fontConvertString(buf, "REGISTRADO");
		fontDrawString8(222, 70, buf);

		if (owned)
		{
			fontDrawString8(222, 84, dataText(TEXT_TYPE1));
			fontDrawChar8(262, 84, CHAR8_SLASH);
			fontDrawString8(270, 84, dataType(pInfo->type1));
			if (pInfo->type2 != pInfo->type1)
			{
				fontDrawString8(222, 96, dataText(TEXT_TYPE2));
				fontDrawChar8(262, 96, CHAR8_SLASH);
				fontDrawString8(270, 96, dataType(pInfo->type2));
			}
		}
	}
	else
	{
		fontDrawString8(222, 58, unkName);

		fontConvertString(buf, "DESCONOCIDO");
		fontDrawString8(222, 70, buf);

		fontDrawString8(222, 84, dataText(TEXT_TYPE1));
		fontDrawChar8(262, 84, CHAR8_SLASH);
		fontDrawString8(270, 84, unkType);
	}

	fontConvertString(buf, "KANTO:");
	fontDrawString8(222, 110, buf);
	fontDrawPaddingUInt8(230, 120, bankStoreCountKanto());
	fontDrawChar8(254, 120, CHAR8_SLASH);
	fontDrawPaddingUInt8(262, 120, 151);

	fontConvertString(buf, "NACIONAL:");
	fontDrawString8(222, 130, buf);
	fontDrawPaddingUInt8(230, 140, totalOwn);
	fontDrawChar8(254, 140, CHAR8_SLASH);
	fontDrawPaddingUInt8(262, 140, 251);

	bool canDip = saveIsGamePokedexComplete();
	drawRetroButton(222, 151, 114, 15,
		canDip ? RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF) : RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF),
		canDip ? RENDER_RGBA(0x28, 0x30, 0x40, 0xFF) : RENDER_RGBA(0x78, 0x82, 0x96, 0xFF));
	if (canDip)
	{
		fontDrawChar8(226, 155, 0xED);
		fontConvertString(buf, "A: DIPLOMA");
		fontDrawString8(236, 155, buf);
	}
	else
	{
		fontConvertString(buf, "SIN DIPLOMA");
		fontDrawString8(235, 155, buf);
	}

	drawRetroButton(222, 168, 114, 15, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF), RENDER_RGBA(0x78, 0x82, 0x96, 0xFF));
	fontConvertString(buf, "B: VOLVER");
	fontDrawString8(243, 172, buf);
}

static void dexViewerDrawBottom(void)
{
	renderDrawRect(0, 0, 320, 240, getGameThemeBgColor());

	bool owned = bankStoreIsOwned(dexCursor);
	bool isKanto = (dexCursor <= 151);
	uint8_t spotIdx = getSpeciesSpotIndex(dexCursor, &isKanto);
	const char* areaName = isKanto ? kantoSpots[spotIdx].area : johtoSpots[spotIdx].area;
	uint8_t mx = isKanto ? kantoSpots[spotIdx].mx : johtoSpots[spotIdx].mx;
	uint8_t my = isKanto ? kantoSpots[spotIdx].my : johtoSpots[spotIdx].my;

	// Header banner: "REGION DE KANTO" / "REGION DE JOHTO"
	char8_t buf[36];
	renderDrawRect(24, 12, 272, 24, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(24, 12, 32, 1);
	fontConvertString(buf, isKanto ? "REGION DE KANTO" : "REGION DE JOHTO");
	fontDrawString8(100, 20, buf);

	// Draw Gold/Silver World Map centered at (80, 44) (160x144)
	int16_t mapX = 80, mapY = 44;
	renderDrawRect(mapX - 4, mapY - 4, 168, 152, RENDER_RGBA(0x20, 0x24, 0x30, 0xFF));
	gfxDrawWorldMap(mapX, mapY, isKanto);

	if (owned)
	{
		fontDrawString8(mapX + 72, mapY + 4, dataSpecies(dexCursor));

		if (((osGetTime() / 250) % 2) == 0)
		{
			renderDrawRect(mapX + mx - 6, mapY + my - 6, 14, 14, RENDER_RGBA(0xF8, 0x20, 0x20, 0xD0));
			renderDrawRect(mapX + mx - 4, mapY + my - 4, 10, 10, RENDER_RGBA(0xFF, 0xE0, 0x30, 0xFF));
		}
		gfxDrawPokemonIcon(mapX + mx - 8, mapY + my - 8, dexCursor, ((osGetTime() / 250) % 2) ? GFX_FRAME_1 : GFX_FRAME_0);
	}
	else
	{
		static const char8_t unkMap[] = { 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x50 };
		fontDrawString8(mapX + 72, mapY + 4, unkMap);
	}

	// Bottom Area Name panel (or Mystery Gift button if available)
	if (bankStoreCanClaimMewGF())
	{
		renderDrawRect(24, 196, 272, 32, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		gfxDrawPanel(24, 196, 32, 2);
		drawRetroButton(34, 204, 252, 16, RENDER_RGBA(0xFF, 0xDC, 0x40, 0xFF), RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
		fontDrawChar8(38, 208, 0xED);
		fontConvertString(buf, "START/TOCAR: RECLAMAR MEW GF");
		fontDrawString8(48, 208, buf);
	}
	else
	{
		renderDrawRect(24, 196, 272, 32, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		gfxDrawPanel(24, 196, 32, 2);
		fontConvertString(buf, owned ? areaName : "AREA DESCONOCIDA");
		fontDrawString8(40, 208, buf);
	}
}

static bool isEventShinyEffective(const HistoricalEvent* ev, bool forceShiny)
{
	if (!ev) return false;
	if (ev->dv_mode == EVENT_DV_SHINY_GEN2) return true;
	if (eventAllowsManualShiny(ev) && forceShiny) return true;
	if (ev->dv_mode == EVENT_DV_FIXED)
	{
		return (ev->fixed_dvs[1] == 10 && ev->fixed_dvs[2] == 10 &&
		        ev->fixed_dvs[3] == 10 && ((ev->fixed_dvs[0] & 2) == 2));
	}
	return false;
}

static void eventsRefreshCategory(bool resetCursor)
{
	evFilteredCount = eventsGetByCategory(evCategory, evFilteredIndices, 128);
	if (evFilteredCount == 0)
	{
		evCursor = 0;
		evScroll = 0;
		evOtIdx = 0;
		evForceShiny = false;
		return;
	}
	if (resetCursor)
	{
		evCursor = 0;
		evScroll = 0;
		evOtIdx = 0;
		evForceShiny = false;
	}
	else
	{
		if (evCursor >= (int16_t)evFilteredCount) evCursor = (int16_t)evFilteredCount - 1;
		if (evCursor < 0) evCursor = 0;
		if (evScroll > evCursor) evScroll = evCursor;
		if (evScroll + 6 <= evCursor) evScroll = evCursor - 5;
		if (evScroll < 0) evScroll = 0;
	}
	const HistoricalEvent* ev = eventsGet(evFilteredIndices[evCursor]);
	if (!ev || evOtIdx >= ev->ot_count) evOtIdx = 0;
	if (!eventAllowsManualShiny(ev)) evForceShiny = false;
}

static void eventsApplyInjection(void)
{
	if (evFilteredCount == 0 || evCursor >= (int16_t)evFilteredCount) return;
	const HistoricalEvent* ev = eventsGet(evFilteredIndices[evCursor]);
	if (!ev) return;

	char msg1[40], msg2[40];
	eventInject(ev, evOtIdx, evForceShiny, evDest, msg1, msg2);
	showBoxAlert(msg1, msg2);
	boxSelectViewBox();
	boxSelectViewPkm();
}

static void eventsViewerUpdate(void)
{
	const HistoricalEvent* curEv = (evFilteredCount > 0 && evCursor < (int16_t)evFilteredCount)
		? eventsGet(evFilteredIndices[evCursor]) : NULL;

	if (ks.down & KEY_B)
	{
		boxSelectViewBox();
		boxSelectViewPkm();
		currentMode = VIEW_MODE_BOXES;
		return;
	}
	if (ks.down & KEY_L)
	{
		evCategory = (EventCategory)((evCategory + 3) % 4);
		eventsRefreshCategory(true);
		return;
	}
	if (ks.down & KEY_R)
	{
		evCategory = (EventCategory)((evCategory + 1) % 4);
		eventsRefreshCategory(true);
		return;
	}
	if ((ks.down & KEY_UP) && evFilteredCount > 0)
	{
		evCursor--;
		if (evCursor < 0) evCursor = (int16_t)evFilteredCount - 1;
		if (evCursor < evScroll) evScroll = evCursor;
		if (evCursor >= evScroll + 6) evScroll = evCursor - 5;
		if (evScroll < 0) evScroll = 0;
		evOtIdx = 0;
		curEv = eventsGet(evFilteredIndices[evCursor]);
		if (!eventAllowsManualShiny(curEv)) evForceShiny = false;
		return;
	}
	if ((ks.down & KEY_DOWN) && evFilteredCount > 0)
	{
		evCursor++;
		if (evCursor >= (int16_t)evFilteredCount) evCursor = 0;
		if (evCursor < evScroll) evScroll = evCursor;
		if (evCursor >= evScroll + 6) evScroll = evCursor - 5;
		evOtIdx = 0;
		curEv = eventsGet(evFilteredIndices[evCursor]);
		if (!eventAllowsManualShiny(curEv)) evForceShiny = false;
		return;
	}
	if ((ks.down & KEY_ZL) && evFilteredCount > 0)
	{
		evCursor = (evCursor > 6) ? (evCursor - 6) : 0;
		if (evCursor < evScroll) evScroll = evCursor;
		evOtIdx = 0;
		curEv = eventsGet(evFilteredIndices[evCursor]);
		if (!eventAllowsManualShiny(curEv)) evForceShiny = false;
		return;
	}
	if ((ks.down & KEY_ZR) && evFilteredCount > 0)
	{
		int16_t maxIdx = (int16_t)evFilteredCount - 1;
		evCursor = (evCursor + 6 <= maxIdx) ? (evCursor + 6) : maxIdx;
		if (evCursor >= evScroll + 6) evScroll = evCursor - 5;
		evOtIdx = 0;
		curEv = eventsGet(evFilteredIndices[evCursor]);
		if (!eventAllowsManualShiny(curEv)) evForceShiny = false;
		return;
	}
	if (ks.down & KEY_LEFT)
	{
		evDest = (EventInjectDest)((evDest + 2) % 3);
		return;
	}
	if (ks.down & KEY_RIGHT)
	{
		evDest = (EventInjectDest)((evDest + 1) % 3);
		return;
	}
	if (ks.down & KEY_X)
	{
		if (curEv && curEv->ot_count > 1)
			evOtIdx = (evOtIdx + 1) % curEv->ot_count;
		return;
	}
	if (ks.down & KEY_Y)
	{
		if (curEv && eventAllowsManualShiny(curEv))
			evForceShiny = !evForceShiny;
		return;
	}
	if (ks.down & (KEY_A | KEY_START))
	{
		eventsApplyInjection();
		return;
	}

	if (ks.down & KEY_TOUCH)
	{
		hidTouchRead(&ks.touch);
		uint16_t px = ks.touch.px;
		uint16_t py = ks.touch.py;

		// Category Tabs (y=10..32)
		if (py >= 10 && py <= 32)
		{
			EventCategory nextCat = evCategory;
			if (px <= 84) nextCat = EVENT_CAT_GEN1;
			else if (px <= 159) nextCat = EVENT_CAT_GEN2;
			else if (px <= 234) nextCat = EVENT_CAT_PCNY;
			else nextCat = EVENT_CAT_EGGS;

			if (nextCat != evCategory)
			{
				evCategory = nextCat;
				eventsRefreshCategory(true);
			}
			return;
		}

		// Event List (6 rows at y=46..165)
		if (py >= 46 && py <= 165 && px >= 12 && px <= 307 && evFilteredCount > 0)
		{
			int16_t row = (int16_t)(py - 46) / 20;
			int16_t idx = evScroll + row;
			if (idx >= 0 && idx < (int16_t)evFilteredCount)
			{
				evCursor = idx;
				evOtIdx = 0;
				curEv = eventsGet(evFilteredIndices[evCursor]);
				if (!eventAllowsManualShiny(curEv)) evForceShiny = false;
			}
			return;
		}

		// Config Row (y=184..203)
		if (py >= 184 && py <= 203)
		{
			if (px <= 127)
			{
				evDest = (EventInjectDest)((evDest + 1) % 3);
			}
			else if (px <= 223)
			{
				if (curEv && curEv->ot_count > 1)
					evOtIdx = (evOtIdx + 1) % curEv->ot_count;
			}
			else
			{
				if (curEv && eventAllowsManualShiny(curEv))
					evForceShiny = !evForceShiny;
			}
			return;
		}

		// Action Row (y=204..226)
		if (py >= 204 && py <= 226)
		{
			if (px <= 207)
			{
				eventsApplyInjection();
			}
			else
			{
				boxSelectViewBox();
				boxSelectViewPkm();
				currentMode = VIEW_MODE_BOXES;
			}
			return;
		}
	}
}

static void eventsViewerDrawTop(void)
{
	renderDrawRect(48, 40, 304, 160, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawFrame(0, 0);

	if (evFilteredCount == 0 || evCursor >= (int16_t)evFilteredCount) return;
	const HistoricalEvent* ev = eventsGet(evFilteredIndices[evCursor]);
	if (!ev) return;

	bool willBeShiny = isEventShinyEffective(ev, evForceShiny);

	// Top-Left 56x56 Portrait inside clean standalone frame (x=52..123, y=40..111)
	renderDrawRect(60, 48, 56, 56, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	if (ev->is_egg)
	{
		gfxDrawEggSprite(60, 48);
	}
	else
	{
		uint8_t sprVer = (ev->gen == 1 && vgame <= POKEMON_YELLOW) ? vgame :
		                 (vgame >= POKEMON_GOLD ? vgame : POKEMON_CRYSTAL);
		gfxDrawGamePokemonSprite(60, 48, ev->species_id, sprVer);
	}
	gfxDrawPanel(52, 40, 7, 7);

	char line[64];
	char8_t buf[64];

	// Title + Level (y=44)
	snprintf(line, sizeof(line), "%.19s :L%d", ev->title, (int)ev->level);
	convertSafeAscii(buf, line, 25);
	fontDrawString8(126, 44, buf);
	if (willBeShiny)
	{
		gfxDrawShinyStar(334, 43);
	}

	// Badges Row (y=57, h=13)
	u32 darkBord = RENDER_RGBA(0x28, 0x30, 0x40, 0xFF);
	u32 badgeBg = willBeShiny
		? RENDER_RGBA(0xFF, 0xDC, 0x40, 0xFF)
		: RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF);
	drawRetroButtonCentered(126, 57, 66, 13, badgeBg, darkBord, ev->ui_meta.badge, false);
	drawRetroButtonCentered(196, 57, 50, 13, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF), darkBord,
		(ev->gen == 1) ? "GEN 1" : "GEN 2", false);
	if (ev->is_egg && evDest == EVENT_DEST_BANK)
	{
		drawRetroButtonCentered(250, 57, 88, 13, RENDER_RGBA(0xFF, 0xDC, 0xB0, 0xFF), darkBord, "NO BANCO", false);
	}
	else if (ev->gen == 2 && vgame <= POKEMON_YELLOW && evDest != EVENT_DEST_BANK)
	{
		drawRetroButtonCentered(250, 57, 88, 13, RENDER_RGBA(0xFF, 0xDC, 0xB0, 0xFF), darkBord, "SOLO GEN2", false);
	}

	// Region & Year (y=73)
	snprintf(line, sizeof(line), "%s  %s", ev->ui_meta.region_label, ev->ui_meta.year_label);
	convertSafeAscii(buf, line, 27);
	fontDrawString8(126, 73, buf);

	// Method (y=85)
	convertSafeAscii(buf, ev->ui_meta.method_label, 27);
	fontDrawString8(126, 85, buf);

	// OT & IDNo (y=97) - static deterministic preview, never re-roll random TID at 60 FPS
	convertSafeAscii(buf, "OT:", 4);
	fontDrawString8(126, 97, buf);
	if (ev->ot_count == 0)
	{
		fontDrawString8(150, 97, saveGetTrainerName());
	}
	else
	{
		uint8_t oi = (evOtIdx < ev->ot_count) ? evOtIdx : 0;
		convertSafeAscii(buf, ev->ot_options[oi], 10);
		fontDrawString8(150, 97, buf);
	}
	convertSafeAscii(buf, "ID:", 4);
	fontDrawString8(242, 97, buf);
	if (ev->tid_mode == EVENT_TID_PLAYER)
	{
		fontDrawPaddingUInt16(266, 97, saveGetTrainerID());
	}
	else if (ev->tid_mode == EVENT_TID_FIXED)
	{
		fontDrawPaddingUInt16(266, 97, ev->fixed_tid);
	}
	else
	{
		convertSafeAscii(buf, "AZAR", 6);
		fontDrawString8(266, 97, buf);
	}

	// Bottom Details Box (x=52..347, y=112..199, interior x=60..339, y=120..191)
	gfxDrawPanel(52, 112, 35, 9);
	renderDrawRect(186, 121, 1, 54, RENDER_RGBA(0x90, 0x98, 0xA8, 0xFF));
	renderDrawRect(60, 177, 279, 1, RENDER_RGBA(0x90, 0x98, 0xA8, 0xFF));

	// Left: Moves
	convertSafeAscii(buf, "MOVIMIENTOS:", 15);
	fontDrawString8(62, 121, buf);
	for (uint8_t i = 0; i < 4; i++)
	{
		int16_t my = 132 + i * 11;
		if (ev->moves[i] != 0)
		{
			fontDrawString8(62, my, dataMove(ev->moves[i]));
		}
		else
		{
			convertSafeAscii(buf, "-", 2);
			fontDrawString8(62, my, buf);
		}
	}

	// Right: DVs & CR/Item
	convertSafeAscii(buf, "DVS:", 16);
	fontDrawString8(192, 121, buf);
	convertSafeAscii(buf, eventGetDvLabel(ev, evForceShiny), 18);
	fontDrawString8(192, 132, buf);

	convertSafeAscii(buf, (ev->gen == 1) ? "CATCH RATE G1:" : "OBJETO EQUIP.:", 18);
	fontDrawString8(192, 143, buf);
	convertSafeAscii(buf, eventGetItemOrCrLabel(ev), 18);
	fontDrawString8(192, 154, buf);

	if (ev->is_egg)
	{
		convertSafeAscii(buf, "CICLOS HUEVO: 5", 18);
		fontDrawString8(192, 165, buf);
	}

	// Footer Note (y=181)
	convertSafeAscii(buf, ev->ui_meta.home_note, 34);
	fontDrawString8(62, 181, buf);
}

static void eventsViewerDrawBottom(void)
{
	renderDrawRect(0, 0, 320, 240, getGameThemeBgColor());

	u32 btnFill = RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF);
	u32 btnBord = RENDER_RGBA(0x78, 0x82, 0x96, 0xFF);
	u32 darkBord = RENDER_RGBA(0x28, 0x30, 0x40, 0xFF);
	u32 activeFill = RENDER_RGBA(0xFF, 0xEC, 0x99, 0xFF);
	u32 goldFill = RENDER_RGBA(0xFF, 0xDC, 0x40, 0xFF);

	// 1. Category Tabs Panel (y=4..35, inner x=12..307, y=12..27)
	renderDrawRect(4, 4, 312, 32, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(4, 4, 37, 2);

	const char* tabLabels[4] = { "GEN 1", "GEN 2", "PCNY", "HUEVOS" };
	for (uint8_t t = 0; t < 4; t++)
	{
		bool sel = (evCategory == (EventCategory)t);
		drawRetroButtonCentered(12 + t * 75, 13, 71, 14,
			sel ? activeFill : btnFill,
			sel ? darkBord : btnBord,
			tabLabels[t], false);
	}

	// 2. Events List Panel (y=38..173, inner x=12..307, y=46..165, 6 rows x 20px)
	renderDrawRect(4, 38, 312, 136, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(4, 38, 37, 15);

	char8_t buf[32];
	const HistoricalEvent* curEv = (evFilteredCount > 0 && evCursor < (int16_t)evFilteredCount)
		? eventsGet(evFilteredIndices[evCursor]) : NULL;

	for (uint8_t r = 0; r < 6; r++)
	{
		int16_t idx = evScroll + r;
		if (idx >= (int16_t)evFilteredCount) break;
		const HistoricalEvent* ev = eventsGet(evFilteredIndices[idx]);
		if (!ev) continue;

		int16_t ry = 46 + r * 20;
		bool isCur = (idx == evCursor);
		if (isCur)
		{
			renderDrawRect(13, ry, 294, 19, RENDER_RGBA(0xDE, 0xE6, 0xF4, 0xFF));
			fontDrawChar8(15, ry + 5, 0xED); // ►
		}

		GFX_Frame anim = (isCur && ((osGetTime() / 250) % 2)) ? GFX_FRAME_1 : GFX_FRAME_0;
		gfxDrawPokemonIcon(24, ry + 2, ev->is_egg ? (DEX_Species)253 : (DEX_Species)ev->species_id, anim);

		convertSafeAscii(buf, ev->short_name, 18);
		fontDrawString8(46, ry + 6, buf);

		convertSafeAscii(buf, ev->ui_meta.badge, 9);
		int bLen = 0;
		while (bLen < 9 && buf[bLen] != 0x50) bLen++;
		fontDrawString8(302 - bLen * 8, ry + 6, buf);
	}

	// Scrollbar hint if list > 6 items
	if (evFilteredCount > 6)
	{
		renderDrawRect(306, 48, 2, 116, RENDER_RGBA(0xD0, 0xD4, 0xE0, 0xFF));
		int16_t barH = (116 * 6) / (int16_t)evFilteredCount;
		if (barH < 12) barH = 12;
		int16_t maxScr = (int16_t)evFilteredCount - 6;
		int16_t barY = 48 + (evScroll * (116 - barH)) / maxScr;
		renderDrawRect(306, barY, 2, barH, darkBord);
	}

	// 3. Controls & Inject Panel (y=176..231, inner x=12..307, y=184..223)
	renderDrawRect(4, 176, 312, 56, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(4, 176, 37, 5);

	// DEST button (x=12, w=114)
	const char* destLabel = "DEST:CAJA PC";
	if (evDest == EVENT_DEST_PARTY) destLabel = "DEST:EQUIPO";
	else if (evDest == EVENT_DEST_BANK) destLabel = "DEST:BANCO";
	drawRetroButtonCentered(12, 185, 114, 17, RENDER_RGBA(0xEE, 0xF3, 0xFC, 0xFF), darkBord, destLabel, false);

	// OT button (x=130, w=92)
	char otLabel[20];
	if (!curEv || curEv->ot_count == 0)
		snprintf(otLabel, sizeof(otLabel), "OT:JUGADOR");
	else
		snprintf(otLabel, sizeof(otLabel), "OT:%.7s", curEv->ot_options[evOtIdx]);
	bool otEnabled = (curEv && curEv->ot_count > 1);
	drawRetroButtonCentered(130, 185, 92, 17, otEnabled ? btnFill : RENDER_RGBA(0xEA, 0xEC, 0xF2, 0xFF),
		otEnabled ? darkBord : btnBord, otLabel, false);

	// DV button (x=226, w=82)
	const char* dvBtn = "DV:FIJO";
	u32 dvFill = RENDER_RGBA(0xEA, 0xEC, 0xF2, 0xFF);
	u32 dvBord = btnBord;
	if (curEv && eventAllowsManualShiny(curEv))
	{
		if (evForceShiny) dvBtn = "DV:SHINY";
		else if (curEv->dv_mode == EVENT_DV_ODDS_SHINY_GEN2) dvBtn = "DV:PROB";
		else dvBtn = "DV:AZAR";
		dvFill = evForceShiny ? goldFill : btnFill;
		dvBord = darkBord;
	}
	else if (curEv && isEventShinyEffective(curEv, false))
	{
		dvBtn = "DV:SHINY";
		dvFill = activeFill;
	}
	drawRetroButtonCentered(226, 185, 82, 17, dvFill, dvBord, dvBtn, false);

	// Inject & Back Row (y=205, h=18)
	drawRetroButtonCentered(12, 205, 194, 18, goldFill, darkBord, "INYECTAR EVENTO", true);
	drawRetroButtonCentered(210, 205, 98, 18, btnFill, btnBord, "VOLVER", false);
}

void boxViewerUpdate(void)
{
	ks.down = hidKeysDown();
	ks.held = hidKeysHeld();
	ks.up = hidKeysUp();

	if (boxAlertActive)
	{
		if (ks.down & (KEY_A | KEY_B | KEY_START | KEY_X | KEY_Y | KEY_TOUCH))
		{
			boxAlertActive = false;
		}
		return;
	}

	if (currentMode == VIEW_MODE_BAG)
	{
		bagViewerUpdate();
		return;
	}
	if (currentMode == VIEW_MODE_POKEDEX || currentMode == VIEW_MODE_DIPLOMA)
	{
		dexViewerUpdate();
		return;
	}
	if (currentMode == VIEW_MODE_PKM_EDIT)
	{
		pkmEditorUpdate();
		return;
	}
	if (currentMode == VIEW_MODE_EVENTS)
	{
		eventsViewerUpdate();
		return;
	}

	if ((ks.held & (KEY_L | KEY_R)) && (ks.down & KEY_SELECT))
	{
		renderSaveScreenshot("manual_snap");
		showBoxAlert("CAPTURA GUARDADA", "EN SDMC:/PK/SNAPSHOTS");
		return;
	}

	if (ks.down & KEY_X)
	{
		bagCatalogMode = false;
		bagDeleteConfirm = false;
		currentMode = VIEW_MODE_BAG;
		return;
	}
	if (ks.down & KEY_SELECT)
	{
		boxExitRequested = true;
		return;
	}
	if (ks.down & KEY_START)
	{
		if (cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm))
		{
			editorRow = 0;
			editorCol = 0;
			editorPage = 0;
			currentMode = VIEW_MODE_PKM_EDIT;
			return;
		}
	}
	if (ks.down & KEY_Y)
	{
		if (cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm))
		{
			dexCursor = cursor.vPkm->nationalDex;
			if (dexCursor < 1 || dexCursor > 251) dexCursor = 1;
			dexScroll = (dexCursor > 4 ? dexCursor - 4 : 0);
			if (dexScroll > 251 - 7) dexScroll = 251 - 7;
		}
		currentMode = VIEW_MODE_POKEDEX;
		return;
	}

	if (ks.down)
	{
		bool boolMod = false;
		int8_t boxMod = 0;
		int8_t rowMod = 0;
		int8_t colMod = 0;

		if (ks.down & KEY_L) boxMod--;
		else if (ks.down & KEY_R) boxMod++;

		if (ks.down & KEY_UP) rowMod--;
		else if (ks.down & KEY_DOWN) rowMod++;

		if (ks.down & KEY_LEFT) colMod--;
		else if (ks.down & KEY_RIGHT) colMod++;

		if (boxMod || rowMod || colMod)
		{
			cursor.box->box += boxMod;
			cursor.box->row += rowMod;
			cursor.box->col += colMod;

			if (cursor.box->box < 0) cursor.box->box = (cursor.box->inBank ? sbank.boxCount : sgame.boxCount)-1;
			else if (cursor.box->box > (cursor.box->inBank ? sbank.boxCount : sgame.boxCount)-1) cursor.box->box = 0;

			if (cursor.box->col < 0)
			{
				uint8_t row = cursor.box->row;
				cursor.box->col = 0;
				boxSwapBox();
				cursor.box->row = row;
				cursor.box->col = boxGetWidth(cursor.box->row)-1;

				if (cursor.box->row > boxGetHeight()-1) cursor.box->row = boxGetHeight()-1;

			}
			else if (cursor.box->col > boxGetWidth(cursor.box->row)-1)
			{
				uint8_t row = cursor.box->row;
				cursor.box->col = boxGetWidth(cursor.box->row)-1;
				boxSwapBox();
				cursor.box->row = row;
				cursor.box->col = 0;

				if (cursor.box->row > boxGetHeight()-1) cursor.box->row = boxGetHeight()-1;
			}

			if (cursor.box->row < BOX_HEADER_SELECTED) cursor.box->row = boxGetHeight()-1;
			else if (cursor.box->row > boxGetHeight()-1) cursor.box->row = BOX_HEADER_SELECTED;

			boolMod = true;
		}

		if (boolMod)
		{
			computeSlot(cursor.box);

			boxSelectViewBox();

			boxSelectViewPkm();
		}
	}

	if (ks.down)
	{
		if (ks.down & KEY_A && !cursor.isPkmDragged)
		{
			if (batchMode)
			{
				if (cursor.box->inBank == batchSourceInBank && cursor.box->box == batchSourceBox)
				{
					if (cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm))
						batchSelected[cursor.box->slot] = !batchSelected[cursor.box->slot];
				}
				else
				{
					executeBatchMove();
				}
			}
			else
			{
				boxSelectMovePokemon();
			}
		}

		if (ks.down & KEY_B)
		{
			if (batchMode)
			{
				clearBatchSelection();
				showBoxAlert("MODO MULTI CANCELADO", "SELECCION DESMARCADA");
			}
			else
			{
				boxCancelMovePokemon();
			}
		}

		if (ks.down & KEY_ZL)
		{
			cursor.box = &cursor.pc;
			boxSelectViewPkm();
		}

		if (ks.down & KEY_ZR)
		{
			cursor.box = &cursor.bk;
			boxSelectViewPkm();
		}
	}

	if (ks.down & KEY_TOUCH)
	{
		hidTouchRead(&ks.touch);
		uint16_t px = ks.touch.px;
		uint16_t py = ks.touch.py;

		touchHoldActive = false;
		touchHoldCloned = false;
		touchHoldStartTime = 0;
		touchHoldSourcePkm = NULL;

		// Bottom shortcut bar touch handling (Barra B: y=188..235):
		if (touchWithin(px, py, 4, 188, 312, 48))
		{
			if (!batchMode)
			{
				if (py <= 211)
				{
					if (px <= 109)
					{
						bagCatalogMode = false;
						bagDeleteConfirm = false;
						currentMode = VIEW_MODE_BAG;
						return;
					}
					else if (px <= 209)
					{
						batchMode = true;
						batchSourceBox = cursor.box->box;
						batchSourceInBank = cursor.box->inBank;
						memset(batchSelected, 0, sizeof(batchSelected));
						showBoxAlert("MODO MULTI ACTIVO", "TOCA POKEMON PARA MARCARLOS");
						return;
					}
					else
					{
						if (cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm))
						{
							editorRow = 0;
							editorCol = 0;
							editorPage = 0;
							currentMode = VIEW_MODE_PKM_EDIT;
							return;
						}
					}
				}
				else
				{
					if (px <= 199)
					{
						eventsRefreshCategory(false);
						currentMode = VIEW_MODE_EVENTS;
						return;
					}
					else
					{
						if (cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm))
						{
							dexCursor = cursor.vPkm->nationalDex;
							if (dexCursor < 1 || dexCursor > 251) dexCursor = 1;
							dexScroll = (dexCursor > 4 ? dexCursor - 4 : 0);
							if (dexScroll > 251 - 7) dexScroll = 251 - 7;
						}
						currentMode = VIEW_MODE_POKEDEX;
						return;
					}
				}
			}
			else
			{
				if (py <= 211)
				{
					executeBatchMove();
					return;
				}
				else if (px <= 159)
				{
					clearBatchSelection();
					showBoxAlert("MODO MULTI CANCELADO", "SELECCION DESMARCADA");
					return;
				}
				else
				{
					memset(batchSelected, 0, sizeof(batchSelected));
					return;
				}
			}
		}

		int16_t marginLeft = -1;

		if (touchWithin(px, py, 8, 40, 128-1, 144-1))
		{
			marginLeft = 8;
		}
		else if (touchWithin(px, py, 184, 40, 128-1, 144-1))
		{
			marginLeft = 184;
		}

		if (marginLeft > 0)
		{
			bool tappedBank = (marginLeft == 184);
			CursorInbox* tappedBox = tappedBank ? &cursor.bk : &cursor.pc;
			if (batchMode && (tappedBank == batchSourceInBank) && (tappedBox->box == batchSourceBox))
			{
				uint8_t tRow = (py - 40) / 24;
				uint8_t tCol = (px - marginLeft) / 32;
				uint8_t tSlot = tRow * 4 + tCol;
				if (tSlot < tappedBox->vBox->capacity)
				{
					SAV_Pokemon* p = saveGetPkm(batchSourceBox, tSlot, batchSourceInBank);
					if (p && !saveIsPkmEmpty(p))
					{
						batchSelected[tSlot] = !batchSelected[tSlot];
						return;
					}
				}
			}

			cursor.box = (marginLeft == 8 ? &cursor.pc : &cursor.bk);
			uint16_t oldRow = cursor.box->row;
			uint16_t oldCol = cursor.box->col;
			cursor.box->row = (py - 40) / 24;
			cursor.box->col = (px - marginLeft) / 32;

			computeSlot(cursor.box);

			if (cursor.box->slot >= cursor.box->vBox->capacity)
			{
				cursor.box->row = (cursor.box->vBox->capacity-1) / BOX_COL_PKM_COUNT;
				cursor.box->col = (cursor.box->vBox->capacity-1) % BOX_COL_PKM_COUNT;

				boxSelectViewPkm();
			}
			else
			{
				boxSelectViewPkm();

				if (cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm))
				{
					touchHoldActive = true;
					touchHoldStartTime = osGetTime();
					touchHoldSourcePkm = cursor.vPkm;
					touchHoldSourceBank = cursor.box->inBank;
					touchHoldSourceRow = cursor.box->row;
					touchHoldSourceCol = cursor.box->col;
				}

				if (!cursor.sPkm && !cursor.isPkmDragged)
				{
					boxSelectMovePokemon();

					if (cursor.sPkm)
					{
						cursor.isPkmDragged = true;
					}
				}
				else
				{
					if (cursor.sPkm && cursor.box->row == oldRow && cursor.box->col == oldCol)
					{
						boxSelectMovePokemon();
					}
				}
			}
		}
	}
	else if (ks.held & KEY_TOUCH)
	{
		hidTouchRead(&ks.touch);
		uint16_t px = ks.touch.px;
		uint16_t py = ks.touch.py;

		if (touchHoldActive && !touchHoldCloned && touchHoldSourcePkm)
		{
			int16_t origX = (touchHoldSourceBank ? 184 : 8) + touchHoldSourceCol * 32 + 16;
			int16_t origY = 40 + touchHoldSourceRow * 24 + 12;
			if (abs(px - origX) > 20 || abs(py - origY) > 16)
			{
				touchHoldActive = false;
			}
			else if (osGetTime() - touchHoldStartTime >= 1800)
			{
				touchHoldCloned = true;
				cursor.sPkm = touchHoldSourcePkm;
				cursor.isPkmDragged = true;
			}
		}

		if (touchWithin(px, py, 8, 40, 128-1, 144-1))
		{
			cursor.box = &cursor.pc;
		}
		else if (touchWithin(px, py, 184, 40, 128-1, 144-1))
		{
			cursor.box = &cursor.bk;
		}
	}
	else if (ks.up & KEY_TOUCH)
	{
		if (cursor.sPkm && cursor.isPkmDragged)
		{
			uint16_t px = ks.touch.px;
			uint16_t py = ks.touch.py;

			int16_t marginLeft = -1;

			if (touchWithin(px, py, 8, 40, 128-1, 144-1))
			{
				marginLeft = 8;
			}
			else if (touchWithin(px, py, 184, 40, 128-1, 144-1))
			{
				marginLeft = 184;
			}

			if (marginLeft > 0)
			{
				cursor.box = (marginLeft == 8 ? &cursor.pc : &cursor.bk);
				cursor.box->row = (py - 40) / 24;
				cursor.box->col = (px - marginLeft) / 32;

				computeSlot(cursor.box);

				if (cursor.box->slot >= cursor.box->vBox->capacity)
				{
					cursor.box->row = (cursor.box->vBox->capacity-1) / BOX_COL_PKM_COUNT;
					cursor.box->col = (cursor.box->vBox->capacity-1) % BOX_COL_PKM_COUNT;

					boxSelectViewPkm();
				}
				else
				{
					boxSelectViewPkm();

					if (touchHoldCloned && touchHoldSourcePkm)
					{
						if (cursor.box->inBank && touchHoldSourcePkm->isEgg)
						{
							showBoxAlert("ACCION BLOQUEADA", "NO SE GUARDAN HUEVOS EN BANCO");
						}
						else if (!saveCanPlacePokemon(touchHoldSourcePkm, cursor.box->inBank))
						{
							showBoxAlert("OBJETO O ESPECIE INVALIDA", "ACCION NO VALIDA EN DESTINO");
						}
						else if (!cursor.box->inBank && vgame <= POKEMON_YELLOW && touchHoldSourcePkm->heldItem != 0)
						{
							showBoxAlert("OBJETO EQUIPADO EN POKEMON", "ACCION NO VALIDA PARA 1A GEN");
						}
						else if (cursor.vPkm && saveIsPkmEmpty(cursor.vPkm))
						{
							bool pasted = savePastePkm(cursor.vPkm, touchHoldSourcePkm, touchHoldSourceBank, cursor.box->inBank);
							if (pasted)
							{
								if (cursor.box->inBank && !cursor.vPkm->isEgg)
									bankStoreRecordSpecies(cursor.vPkm->nationalDex, vgame);
								if (touchHoldSourceBank && !cursor.box->inBank && !cursor.vPkm->isEgg)
								{
									pokedexAddOwned(cursor.vPkm->nationalDex);
									pokedexAddSeen(cursor.vPkm->nationalDex);
								}
								showBoxAlert("POKEMON CLONADO", "CON EXITO");
							}
						}
						boxCancelMovePokemon();
					}
					else
					{
						boxSelectMovePokemon();
					}
				}
			}
			else
			{
				boxCancelMovePokemon();
				boxSelectViewPkm();
			}

			cursor.isPkmDragged = false;
		}

		touchHoldActive = false;
		touchHoldCloned = false;
		touchHoldStartTime = 0;
		touchHoldSourcePkm = NULL;
	}
}

static void boxViewerDrawTop(void)
{
	renderDrawRect(48, 40, 304, 160, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawFrame(0, 0);

	gfxDrawFolio(224, 112, 14, 10);
	gfxDrawFolio(224, 40, 14, 7);

	if ((cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm)) || cursor.sPkm)
	{
		SAV_Pokemon* pkm = (cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm) ? cursor.vPkm : cursor.sPkm);

		/** Species sprite (56x56 at x=60..115, y=48..103) drawn BEFORE the 7x7 panel border **/
		renderDrawRect(60, 48, 56, 56, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		if (pkm->isEgg)
			gfxDrawEggSprite(52 + 8, 40 + 8);
		else
			gfxDrawGamePokemonSprite(52 + 8, 40 + 8, pkm->nationalDex, vgame);
	}

	/** Separated Portrait (y=40..111) & Stats (y=112..199) Panels - Opcion 1 **/
	gfxDrawPanel(52, 112, 19, 9);
	gfxDrawPanel(52, 40, 7, 7);

	if ((cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm)) || cursor.sPkm)
	{
		SAV_Pokemon* pkm = (cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm) ? cursor.vPkm : cursor.sPkm);

		/** Nickname & Shiny Star (Row 1: y=47) **/
		fontDrawString8(126, 47, pkm->nameNK);
		if (saveIsPkmShiny(pkm))
		{
			gfxDrawShinyStar(210, 46);
		}

		/** Status (Row 2: y=62, inline) **/
		fontDrawString8(126, 62, dataText(TEXT_STATUS));
		if (pkm->currentHP == 0)
		{
			fontDrawString8(182, 62, dataStatus(STATUS_KO));
		}
		else
		{
			uint8_t status;
			switch (pkm->status)
			{
				case STATUS_ASLEEP: status = 2; break;
				case STATUS_POISONED: status = 3; break;
				case STATUS_BURNED: status = 4; break;
				case STATUS_FROZEN: status = 5; break;
				case STATUS_PARALYZED: status = 6; break;
				default: status = 0; break;
			}
			fontDrawString8(182, 62, dataStatus(status));
		}

		/** OTrainer (Row 3: y=77, inline) & IDNo (Row 4: y=92, inline) **/
		fontDrawString8(126, 77, dataText(TEXT_OT));
		fontDrawString8(150, 77, pkm->nameOT);
		fontDrawChar8(126, 92, CHAR8_ID);
		fontDrawChar8(134, 92, CHAR8_NO);
		fontDrawChar8(142, 92, CHAR8_SLASH);
		fontDrawPaddingUInt16(150, 92, pkm->originalTID);

		/** Level & Life Bar (Right Folio Top: x=228..343) **/
		gfxDrawLevel(272, 44, pkm->level);
		gfxDrawLifeBar(248, 54, pkm->currentHP, pkm->maxHP, false);

		/** Types (Inline right after '/', matching Pokedex style) **/
		fontDrawString8(232, 74, dataText(TEXT_TYPE1));
		fontDrawString8(280, 74, dataType(pkm->types[0]));
		if (pkm->types[0] != pkm->types[1])
		{
			fontDrawString8(232, 86, dataText(TEXT_TYPE2));
			fontDrawString8(280, 86, dataType(pkm->types[1]));
		}

		/** Moves (Right Folio Bottom: y=108..189) **/
		fontDrawString8(228, 108, dataText(TEXT_MOVE));
		for (uint8_t i = 0; i < 4; i++)
		{
			if (pkm->moves[i] != 0)
			{
				int16_t my = 119 + i * 19;
				fontDrawString8(228, my, dataMove(pkm->moves[i]));
				fontDrawFromRightUInt32(306, my + 8, pkm->PPs[i]);
				fontDrawChar8(314, my + 8, CHAR8_SLASH);
				fontDrawFromRightUInt32(330, my + 8, PersonalMove(pkm->moves[i])->PP);
			}
		}

		/** Stats Table (Left Panel: x=52..219, inner x=60..211, y=120..191) **/
		fontDrawString8(146, 121, dataText(TEXT_IV));
		fontDrawString8(184, 121, dataText(TEXT_EV));
		renderDrawRect(60, 130, 150, 1, RENDER_RGBA(0x90, 0x98, 0xA8, 0xFF));

		fontDrawString8(60, 133, dataText(TEXT_HP));
		fontDrawString8(60, 145, dataText(TEXT_ATTACK));
		fontDrawString8(60, 157, dataText(TEXT_DEFENSE));
		fontDrawString8(60, 169, dataText(TEXT_SPEED));
		fontDrawString8(60, 181, dataText(TEXT_SPECIAL));

		// Stat Values (horizontally aligned on the same row as each label)
		fontDrawFromRightUInt32(134, 133, pkm->maxHP);
		fontDrawFromRightUInt32(134, 145, pkm->ATK);
		fontDrawFromRightUInt32(134, 157, pkm->DEF);
		fontDrawFromRightUInt32(134, 169, pkm->SPE);
		fontDrawFromRightUInt32(134, 181, pkm->SPC);

		// IVs
		fontDrawFromRightUInt32(160, 133, pkm->IVs[STAT_HP]);
		fontDrawFromRightUInt32(160, 145, pkm->IVs[STAT_ATTACK]);
		fontDrawFromRightUInt32(160, 157, pkm->IVs[STAT_DEFENSE]);
		fontDrawFromRightUInt32(160, 169, pkm->IVs[STAT_SPEED]);
		fontDrawFromRightUInt32(160, 181, pkm->IVs[STAT_SPECIAL]);

		// EVs
		fontDrawFromRightUInt32(202, 133, pkm->EVs[STAT_HP]);
		fontDrawFromRightUInt32(202, 145, pkm->EVs[STAT_ATTACK]);
		fontDrawFromRightUInt32(202, 157, pkm->EVs[STAT_DEFENSE]);
		fontDrawFromRightUInt32(202, 169, pkm->EVs[STAT_SPEED]);
		fontDrawFromRightUInt32(202, 181, pkm->EVs[STAT_SPECIAL]);
	}
}

static void boxViewerDrawBottom(void)
{
	// Characteristic solid background color for the active game!
	renderDrawRect(0, 0, 320, 240, getGameThemeBgColor());

	boxDrawBox(&cursor.pc, 0, 0);
	boxDrawBox(&cursor.bk, 176, 0);

	// Bottom Shortcut Bar - Barra B (2 rows inside x=4..315, y=188..235)
	renderDrawRect(4, 188, 312, 48, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(4, 188, 37, 4);

	u32 btnFill = RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF);
	u32 btnBord = RENDER_RGBA(0x78, 0x82, 0x96, 0xFF);
	u32 darkBord = RENDER_RGBA(0x28, 0x30, 0x40, 0xFF);

	if (!batchMode)
	{
		// Row 1 (y=196..210, h=15): BOLSA | MULTI | EDITAR (clean text, no key labels)
		drawRetroButtonCentered(12, 196, 96, 15, btnFill, btnBord, "BOLSA", false);
		drawRetroButtonCentered(112, 196, 96, 15, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF), darkBord, "MULTI", false);
		drawRetroButtonCentered(212, 196, 96, 15, btnFill, btnBord, "EDITAR", false);

		// Row 2 (y=213..227, h=15): ► REGALO MISTERIOSO | POKEDEX
		drawRetroButtonCentered(12, 213, 186, 15, RENDER_RGBA(0xFF, 0xDC, 0x40, 0xFF), darkBord, "REGALO MISTERIOSO", true);
		drawRetroButtonCentered(202, 213, 106, 15, btnFill, btnBord, "POKEDEX", false);
	}
	else
	{
		char batchStr[36];
		snprintf(batchStr, sizeof(batchStr), "MOVER SELECCION (%d)", (int)getBatchSelectedCount());
		drawRetroButtonCentered(12, 196, 296, 15, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF), darkBord, batchStr, true);

		drawRetroButtonCentered(12, 213, 146, 15, btnFill, btnBord, "CANCELAR (B)", false);
		drawRetroButtonCentered(162, 213, 146, 15, RENDER_RGBA(0xEC, 0xDC, 0xDC, 0xFF), RENDER_RGBA(0x98, 0x78, 0x78, 0xFF), "LIMPIAR (Y)", false);
	}

	if (cursor.sPkm)
	{
		if (cursor.isPkmDragged)
		{
			boxDrawPokemon(cursor.sPkm, ks.touch.px - 8, ks.touch.py - 8);
		}
		else
		{
			boxDrawPokemon(cursor.sPkm, 152, 8);
		}
	}

	// Long Press Cloning visual indicator
	if (touchHoldActive && !touchHoldCloned && touchHoldStartTime > 0)
	{
		uint64_t holdElapsed = osGetTime() - touchHoldStartTime;
		if (holdElapsed > 150)
		{
			int16_t bx = (touchHoldSourceBank ? 176 : 0) + 20 + 32 * touchHoldSourceCol;
			int16_t by = 44 + 24 * touchHoldSourceRow;
			uint16_t prog = (holdElapsed > 1800 ? 24 : (uint16_t)(holdElapsed * 24 / 1800));
			renderDrawRect(bx - 2, by + 18, 24, 4, RENDER_RGBA(0x20, 0x20, 0x20, 0xD0));
			renderDrawRect(bx - 2, by + 18, prog, 4, RENDER_RGBA(0x30, 0xE0, 0x40, 0xFF));
		}
	}
}

static void boxViewerDrawAlert(void)
{
	if (!boxAlertActive) return;

	// Dim background with semi-transparent dark backdrop
	renderDrawRect(0, 0, 320, 240, RENDER_RGBA(0x00, 0x00, 0x00, 0x90));

	// Crisp Game Boy dialog window in white with dark border
	renderDrawRect(20, 72, 280, 72, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(20, 72, 33, 7);

	// Black high-contrast text
	fontDrawString8(32, 88, boxAlert1);
	fontDrawString8(32, 104, boxAlert2);

	char8_t prompt[24];
	fontConvertString(prompt, "A / TACTIL: CONTINUAR");
	fontDrawString8(76, 124, prompt);
}

void boxViewerDraw(void)
{
	renderFrameBegin();
	renderSceneTop();
	if (currentMode == VIEW_MODE_BAG)
		bagViewerDrawTop();
	else if (currentMode == VIEW_MODE_POKEDEX || currentMode == VIEW_MODE_DIPLOMA)
		dexViewerDrawTop();
	else if (currentMode == VIEW_MODE_EVENTS)
		eventsViewerDrawTop();
	else
		boxViewerDrawTop();

	renderSceneBottom();
	if (currentMode == VIEW_MODE_BAG)
		bagViewerDrawBottom();
	else if (currentMode == VIEW_MODE_POKEDEX || currentMode == VIEW_MODE_DIPLOMA)
		dexViewerDrawBottom();
	else if (currentMode == VIEW_MODE_PKM_EDIT)
		pkmEditorDrawBottom();
	else if (currentMode == VIEW_MODE_EVENTS)
		eventsViewerDrawBottom();
	else
		boxViewerDrawBottom();

	boxViewerDrawAlert();
	renderFrameEnd();
}

static void boxDrawBox(CursorInbox* cursorin, int16_t x, int16_t y)
{
	// White interior fill for title panel and 5-row (4x5) Pokémon box panel
	renderDrawRect(x, y, 144, 32, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(x, y, 16, 2);

	renderDrawRect(x, y + 32, 144, 152, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(x, y + 32, 16, 17);

	fontDrawString8(x + 32, y + 12, cursorin->vBox->title);

	for (uint8_t i = 0; i < cursorin->vBox->capacity; i++)
	{
		int16_t px = x + 20 + 32 * (i % 4);
		int16_t py = y + 44 + 24 * (i / 4);

		if (batchMode && (cursorin->inBank == batchSourceInBank) && (cursorin->box == batchSourceBox) && batchSelected[i])
		{
			// Retro GB selection highlight: elegant dark-charcoal frame with soft inner fill
			renderDrawRect(px - 3, py - 3, 24, 24, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
			renderDrawRect(px - 2, py - 2, 22, 22, RENDER_RGBA(0xDE, 0xE4, 0xEE, 0xFF));
		}

		if (&cursorin->vBox->slots[i] != cursor.sPkm)
			boxDrawPokemon(&cursorin->vBox->slots[i], px, py);
	}

	if (!cursor.isPkmDragged && cursorin == cursor.box)
		gfxDrawFullArrow(x + 8 + 32 * cursorin->col, y + 48 + 24 * cursorin->row);
}

static void boxDrawPokemon(SAV_Pokemon* pkm, int16_t x, int16_t y)
{
	if (saveIsPkmEmpty(pkm)) return;

	gfxDrawPokemonIcon(x, y, pkm->isEgg ? (DEX_Species)253 : pkm->nationalDex, (pkm == cursor.vPkm && (osGetTime() / 250) % 2 ? GFX_FRAME_1 : GFX_FRAME_0));

	if (saveIsPkmShiny(pkm))
	{
		gfxDrawShinyStarSmall(x + 20, y + 2);
	}
}

static void boxSelectViewBox(void)
{
	computeSlot(cursor.box);

	cursor.box->vBox = saveGetBox(cursor.box->box, cursor.box->inBank);
}

static void boxSelectViewPkm(void)
{
	computeSlot(cursor.box);

	cursor.vPkm = saveGetPkm(cursor.box->box, cursor.box->slot, cursor.box->inBank);
}

static void boxSelectMovePokemon(void)
{
	computeSlot(cursor.box);

	if (!cursor.sPkm)
	{
		if (!saveIsPkmEmpty(cursor.vPkm))
		{
			cursor.sPkm = cursor.vPkm;
			extractSlot(cursor.box, &cursor.sSlot);
		}
	}
	else if (cursor.vPkm)
	{
		if (cursor.sPkm != cursor.vPkm)
		{
			if (!cursor.box->inBank && vgame <= POKEMON_YELLOW && cursor.sPkm->heldItem != 0)
			{
				showBoxAlert("OBJETO EQUIPADO EN POKEMON", "ACCION NO VALIDA PARA 1A GEN");
				boxCancelMovePokemon();
				return;
			}

			bool moved = saveMovePkm(cursor.vPkm, cursor.sPkm, cursor.sSlot.inBank, cursor.box->inBank);

			if (moved)
			{
				if (cursor.box->inBank && !saveIsPkmEmpty(cursor.vPkm) && !cursor.vPkm->isEgg)
					bankStoreRecordSpecies(cursor.vPkm->nationalDex, vgame);
				if (cursor.sSlot.inBank && !saveIsPkmEmpty(cursor.sPkm) && !cursor.sPkm->isEgg)
					bankStoreRecordSpecies(cursor.sPkm->nationalDex, vgame);

				// Only add to in-game Pokédex when transferring from Bank into Game
				if (cursor.sSlot.inBank && !cursor.box->inBank && !saveIsPkmEmpty(cursor.vPkm) && !cursor.vPkm->isEgg)
				{
					pokedexAddOwned(cursor.vPkm->nationalDex);
					pokedexAddSeen(cursor.vPkm->nationalDex);
				}
				boxCancelMovePokemon();
			}
		}
		else
		{
			boxCancelMovePokemon();
		}
	}
}

static void boxCancelMovePokemon(void)
{
	cursor.sPkm = NULL;
	cursor.isPkmDragged = false;
}

static void boxSwapBox(void)
{
	if (cursor.box == &cursor.pc)
		cursor.box = &cursor.bk;
	else if (cursor.box == &cursor.bk)
		cursor.box = &cursor.pc;
	computeSlot(cursor.box);
}

static uint8_t boxGetHeight(void)
{
	return (cursor.box->vBox->capacity-1) / BOX_COL_PKM_COUNT + 1;
}

static uint8_t boxGetWidth(uint8_t row)
{
	return (row == boxGetHeight()-1 ? (cursor.box->vBox->capacity-1) % BOX_COL_PKM_COUNT + 1 : BOX_COL_PKM_COUNT);
}


static void drawTwoDigit(int16_t x, int16_t y, uint8_t v)
{
	fontDrawChar8(x, y, 0xF6 + ((v / 10) % 10));
	fontDrawChar8(x + 8, y, 0xF6 + (v % 10));
}

static void getEditorElementRect(uint8_t row, uint8_t col, int16_t* rx, int16_t* ry, int16_t* rw, int16_t* rh, const SAV_Pokemon* pkm, bool canHold)
{
	if (editorPage == 1) // Moves Editor Page
	{
		if (row < 4)
		{
			*rx = 14; *ry = 46 + row * 35; *rw = 292; *rh = 31;
		}
		else
		{
			*rx = 16; *ry = 198; *rw = 288; *rh = 24;
		}
		return;
	}

	if (row == 0) // Top Row: Shiny, Nivel, Max DV, Max EV
	{
		*ry = 31; *rh = 16;
		if (col == 0) { *rx = 14; *rw = 78; }
		else if (col == 1) { *rx = 96; *rw = 90; }
		else if (col == 2) { *rx = 190; *rw = 54; }
		else { *rx = 248; *rw = 56; }
	}
	else if (row >= 1 && row <= 4) // Stats: ATAQ, DEFE, VELO, ESPE
	{
		*ry = 60 + (row - 1) * 20; *rh = 18;
		if (col == 0) { *rx = 52; *rw = 68; }
		else { *rx = 182; *rw = 118; }
	}
	else if (row == 5) // PS: EV only
	{
		*ry = 140; *rh = 18;
		*rx = 182; *rw = 118;
	}
	else if (row == 6) // Item / Egg / Trade Evolution
	{
		*ry = 164; *rh = 22;
		if (pkm->isEgg || saveCanTradeEvolve((SAV_Pokemon*)pkm, (vgame < POKEMON_GOLD && !cursor.box->inBank)))
		{
			*rx = 16; *rw = 288;
		}
		else if (canHold)
		{
			if (col == 0) { *rx = 44; *rw = 154; }
			else { *rx = 202; *rw = 102; }
		}
		else
		{
			*rx = 16; *rw = 288;
		}
	}
	else // Row 7: Save & Exit
	{
		*rx = 16; *ry = 198; *rw = 288; *rh = 24;
	}
}

static uint8_t countPkmMoves(const SAV_Pokemon* pkm)
{
	uint8_t cnt = 0;
	for (uint8_t i = 0; i < 4; i++)
		if (pkm->moves[i] != 0) cnt++;
	return cnt;
}

static bool pkmHasMoveOtherSlot(const SAV_Pokemon* pkm, uint8_t moveId, uint8_t skipSlot)
{
	if (moveId == 0) return false;
	for (uint8_t i = 0; i < 4; i++)
	{
		if (i != skipSlot && pkm->moves[i] == moveId) return true;
	}
	return false;
}

static void compactPkmMoves(SAV_Pokemon* pkm)
{
	uint8_t dst = 0;
	for (uint8_t i = 0; i < 4; i++)
	{
		if (pkm->moves[i] != 0)
		{
			if (dst != i)
			{
				pkm->moves[dst] = pkm->moves[i];
				pkm->PPs[dst] = pkm->PPs[i];
				pkm->PPUps[dst] = pkm->PPUps[i];
			}
			dst++;
		}
	}
	for (uint8_t i = dst; i < 4; i++)
	{
		pkm->moves[i] = 0;
		pkm->PPs[i] = 0;
		pkm->PPUps[i] = 0;
	}
}

static uint8_t cycleValidMove(const SAV_Pokemon* pkm, uint8_t slot, int8_t dir, uint8_t maxMove)
{
	uint8_t m = pkm->moves[slot];
	if (m == 0) m = (dir < 0) ? 1 : maxMove;
	for (uint16_t step = 0; step < maxMove; step++)
	{
		if (dir < 0) m = (m > 1) ? (m - 1) : maxMove;
		else m = (m < maxMove) ? (m + 1) : 1;
		if (!pkmHasMoveOtherSlot(pkm, m, slot)) return m;
	}
	return (m == 0) ? 1 : m;
}

static void setMoveAndCompact(SAV_Pokemon* pkm, uint8_t slot, uint8_t newMove)
{
	pkm->moves[slot] = newMove;
	pkm->PPUps[slot] = 0;
	pkm->PPs[slot] = PersonalMove(newMove)->PP;
	compactPkmMoves(pkm);
	for (uint8_t i = 0; i < 4; i++)
	{
		if (pkm->moves[i] == newMove)
		{
			editorMoveSlot = i;
			break;
		}
	}
}

static void deleteMoveAndCompact(SAV_Pokemon* pkm, uint8_t slot)
{
	if (pkm->moves[slot] == 0) return;
	if (countPkmMoves(pkm) <= 1)
	{
		showBoxAlert("ACCION BLOQUEADA", "EL POKEMON DEBE TENER 1 ATAQUE");
		return;
	}
	pkm->moves[slot] = 0;
	pkm->PPs[slot] = 0;
	pkm->PPUps[slot] = 0;
	compactPkmMoves(pkm);
	uint8_t cnt = countPkmMoves(pkm);
	if (editorMoveSlot >= cnt && cnt > 0)
		editorMoveSlot = cnt - 1;
}

static bool isValidEditableHeldItem(uint8_t id)
{
	if (id == 0) return true;
	uint8_t p = items_table[id].pocket;
	return (p == ITEM_POCKET_ITEMS || p == ITEM_POCKET_BALLS);
}

static uint8_t nextValidHeldItem(uint8_t cur)
{
	uint8_t id = cur;
	for (uint16_t i = 0; i < 256; i++)
	{
		id = (id < 255) ? (id + 1) : 0;
		if (isValidEditableHeldItem(id)) return id;
	}
	return 0;
}

static uint8_t prevValidHeldItem(uint8_t cur)
{
	uint8_t id = cur;
	for (uint16_t i = 0; i < 256; i++)
	{
		id = (id > 0) ? (id - 1) : 255;
		if (isValidEditableHeldItem(id)) return id;
	}
	return 0;
}

static void hatchEggInEditor(SAV_Pokemon* pkm)
{
	pkm->isEgg = false;
	pkm->friendship = 120;
	saveSetPkmLevel(pkm, 5);

	const char8_t* spName = dataSpecies(pkm->nationalDex);
	bool endSeen = false;
	for (uint8_t i = 0; i < 11; i++)
	{
		if (endSeen) pkm->nameNK[i] = 0x50;
		else
		{
			pkm->nameNK[i] = spName[i];
			if (spName[i] == 0x50) endSeen = true;
		}
	}
	pkm->nameNK[10] = 0x50;

	if (!cursor.box->inBank)
	{
		pokedexAddOwned(pkm->nationalDex);
		pokedexAddSeen(pkm->nationalDex);
	}
}

static void pkmEditorUpdate(void)
{
	SAV_Pokemon* pkm = cursor.vPkm;
	if (!pkm || saveIsPkmEmpty(pkm))
	{
		currentMode = VIEW_MODE_BOXES;
		return;
	}

	if (ks.down & (KEY_START | KEY_B))
	{
		saveRecalculateStats(pkm);
		currentMode = VIEW_MODE_BOXES;
		return;
	}

	// Page tab switching: L / R
	if (ks.down & KEY_L)
	{
		editorPage = (editorPage == 0) ? 1 : 0;
		return;
	}
	if (ks.down & KEY_R)
	{
		editorPage = (editorPage == 1) ? 0 : 1;
		return;
	}

	bool inGen1 = (vgame < POKEMON_GOLD && !cursor.box->inBank);
	bool canHold = (cursor.box->inBank || vgame >= POKEMON_GOLD);
	uint8_t maxMove = (cursor.box->inBank || vgame >= POKEMON_GOLD) ? 251 : 165;

	// Touch Controls (Tabs Header: y=12..28)
	if (ks.down & KEY_TOUCH)
	{
		hidTouchRead(&ks.touch);
		uint16_t px = ks.touch.px;
		uint16_t py = ks.touch.py;

		if (py >= 10 && py <= 28)
		{
			if (px >= 12 && px <= 157) { editorPage = 0; return; }
			if (px >= 160 && px <= 307) { editorPage = 1; return; }
		}
	}

	if (editorPage == 1) // Moves Editor Page
	{
		if (ks.down & KEY_UP)
		{
			if (editorMoveSlot > 0) editorMoveSlot--;
			else editorMoveSlot = 3;
			return;
		}
		if (ks.down & KEY_DOWN)
		{
			if (editorMoveSlot < 3) editorMoveSlot++;
			else editorMoveSlot = 0;
			return;
		}

		if (ks.down & KEY_LEFT)
		{
			if (pkm->isEgg)
			{
				showBoxAlert("HUEVO PROTEGIDO", "NO SE EDITAN ATAQUES DE HUEVO");
				return;
			}
			setMoveAndCompact(pkm, editorMoveSlot, cycleValidMove(pkm, editorMoveSlot, -1, maxMove));
			return;
		}

		if (ks.down & (KEY_RIGHT | KEY_A))
		{
			if (pkm->isEgg)
			{
				showBoxAlert("HUEVO PROTEGIDO", "NO SE EDITAN ATAQUES DE HUEVO");
				return;
			}
			setMoveAndCompact(pkm, editorMoveSlot, cycleValidMove(pkm, editorMoveSlot, 1, maxMove));
			return;
		}

		if (ks.down & KEY_X)
		{
			if (pkm->isEgg)
			{
				showBoxAlert("HUEVO PROTEGIDO", "NO SE EDITAN ATAQUES DE HUEVO");
				return;
			}
			deleteMoveAndCompact(pkm, editorMoveSlot);
			return;
		}

		if (ks.down & KEY_TOUCH)
		{
			uint16_t px = ks.touch.px;
			uint16_t py = ks.touch.py;

			for (uint8_t s = 0; s < 4; s++)
			{
				int16_t my = 46 + s * 35;
				if (py >= my && py <= my + 31)
				{
					editorMoveSlot = s;
					if (px >= 36 && px <= 58) // Left button (-)
					{
						if (pkm->isEgg)
						{
							showBoxAlert("HUEVO PROTEGIDO", "NO SE EDITAN ATAQUES DE HUEVO");
							return;
						}
						setMoveAndCompact(pkm, s, cycleValidMove(pkm, s, -1, maxMove));
						return;
					}
					if (px >= 166 && px <= 188) // Right button (+)
					{
						if (pkm->isEgg)
						{
							showBoxAlert("HUEVO PROTEGIDO", "NO SE EDITAN ATAQUES DE HUEVO");
							return;
						}
						setMoveAndCompact(pkm, s, cycleValidMove(pkm, s, 1, maxMove));
						return;
					}
					if (px >= 250 && px <= 300) // BOR button
					{
						if (pkm->isEgg)
						{
							showBoxAlert("HUEVO PROTEGIDO", "NO SE EDITAN ATAQUES DE HUEVO");
							return;
						}
						deleteMoveAndCompact(pkm, s);
						return;
					}
					return;
				}
			}

			// Exit button (y=196..226, px=14..306)
			if (py >= 196 && py <= 226 && px >= 14 && px <= 306)
			{
				saveRecalculateStats(pkm);
				currentMode = VIEW_MODE_BOXES;
				return;
			}
		}
		return;
	}

	// Page 0: Stats, Item, Trade Evolution
	static const uint8_t rowStatIdx[4] = { STAT_ATK, STAT_DEF, STAT_SPE, STAT_SPC };

	if (ks.down & KEY_UP)
	{
		if (editorRow > 0) editorRow--;
		if (editorRow == 0 && editorCol > 3) editorCol = 3;
		else if (editorRow >= 1 && editorRow <= 4 && editorCol > 1) editorCol = 1;
		else if (editorRow == 5) editorCol = 0;
		else if (editorRow == 6 && (!canHold || pkm->isEgg)) editorCol = 0;
		else if (editorRow == 6 && editorCol > 1) editorCol = 1;
		else if (editorRow == 7) editorCol = 0;
		return;
	}

	if (ks.down & KEY_DOWN)
	{
		if (editorRow < 7) editorRow++;
		if (editorRow >= 1 && editorRow <= 4 && editorCol > 1) editorCol = 1;
		else if (editorRow == 5) editorCol = 0;
		else if (editorRow == 6 && (!canHold || pkm->isEgg)) editorCol = 0;
		else if (editorRow == 6 && editorCol > 1) editorCol = 1;
		else if (editorRow == 7) editorCol = 0;
		return;
	}

	if (ks.down & KEY_LEFT)
	{
		if (editorRow == 0)
		{
			if (editorCol > 0) editorCol--;
		}
		else if (editorRow >= 1 && editorRow <= 4)
		{
			if (editorCol > 0) editorCol--;
			else
			{
				uint8_t st = rowStatIdx[editorRow - 1];
				if (pkm->IVs[st] > 0) pkm->IVs[st]--;
				saveRecalculateStats(pkm);
			}
		}
		else if (editorRow == 5)
		{
			if (pkm->EVs[STAT_HP] >= 5000) pkm->EVs[STAT_HP] -= 5000;
			else pkm->EVs[STAT_HP] = 0;
			saveRecalculateStats(pkm);
		}
		else if (editorRow == 6)
		{
			if (canHold && !pkm->isEgg)
			{
				if (editorCol > 0) editorCol--;
				else
				{
					pkm->heldItem = prevValidHeldItem(pkm->heldItem);
				}
			}
		}
		return;
	}

	if (ks.down & KEY_RIGHT)
	{
		if (editorRow == 0)
		{
			if (editorCol < 3) editorCol++;
		}
		else if (editorRow >= 1 && editorRow <= 4)
		{
			if (editorCol < 1) editorCol++;
			else
			{
				uint8_t st = rowStatIdx[editorRow - 1];
				if (pkm->EVs[st] <= 65535 - 5000) pkm->EVs[st] += 5000;
				else pkm->EVs[st] = 65535;
				saveRecalculateStats(pkm);
			}
		}
		else if (editorRow == 5)
		{
			if (pkm->EVs[STAT_HP] <= 65535 - 5000) pkm->EVs[STAT_HP] += 5000;
			else pkm->EVs[STAT_HP] = 65535;
			saveRecalculateStats(pkm);
		}
		else if (editorRow == 6)
		{
			if (canHold && !pkm->isEgg)
			{
				if (editorCol < 1) editorCol++;
				else
				{
					pkm->heldItem = nextValidHeldItem(pkm->heldItem);
				}
			}
		}
		return;
	}

	// Action button A
	if (ks.down & KEY_A)
	{
		if (editorRow == 0)
		{
			if (editorCol == 0) saveSetPkmShiny(pkm, !saveIsPkmShiny(pkm));
			else if (editorCol == 1) { if (pkm->level < 100) saveSetPkmLevel(pkm, pkm->level + 1); }
			else if (editorCol == 2)
			{
				pkm->IVs[STAT_ATK] = 15; pkm->IVs[STAT_DEF] = 15; pkm->IVs[STAT_SPE] = 15; pkm->IVs[STAT_SPC] = 15;
				saveRecalculateStats(pkm);
			}
			else if (editorCol == 3)
			{
				for (int i = 0; i < 5; i++) pkm->EVs[i] = 65535;
				saveRecalculateStats(pkm);
			}
		}
		else if (editorRow >= 1 && editorRow <= 4)
		{
			uint8_t st = rowStatIdx[editorRow - 1];
			if (editorCol == 0)
			{
				if (pkm->IVs[st] < 15) pkm->IVs[st]++;
				saveRecalculateStats(pkm);
			}
			else
			{
				if (pkm->EVs[st] <= 65535 - 5000) pkm->EVs[st] += 5000;
				else pkm->EVs[st] = 65535;
				saveRecalculateStats(pkm);
			}
		}
		else if (editorRow == 5)
		{
			if (pkm->EVs[STAT_HP] <= 65535 - 5000) pkm->EVs[STAT_HP] += 5000;
			else pkm->EVs[STAT_HP] = 65535;
			saveRecalculateStats(pkm);
		}
		else if (editorRow == 6)
		{
			if (saveCanTradeEvolve(pkm, inGen1))
			{
				if (saveTradeEvolve(pkm, inGen1))
					showBoxAlert("¡EVOLUCION CON EXITO!", "SIMULADO INTERCAMBIO");
				return;
			}

			if (pkm->isEgg)
			{
				hatchEggInEditor(pkm);
				showBoxAlert("HUEVO ECLOSIONADO", "POKEMON A NIVEL 5");
			}
			else if (canHold)
			{
				if (editorCol == 0)
				{
					pkm->heldItem = nextValidHeldItem(pkm->heldItem);
				}
				else
				{
					pkm->heldItem = 0;
				}
			}
		}
		else if (editorRow == 7)
		{
			saveRecalculateStats(pkm);
			currentMode = VIEW_MODE_BOXES;
			return;
		}
		return;
	}

	if (ks.down & KEY_Y)
	{
		if (editorRow == 0)
		{
			if (editorCol == 0) saveSetPkmShiny(pkm, !saveIsPkmShiny(pkm));
			else if (editorCol == 1) { if (pkm->level > 1) saveSetPkmLevel(pkm, pkm->level - 1); }
		}
		else if (editorRow >= 1 && editorRow <= 4)
		{
			uint8_t st = rowStatIdx[editorRow - 1];
			if (editorCol == 0)
			{
				if (pkm->IVs[st] > 0) pkm->IVs[st]--;
				saveRecalculateStats(pkm);
			}
			else
			{
				if (pkm->EVs[st] >= 5000) pkm->EVs[st] -= 5000;
				else pkm->EVs[st] = 0;
				saveRecalculateStats(pkm);
			}
		}
		else if (editorRow == 5)
		{
			if (pkm->EVs[STAT_HP] >= 5000) pkm->EVs[STAT_HP] -= 5000;
			else pkm->EVs[STAT_HP] = 0;
			saveRecalculateStats(pkm);
		}
		else if (editorRow == 6)
		{
			if (canHold && !pkm->isEgg && editorCol == 0)
			{
				pkm->heldItem = prevValidHeldItem(pkm->heldItem);
			}
		}
		return;
	}

	if (ks.down & KEY_X)
	{
		if (editorRow == 6 && canHold && !pkm->isEgg)
		{
			pkm->heldItem = 0;
		}
		return;
	}

	// Touch Controls (Page 0)
	if (ks.down & KEY_TOUCH)
	{
		uint16_t px = ks.touch.px;
		uint16_t py = ks.touch.py;

		// Top Row: y=30..48
		if (py >= 30 && py <= 48)
		{
			if (px >= 12 && px <= 92)
			{
				editorRow = 0; editorCol = 0;
				saveSetPkmShiny(pkm, !saveIsPkmShiny(pkm));
				return;
			}
			if (px >= 118 && px <= 140)
			{
				editorRow = 0; editorCol = 1;
				if (pkm->level > 1) saveSetPkmLevel(pkm, pkm->level - 1);
				return;
			}
			if (px >= 166 && px <= 188)
			{
				editorRow = 0; editorCol = 1;
				if (pkm->level < 100) saveSetPkmLevel(pkm, pkm->level + 1);
				return;
			}
			if (px >= 190 && px <= 245)
			{
				editorRow = 0; editorCol = 2;
				pkm->IVs[STAT_ATK] = 15; pkm->IVs[STAT_DEF] = 15; pkm->IVs[STAT_SPE] = 15; pkm->IVs[STAT_SPC] = 15;
				saveRecalculateStats(pkm);
				return;
			}
			if (px >= 248 && px <= 305)
			{
				editorRow = 0; editorCol = 3;
				for (int i = 0; i < 5; i++) pkm->EVs[i] = 65535;
				saveRecalculateStats(pkm);
				return;
			}
		}

		// Stat Rows 1..4: ATAQ, DEFE, VELO, ESPE (y=60 + r * 20)
		for (uint8_t r = 0; r < 4; r++)
		{
			int16_t ry = 60 + r * 20;
			if (py >= ry && py <= ry + 18)
			{
				uint8_t st = rowStatIdx[r];
				if (px >= 50 && px <= 76)
				{
					editorRow = r + 1; editorCol = 0;
					if (pkm->IVs[st] > 0) pkm->IVs[st]--;
					saveRecalculateStats(pkm);
					return;
				}
				if (px >= 96 && px <= 122)
				{
					editorRow = r + 1; editorCol = 0;
					if (pkm->IVs[st] < 15) pkm->IVs[st]++;
					saveRecalculateStats(pkm);
					return;
				}
				if (px >= 180 && px <= 240)
				{
					editorRow = r + 1; editorCol = 1;
					if (pkm->EVs[st] >= 5000) pkm->EVs[st] -= 5000;
					else pkm->EVs[st] = 0;
					saveRecalculateStats(pkm);
					return;
				}
				if (px >= 242 && px <= 302)
				{
					editorRow = r + 1; editorCol = 1;
					if (pkm->EVs[st] <= 65535 - 5000) pkm->EVs[st] += 5000;
					else pkm->EVs[st] = 65535;
					saveRecalculateStats(pkm);
					return;
				}
			}
		}

		// Row 5: PS (y=140..158)
		if (py >= 140 && py <= 158)
		{
			if (px >= 180 && px <= 240)
			{
				editorRow = 5; editorCol = 0;
				if (pkm->EVs[STAT_HP] >= 5000) pkm->EVs[STAT_HP] -= 5000;
				else pkm->EVs[STAT_HP] = 0;
				saveRecalculateStats(pkm);
				return;
			}
			if (px >= 242 && px <= 302)
			{
				editorRow = 5; editorCol = 0;
				if (pkm->EVs[STAT_HP] <= 65535 - 5000) pkm->EVs[STAT_HP] += 5000;
				else pkm->EVs[STAT_HP] = 65535;
				saveRecalculateStats(pkm);
				return;
			}
		}

		// Row 6: Item / Egg / Trade Evo (y=162..188)
		if (py >= 162 && py <= 188)
		{
			if (saveCanTradeEvolve(pkm, inGen1))
			{
				editorRow = 6; editorCol = 0;
				if (saveTradeEvolve(pkm, inGen1))
					showBoxAlert("¡EVOLUCION CON EXITO!", "SIMULADO INTERCAMBIO");
				return;
			}

			if (pkm->isEgg)
			{
				editorRow = 6; editorCol = 0;
				hatchEggInEditor(pkm);
				showBoxAlert("HUEVO ECLOSIONADO", "POKEMON A NIVEL 5");
				return;
			}
			else if (canHold)
			{
				if (px >= 44 && px <= 70)
				{
					editorRow = 6; editorCol = 0;
					pkm->heldItem = prevValidHeldItem(pkm->heldItem);
					return;
				}
				if (px >= 174 && px <= 200)
				{
					editorRow = 6; editorCol = 0;
					pkm->heldItem = nextValidHeldItem(pkm->heldItem);
					return;
				}
				if (px >= 204 && px <= 302)
				{
					editorRow = 6; editorCol = 1;
					pkm->heldItem = 0;
					return;
				}
			}
		}

		// Row 7: Save & Exit (y=194..224, px=14..306)
		if (py >= 194 && py <= 224 && px >= 14 && px <= 306)
		{
			editorRow = 7; editorCol = 0;
			saveRecalculateStats(pkm);
			currentMode = VIEW_MODE_BOXES;
			return;
		}
	}
}

static void pkmEditorDrawBottom(void)
{
	SAV_Pokemon* pkm = cursor.vPkm;
	renderDrawRect(0, 0, 320, 240, getGameThemeBgColor());

	// Outer frame: borders at x=4..11, 308..315, y=4..11, 228..235 (usable interior x=12..307, y=12..227)
	renderDrawRect(4, 4, 312, 232, RENDER_RGBA(0xF4, 0xF5, 0xF8, 0xFF));
	gfxDrawPanel(4, 4, 37, 27);

	char8_t buf[36];
	bool inGen1 = (vgame < POKEMON_GOLD && !cursor.box->inBank);
	bool canHold = (cursor.box->inBank || vgame >= POKEMON_GOLD);

	// Tabs Header strictly inside inner area (x=12..307, y=12..28)
	renderDrawRect(12, 27, 296, 1, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
	// Tab 0: [1. STATS / OBJ] (x=12..157, w=146)
	if (editorPage == 0)
	{
		renderDrawRect(12, 12, 146, 16, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		renderDrawRect(12, 12, 146, 1, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
		renderDrawRect(12, 12, 1, 16, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
		renderDrawRect(157, 12, 1, 16, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
		fontDrawChar8(20, 16, 0xED); // ►
		fontConvertString(buf, "1. STATS / OBJ");
		fontDrawString8(32, 16, buf);
	}
	else
	{
		renderDrawRect(12, 13, 146, 14, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
		renderDrawRect(12, 13, 146, 1, RENDER_RGBA(0x90, 0x98, 0xA8, 0xFF));
		renderDrawRect(12, 13, 1, 14, RENDER_RGBA(0x90, 0x98, 0xA8, 0xFF));
		renderDrawRect(157, 13, 1, 14, RENDER_RGBA(0x90, 0x98, 0xA8, 0xFF));
		fontConvertString(buf, "1. STATS / OBJ");
		fontDrawString8(28, 16, buf);
	}

	// Tab 1: [2. ATAQUES] (x=161..307, w=146)
	if (editorPage == 1)
	{
		renderDrawRect(161, 12, 146, 16, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		renderDrawRect(161, 12, 146, 1, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
		renderDrawRect(161, 12, 1, 16, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
		renderDrawRect(306, 12, 1, 16, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
		fontDrawChar8(174, 16, 0xED); // ►
		fontConvertString(buf, "2. ATAQUES");
		fontDrawString8(186, 16, buf);
	}
	else
	{
		renderDrawRect(161, 13, 146, 14, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
		renderDrawRect(161, 13, 146, 1, RENDER_RGBA(0x90, 0x98, 0xA8, 0xFF));
		renderDrawRect(161, 13, 1, 14, RENDER_RGBA(0x90, 0x98, 0xA8, 0xFF));
		renderDrawRect(306, 13, 1, 14, RENDER_RGBA(0x90, 0x98, 0xA8, 0xFF));
		fontConvertString(buf, "2. ATAQUES");
		fontDrawString8(182, 16, buf);
	}

	u32 btnFill = RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF);
	u32 btnBord = RENDER_RGBA(0x78, 0x82, 0x96, 0xFF);

	if (editorPage == 1) // Moves Editor UI
	{
		fontConvertString(buf, "MOVIMIENTO");
		fontDrawString8(60, 33, buf);
		fontConvertString(buf, "PP");
		fontDrawString8(210, 33, buf);

		for (uint8_t s = 0; s < 4; s++)
		{
			int16_t my = 46 + s * 35;
			bool isCur = (editorMoveSlot == s);

			drawRetroButton(14, my, 292, 31,
				isCur ? RENDER_RGBA(0xFF, 0xFF, 0xFF, 0xFF) : RENDER_RGBA(0xF4, 0xF6, 0xFA, 0xFF),
				isCur ? RENDER_RGBA(0x28, 0x30, 0x40, 0xFF) : RENDER_RGBA(0xB8, 0xBE, 0xCC, 0xFF));

			// Cursor arrow if currently selected (x=16..23)
			if (isCur)
			{
				fontDrawChar8(16, my + 11, 0xED); // ►
			}

			// Slot number (single digit at x=26..33)
			fontDrawChar8(26, my + 11, 0xF6 + s + 1);

			// Dec button (-) at x=38..55 (w=18)
			drawRetroButton(38, my + 5, 18, 21, btnFill, btnBord);
			fontConvertString(buf, "-");
			fontDrawString8(43, my + 11, buf);

			// Move Name at x=60..163 (max 13 chars = 104px)
			uint8_t m = pkm->moves[s];
			if (m == 0)
			{
				fontConvertString(buf, "-------");
				fontDrawString8(60, my + 11, buf);
			}
			else
			{
				fontDrawString8(60, my + 11, dataMove(m));
			}

			// Inc button (+) at x=168..185 (w=18)
			drawRetroButton(168, my + 5, 18, 21, btnFill, btnBord);
			fontConvertString(buf, "+");
			fontDrawString8(173, my + 11, buf);

			// PP cleanly separated: current PP at 198..213, '/' at 216..223, max PP at 226..241
			if (m == 0)
			{
				fontConvertString(buf, "0/0");
				fontDrawString8(208, my + 11, buf);
			}
			else
			{
				fontDrawFromRightUInt32(206, my + 11, pkm->PPs[s]);
				fontDrawChar8(216, my + 11, CHAR8_SLASH);
				fontDrawFromRightUInt32(234, my + 11, PersonalMove(m)->PP);
			}

			// Delete button (BOR) at x=252..297 (w=46)
			drawRetroButton(252, my + 5, 46, 21, RENDER_RGBA(0xEC, 0xDC, 0xDC, 0xFF), RENDER_RGBA(0x98, 0x78, 0x78, 0xFF));
			fontConvertString(buf, "BOR");
			fontDrawString8(263, my + 11, buf);
		}

		// Help line
		fontConvertString(buf, "L/R: TAB   DPAD: +/-   X: BORRAR");
		fontDrawString8(32, 186, buf);

		// Save & Exit button strictly inside bottom border (y=198..221 < 228)
		renderDrawRect(16, 198, 288, 24, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
		fontSetWhite(true);
		fontConvertString(buf, "START / B: GUARDAR Y SALIR");
		fontDrawString8(56, 206, buf);
		fontSetWhite(false);
		return;
	}

	// Page 0: Stats / Obj / Evolución
	bool isShiny = saveIsPkmShiny(pkm);

	// Top Row: Shiny, Nivel, Max DV, Max EV (y=31..46, h=16)
	drawRetroButton(14, 31, 78, 16, isShiny ? RENDER_RGBA(0xFF, 0xDC, 0x40, 0xFF) : btnFill, btnBord);
	gfxDrawShinyStarSmall(16, 35);
	fontConvertString(buf, isShiny ? "SHINY:SI" : "SHINY:NO");
	fontDrawString8(26, 35, buf);

	// Nivel (x=96..185, w=90)
	drawRetroButton(96, 31, 90, 16, RENDER_RGBA(0xEC, 0xEF, 0xF6, 0xFF), btnBord);
	fontConvertString(buf, "NV:");
	fontDrawString8(99, 35, buf);
	drawRetroButton(122, 32, 16, 14, btnFill, btnBord);
	fontConvertString(buf, "-");
	fontDrawString8(126, 35, buf);
	fontDrawFromRightUInt32(156, 35, pkm->level);
	drawRetroButton(168, 32, 16, 14, btnFill, btnBord);
	fontConvertString(buf, "+");
	fontDrawString8(172, 35, buf);

	// Max DV (x=190..243, w=54)
	drawRetroButton(190, 31, 54, 16, btnFill, btnBord);
	fontConvertString(buf, "MAX DV");
	fontDrawString8(193, 35, buf);

	// Max EV (x=248..303, w=56)
	drawRetroButton(248, 31, 56, 16, btnFill, btnBord);
	fontConvertString(buf, "MAX EV");
	fontDrawString8(252, 35, buf);

	// Table Header
	fontConvertString(buf, "ESTAD");
	fontDrawString8(14, 50, buf);
	fontConvertString(buf, "DV (0-15)");
	fontDrawString8(60, 50, buf);
	fontConvertString(buf, "EV (0-65535)");
	fontDrawString8(168, 50, buf);
	renderDrawRect(14, 58, 292, 1, RENDER_RGBA(0xB0, 0xB8, 0xC8, 0xFF));

	// Stats Rows (y=60 + r * 20)
	static const char* statNames[4] = { "ATAQ", "DEFE", "VELO", "ESPE" };
	static const uint8_t rowStatIdx[4] = { STAT_ATK, STAT_DEF, STAT_SPE, STAT_SPC };

	for (uint8_t r = 0; r < 4; r++)
	{
		int16_t ry = 60 + r * 20;
		uint8_t st = rowStatIdx[r];

		renderDrawRect(14, ry, 292, 18, (r % 2 == 0) ? RENDER_RGBA(0xEE, 0xF1, 0xF6, 0xFF) : RENDER_RGBA(0xF8, 0xFA, 0xFC, 0xFF));
		fontConvertString(buf, statNames[r]);
		fontDrawString8(16, ry + 5, buf);

		// DV [-] 15 [+]
		drawRetroButton(54, ry + 1, 18, 16, btnFill, btnBord);
		fontConvertString(buf, "-");
		fontDrawString8(59, ry + 5, buf);
		drawTwoDigit(78, ry + 5, pkm->IVs[st]);
		drawRetroButton(100, ry + 1, 18, 16, btnFill, btnBord);
		fontConvertString(buf, "+");
		fontDrawString8(105, ry + 5, buf);

		// EV value [-5k] [+5k]
		fontDrawFromRightUInt32(168, ry + 5, pkm->EVs[st]);
		drawRetroButton(184, ry + 1, 54, 16, btnFill, btnBord);
		fontConvertString(buf, "-5k");
		fontDrawString8(198, ry + 5, buf);
		drawRetroButton(244, ry + 1, 54, 16, btnFill, btnBord);
		fontConvertString(buf, "+5k");
		fontDrawString8(258, ry + 5, buf);
	}

	// PS Row (ry=140)
	renderDrawRect(14, 140, 292, 18, RENDER_RGBA(0xEE, 0xF1, 0xF6, 0xFF));
	fontConvertString(buf, "PS (AUTO:");
	fontDrawString8(16, 145, buf);
	drawTwoDigit(88, 145, pkm->IVs[STAT_HP]);
	fontConvertString(buf, ")");
	fontDrawString8(104, 145, buf);

	fontDrawFromRightUInt32(168, 145, pkm->EVs[STAT_HP]);
	drawRetroButton(184, 141, 54, 16, btnFill, btnBord);
	fontConvertString(buf, "-5k");
	fontDrawString8(198, 145, buf);
	drawRetroButton(244, 141, 54, 16, btnFill, btnBord);
	fontConvertString(buf, "+5k");
	fontDrawString8(258, 145, buf);

	// Row 6: Item / Egg / Trade Evolution (y=164, h=22)
	if (saveCanTradeEvolve(pkm, inGen1))
	{
		drawRetroButton(16, 164, 288, 22, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF), RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
		fontDrawChar8(34, 171, 0xED); // Classic Pokemon triangle arrow ►
		fontConvertString(buf, "EVOLUCIONAR (INTERCAMBIO)");
		fontDrawString8(46, 171, buf);
	}
	else if (pkm->isEgg)
	{
		drawRetroButton(16, 164, 288, 22, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF), RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
		fontDrawChar8(58, 171, 0xED);
		fontConvertString(buf, "ECLOSIONAR HUEVO AHORA");
		fontDrawString8(70, 171, buf);
	}
	else
	{
		drawRetroButton(16, 164, 288, 22, RENDER_RGBA(0xF0, 0xF2, 0xF6, 0xFF), RENDER_RGBA(0xB8, 0xBE, 0xCC, 0xFF));
		if (!canHold)
		{
			fontConvertString(buf, "OBJETO: NO DISPONIBLE EN 1A GEN");
			fontDrawString8(22, 171, buf);
		}
		else
		{
			fontConvertString(buf, "OBJ:");
			fontDrawString8(18, 171, buf);
			drawRetroButton(50, 166, 18, 18, btnFill, btnBord);
			fontConvertString(buf, "-");
			fontDrawString8(55, 171, buf);

			if (pkm->heldItem == 0)
			{
				fontConvertString(buf, "NINGUNO");
				fontDrawString8(74, 171, buf);
			}
			else
			{
				fontDrawString8(74, 171, items_table[pkm->heldItem].name);
			}

			drawRetroButton(176, 166, 18, 18, btnFill, btnBord);
			fontConvertString(buf, "+");
			fontDrawString8(181, 171, buf);

			drawRetroButton(202, 166, 98, 18, btnFill, btnBord);
			fontConvertString(buf, "QUITAR");
			fontDrawString8(227, 171, buf);
		}
	}

	// Row 7: Save & Exit strictly inside bottom border (y=198..221 < 228)
	renderDrawRect(16, 198, 288, 24, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
	fontSetWhite(true);
	fontConvertString(buf, "START / B: GUARDAR Y SALIR");
	fontDrawString8(56, 206, buf);
	fontSetWhite(false);

	// Focus highlight outline in authentic Game Boy dark charcoal
	int16_t fx, fy, fw, fh;
	getEditorElementRect(editorRow, editorCol, &fx, &fy, &fw, &fh, pkm, canHold);
	renderDrawRect(fx - 2, fy - 2, fw + 4, 2, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
	renderDrawRect(fx - 2, fy + fh, fw + 4, 2, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
	renderDrawRect(fx - 2, fy - 2, 2, fh + 4, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
	renderDrawRect(fx + fw, fy - 2, 2, fh + 4, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
}
