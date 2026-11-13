#pragma once

#include <3ds/types.h>
#include <stdbool.h>

typedef uint8_t char8_t;

typedef enum
{
	CHAR8_LINE_BREAK = 0x4F,
	CHAR8_DOT = 0xE8,
	CHAR8_SLASH = 0xF3,

	// Extra characters
	CHAR8_ID = 0xD0,
	CHAR8_NO = 0xD1,
	CHAR8_LVL = 0xD2,
	CHAR8_HP = 0xD3,
	CHAR8_HPBAR_WHITE = 0xD4,
	CHAR8_HPBAR_GREEN = 0xD5,
	CHAR8_HPBAR_ORANGE = 0xD6,
	CHAR8_HPBAR_RED = 0xD7,
	CHAR8_POKEBALL_SEEN = 0xD8,
	CHAR8_DEX_SEEN = 0xD9,
} FONT_Char8;

typedef enum
{
	SUBCHAR_TOP_LEFT = 0,
	SUBCHAR_TOP_RIGHT = 1,
	SUBCHAR_BOTTOM_LEFT = 2,
	SUBCHAR_BOTTOM_RIGHT = 3,
} FONT_Subchar8;

Result fontLoad(void);
void fontFree(void);
void fontSetWhite(bool white);

void fontConvertString(char8_t* str8, const char* str);
int8_t fontDrawChar8(int16_t x, int16_t y, char8_t c);
int8_t fontDrawJPChar8(int16_t x, int16_t y, char8_t c);
int8_t fontDrawSubchar8(int16_t x, int16_t y, char8_t c, FONT_Subchar8 i);
uint16_t fontDrawString8(int16_t x, int16_t y, const char8_t* str8);
uint16_t fontDrawJPString8(int16_t x, int16_t y, const char8_t* str8);
uint16_t fontDrawUInt8(int16_t x, int16_t y, int8_t v);
uint16_t fontDrawFromRightUInt32(int16_t x, int16_t y, uint32_t v);
uint16_t fontDrawPaddingUInt8(int16_t x, int16_t y, uint8_t v);
uint16_t fontDrawPaddingUInt16(int16_t x, int16_t y, uint16_t v);
