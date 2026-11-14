#include "gfx.h"
#include "font.h"
#include "save.h"
#include "render.h"

#include "gb_sprites.h"
#include "gb_extra.h"
#include "gb_pkm_anim1.h"
#include "gb_pkm_anim2.h"
#include "gb_front_rb.h"
#include "gb_front_yellow.h"
#include "gb_front_gs.h"

#define PKM_PER_ROW (8)
#define SPRITES_PATH "romfs:/gfx/gb_sprites.t3x"
#define EXTRA_PATH   "romfs:/gfx/gb_extra.t3x"
#define ANIM1_PATH   "romfs:/gfx/gb_pkm_anim1.t3x"
#define ANIM2_PATH   "romfs:/gfx/gb_pkm_anim2.t3x"
#define FRONT_RB_PATH     "romfs:/gfx/gb_front_rb.t3x"
#define FRONT_YELLOW_PATH "romfs:/gfx/gb_front_yellow.t3x"
#define FRONT_GS_PATH     "romfs:/gfx/gb_front_gs.t3x"

static C2D_SpriteSheet spriteSheet;
static C2D_SpriteSheet extraSheet;
static C2D_SpriteSheet animSheet1;
static C2D_SpriteSheet animSheet2;
static C2D_SpriteSheet frontRbSheet;
static C2D_SpriteSheet frontYellowSheet;
static C2D_SpriteSheet frontGsSheet;

static C2D_Image pkmIconsGBC;
static C2D_Image pkmAnim1;
static C2D_Image pkmAnim2;
static C2D_Image frontRb;
static C2D_Image frontYellow;
static C2D_Image frontGs;
static C2D_Image topFrame;

static C2D_Image bezelGB;
static C2D_Image bezelGBC;
static C2D_Image titleScreens[8];
static C2D_Image titleSprites;
static C2D_Image titleRbPkm;
static C2D_Image tcRb;
static C2D_Image tcYellow;
static C2D_Image tcGs;
static C2D_Image tcCrystalBoy;
static C2D_Image tcCrystalGirl;
static C2D_Image tcBadges;
static C2D_Image bagUi;
static C2D_Image dexBg;
static C2D_Image mapJohto;
static C2D_Image mapKanto;
static C2D_Image diplomaGen1;
static C2D_Image diplomaGen2;
static C2D_Image imgShinyStar;
static C2D_Image imgShinyStarSmall;
static C2D_Image imgEggFront;

Result gfxLoad(void)
{
	spriteSheet = renderLoadSheet(SPRITES_PATH);
	if (!spriteSheet) return -5;

	extraSheet = renderLoadSheet(EXTRA_PATH);
	if (!extraSheet) return -5;

	animSheet1 = renderLoadSheet(ANIM1_PATH);
	if (!animSheet1) return -5;

	animSheet2 = renderLoadSheet(ANIM2_PATH);
	if (!animSheet2) return -5;

	frontRbSheet = renderLoadSheet(FRONT_RB_PATH);
	frontYellowSheet = renderLoadSheet(FRONT_YELLOW_PATH);
	frontGsSheet = renderLoadSheet(FRONT_GS_PATH);

	pkmIconsGBC = renderSheetImage(spriteSheet, gb_sprites_pokemon_icons_gbc_idx);
	pkmAnim1    = renderSheetImage(animSheet1, gb_pkm_anim1_pkm_anim_1_idx);
	pkmAnim2    = renderSheetImage(animSheet2, gb_pkm_anim2_pkm_anim_2_idx);
	if (frontRbSheet) frontRb = renderSheetImage(frontRbSheet, gb_front_rb_front_rb_idx);
	if (frontYellowSheet) frontYellow = renderSheetImage(frontYellowSheet, gb_front_yellow_front_yellow_idx);
	if (frontGsSheet) frontGs = renderSheetImage(frontGsSheet, gb_front_gs_front_gs_idx);

	bezelGB  = renderSheetImage(extraSheet, gb_extra_bezel_gb_idx);
	bezelGBC = renderSheetImage(extraSheet, gb_extra_bezel_gbc_idx);

	titleScreens[POKEMON_RED]     = renderSheetImage(extraSheet, gb_extra_title_red_idx);
	titleScreens[POKEMON_GREEN]   = renderSheetImage(extraSheet, gb_extra_title_green_idx);
	titleScreens[POKEMON_BLUE]    = renderSheetImage(extraSheet, gb_extra_title_blue_idx);
	titleScreens[POKEMON_YELLOW]  = renderSheetImage(extraSheet, gb_extra_title_yellow_idx);
	titleScreens[POKEMON_GOLD]    = renderSheetImage(extraSheet, gb_extra_title_gold_idx);
	titleScreens[POKEMON_SILVER]  = renderSheetImage(extraSheet, gb_extra_title_silver_idx);
	titleScreens[POKEMON_CRYSTAL] = renderSheetImage(extraSheet, gb_extra_title_crystal_idx);

	titleSprites  = renderSheetImage(extraSheet, gb_extra_title_sprites_idx);
	titleRbPkm    = renderSheetImage(extraSheet, gb_extra_title_rb_pkm_idx);
	tcRb          = renderSheetImage(extraSheet, gb_extra_tc_rb_idx);
	tcYellow      = renderSheetImage(extraSheet, gb_extra_tc_yellow_idx);
	tcGs          = renderSheetImage(extraSheet, gb_extra_tc_gs_idx);
	tcCrystalBoy  = renderSheetImage(extraSheet, gb_extra_tc_crystal_boy_idx);
	tcCrystalGirl = renderSheetImage(extraSheet, gb_extra_tc_crystal_girl_idx);
	tcBadges      = renderSheetImage(extraSheet, gb_extra_tc_badges_idx);
	bagUi         = renderSheetImage(extraSheet, gb_extra_bag_ui_idx);
	dexBg         = renderSheetImage(extraSheet, gb_extra_dex_bg_idx);
	mapJohto      = renderSheetImage(extraSheet, gb_extra_map_johto_idx);
	mapKanto      = renderSheetImage(extraSheet, gb_extra_map_kanto_idx);
	diplomaGen1   = renderSheetImage(extraSheet, gb_extra_diploma_gen1_idx);
	diplomaGen2   = renderSheetImage(extraSheet, gb_extra_diploma_gen2_idx);
	imgShinyStar      = renderSheetImage(extraSheet, gb_extra_shiny_star_idx);
	imgShinyStarSmall = renderSheetImage(extraSheet, gb_extra_shiny_star_small_idx);
	imgEggFront       = renderSheetImage(extraSheet, gb_extra_egg_front_idx);

	return (pkmIconsGBC.tex && bezelGB.tex && pkmAnim1.tex ? 0 : -5);
}

Result gfxLoadFrame(uint64_t titleid)
{
	topFrame = (C2D_Image) { NULL, NULL };

	switch (saveGetGameVersion(titleid))
	{
		case POKEMON_RED: topFrame = renderSheetImage(spriteSheet, gb_sprites_frame_gbc_red_idx); break;
		case POKEMON_GREEN: topFrame = renderSheetImage(spriteSheet, gb_sprites_frame_gbc_green_idx); break;
		case POKEMON_BLUE: topFrame = renderSheetImage(spriteSheet, gb_sprites_frame_gbc_blue_idx); break;
		case POKEMON_YELLOW: topFrame = renderSheetImage(spriteSheet, gb_sprites_frame_gbc_yellow_idx); break;
		case POKEMON_GOLD: topFrame = renderSheetImage(spriteSheet, gb_sprites_frame_gbc_gold_idx); break;
		case POKEMON_SILVER: topFrame = renderSheetImage(spriteSheet, gb_sprites_frame_gbc_silver_idx); break;
		case POKEMON_CRYSTAL: topFrame = renderSheetImage(spriteSheet, gb_sprites_frame_gbc_crystal_idx); break;
		default: break;
	}

	return (topFrame.tex ? 0 : -5);
}

void gfxFree(void)
{
	if (spriteSheet) C2D_SpriteSheetFree(spriteSheet);
	if (extraSheet) C2D_SpriteSheetFree(extraSheet);
	if (animSheet1) C2D_SpriteSheetFree(animSheet1);
	if (animSheet2) C2D_SpriteSheetFree(animSheet2);
	if (frontRbSheet) C2D_SpriteSheetFree(frontRbSheet);
	if (frontYellowSheet) C2D_SpriteSheetFree(frontYellowSheet);
	if (frontGsSheet) C2D_SpriteSheetFree(frontGsSheet);
	spriteSheet = NULL;
	extraSheet = NULL;
	animSheet1 = NULL;
	animSheet2 = NULL;
	frontRbSheet = NULL;
	frontYellowSheet = NULL;
	frontGsSheet = NULL;
}

void gfxFreeFrame(void)
{
	topFrame = (C2D_Image) { NULL, NULL };
}

void gfxDrawPokemonIcon(int16_t x, int16_t y, DEX_Species icon, GFX_Frame frame)
{
	if (icon > 253) icon = SPECIES_MISSINGNO;

	renderDrawImagePart(pkmIconsGBC, x, y, 16 * (frame + (icon % PKM_PER_ROW) * 2), 16 * (icon / PKM_PER_ROW), 16, 16);
}

void gfxDrawCrystalSprite(int16_t x, int16_t y, uint8_t species, GFX_Frame frame)
{
	if (species == 0 || species > 251) return;

	if (species <= 128)
	{
		uint8_t idx = species - 1;
		uint8_t pairCol = idx % 8;
		uint8_t pairRow = idx / 8;
		int16_t sx = (pairCol * 2 + (frame ? 1 : 0)) * 56;
		int16_t sy = pairRow * 56;
		renderDrawImagePart(pkmAnim1, x, y, sx, sy, 56, 56);
	}
	else
	{
		uint8_t idx = species - 129;
		uint8_t pairCol = idx % 8;
		uint8_t pairRow = idx / 8;
		int16_t sx = (pairCol * 2 + (frame ? 1 : 0)) * 56;
		int16_t sy = pairRow * 56;
		renderDrawImagePart(pkmAnim2, x, y, sx, sy, 56, 56);
	}
}

void gfxDrawGamePokemonSprite(int16_t x, int16_t y, uint8_t species, uint8_t gameVersion)
{
	if (species == 0 || species > 251) return;

	uint16_t idx = species - 1;
	int16_t sx = (idx % 16) * 56;
	int16_t sy = (idx / 16) * 56;

	switch (gameVersion)
	{
		case POKEMON_RED:
		case POKEMON_GREEN:
		case POKEMON_BLUE:
			if (species <= 151 && frontRb.tex)
			{
				renderDrawImagePart(frontRb, x, y, sx, sy, 56, 56);
				return;
			}
			break;
		case POKEMON_YELLOW:
			if (species <= 151 && frontYellow.tex)
			{
				renderDrawImagePart(frontYellow, x, y, sx, sy, 56, 56);
				return;
			}
			break;
		case POKEMON_GOLD:
		case POKEMON_SILVER:
			if (frontGs.tex)
			{
				renderDrawImagePart(frontGs, x, y, sx, sy, 56, 56);
				return;
			}
			break;
		case POKEMON_CRYSTAL:
		default:
			break;
	}

	// Fallback or Crystal: use authentic Crystal animated front sprites
	gfxDrawCrystalSprite(x, y, species, GFX_FRAME_0);
}

void gfxDrawFrame(int16_t x, int16_t y)
{
	renderDrawImage(topFrame, x, y, 1.0f, 1.0f);
}

void gfxDrawPanel(int16_t x, int16_t y, uint8_t w, uint8_t h)
{
	fontDrawChar8(x, y, 0x79);
	fontDrawChar8(x + (w + 1) * 8, y, 0x7B);
	fontDrawChar8(x, y + (h + 1) * 8, 0x7D);
	fontDrawChar8(x + (w + 1) * 8, y + (h + 1) * 8, 0x7E);

	for (uint8_t xw = 0; xw < w; xw++)
	{
		fontDrawChar8(x + (xw + 1) * 8, y, 0x7A);
		fontDrawChar8(x + (xw + 1) * 8, y + (h + 1) * 8, 0x7A);
	}

	for (uint8_t yh = 0; yh < h; yh++)
	{
		fontDrawChar8(x, y + (yh + 1) * 8, 0x7C);
		fontDrawChar8(x + (w + 1) * 8, y + (yh + 1) * 8, 0x7C);
	}
}

void gfxDrawFolio(int16_t x, int16_t y, uint8_t w, uint8_t h)
{
	fontDrawChar8(x, y + h * 8, 0xDC);
	fontDrawChar8(x + (w + 1) * 8, y + h * 8, 0xDE);

	for (uint8_t xw = 0; xw < w; xw++)
	{
		fontDrawChar8(x + (xw + 1) * 8, y + h * 8, 0xDD);
	}

	for (uint8_t yh = 0; yh < h; yh++)
	{
		fontDrawChar8(x + (w + 1) * 8, y + yh * 8, 0xDF);
	}
}

void gfxDrawLevel(int16_t x, int16_t y, uint8_t level)
{
	x += fontDrawChar8(x, y, CHAR8_LVL);
	x += fontDrawUInt8(x, y, level);
}

void gfxDrawLifeBar(int16_t x, int16_t y, uint16_t currentHP, uint16_t maxHP, bool onlyBar)
{
	x += fontDrawSubchar8(x, y, CHAR8_HP, SUBCHAR_TOP_LEFT) + 1;
	x += fontDrawSubchar8(x, y, CHAR8_HP, SUBCHAR_TOP_RIGHT) + 1;
	x += fontDrawSubchar8(x, y, CHAR8_HP, SUBCHAR_BOTTOM_LEFT);

	char8_t c8;
	uint8_t percent = (float) currentHP / (float) maxHP * 100 + 0.5f;
	if (percent == 0) c8 = CHAR8_HPBAR_WHITE;
	else if (percent < 21) c8 = CHAR8_HPBAR_RED;
	else if (percent < 57) c8 = CHAR8_HPBAR_ORANGE;
	else c8 = CHAR8_HPBAR_GREEN;

	x += fontDrawSubchar8(x, y, c8, SUBCHAR_TOP_LEFT);
	for (uint8_t i = 0; i < 11; i++)
	{
		if (i * 100 / 11 > percent)
			c8 = CHAR8_HPBAR_WHITE;
		x += fontDrawSubchar8(x, y, c8, SUBCHAR_BOTTOM_LEFT);
	}
	x += fontDrawSubchar8(x, y, c8, SUBCHAR_TOP_RIGHT);

	if (!onlyBar)
	{
		fontDrawFromRightUInt32(x - 8, y + 6, maxHP);
		fontDrawChar8(x - 32, y + 6, CHAR8_SLASH);
		fontDrawFromRightUInt32(x - 40, y + 6, currentHP);
	}
}

void gfxDrawEmptyArrow(int16_t x, int16_t y)
{
	fontDrawChar8(x, y, 0xEC);
}

void gfxDrawFullArrow(int16_t x, int16_t y)
{
	fontDrawChar8(x, y, 0xED);
}

void gfxDrawBezel(bool isGBC)
{
	renderDrawImage(isGBC ? bezelGBC : bezelGB, 0, 0, 1.0f, 1.0f);
}

void gfxDrawAnimatedTitleScreen(int16_t x, int16_t y, uint8_t version)
{
	if (version < POKEMON_RED || version > POKEMON_CRYSTAL) version = POKEMON_RED;
	C2D_Image img = titleScreens[version];
	uint64_t ticks = osGetTime();

	if (version == POKEMON_GOLD)
	{
		renderDrawImagePart(img, x, y, 0, 0, 160, 92);
		renderDrawImagePart(img, x, y + 132, 0, 132, 160, 12);
		int16_t scroll = (ticks / 35) % 160;
		renderDrawImagePart(img, x, y + 92, scroll, 92, 160 - scroll, 40);
		if (scroll > 0)
			renderDrawImagePart(img, x + (160 - scroll), y + 92, 0, 92, scroll, 40);
		static const uint8_t hoohSeq[6] = { 0, 1, 2, 3, 4, 3 };
		uint8_t f = hoohSeq[(ticks / 130) % 6];
		int16_t bob = ((ticks / 260) % 2 == 0) ? -1 : 1;
		renderDrawImagePart(titleSprites, x + 48, y + 58 + bob, f * 64, 0, 64, 48);
	}
	else if (version == POKEMON_SILVER)
	{
		renderDrawImagePart(img, x, y, 0, 0, 160, 92);
		renderDrawImagePart(img, x, y + 132, 0, 132, 160, 12);
		int16_t scroll = (ticks / 35) % 160;
		renderDrawImagePart(img, x, y + 92, scroll, 92, 160 - scroll, 40);
		if (scroll > 0)
			renderDrawImagePart(img, x + (160 - scroll), y + 92, 0, 92, scroll, 40);
		static const uint8_t lugiaSeq[6] = { 0, 1, 2, 3, 2, 1 };
		uint8_t f = lugiaSeq[(ticks / 140) % 6];
		int16_t bob = ((ticks / 280) % 2 == 0) ? 0 : 2;
		renderDrawImagePart(titleSprites, x + 40, y + 60 + bob, f * 80, 48, 80, 40);
	}
	else if (version == POKEMON_CRYSTAL)
	{
		renderDrawImage(img, x, y, 1.0f, 1.0f);
		uint8_t f = (ticks / 110) % 4;
		renderDrawImagePart(titleSprites, x + 48, y + 84, f * 64, 88, 64, 48);
	}
	else if (version == POKEMON_RED || version == POKEMON_BLUE || version == POKEMON_GREEN)
	{
		renderDrawRect(x, y, 160, 144, RENDER_RGBA(248, 232, 248, 255));
		uint32_t phase = (ticks / 2400) % 6;
		uint32_t sub = ticks % 2400;
		int16_t offset = 0;
		if (sub < 300) offset = -((300 - sub) / 3);
		else if (sub > 2100) offset = -((sub - 2100) / 3);

		uint8_t pkmIdx = (version == POKEMON_BLUE ? 6 : 0) + phase;
		renderDrawImagePart(titleRbPkm, x + 32 + offset, y + 80, pkmIdx * 56, 0, 56, 56);
		renderDrawImage(img, x, y, 1.0f, 1.0f);
	}
	else
	{
		renderDrawImage(img, x, y, 1.0f, 1.0f);
		// Yellow title screen: Pikachu blinks his eyes
		if (((ticks / 180) % 8) == 0)
		{
			// Left eye: x+61..71, y+86..94; Right eye: x+89..99, y+86..94
			renderDrawRect(x + 61, y + 86, 10, 9, RENDER_RGBA(0xF8, 0xF8, 0x00, 0xFF));
			renderDrawRect(x + 89, y + 86, 10, 9, RENDER_RGBA(0xF8, 0xF8, 0x00, 0xFF));
			// Draw horizontal closed eye slit line
			renderDrawRect(x + 61, y + 90, 10, 2, RENDER_RGBA(0x00, 0x00, 0x00, 0xFF));
			renderDrawRect(x + 89, y + 90, 10, 2, RENDER_RGBA(0x00, 0x00, 0x00, 0xFF));
		}
	}
}

void gfxDrawTrainerCardBg(int16_t x, int16_t y, uint8_t version, uint8_t gender)
{
	if (version <= POKEMON_BLUE)
		renderDrawImage(tcRb, x, y, 1.0f, 1.0f);
	else if (version == POKEMON_YELLOW)
		renderDrawImage(tcYellow, x, y, 1.0f, 1.0f);
	else if (version == POKEMON_CRYSTAL)
		renderDrawImage(gender ? tcCrystalGirl : tcCrystalBoy, x, y, 1.0f, 1.0f);
	else
		renderDrawImage(tcGs, x, y, 1.0f, 1.0f);
}

void gfxDrawBadge(int16_t x, int16_t y, bool isJohto, uint8_t badgeIdx)
{
	if (badgeIdx >= 8) return;
	renderDrawImagePart(tcBadges, x, y, badgeIdx * 16, isJohto ? 16 : 0, 16, 16);
}

void gfxDrawBagUI(int16_t x, int16_t y, uint8_t pocketIdx, bool isFemale)
{
	uint8_t iconIdx = (pocketIdx < 4 ? pocketIdx : 0);
	renderDrawImagePart(bagUi, x, y, iconIdx * 40, isFemale ? 0 : 24, 40, 24);
	if (pocketIdx < 4)
		renderDrawImagePart(bagUi, x, y + 28, iconIdx * 40, 48, 40, 24);
}

void gfxDrawPokedexBg(int16_t x, int16_t y)
{
	renderDrawImage(dexBg, x, y, 1.0f, 1.0f);
}

void gfxDrawWorldMap(int16_t x, int16_t y, bool isKanto)
{
	renderDrawImage(isKanto ? mapKanto : mapJohto, x, y, 1.0f, 1.0f);
}

void gfxDrawDiplomaBg(int16_t x, int16_t y, bool isGen2)
{
	renderDrawImage(isGen2 ? diplomaGen2 : diplomaGen1, x, y, 1.0f, 1.0f);
}

void gfxDrawShinyStar(int16_t x, int16_t y)
{
	renderDrawImage(imgShinyStar, x, y, 1.0f, 1.0f);
}

void gfxDrawShinyStarSmall(int16_t x, int16_t y)
{
	renderDrawImage(imgShinyStarSmall, x, y, 1.0f, 1.0f);
}

void gfxDrawEggSprite(int16_t x, int16_t y)
{
	renderDrawImage(imgEggFront, x, y, 1.0f, 1.0f);
}

void gfxDrawCartridgeIcon(int16_t x, int16_t y, uint8_t version)
{
	if (version < POKEMON_RED || version > POKEMON_CRYSTAL) version = POKEMON_RED;

	u32 cartCol   = RENDER_RGBA(0xD8, 0x28, 0x20, 0xFF);
	u32 cartShade = RENDER_RGBA(0x98, 0x18, 0x14, 0xFF);
	if (version == POKEMON_BLUE)        { cartCol = RENDER_RGBA(0x20, 0x58, 0xD8, 0xFF); cartShade = RENDER_RGBA(0x14, 0x38, 0x98, 0xFF); }
	else if (version == POKEMON_GREEN)  { cartCol = RENDER_RGBA(0x28, 0x98, 0x38, 0xFF); cartShade = RENDER_RGBA(0x18, 0x68, 0x24, 0xFF); }
	else if (version == POKEMON_YELLOW) { cartCol = RENDER_RGBA(0xF0, 0xC0, 0x18, 0xFF); cartShade = RENDER_RGBA(0xB8, 0x88, 0x08, 0xFF); }
	else if (version == POKEMON_GOLD)   { cartCol = RENDER_RGBA(0xC8, 0x98, 0x24, 0xFF); cartShade = RENDER_RGBA(0x88, 0x64, 0x10, 0xFF); }
	else if (version == POKEMON_SILVER) { cartCol = RENDER_RGBA(0x90, 0x9C, 0xAC, 0xFF); cartShade = RENDER_RGBA(0x5C, 0x66, 0x76, 0xFF); }
	else if (version == POKEMON_CRYSTAL){ cartCol = RENDER_RGBA(0x48, 0xB8, 0xE8, 0xFF); cartShade = RENDER_RGBA(0x24, 0x78, 0xA8, 0xFF); }

	// 1. Cartridge 48x48 shell with authentic GB top right corner notch & grip ribs
	renderDrawRect(x, y, 48, 48, RENDER_RGBA(0x14, 0x18, 0x20, 0xFF));
	renderDrawRect(x + 2, y + 2, 44, 44, cartCol);
	renderDrawRect(x + 2, y + 43, 44, 3, cartShade);
	// Molded top header & grip lines
	renderDrawRect(x + 6, y + 3, 36, 6, cartShade);
	renderDrawRect(x + 10, y + 4, 28, 1, RENDER_RGBA(0xFF, 0xFF, 0xFF, 0x55));
	renderDrawRect(x + 10, y + 6, 28, 1, RENDER_RGBA(0xFF, 0xFF, 0xFF, 0x55));
	// Side grip notches
	renderDrawRect(x + 1, y + 16, 1, 20, cartShade);
	renderDrawRect(x + 46, y + 16, 1, 20, cartShade);

	// 2. Cartridge Label Frame (x+4..x+43, y+10..y+43 -> 40x34 outer, 38x32 inner at x+5, y+11)
	renderDrawRect(x + 4, y + 10, 40, 34, RENDER_RGBA(0x18, 0x1C, 0x24, 0xFF));
	int16_t lx = x + 5;
	int16_t ly = y + 11;

	// 3. Pixel-Art Original Cover Artwork inside the 38x32 label
	if (version == POKEMON_RED)
	{
		renderDrawRect(lx, ly, 38, 32, RENDER_RGBA(0xF8, 0xF4, 0xF0, 0xFF));
		renderDrawRect(lx, ly, 38, 9, RENDER_RGBA(0xD8, 0x28, 0x20, 0xFF));
		renderDrawRect(lx, ly + 28, 38, 4, RENDER_RGBA(0xD8, 0x28, 0x20, 0xFF));
		// Charizard pixel-art sprite (56x56 at 0.42f = 23.5x23.5)
		renderDrawImagePartScaled(titleRbPkm, lx + 8, ly + 7, 2 * 56, 0, 56, 56, 0.42f, 0.42f);
		// Official Pokémon Logo from title screen (136x48 at 0.25f = 34x12)
		renderDrawImagePartScaled(titleScreens[POKEMON_RED], lx + 2, ly + 1, 12, 8, 136, 48, 0.25f, 0.22f);
	}
	else if (version == POKEMON_BLUE)
	{
		renderDrawRect(lx, ly, 38, 32, RENDER_RGBA(0xF0, 0xF6, 0xF8, 0xFF));
		renderDrawRect(lx, ly, 38, 9, RENDER_RGBA(0x20, 0x58, 0xD8, 0xFF));
		renderDrawRect(lx, ly + 28, 38, 4, RENDER_RGBA(0x20, 0x58, 0xD8, 0xFF));
		// Blastoise pixel-art sprite (index 8 in titleRbPkm)
		renderDrawImagePartScaled(titleRbPkm, lx + 8, ly + 7, 8 * 56, 0, 56, 56, 0.42f, 0.42f);
		renderDrawImagePartScaled(titleScreens[POKEMON_BLUE], lx + 2, ly + 1, 12, 8, 136, 48, 0.25f, 0.22f);
	}
	else if (version == POKEMON_GREEN)
	{
		renderDrawRect(lx, ly, 38, 32, RENDER_RGBA(0xF0, 0xF8, 0xF0, 0xFF));
		renderDrawRect(lx, ly, 38, 9, RENDER_RGBA(0x28, 0x98, 0x38, 0xFF));
		renderDrawRect(lx, ly + 28, 38, 4, RENDER_RGBA(0x28, 0x98, 0x38, 0xFF));
		// Scyther/Venusaur pixel-art sprite
		renderDrawImagePartScaled(titleRbPkm, lx + 8, ly + 7, 5 * 56, 0, 56, 56, 0.42f, 0.42f);
		renderDrawImagePartScaled(titleScreens[POKEMON_GREEN], lx + 2, ly + 1, 12, 8, 136, 48, 0.25f, 0.22f);
	}
	else if (version == POKEMON_YELLOW)
	{
		// Full Yellow cover art (Pikachu + ¡Pika! + Pokémon logo) scaled to 38x32
		renderDrawImagePartScaled(titleScreens[POKEMON_YELLOW], lx, ly, 0, 4, 160, 132, 38.0f / 160.0f, 32.0f / 132.0f);
	}
	else if (version == POKEMON_GOLD)
	{
		// Gold sky & clouds background + Ho-Oh + Gold Pokémon logo
		renderDrawImagePartScaled(titleScreens[POKEMON_GOLD], lx, ly, 0, 4, 160, 132, 38.0f / 160.0f, 32.0f / 132.0f);
		if (frontGs.tex)
		{
			// Full color Ho-Oh (#250 -> idx 249: col 9, row 15)
			renderDrawImagePartScaled(frontGs, lx + 9, ly + 10, 9 * 56, 15 * 56, 56, 56, 0.38f, 0.38f);
		}
		else
		{
			renderDrawImagePartScaled(titleSprites, lx + 6, ly + 12, 4 * 64, 0, 64, 48, 0.42f, 0.42f);
		}
	}
	else if (version == POKEMON_SILVER)
	{
		// Silver sky & clouds background + Lugia + Silver Pokémon logo
		renderDrawImagePartScaled(titleScreens[POKEMON_SILVER], lx, ly, 0, 4, 160, 132, 38.0f / 160.0f, 32.0f / 132.0f);
		if (frontGs.tex)
		{
			// Full color Lugia (#249 -> idx 248: col 8, row 15)
			renderDrawImagePartScaled(frontGs, lx + 9, ly + 10, 8 * 56, 15 * 56, 56, 56, 0.38f, 0.38f);
		}
		else
		{
			renderDrawImagePartScaled(titleSprites, lx + 4, ly + 13, 0, 48, 80, 40, 0.38f, 0.38f);
		}
	}
	else
	{
		// Crystal cover art + Suicune (#245 -> idx 244 in pkmAnim2)
		renderDrawImagePartScaled(titleScreens[POKEMON_CRYSTAL], lx, ly, 0, 4, 160, 132, 38.0f / 160.0f, 32.0f / 132.0f);
		uint8_t idx = 245 - 129; // 116 -> pairCol 4, pairRow 14
		renderDrawImagePartScaled(pkmAnim2, lx + 9, ly + 11, (4 * 2) * 56, 14 * 56, 56, 56, 0.38f, 0.38f);
	}
}
