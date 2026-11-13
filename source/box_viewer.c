#include "box_viewer.h"

#include "key.h"
#include "gfx.h"
#include "font.h"
#include "data.h"
#include "save.h"
#include "items.h"
#include "personal.h"
#include "render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
		renderDrawRect(88, 102, 224, 60, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		gfxDrawPanel(88, 102, 26, 5);
		fontConvertString(buf, "ELIMINAR EL OBJETO?");
		fontDrawString8(120, 114, buf);
		fontDrawString8(136, 126, itemGetName(bagDeleteTargetG2, false));
		fontConvertString(buf, "A: SI      B: NO");
		fontDrawString8(132, 142, buf);
		return;
	}

	if (bagCatalogMode)
	{
		renderDrawRect(104, 100, 240, 60, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		gfxDrawPanel(104, 100, 28, 5);
		fontConvertString(buf, "AGREGAR AL ALMACEN:");
		fontDrawString8(120, 110, buf);
		gfxDrawFullArrow(120, 126);
		fontDrawString8(132, 126, itemGetName(bagCatalogItem, false));
		fontDrawChar8(240, 126, 0xF1);
		fontDrawFromRightUInt32(256, 126, bankStore.itemCounts[bagCatalogItem]);
		fontConvertString(buf, "A: +1           B: CERRAR");
		fontDrawString8(120, 142, buf);
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

	// Left Column: Game Pocket (x=0, w=152)
	renderDrawRect(0, 0, 152, 32, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(0, 0, 17, 2);
	renderDrawRect(0, 32, 152, 208, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(0, 32, 17, 24);
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
			gfxDrawFullArrow(6, ry);
		fontDrawString8(14, ry, itemGetName(id, isGen1));
		if (gPocket != ITEM_POCKET_KEY && itemCanStoreInBank(id, isGen1))
		{
			fontDrawChar8(114, ry, 0xF1); // ×
			fontDrawFromRightUInt32(130, ry, qty);
		}
	}

	// Right Column: Bank Store (x=164, w=152)
	renderDrawRect(164, 0, 152, 32, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(164, 0, 17, 2);
	renderDrawRect(164, 32, 152, 208, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(164, 32, 17, 24);
	fontConvertString(buf, "ALMACENAMIENTO");
	fontDrawString8(172, 12, buf);

	uint8_t storeIds[256];
	uint16_t sCount = buildStoreList(storeIds);

	// Row 0 is ALWAYS anchored at top: "X: AGREGAR OBJ."
	if (bagInStore && bagStoreCursor == 0)
		gfxDrawFullArrow(170, 46);
	fontConvertString(buf, "X: AGREGAR OBJ.");
	fontDrawString8(178, 46, buf);

	// Rows 1..7 (up to 7 items visible) scroll smoothly underneath
	for (uint8_t i = 0; i < 7; i++)
	{
		uint16_t itemIdx = bagStoreScroll + i;
		if (itemIdx >= sCount) break;
		int16_t ry = 46 + (i + 1) * 22;
		if (bagInStore && (itemIdx + 1) == bagStoreCursor)
			gfxDrawFullArrow(170, ry);

		uint8_t g2 = storeIds[itemIdx];
		fontDrawString8(178, ry, itemGetName(g2, false));
		fontDrawChar8(278, ry, 0xF1); // ×
		fontDrawFromRightUInt32(294, ry, bankStore.itemCounts[g2]);
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

	// Right Panel (216, 48, 128x144): Detailed Species Info, Types, Progress & Clean Shortcuts
	renderDrawRect(216, 48, 128, 144, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(216, 48, 14, 16);

	static const char8_t unkName[] = { 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x50 };
	static const char8_t unkType[] = { 0xE3, 0xE3, 0xE3, 0x50 };

	if (owned)
	{
		const PersonalInfo* pInfo = Personal(dexCursor);
		fontDrawString8(226, 58, dataSpecies(dexCursor));

		fontConvertString(buf, "REGISTRADO");
		fontDrawString8(226, 72, buf);

		fontDrawString8(226, 88, dataText(TEXT_TYPE1));
		fontDrawString8(274, 88, dataType(pInfo->type1));
		if (pInfo->type2 != pInfo->type1)
		{
			fontDrawString8(226, 100, dataText(TEXT_TYPE2));
			fontDrawString8(274, 100, dataType(pInfo->type2));
		}
	}
	else
	{
		fontDrawString8(226, 58, unkName);

		fontConvertString(buf, "DESCONOCIDO");
		fontDrawString8(226, 72, buf);

		fontDrawString8(226, 88, dataText(TEXT_TYPE1));
		fontDrawString8(274, 88, unkType);
	}

	fontConvertString(buf, "KANTO:");
	fontDrawString8(226, 116, buf);
	fontDrawPaddingUInt8(234, 126, bankStoreCountKanto());
	fontDrawChar8(258, 126, CHAR8_SLASH);
	fontDrawPaddingUInt8(266, 126, 151);

	fontConvertString(buf, "NACIONAL:");
	fontDrawString8(226, 138, buf);
	fontDrawPaddingUInt8(234, 148, totalOwn);
	fontDrawChar8(258, 148, CHAR8_SLASH);
	fontDrawPaddingUInt8(266, 148, 251);

	if (saveIsGamePokedexComplete())
		fontConvertString(buf, "A: DIPLOMA");
	else
		fontConvertString(buf, "DIPLOMA: BLOQ.");
	fontDrawString8(226, 162, buf);
	fontConvertString(buf, "B: VOLVER");
	fontDrawString8(226, 174, buf);
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

	// Bottom Area Name panel
	renderDrawRect(24, 196, 272, 28, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(24, 196, 32, 2);
	fontConvertString(buf, owned ? areaName : "AREA DESCONOCIDA");
	fontDrawString8(40, 206, buf);
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
			boxSelectMovePokemon();
		}

		if (ks.down & KEY_B)
		{
			boxCancelMovePokemon();
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

		// Bottom shortcut bar touch handling:
		if (touchWithin(px, py, 8, 200, 304, 32))
		{
			if (px < 102)
			{
				bagCatalogMode = false;
				bagDeleteConfirm = false;
				currentMode = VIEW_MODE_BAG;
				return;
			}
			else if (px < 216)
			{
				if (cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm))
				{
					editorRow = 0;
					editorCol = 0;
					currentMode = VIEW_MODE_PKM_EDIT;
					return;
				}
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
			uint16_t oldRow = cursor.box->row;
			uint16_t oldCol = cursor.box->col;
			cursor.box->row = (py - 40) / 24;
			cursor.box->col = (px - marginLeft) / 32;

			computeSlot(cursor.box);

			if (cursor.box->slot > cursor.box->vBox->capacity)
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

				if (cursor.box->slot > cursor.box->vBox->capacity)
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
						if (!cursor.box->inBank && vgame <= POKEMON_YELLOW && touchHoldSourcePkm->heldItem != 0)
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

	gfxDrawPanel(48 + 8, 40 + 64, 18, 10);
	gfxDrawFolio(224, 112, 13, 10);
	gfxDrawFolio(224, 40, 13, 8);

	if ((cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm)) || cursor.sPkm)
	{
		SAV_Pokemon* pkm = (cursor.vPkm && !saveIsPkmEmpty(cursor.vPkm) ? cursor.vPkm : cursor.sPkm);

		/** Species panel **/
		gfxDrawPanel(52, 40, 7, 7);
		if (pkm->isEgg)
			gfxDrawEggSprite(52 + 8, 40 + 8);
		else
			gfxDrawGamePokemonSprite(52 + 8, 40 + 8, pkm->nationalDex, vgame);

		fontDrawString8(128, 48, pkm->nameNK);

		if (saveIsPkmShiny(pkm))
		{
			gfxDrawShinyStar(208, 44);
		}

		/** Status **/
		fontDrawString8(128, 60, dataText(TEXT_STATUS));
		if (pkm->currentHP == 0)
		{
			fontDrawString8(184, 60, dataStatus(STATUS_KO));
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

			fontDrawString8(184, 60, dataStatus(status));
		}

		/** OTrainer **/
		fontDrawString8(128, 72, dataText(TEXT_OT));
		fontDrawString8(144, 80, pkm->nameOT);
		fontDrawChar8(128, 88, CHAR8_ID);
		fontDrawChar8(136, 88, CHAR8_NO);
		fontDrawChar8(144, 88, CHAR8_SLASH);
		fontDrawPaddingUInt16(144, 96, pkm->originalTID);

		/** Life Bar **/
		gfxDrawLevel(272, 44, pkm->level);
		gfxDrawLifeBar(248, 54, pkm->currentHP, pkm->maxHP, false);

		/** Types **/
		fontDrawString8(232, 72, dataText(TEXT_TYPE1));
		fontDrawString8(248, 80, dataType(pkm->types[0]));
		if (pkm->types[0] != pkm->types[1])
		{
			fontDrawString8(232, 88, dataText(TEXT_TYPE2));
			fontDrawString8(248, 96, dataType(pkm->types[1]));
		}

		/** Moves **/
		fontDrawString8(224, 112, dataText(TEXT_MOVE));
		for (uint8_t i = 0; i < 4; i++)
		{
			if (pkm->moves[i] != 0)
			{
				fontDrawString8(236, 128+i*16, dataMove(pkm->moves[i]));
				fontDrawFromRightUInt32(304, 136+i*16, pkm->PPs[i]);
				fontDrawChar8(312, 136+i*16, CHAR8_SLASH);
				fontDrawFromRightUInt32(328, 136+i*16, PersonalMove(pkm->moves[i])->PP);
			}
		}

		/** Stats **/
		fontDrawString8(64, 112, dataText(TEXT_HP));
		fontDrawString8(64, 128, dataText(TEXT_ATTACK));
		fontDrawString8(64, 144, dataText(TEXT_DEFENSE));
		fontDrawString8(64, 160, dataText(TEXT_SPEED));
		fontDrawString8(64, 176, dataText(TEXT_SPECIAL));
		fontDrawString8(136, 112, dataText(TEXT_IV));
		fontDrawString8(176, 112, dataText(TEXT_EV));
		// Stats
		fontDrawFromRightUInt32(104+16, 120, pkm->maxHP);
		fontDrawFromRightUInt32(104+16, 136, pkm->ATK);
		fontDrawFromRightUInt32(104+16, 152, pkm->DEF);
		fontDrawFromRightUInt32(104+16, 168, pkm->SPE);
		fontDrawFromRightUInt32(104+16, 184, pkm->SPC);
		// IVs
		fontDrawFromRightUInt32(136+16, 120, pkm->IVs[STAT_HP]);
		fontDrawFromRightUInt32(136+16, 136, pkm->IVs[STAT_ATTACK]);
		fontDrawFromRightUInt32(136+16, 152, pkm->IVs[STAT_DEFENSE]);
		fontDrawFromRightUInt32(136+16, 168, pkm->IVs[STAT_SPEED]);
		fontDrawFromRightUInt32(136+16, 184, pkm->IVs[STAT_SPECIAL]);
		// EVs
		fontDrawFromRightUInt32(160+40, 120, pkm->EVs[STAT_HP]);
		fontDrawFromRightUInt32(160+40, 136, pkm->EVs[STAT_ATTACK]);
		fontDrawFromRightUInt32(160+40, 152, pkm->EVs[STAT_DEFENSE]);
		fontDrawFromRightUInt32(160+40, 168, pkm->EVs[STAT_SPEED]);
		fontDrawFromRightUInt32(160+40, 184, pkm->EVs[STAT_SPECIAL]);
	}
}

static void boxViewerDrawBottom(void)
{
	// Characteristic solid background color for the active game!
	renderDrawRect(0, 0, 320, 240, getGameThemeBgColor());

	boxDrawBox(&cursor.pc, 0, 0);
	boxDrawBox(&cursor.bk, 176, 0);

	// Bottom Shortcut Bar below both 5x4 boxes: 3 crisp touchable buttons
	char8_t buf[32];
	renderDrawRect(8, 200, 304, 32, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(8, 200, 36, 2);

	renderDrawRect(102, 204, 2, 24, RENDER_RGBA(0xC0, 0xC4, 0xD0, 0xFF));
	renderDrawRect(216, 204, 2, 24, RENDER_RGBA(0xC0, 0xC4, 0xD0, 0xFF));

	fontConvertString(buf, "X: MOCHILA");
	fontDrawString8(16, 212, buf);

	fontConvertString(buf, "START: EDITAR");
	fontDrawString8(108, 212, buf);

	fontConvertString(buf, "Y: POKEDEX");
	fontDrawString8(224, 212, buf);

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
	else
		boxViewerDrawTop();

	renderSceneBottom();
	if (currentMode == VIEW_MODE_BAG)
		bagViewerDrawBottom();
	else if (currentMode == VIEW_MODE_POKEDEX || currentMode == VIEW_MODE_DIPLOMA)
		dexViewerDrawBottom();
	else if (currentMode == VIEW_MODE_PKM_EDIT)
		pkmEditorDrawBottom();
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
		if (&cursorin->vBox->slots[i] != cursor.sPkm)
			boxDrawPokemon(&cursorin->vBox->slots[i], x + 20 + 32 * (i % 4), y + 44 + 24 * (i / 4));
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
	if (row == 0) // Top Row: Shiny, Nivel, Max DV, Max EV
	{
		*ry = 12; *rh = 20;
		if (col == 0) { *rx = 12; *rw = 84; }
		else if (col == 1) { *rx = 98; *rw = 96; }
		else if (col == 2) { *rx = 198; *rw = 52; }
		else { *rx = 254; *rw = 52; }
	}
	else if (row >= 1 && row <= 4) // Stats: ATAQ, DEFE, VELO, ESPE
	{
		*ry = 48 + (row - 1) * 23; *rh = 21;
		if (col == 0) { *rx = 52; *rw = 68; }
		else { *rx = 182; *rw = 118; }
	}
	else if (row == 5) // PS: EV only
	{
		*ry = 140; *rh = 21;
		*rx = 182; *rw = 118;
	}
	else if (row == 6) // Item / Egg
	{
		*ry = 166; *rh = 25;
		if (pkm->isEgg)
		{
			*rx = 14; *rw = 292;
		}
		else if (canHold)
		{
			if (col == 0) { *rx = 44; *rw = 154; }
			else { *rx = 202; *rw = 102; }
		}
		else
		{
			*rx = 12; *rw = 294;
		}
	}
	else // Row 7: Save & Exit
	{
		*rx = 14; *ry = 196; *rw = 292; *rh = 28;
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

	static const uint8_t rowStatIdx[4] = { STAT_ATK, STAT_DEF, STAT_SPE, STAT_SPC };
	bool canHold = (cursor.box->inBank || vgame >= POKEMON_GOLD);

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
					if (pkm->heldItem > 0) pkm->heldItem--;
					else pkm->heldItem = 255;
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
					if (pkm->heldItem < 255) pkm->heldItem++;
					else pkm->heldItem = 0;
				}
			}
		}
		return;
	}

	// Action buttons: A / R (Increment / Execute), Y / L (Decrement)
	if (ks.down & (KEY_A | KEY_R))
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
			if (pkm->isEgg)
			{
				pkm->isEgg = false;
				saveSetPkmLevel(pkm, 5);
				showBoxAlert("HUEVO ECLOSIONADO", "POKEMON A NIVEL 5");
			}
			else if (canHold)
			{
				if (editorCol == 0)
				{
					if (pkm->heldItem < 255) pkm->heldItem++;
					else pkm->heldItem = 0;
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

	if (ks.down & (KEY_Y | KEY_L))
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
				if (pkm->heldItem > 0) pkm->heldItem--;
				else pkm->heldItem = 255;
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

	// Touch Controls
	if (ks.down & KEY_TOUCH)
	{
		hidTouchRead(&ks.touch);
		uint16_t px = ks.touch.px;
		uint16_t py = ks.touch.py;

		// Top Row: y=10..34
		if (py >= 10 && py <= 34)
		{
			if (px >= 12 && px <= 96)
			{
				editorRow = 0; editorCol = 0;
				saveSetPkmShiny(pkm, !saveIsPkmShiny(pkm));
				return;
			}
			if (px >= 120 && px <= 150)
			{
				editorRow = 0; editorCol = 1;
				if (pkm->level > 1) saveSetPkmLevel(pkm, pkm->level - 1);
				return;
			}
			if (px >= 168 && px <= 198)
			{
				editorRow = 0; editorCol = 1;
				if (pkm->level < 100) saveSetPkmLevel(pkm, pkm->level + 1);
				return;
			}
			if (px >= 200 && px <= 250)
			{
				editorRow = 0; editorCol = 2;
				pkm->IVs[STAT_ATK] = 15; pkm->IVs[STAT_DEF] = 15; pkm->IVs[STAT_SPE] = 15; pkm->IVs[STAT_SPC] = 15;
				saveRecalculateStats(pkm);
				return;
			}
			if (px >= 254 && px <= 308)
			{
				editorRow = 0; editorCol = 3;
				for (int i = 0; i < 5; i++) pkm->EVs[i] = 65535;
				saveRecalculateStats(pkm);
				return;
			}
		}

	// Stat Rows 1..4: ATAQ, DEFE, VELO, ESPE
		for (uint8_t r = 0; r < 4; r++)
		{
			int16_t ry = 48 + r * 23;
			if (py >= ry && py <= ry + 21)
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

	// Row 5: PS (y=140..161)
	if (py >= 140 && py <= 161)
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

	// Row 6: Item / Egg (y=165..194)
	if (py >= 165 && py <= 194)
	{
		if (pkm->isEgg)
		{
			editorRow = 6; editorCol = 0;
			pkm->isEgg = false;
			saveSetPkmLevel(pkm, 5);
			showBoxAlert("HUEVO ECLOSIONADO", "POKEMON A NIVEL 5");
			return;
		}
		else if (canHold)
		{
			if (px >= 44 && px <= 70)
			{
				editorRow = 6; editorCol = 0;
				if (pkm->heldItem > 0) pkm->heldItem--;
				else pkm->heldItem = 255;
				return;
			}
			if (px >= 174 && px <= 200)
			{
				editorRow = 6; editorCol = 0;
				if (pkm->heldItem < 255) pkm->heldItem++;
				else pkm->heldItem = 0;
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

	// Row 7: Save & Exit (y=196..228, px=14..306)
	if (py >= 196 && py <= 228 && px >= 14 && px <= 306)
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

	// Outer frame: single outer panel with authentic Game Boy border
	renderDrawRect(4, 4, 312, 232, RENDER_RGBA(0xF4, 0xF5, 0xF8, 0xFF));
	gfxDrawPanel(4, 4, 37, 27);

	char8_t buf[36];
	bool isShiny = saveIsPkmShiny(pkm);
	bool canHold = (cursor.box->inBank || vgame >= POKEMON_GOLD);

	// Top Row: Shiny, Nivel, Max DV, Max EV
	// Shiny button
	renderDrawRect(12, 12, 84, 20, isShiny ? RENDER_RGBA(0xFF, 0xDC, 0x40, 0xFF) : RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
	gfxDrawShinyStarSmall(15, 18);
	fontConvertString(buf, isShiny ? "SHINY:SI" : "SHINY:NO");
	fontDrawString8(26, 18, buf);

	// Nivel
	renderDrawRect(98, 12, 96, 20, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
	fontConvertString(buf, "NV:");
	fontDrawString8(102, 18, buf);
	renderDrawRect(126, 14, 18, 16, RENDER_RGBA(0xC8, 0xD0, 0xDE, 0xFF));
	fontConvertString(buf, "-");
	fontDrawString8(131, 18, buf);
	fontDrawFromRightUInt32(164, 18, pkm->level);
	renderDrawRect(172, 14, 18, 16, RENDER_RGBA(0xC8, 0xD0, 0xDE, 0xFF));
	fontConvertString(buf, "+");
	fontDrawString8(177, 18, buf);

	// Max DV
	renderDrawRect(198, 12, 52, 20, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
	fontConvertString(buf, "MAX DV");
	fontDrawString8(200, 18, buf);

	// Max EV
	renderDrawRect(254, 12, 52, 20, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
	fontConvertString(buf, "MAX EV");
	fontDrawString8(256, 18, buf);

	// Table Header
	fontConvertString(buf, "ESTAD");
	fontDrawString8(14, 35, buf);
	fontConvertString(buf, "DV (0-15)");
	fontDrawString8(60, 35, buf);
	fontConvertString(buf, "EV (0-65535)");
	fontDrawString8(168, 35, buf);
	renderDrawRect(12, 45, 294, 1, RENDER_RGBA(0xB0, 0xB8, 0xC8, 0xFF));

	// Stats Rows
	static const char* statNames[4] = { "ATAQ", "DEFE", "VELO", "ESPE" };
	static const uint8_t rowStatIdx[4] = { STAT_ATK, STAT_DEF, STAT_SPE, STAT_SPC };

	for (uint8_t r = 0; r < 4; r++)
	{
		int16_t ry = 48 + r * 23;
		uint8_t st = rowStatIdx[r];

		renderDrawRect(12, ry, 294, 21, (r % 2 == 0) ? RENDER_RGBA(0xEE, 0xF1, 0xF6, 0xFF) : RENDER_RGBA(0xF8, 0xFA, 0xFC, 0xFF));
		fontConvertString(buf, statNames[r]);
		fontDrawString8(14, ry + 6, buf);

		// DV [-] 15 [+]
		renderDrawRect(54, ry + 2, 18, 17, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
		fontConvertString(buf, "-");
		fontDrawString8(59, ry + 6, buf);
		drawTwoDigit(78, ry + 6, pkm->IVs[st]);
		renderDrawRect(100, ry + 2, 18, 17, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
		fontConvertString(buf, "+");
		fontDrawString8(105, ry + 6, buf);

		// EV value [-5k] [+5k]
		fontDrawFromRightUInt32(168, ry + 6, pkm->EVs[st]);
		renderDrawRect(184, ry + 2, 54, 17, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
		fontConvertString(buf, "-5k");
		fontDrawString8(198, ry + 6, buf);
		renderDrawRect(244, ry + 2, 54, 17, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
		fontConvertString(buf, "+5k");
		fontDrawString8(258, ry + 6, buf);
	}

	// PS Row (ry=140)
	renderDrawRect(12, 140, 294, 21, RENDER_RGBA(0xEE, 0xF1, 0xF6, 0xFF));
	fontConvertString(buf, "PS (AUTO:");
	fontDrawString8(14, 146, buf);
	drawTwoDigit(86, 146, pkm->IVs[STAT_HP]);
	fontConvertString(buf, ")");
	fontDrawString8(102, 146, buf);

	fontDrawFromRightUInt32(168, 146, pkm->EVs[STAT_HP]);
	renderDrawRect(184, 142, 54, 17, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
	fontConvertString(buf, "-5k");
	fontDrawString8(198, 146, buf);
	renderDrawRect(244, 142, 54, 17, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
	fontConvertString(buf, "+5k");
	fontDrawString8(258, 146, buf);

	// Row 6: Item / Egg
	if (pkm->isEgg)
	{
		renderDrawRect(14, 166, 292, 25, RENDER_RGBA(0x38, 0x98, 0x48, 0xFF));
		fontSetWhite(true);
		fontConvertString(buf, "ECLOSIONAR HUEVO AHORA");
		fontDrawString8(72, 175, buf);
		fontSetWhite(false);
	}
	else
	{
		renderDrawRect(12, 166, 294, 25, RENDER_RGBA(0xF0, 0xF2, 0xF6, 0xFF));
		if (!canHold)
		{
			fontConvertString(buf, "OBJETO: NO DISPONIBLE EN 1A GEN");
			fontDrawString8(16, 175, buf);
		}
		else
		{
			fontConvertString(buf, "OBJ:");
			fontDrawString8(14, 175, buf);
			renderDrawRect(46, 168, 20, 21, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
			fontConvertString(buf, "<");
			fontDrawString8(52, 175, buf);

			if (pkm->heldItem == 0)
			{
				fontConvertString(buf, "NINGUNO");
				fontDrawString8(72, 175, buf);
			}
			else
			{
				fontDrawString8(72, 175, items_table[pkm->heldItem].name);
			}

			renderDrawRect(176, 168, 20, 21, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
			fontConvertString(buf, ">");
			fontDrawString8(182, 175, buf);

			renderDrawRect(204, 168, 98, 21, RENDER_RGBA(0xDE, 0xE2, 0xEC, 0xFF));
			fontConvertString(buf, "QUITAR");
			fontDrawString8(230, 175, buf);
		}
	}

	// Row 7: Save & Exit
	renderDrawRect(14, 196, 292, 28, RENDER_RGBA(0x28, 0x30, 0x40, 0xFF));
	fontSetWhite(true);
	fontConvertString(buf, "START / B: GUARDAR Y SALIR");
	fontDrawString8(56, 206, buf);
	fontSetWhite(false);

	// Focus highlight outline
	int16_t fx, fy, fw, fh;
	getEditorElementRect(editorRow, editorCol, &fx, &fy, &fw, &fh, pkm, canHold);
	renderDrawRect(fx - 2, fy - 2, fw + 4, 2, RENDER_RGBA(0x00, 0x78, 0xD7, 0xFF));
	renderDrawRect(fx - 2, fy + fh, fw + 4, 2, RENDER_RGBA(0x00, 0x78, 0xD7, 0xFF));
	renderDrawRect(fx - 2, fy - 2, 2, fh + 4, RENDER_RGBA(0x00, 0x78, 0xD7, 0xFF));
	renderDrawRect(fx + fw, fy - 2, 2, fh + 4, RENDER_RGBA(0x00, 0x78, 0xD7, 0xFF));
}
