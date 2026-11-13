#include "ts.h"
#include "key.h"
#include "gfx.h"
#include "font.h"
#include "save.h"
#include "fs.h"
#include "pkdir.h"
#include "render.h"

#include <stdlib.h>
#include <string.h>

AM_TitleMediaEntry titleEntry;

static u32 titleCount;
static s32 titleCurrent;
static AM_TitleMediaEntry* titleList;
static C3D_Tex* titleTexs;
static C2D_Image* titleIcons;

typedef struct
{
	bool hasSave;
	SAV_GameVersion version;
	SAV_GameLang lang;
	char8_t trainerName[11];
	uint16_t tid;
	uint32_t money;
	uint16_t hours;
	uint8_t minutes;
	uint16_t dexOwned;
	uint8_t badgesKanto;
	uint8_t badgesJohto;
	uint8_t gender;
} TS_SavePreview;

static TS_SavePreview* previews = NULL;
static bool tcShowKantoBadges = false;
static uint8_t* previewBuf = NULL;

static const Tex3DS_SubTexture iconSubtex = {
	48, 48,
	0.0f, 1.0f,
	48.0f / 64.0f, 1.0f - (48.0f / 64.0f)
};

static uint32_t decodeBCD3(const uint8_t* bcd)
{
	uint32_t val = 0;
	for (int i = 0; i < 3; i++)
	{
		val = val * 10 + ((bcd[i] >> 4) & 0x0F);
		val = val * 10 + (bcd[i] & 0x0F);
	}
	return val;
}

static uint16_t countBits(const uint8_t* buf, uint16_t maxBits)
{
	uint16_t cnt = 0;
	for (uint16_t b = 0; b < maxBits; b++)
	{
		if ((buf[b / 8] >> (b % 8)) & 1) cnt++;
	}
	return cnt;
}

static void TS_DrawLoadingFrame(u32 step, u32 total, const char* msg)
{
	char8_t buf[36];
	renderFrameBegin();

	renderSceneTop();
	{
		renderDrawRect(0, 0, 400, 240, RENDER_RGBA(0x18, 0x1C, 0x28, 0xFF));
		renderDrawRect(72, 68, 256, 104, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		gfxDrawPanel(72, 68, 30, 11);

		fontConvertString(buf, "PKBANKVC - CARGANDO");
		fontDrawString8(124, 84, buf);

		fontConvertString(buf, msg);
		fontDrawString8(96, 108, buf);

		int16_t barW = 208;
		int16_t fillW = (total > 0) ? (int16_t)((step * barW) / total) : barW;
		if (fillW > barW) fillW = barW;

		renderDrawRect(94, 130, barW + 4, 16, RENDER_RGBA(0x30, 0x38, 0x48, 0xFF));
		renderDrawRect(96, 132, barW, 12, RENDER_RGBA(0xD0, 0xD8, 0xE0, 0xFF));
		if (fillW > 0)
		{
			renderDrawRect(96, 132, fillW, 12, RENDER_RGBA(0x38, 0xB8, 0x58, 0xFF));
		}

		GFX_Frame animFrame = (step & 1) ? GFX_FRAME_1 : GFX_FRAME_0;
		gfxDrawPokemonIcon(96, 80, SPECIES_PIKACHU, animFrame);
		gfxDrawPokemonIcon(288, 80, SPECIES_PIKACHU, animFrame);
	}

	renderSceneBottom();
	{
		renderDrawRect(0, 0, 320, 240, RENDER_RGBA(0x18, 0x1C, 0x28, 0xFF));
		renderDrawRect(48, 84, 224, 72, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
		gfxDrawPanel(48, 84, 26, 7);

		fontConvertString(buf, "LEYENDO PARTIDAS...");
		fontDrawString8(84, 106, buf);

		if (total > 0)
		{
			fontDrawPaddingUInt16(136, 126, (uint16_t)step);
			fontDrawChar8(160, 126, 0xF3); // /
			fontDrawPaddingUInt16(168, 126, (uint16_t)total);
		}
	}

	renderFrameEnd();
}

static void TS_LoadSinglePreview(u32 idx)
{
	TS_SavePreview* p = &previews[idx];
	memset(p, 0, sizeof(TS_SavePreview));
	p->version = saveGetGameVersion(titleList[idx].titleid);
	p->lang    = saveGetGameLang(titleList[idx].titleid);
	memset(p->trainerName, 0x50, sizeof(p->trainerName));

	if (!previewBuf) return;

	Result ret = FSCIA_InitSaveOnly(titleList[idx].titleid, titleList[idx].mediatype);
	if (R_FAILED(ret)) return;

	u32 bytesRead = 0;
	ret = FS_ReadFile(ROOT_FOLDER SAVEGB_FILE, previewBuf, SAVE_SIZE, saveArchive, &bytesRead);
	FSCIA_Exit();

	if (R_FAILED(ret) || bytesRead < 0x8000) return;

	p->hasSave = true;
	bool isJP = (p->lang == POKEMON_JP);

	if (p->version <= POKEMON_YELLOW)
	{
		memcpy(p->trainerName, previewBuf + 0x2598, isJP ? 6 : 11);
		p->trainerName[10] = 0x50;
		p->dexOwned = countBits(previewBuf + (isJP ? 0x259E : 0x25A3), 151);
		p->money = decodeBCD3(previewBuf + (isJP ? 0x25EE : 0x25F3));
		p->badgesKanto = previewBuf[isJP ? 0x25FD : 0x2602];
		uint16_t tidOff = isJP ? 0x2600 : 0x2605;
		p->tid = (previewBuf[tidOff] << 8) | previewBuf[tidOff + 1];
		uint16_t timeOff = isJP ? 0x2CA0 : 0x2CED;
		p->hours = previewBuf[timeOff];
		p->minutes = previewBuf[timeOff + 2];
	}
	else
	{
		bool isCrystal = (p->version == POKEMON_CRYSTAL);
		p->tid = (previewBuf[0x2009] << 8) | previewBuf[0x200A];
		memcpy(p->trainerName, previewBuf + 0x200B, isJP ? 6 : 11);
		p->trainerName[10] = 0x50;
		p->hours = previewBuf[0x2054];
		p->minutes = previewBuf[0x2055];
		if (isCrystal)
		{
			p->gender = (previewBuf[isJP ? 0x3E4D : 0x3E3D] & 1);
		}
		uint16_t moneyOff = isJP ? (isCrystal ? 0x23CE : 0x23CD) : (isCrystal ? 0x23DC : 0x23DB);
		p->money = decodeBCD3(previewBuf + moneyOff);
		uint16_t badgeOff = isJP ? (isCrystal ? 0x23D7 : 0x23D6) : (isCrystal ? 0x23E5 : 0x23E4);
		p->badgesJohto = previewBuf[badgeOff];
		p->badgesKanto = previewBuf[badgeOff + 1];
		uint16_t dexOff = isJP ? (isCrystal ? 0x29AA : 0x29CE) : (isCrystal ? 0x2A27 : 0x2A4C);
		p->dexOwned = countBits(previewBuf + dexOff, 251);
	}

	if (p->trainerName[0] == 0x00 || p->trainerName[0] == 0xFF || p->trainerName[0] == 0x50)
	{
		p->hasSave = false;
	}
}

static void TS_Select(void)
{
	titleEntry = titleList[titleCurrent];
}

static Result TS_Init(void)
{
	titleCount = 0;
	titleCurrent = 0;
	titleList = NULL;
	titleTexs = NULL;
	titleIcons = NULL;
	previews = NULL;
	previewBuf = (uint8_t*) malloc(SAVE_SIZE);

	TS_DrawLoadingFrame(0, 1, "BUSCANDO JUEGOS...");

	amInit();

	AM_GetPokemonTitleEntryList(&titleList, &titleCount);

	if (titleCount > 0)
	{
		titleTexs  = (C3D_Tex*) calloc(titleCount, sizeof(C3D_Tex));
		titleIcons = (C2D_Image*) calloc(titleCount, sizeof(C2D_Image));
		previews   = (TS_SavePreview*) calloc(titleCount, sizeof(TS_SavePreview));

		for (u32 i = 0; i < titleCount; i++)
		{
			TS_DrawLoadingFrame(i + 1, titleCount, "CARGANDO SAV.DAT...");

			C3D_TexInit(&titleTexs[i], 64, 64, GPU_RGB565);
			C3D_TexSetFilter(&titleTexs[i], GPU_NEAREST, GPU_NEAREST);
			memset(titleTexs[i].data, 0, 64 * 64 * sizeof(u16));

			if (titleList[i].smdh)
			{
				u16* dst = (u16*) titleTexs[i].data;
				u16* src = (u16*) titleList[i].smdh->bigIconData;
				for (u8 j = 0; j < 48; j += 8)
				{
					memcpy(dst, src, 48 * 8 * sizeof(u16));
					src += 48 * 8;
					dst += 64 * 8;
				}
				C3D_TexFlush(&titleTexs[i]);
			}

			titleIcons[i].tex = &titleTexs[i];
			titleIcons[i].subtex = &iconSubtex;

			TS_LoadSinglePreview(i);
		}

		TS_Select();
	}

	if (previewBuf)
	{
		free(previewBuf);
		previewBuf = NULL;
	}

	renderSetClearColor(RENDER_RGBA(0x00, 0x00, 0x00, 0xFF));

	return (titleCount > 0 ? 0 : -1);
}

static void TS_Exit(void)
{
	if (previewBuf)
	{
		free(previewBuf);
		previewBuf = NULL;
	}
	if (previews)
	{
		free(previews);
		previews = NULL;
	}
	if (titleTexs)
	{
		for (u32 i = 0; i < titleCount; i++)
		{
			C3D_TexDelete(&titleTexs[i]);
		}
		free(titleTexs);
		titleTexs = NULL;
	}
	if (titleIcons)
	{
		free(titleIcons);
		titleIcons = NULL;
	}

	AM_FreePokemonTitleEntryList(titleList, titleCount);
	titleList = NULL;

	amExit();
}

static void TS_Prev(void)
{
	tcShowKantoBadges = false;
	titleCurrent--;
	if (titleCurrent < 0)
		titleCurrent = titleCount - 1;
	TS_Select();
}

static void TS_Next(void)
{
	tcShowKantoBadges = false;
	titleCurrent++;
	if (titleCurrent > (s32)titleCount - 1)
		titleCurrent = 0;
	TS_Select();
}

static void TS_DrawTopScreen(void)
{
	const TS_SavePreview* cur = &previews[titleCurrent];
	bool isGBC = (cur->version >= POKEMON_YELLOW);

	// Draw animated Title Screen inside the 160x144 LCD window at (120, 39) first
	gfxDrawAnimatedTitleScreen(120, 39, cur->version);

	// Draw Bezel on top so sliding sprites and graphics stay cleanly masked underneath the frame
	gfxDrawBezel(isGBC);
}

static void TS_DrawBottomScreen(void)
{
	const TS_SavePreview* cur = &previews[titleCurrent];
	char8_t buf[32];

	// Background frame on 320x240 bottom screen
	renderDrawRect(0, 0, 320, 240, RENDER_RGBA(0x18, 0x1C, 0x26, 0xFF));

	// Top Game Icon Carousel Dock on Bottom Screen (y = 2..58)
	renderDrawRect(8, 2, 304, 56, RENDER_RGBA(0x28, 0x2E, 0x3E, 0xFF));
	if (titleCount > 0)
	{
		int32_t maxVisible = (titleCount < 5 ? titleCount : 5);
		int32_t half = maxVisible / 2;

		for (int32_t offset = -half; offset <= half; offset++)
		{
			if (maxVisible % 2 == 0 && offset == half) continue;
			int32_t idx = (titleCurrent + offset + (int32_t)titleCount) % (int32_t)titleCount;
			int16_t ix = 136 + offset * 56;
			int16_t iy = 6;

			if (offset == 0)
			{
				renderDrawRect(ix - 2, iy - 2, 52, 52, RENDER_RGBA(0xF8, 0xD0, 0x30, 0xFF));
			}
			renderDrawImage(titleIcons[idx], ix, iy, 1.0f, 1.0f);
		}
	}

	// Trainer Card centered below dock (x = 80, y = 62, size 160x144)
	int16_t cx = 80;
	int16_t cy = 62;
	renderDrawRect(cx - 4, cy - 2, 168, 148, RENDER_RGBA(0x38, 0x3E, 0x50, 0xFF));
	gfxDrawTrainerCardBg(cx, cy, cur->version, cur->gender);

	// Localized labels according to game language
	const char* lblName   = "NOM./";
	const char* lblMoney  = "DIN./";
	const char* lblTime   = "TIEMPO/";
	const char* lblTimeG2 = "TIEMPO J.";
	const char* lblStatus = "ESTAD.";
	const char* lblBadges = "MEDALLAS";

	if (cur->lang == POKEMON_EN || cur->lang == POKEMON_JP)
	{
		lblName   = "NAME/";
		lblMoney  = "MON./";
		lblTime   = "TIME/";
		lblTimeG2 = "PLAY TIME";
		lblStatus = "STATUS";
		lblBadges = "BADGES";
	}
	else if (cur->lang == POKEMON_FR)
	{
		lblName   = "NOM/";
		lblMoney  = "ARG./";
		lblTime   = "TEMPS/";
		lblTimeG2 = "TEMPS J.";
		lblStatus = "STATUT";
		lblBadges = "BADGES";
	}
	else if (cur->lang == POKEMON_DE)
	{
		lblName   = "NAME/";
		lblMoney  = "GELD/";
		lblTime   = "ZEIT/";
		lblTimeG2 = "SPIELZEIT";
		lblStatus = "STATUS";
		lblBadges = "ORDEN";
	}
	else if (cur->lang == POKEMON_IT)
	{
		lblName   = "NOME/";
		lblMoney  = "SOL./";
		lblTime   = "TEMPO/";
		lblTimeG2 = "TEMPO G.";
		lblStatus = "STATO";
		lblBadges = "MEDAGLIE";
	}

	if (cur->version <= POKEMON_YELLOW)
	{
		fontConvertString(buf, lblName);
		fontDrawString8(cx + 16, cy + 16, buf);
		fontConvertString(buf, lblMoney);
		fontDrawString8(cx + 16, cy + 32, buf);
		fontConvertString(buf, lblTime);
		fontDrawString8(cx + 16, cy + 48, buf);
		fontConvertString(buf, lblBadges);
		fontDrawString8(cx + (cur->lang == POKEMON_EN || cur->lang == POKEMON_JP ? 56 : 48), cy + 72, buf);

		if (!cur->hasSave)
		{
			fontConvertString(buf, "SIN DATOS");
			fontDrawString8(cx + 56, cy + 16, buf);
		}
		else
		{
			if (cur->lang == POKEMON_JP)
				fontDrawJPString8(cx + 56, cy + 16, cur->trainerName);
			else
				fontDrawString8(cx + 56, cy + 16, cur->trainerName);

			fontDrawChar8(cx + 56, cy + 32, 0xF0); // Poké-yen
			fontDrawFromRightUInt32(cx + 104, cy + 32, cur->money);

			fontDrawFromRightUInt32(cx + 88, cy + 48, cur->hours);
			fontDrawChar8(cx + 96, cy + 48, 0x9C); // :
			fontDrawChar8(cx + 104, cy + 48, 0xF6 + (cur->minutes / 10) % 10);
			fontDrawChar8(cx + 112, cy + 48, 0xF6 + (cur->minutes % 10));

			for (uint8_t i = 0; i < 8; i++)
			{
				if ((cur->badgesKanto >> i) & 1)
				{
					int16_t bx = cx + 24 + (i % 4) * 32;
					int16_t by = cy + (i < 4 ? 96 : 120);
					gfxDrawBadge(bx, by, false, i);
				}
			}
		}
	}
	else
	{
		fontConvertString(buf, lblName);
		fontDrawString8(cx + 16, cy + 16, buf);
		fontConvertString(buf, lblMoney);
		fontDrawString8(cx + 16, cy + 48, buf);
		fontConvertString(buf, lblStatus);
		fontDrawString8(cx + 16, cy + 64, buf);
		fontConvertString(buf, lblTimeG2);
		fontDrawString8(cx + 16, cy + 96, buf);

		if (!cur->hasSave)
		{
			fontConvertString(buf, "SIN DATOS");
			fontDrawString8(cx + 56, cy + 16, buf);
		}
		else
		{
			if (cur->lang == POKEMON_JP)
				fontDrawJPString8(cx + 56, cy + 16, cur->trainerName);
			else
				fontDrawString8(cx + 56, cy + 16, cur->trainerName);

			fontDrawPaddingUInt16(cx + 48, cy + 32, cur->tid);

			fontDrawChar8(cx + 56, cy + 48, 0xF0); // Poké-yen
			fontDrawFromRightUInt32(cx + 104, cy + 48, cur->money);

			fontDrawFromRightUInt32(cx + 136, cy + 80, cur->dexOwned);

			fontDrawFromRightUInt32(cx + 112, cy + 96, cur->hours);
			fontDrawChar8(cx + 120, cy + 96, 0x9C); // :
			fontDrawChar8(cx + 128, cy + 96, 0xF6 + (cur->minutes / 10) % 10);
			fontDrawChar8(cx + 136, cy + 96, 0xF6 + (cur->minutes % 10));

			uint8_t curBadges = tcShowKantoBadges ? cur->badgesKanto : cur->badgesJohto;
			bool isJohto = !tcShowKantoBadges;
			for (uint8_t i = 0; i < 8; i++)
			{
				if ((curBadges >> i) & 1)
				{
					gfxDrawBadge(cx + 16 + i * 16, cy + 114, isJohto, i);
				}
			}
		}
	}

	// Bottom footer bar: "A:ABRIR   B:SALIR" properly centered inside a 24px tall panel (y=212..236)
	static const char8_t ftr[] = {
		0x80, 0x9C, 0x80, 0x81, 0x91, 0x88, 0x91, 0x7F, 0x7F, 0x7F,
		0x81, 0x9C, 0x92, 0x80, 0x8B, 0x88, 0x91, 0x50
	};
	renderDrawRect(64, 212, 192, 24, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
	gfxDrawPanel(64, 212, 22, 1);
	fontDrawString8(92, 220, ftr);
}

bool TS_Loop(void)
{
	if (R_FAILED(TS_Init()) || titleCount == 0)
	{
		TS_Exit();
		return false;
	}

	bool tsReturn = false;

	while (aptMainLoop())
	{
		hidScanInput();

		u32 kState = hidKeysDown();

		if (kState & (KEY_UP | KEY_LEFT | KEY_L))
		{
			TS_Prev();
		}
		if (kState & (KEY_DOWN | KEY_RIGHT | KEY_R))
		{
			TS_Next();
		}

		touchPosition touch;
		hidTouchRead(&touch);
		if (kState & KEY_TOUCH)
		{
			if (previews[titleCurrent].version >= POKEMON_GOLD &&
			    touch.px >= 88 && touch.px <= 236 && touch.py >= 166 && touch.py <= 204)
			{
				tcShowKantoBadges = !tcShowKantoBadges;
			}
		}

		if (kState & KEY_A)
		{
			tsReturn = true;
			break;
		}

		if (kState & (KEY_B | KEY_START))
		{
			tsReturn = false;
			break;
		}

		renderFrameBegin();
		renderSceneTop();
			TS_DrawTopScreen();
		renderSceneBottom();
			TS_DrawBottomScreen();
		renderFrameEnd();
	}

	TS_Exit();
	return tsReturn;
}

void TS_DrawNoSaveMessage(void)
{
	char8_t line1[32];
	char8_t line2[32];
	char8_t line3[32];
	memset(line1, 0x50, sizeof(line1));
	memset(line2, 0x50, sizeof(line2));
	memset(line3, 0x50, sizeof(line3));

	fontConvertString(line1, "SIN DATOS DE GUARDADO");
	fontConvertString(line2, "GUARDA EN EL JUEGO");
	fontConvertString(line3, "PULSA A O B");

	renderSetClearColor(RENDER_RGBA(0x28, 0x2A, 0x34, 0xFF));

	while (aptMainLoop())
	{
		hidScanInput();
		if (hidKeysDown() & (KEY_A | KEY_B | KEY_START)) break;

		renderFrameBegin();
		renderSceneTop();
		{
			renderDrawRect(104, 80, 192, 72, RENDER_RGBA(0xF8, 0xF8, 0xF8, 0xFF));
			gfxDrawPanel(104, 80, 22, 7);
			fontDrawString8(116, 96, line1);
			fontDrawString8(124, 112, line2);
			fontDrawString8(140, 132, line3);
		}
		renderSceneBottom();
		renderFrameEnd();
	}
}
