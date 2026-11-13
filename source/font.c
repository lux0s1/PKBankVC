#include "font.h"
#include "save.h"
#include "render.h"

#include "gb_font.h"

#define CHAR_SIZE (8)
#define FONT_PATH "romfs:/gfx/gb_font.t3x"

typedef enum
{
	C_NULL = 0x00,			///< \0
	C_PKMN = 0x4A,			///< PKMN
	C_LINE_BREAK = 0x4F,	///< \n
	C_TERMINATOR = 0x50,	///< \0
	C_PLAYER_NAME = 0x52,	///< NINTEN
	C_RIVAL_NAME = 0x53,	///< SONY
	C_POKE = 0x54,			///< POKé
	C_ELLIPSIS = 0x56,		///< …
	C_INACTIVE_PKM = 0x59,	///< Enemy
	C_ACTIVE_PKM = 0x54,	///< Friend
	C_PC = 0x5B,			///< PC
	C_TM = 0x5C,			///< TM
	C_TRAINER = 0x5D,		///< TRAINER
	C_ROCKET = 0x5E,		///< ROCKET
} FONT_ControlChar;

static C2D_SpriteSheet fontSheet;
static C2D_Image fontGB;
static C2D_Image fontJPGB;
static C2D_Image fontWhiteGB;
static bool useWhiteFont = false;

Result fontLoad(void)
{
	fontSheet = renderLoadSheet(FONT_PATH);
	if (!fontSheet) return -5;

	fontGB = renderSheetImage(fontSheet, gb_font_font_gb_idx);
	fontJPGB = renderSheetImage(fontSheet, gb_font_font_gb_jp_idx);
	fontWhiteGB = renderSheetImage(fontSheet, gb_font_font_gb_white_idx);
	useWhiteFont = false;

	return (fontGB.tex && fontJPGB.tex ? 0 : -5);
}

void fontFree(void)
{
	if (fontSheet) C2D_SpriteSheetFree(fontSheet);
	fontSheet = NULL;
}

void fontSetWhite(bool white)
{
	useWhiteFont = white;
}

static char8_t fontConvertChar(char c)
{
	switch (c)
	{
		case 'A' ... 'Z': return 0x80 + (c - 'A');
		case 'a' ... 'z': return 0xA0 + (c - 'a');
		case '0' ... '9': return 0xF6 + (c - '0');
		case ' ': return 0x7F;
		case '-': return 0xE3;
		case '?': return 0xE6;
		case '!': return 0xE7;
		case '.': return 0xE8;
		case '/': return 0xF3;
		case ',': return 0xF4;
		case ':': return 0x9C;
		case '(': return 0x9A;
		case ')': return 0x9B;
		case '+': return 0x10;
		default: return 0x50;
	}
}

void fontConvertString(char8_t* str8, const char* str)
{
	while (*str != 0x00)
	{
		*str8 = fontConvertChar(*str);
		str8++;
		str++;
	}
	*str8 = 0x50;
}

int8_t fontDrawChar8(int16_t x, int16_t y, char8_t c)
{
	C2D_Image img = (useWhiteFont && c != 0x00 && c != 0xD8 && c != 0xD9) ? fontWhiteGB : fontGB;
	renderDrawImagePart(img, x, y, (c % 0x10) * CHAR_SIZE, (c / 0x10) * CHAR_SIZE, CHAR_SIZE, CHAR_SIZE);

	return CHAR_SIZE;
}

int8_t fontDrawJPChar8(int16_t x, int16_t y, char8_t c)
{
	renderDrawImagePart(fontJPGB, x, y, (c % 0x10) * CHAR_SIZE, (c / 0x10) * CHAR_SIZE, CHAR_SIZE, CHAR_SIZE);

	return CHAR_SIZE;
}

int8_t fontDrawSubchar8(int16_t x, int16_t y, char8_t c, FONT_Subchar8 i)
{
	renderDrawImagePart(fontGB, x, y, (c % 0x10) * CHAR_SIZE + (i % 4 % 2) * CHAR_SIZE / 2, (c / 0x10) * CHAR_SIZE + (i % 4 / 2) * CHAR_SIZE / 2, CHAR_SIZE / 2, CHAR_SIZE / 2);

	return CHAR_SIZE / 2;
}

static void fontDrawControlChar8(int16_t o_x, int16_t o_y, int16_t* x, int16_t* y, char8_t c)
{
	switch (c)
	{
		case C_PKMN:
			*x += fontDrawChar8(*x, *y, 0xE1); // PK
			*x += fontDrawChar8(*x, *y, 0xE2); // MN
			break;
		case C_LINE_BREAK:
			*x = o_x;
			*y += CHAR_SIZE;
			break;
		case C_PLAYER_NAME:
			*x += fontDrawString8(*x, *y, save + 0x2598);
			break;
		case C_RIVAL_NAME:
			*x += fontDrawString8(*x, *y, save + 0x25F6);
			break;
		case C_POKE:
			*x += fontDrawChar8(*x, *y, 0x8F); // P
			*x += fontDrawChar8(*x, *y, 0x8E); // O
			*x += fontDrawChar8(*x, *y, 0x8A); // K
			*x += fontDrawChar8(*x, *y, 0xBA); // é
			break;
		case C_ELLIPSIS:
			*x += fontDrawChar8(*x, *y, 0x75); // …
			break;
		case C_PC:
			*x += fontDrawChar8(*x, *y, 0x8F); // P
			*x += fontDrawChar8(*x, *y, 0x82); // C
			break;
		case C_TM:
			*x += fontDrawChar8(*x, *y, 0x82); // T
			*x += fontDrawChar8(*x, *y, 0x8C); // M
			break;
		case C_TRAINER:
			*x += fontDrawChar8(*x, *y, 0x93); // T
			*x += fontDrawChar8(*x, *y, 0x91); // R
			*x += fontDrawChar8(*x, *y, 0x80); // A
			*x += fontDrawChar8(*x, *y, 0x88); // I
			*x += fontDrawChar8(*x, *y, 0x8D); // N
			*x += fontDrawChar8(*x, *y, 0x84); // E
			*x += fontDrawChar8(*x, *y, 0x91); // R
			break;
		case C_ROCKET:
			*x += fontDrawChar8(*x, *y, 0x91); // R
			*x += fontDrawChar8(*x, *y, 0x8E); // O
			*x += fontDrawChar8(*x, *y, 0x82); // C
			*x += fontDrawChar8(*x, *y, 0x8A); // K
			*x += fontDrawChar8(*x, *y, 0x84); // E
			*x += fontDrawChar8(*x, *y, 0x93); // T
			break;
	}
}

uint16_t fontDrawString8(int16_t x, int16_t y, const char8_t* str8)
{
	int16_t o_x = x, o_y = y;
	while (*str8 != C_TERMINATOR && *str8 != C_NULL)
	{
		char8_t c = *str8;
		if (c >= 0xD0 && c <= 0xD5)
		{
			c = 0x20 + (c - 0xD0);
		}

		if (0x49 <= c && c <= 0x5F)
			fontDrawControlChar8(o_x, o_y, &x, &y, c);
		else
			x += fontDrawChar8(x, y, c);
		str8++;
	}

	return x - o_x;
}

uint16_t fontDrawJPString8(int16_t x, int16_t y, const char8_t* str8)
{
	int16_t o_x = x;
	while (*str8 != C_TERMINATOR && *str8 != C_NULL)
	{
		if (*str8 == C_LINE_BREAK) { x = o_x; y += CHAR_SIZE; }
		else if (*str8 == 0xE3) { x += fontDrawJPChar8(x, y, 0xE3); }
		else
			x += fontDrawJPChar8(x, y, *str8);
		str8++;
	}

	return x - o_x;
}

uint16_t fontDrawUInt8(int16_t x, int16_t y, int8_t v)
{
	int16_t o_x = x;

	if (v < 0)
	{
		x += fontDrawChar8(x, y, 0xE3);
		v = -v;
	}

	if (v / 100 != 0)
		x += fontDrawChar8(x, y, 0xF6 + (v / 100) % 10);
	if (v / 10 != 0)
		x += fontDrawChar8(x, y, 0xF6 + (v / 10) % 10);
	x += fontDrawChar8(x, y, 0xF6 + (v / 1) % 10);

	return x - o_x;
}

uint16_t fontDrawFromRightUInt32(int16_t x, int16_t y, uint32_t v)
{
	int16_t o_x = x;

	x -= fontDrawChar8(x, y, 0xF6 + (v % 10));
	if (v >= 10)      x -= fontDrawChar8(x, y, 0xF6 + (v / 10) % 10);
	if (v >= 100)     x -= fontDrawChar8(x, y, 0xF6 + (v / 100) % 10);
	if (v >= 1000)    x -= fontDrawChar8(x, y, 0xF6 + (v / 1000) % 10);
	if (v >= 10000)   x -= fontDrawChar8(x, y, 0xF6 + (v / 10000) % 10);
	if (v >= 100000)  x -= fontDrawChar8(x, y, 0xF6 + (v / 100000) % 10);
	if (v >= 1000000) x -= fontDrawChar8(x, y, 0xF6 + (v / 1000000) % 10);

	return o_x - x;
}

uint16_t fontDrawPaddingUInt8(int16_t x, int16_t y, uint8_t v)
{
	int16_t o_x = x;

	x += fontDrawChar8(x, y, 0xF6 + (v / 100) % 10);
	x += fontDrawChar8(x, y, 0xF6 + (v / 10) % 10);
	x += fontDrawChar8(x, y, 0xF6 + (v / 1) % 10);

	return x - o_x;
}

uint16_t fontDrawPaddingUInt16(int16_t x, int16_t y, uint16_t v)
{
	int16_t o_x = x;

	x += fontDrawChar8(x, y, 0xF6 + (v / 10000) % 10);
	x += fontDrawChar8(x, y, 0xF6 + (v / 1000) % 10);
	x += fontDrawChar8(x, y, 0xF6 + (v / 100) % 10);
	x += fontDrawChar8(x, y, 0xF6 + (v / 10) % 10);
	x += fontDrawChar8(x, y, 0xF6 + (v / 1) % 10);

	return x - o_x;
}
