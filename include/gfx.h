#pragma once

#include <3ds/types.h>

#include "pokedex.h"

typedef enum
{
	PKM_ICON_BIPED = 0,		///< SPRITE_MON       Pokémon icon (2 frames) 
	PKM_ICON_BALL = 1,		///< SPRITE_BALL_M    Ball pkm icon (1 frame) 
	PKM_ICON_FOSSIL = 2,	///< SPRITE_HELIX     Fossil icon (1 frame)
	PKM_ICON_FAIRY = 3,		///< SPRITE_FAIRY     Fairy icon (2 frames)
	PKM_ICON_BIRD = 4,		///< SPRITE_WATER     Bird icon (2 frames)
	PKM_ICON_WATER = 5,		///< SPRITE_WATER     Water icon (2 frames)
	PKM_ICON_BUG = 6,		///< SPRITE_BUG       Insect icon (2 frames)
	PKM_ICON_PLANT = 7,		///< SPRITE_GRASS     Plant icon (2 frames) 
	PKM_ICON_SNAKE = 8,		///< SPRITE_SNAKE     Snake icon (2 frames) 
	PKM_ICON_QUADRUPED = 9,	///< SPRITE_QUADRUPED Quadruped icon (2 frames) 
	PKM_ICON_MISSINGNO = 0,	///<                  Missingno icon (0 frames)
} GFX_PkmIcon;

typedef enum
{
	GFX_FRAME_0 = 0,	///< First frame.
	GFX_FRAME_1 = 1,	///< Second frame.
} GFX_Frame;

Result gfxLoad(void);
Result gfxLoadFrame(uint64_t titleid);
void gfxFree(void);
void gfxFreeFrame(void);

void gfxDrawPokemonIcon(int16_t x, int16_t y, DEX_Species icon, GFX_Frame frame);
void gfxDrawCrystalSprite(int16_t x, int16_t y, uint8_t species, GFX_Frame frame);
void gfxDrawGamePokemonSprite(int16_t x, int16_t y, uint8_t species, uint8_t gameVersion);
void gfxDrawFrame(int16_t x, int16_t y);
void gfxDrawFolio(int16_t x, int16_t y, uint8_t w, uint8_t h);
void gfxDrawPanel(int16_t x, int16_t y, uint8_t w, uint8_t h);
void gfxDrawLevel(int16_t x, int16_t y, uint8_t level);
void gfxDrawLifeBar(int16_t x, int16_t y, uint16_t currentHP, uint16_t maxHP, bool onlyBar);
void gfxDrawEmptyArrow(int16_t x, int16_t y);
void gfxDrawFullArrow(int16_t x, int16_t y);

// Extra UI & Carousel Drawing
void gfxDrawBezel(bool isGBC);
void gfxDrawAnimatedTitleScreen(int16_t x, int16_t y, uint8_t version);
void gfxDrawTrainerCardBg(int16_t x, int16_t y, uint8_t version, uint8_t gender);
void gfxDrawBadge(int16_t x, int16_t y, bool isJohto, uint8_t badgeIdx);
void gfxDrawBagUI(int16_t x, int16_t y, uint8_t pocketIdx, bool isFemale);
void gfxDrawPokedexBg(int16_t x, int16_t y);
void gfxDrawWorldMap(int16_t x, int16_t y, bool isKanto);
void gfxDrawDiplomaBg(int16_t x, int16_t y, bool isGen2);
void gfxDrawShinyStar(int16_t x, int16_t y);
void gfxDrawShinyStarSmall(int16_t x, int16_t y);
void gfxDrawEggSprite(int16_t x, int16_t y);
