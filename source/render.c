#include "render.h"
#include <stdlib.h>

#define RENDER_DEPTH (0.5f)
#define RENDER_MAX_OBJECTS (4096)

static C3D_RenderTarget* targetTop;
static C3D_RenderTarget* targetBottom;
static u32 clearColor;
static bool renderInitialized = false;

Result renderInit(void)
{
	gfxInitDefault();

	if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))
	{
		gfxExit();
		return -1;
	}

	if (!C2D_Init(RENDER_MAX_OBJECTS))
	{
		C3D_Fini();
		gfxExit();
		return -2;
	}

	C2D_Prepare();

	targetTop = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	targetBottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	clearColor = RENDER_RGBA(0x40,0x40,0x40,0xFF);

	renderInitialized = true;
	return 0;
}

void renderExit(void)
{
	if (!renderInitialized) return;

	C2D_Fini();
	C3D_Fini();
	gfxExit();

	renderInitialized = false;
}

void renderSetClearColor(u32 color)
{
	clearColor = color;
}

void renderFrameBegin(void)
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(targetTop, clearColor);
	C2D_TargetClear(targetBottom, clearColor);
}

void renderSceneTop(void)
{
	C2D_SceneBegin(targetTop);
}

void renderSceneBottom(void)
{
	C2D_SceneBegin(targetBottom);
}

void renderFrameEnd(void)
{
	C3D_FrameEnd(0);
}

C2D_SpriteSheet renderLoadSheet(const char* path)
{
	C2D_SpriteSheet sheet = C2D_SpriteSheetLoad(path);
	if (!sheet) return NULL;

	// Pixel art: no bilinear filtering.
	size_t count = C2D_SpriteSheetCount(sheet);
	for (size_t i = 0; i < count; i++)
	{
		C2D_Image img = C2D_SpriteSheetGetImage(sheet, i);
		C3D_TexSetFilter(img.tex, GPU_NEAREST, GPU_NEAREST);
	}

	return sheet;
}

C2D_Image renderSheetImage(C2D_SpriteSheet sheet, size_t index)
{
	if (!sheet || index >= C2D_SpriteSheetCount(sheet))
		return (C2D_Image) { NULL, NULL };

	return C2D_SpriteSheetGetImage(sheet, index);
}

void renderDrawImage(C2D_Image img, float x, float y, float scaleX, float scaleY)
{
	if (!img.tex || !img.subtex) return;

	C2D_DrawImageAt(img, x, y, RENDER_DEPTH, NULL, scaleX, scaleY);
}

void renderDrawImagePartScaled(C2D_Image img, float x, float y, int sx, int sy, int sw, int sh, float scaleX, float scaleY)
{
	if (!img.tex || !img.subtex) return;

	const Tex3DS_SubTexture* src = img.subtex;
	Tex3DS_SubTexture part;

	// Interpolate the UVs of the source subtexture (works for any UV orientation).
	float du = (src->right - src->left) / (float) src->width;
	float dv = (src->bottom - src->top) / (float) src->height;

	part.width = sw;
	part.height = sh;
	part.left = src->left + sx * du;
	part.right = part.left + sw * du;
	part.top = src->top + sy * dv;
	part.bottom = part.top + sh * dv;

	C2D_Image partImg = { img.tex, &part };
	C2D_DrawImageAt(partImg, x, y, RENDER_DEPTH, NULL, scaleX, scaleY);
}

void renderDrawImagePart(C2D_Image img, float x, float y, int sx, int sy, int sw, int sh)
{
	renderDrawImagePartScaled(img, x, y, sx, sy, sw, sh, 1.0f, 1.0f);
}

void renderDrawRect(float x, float y, float w, float h, u32 color)
{
	C2D_DrawRectSolid(x, y, RENDER_DEPTH, w, h, color);
}

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#pragma pack(push, 1)
typedef struct {
	uint16_t bfType;
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
} BMPHeader;

typedef struct {
	uint32_t biSize;
	int32_t  biWidth;
	int32_t  biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t  biXPelsPerMeter;
	int32_t  biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} BMPInfoHeader;
#pragma pack(pop)

bool renderAutoTestFlagPresent(void)
{
	FILE* f = fopen("/pk/auto_test.flag", "rb");
	if (!f) f = fopen("sdmc:/pk/auto_test.flag", "rb");
	if (f)
	{
		fclose(f);
		return true;
	}
	return false;
}

void renderAutoTestFlagRemove(void)
{
	remove("/pk/auto_test.flag");
	remove("sdmc:/pk/auto_test.flag");
}

static void writeBmp24(const char* path, int width, int height, const uint8_t* bgrData, int stride)
{
	FILE* f = fopen(path, "wb");
	if (!f && path[0] == '/')
	{
		char altPath[160];
		snprintf(altPath, sizeof(altPath), "sdmc:%s", path);
		f = fopen(altPath, "wb");
	}
	if (!f) return;

	int rowSize = ((width * 3 + 3) / 4) * 4;
	uint32_t imageSize = rowSize * height;

	BMPHeader hdr;
	hdr.bfType = 0x4D42;
	hdr.bfSize = 54 + imageSize;
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;
	hdr.bfOffBits = 54;
	fwrite(&hdr, 1, sizeof(hdr), f);

	BMPInfoHeader info;
	memset(&info, 0, sizeof(info));
	info.biSize = sizeof(info);
	info.biWidth = width;
	info.biHeight = height; // Bottom-up
	info.biPlanes = 1;
	info.biBitCount = 24;
	info.biCompression = 0;
	info.biSizeImage = imageSize;
	fwrite(&info, 1, sizeof(info), f);

	uint8_t pad[4] = {0, 0, 0, 0};
	int padBytes = rowSize - (width * 3);

	for (int y = height - 1; y >= 0; y--)
	{
		const uint8_t* row = bgrData + y * stride;
		fwrite(row, 1, width * 3, f);
		if (padBytes > 0)
			fwrite(pad, 1, padBytes, f);
	}

	fclose(f);
}

Result renderSaveScreenshot(const char* filenamePrefix)
{
	mkdir("/pk", 0777);
	mkdir("/pk/snapshots", 0777);
	mkdir("sdmc:/pk", 0777);
	mkdir("sdmc:/pk/snapshots", 0777);

	u16 topW = 0, topH = 0;
	u16 botW = 0, botH = 0;
	u8* fbTop = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &topW, &topH);
	u8* fbBot = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &botW, &botH);

	if (!fbTop || !fbBot) return -1;

	uint8_t* topImg = (uint8_t*) malloc(400 * 240 * 3);
	uint8_t* botImg = (uint8_t*) malloc(320 * 240 * 3);
	uint8_t* bothImg = (uint8_t*) malloc(400 * 480 * 3);

	if (!topImg || !botImg || !bothImg)
	{
		if (topImg) free(topImg);
		if (botImg) free(botImg);
		if (bothImg) free(bothImg);
		return -2;
	}

	// Transpose Top (physical: 240 width, 400 height -> logical: 400x240)
	for (int y = 0; y < 240; y++)
	{
		for (int x = 0; x < 400; x++)
		{
			int srcIdx = (x * 240 + (239 - y)) * 3;
			int dstIdx = (y * 400 + x) * 3;
			topImg[dstIdx + 0] = fbTop[srcIdx + 0]; // B
			topImg[dstIdx + 1] = fbTop[srcIdx + 1]; // G
			topImg[dstIdx + 2] = fbTop[srcIdx + 2]; // R
		}
	}

	// Transpose Bottom (physical: 240 width, 320 height -> logical: 320x240)
	for (int y = 0; y < 240; y++)
	{
		for (int x = 0; x < 320; x++)
		{
			int srcIdx = (x * 240 + (239 - y)) * 3;
			int dstIdx = (y * 320 + x) * 3;
			botImg[dstIdx + 0] = fbBot[srcIdx + 0]; // B
			botImg[dstIdx + 1] = fbBot[srcIdx + 1]; // G
			botImg[dstIdx + 2] = fbBot[srcIdx + 2]; // R
		}
	}

	char pathBuf[128];
	snprintf(pathBuf, sizeof(pathBuf), "/pk/snapshots/%s_top.bmp", filenamePrefix);
	writeBmp24(pathBuf, 400, 240, topImg, 400 * 3);

	snprintf(pathBuf, sizeof(pathBuf), "/pk/snapshots/%s_bottom.bmp", filenamePrefix);
	writeBmp24(pathBuf, 320, 240, botImg, 320 * 3);

	// Combined 400x480 dual-screen view
	memset(bothImg, 0x20, 400 * 480 * 3); // Dark background border
	// Copy top screen (400x240)
	memcpy(bothImg, topImg, 400 * 240 * 3);
	// Copy bottom screen (320x240) centered at x = 40
	for (int y = 0; y < 240; y++)
	{
		memcpy(bothImg + ((240 + y) * 400 + 40) * 3, botImg + (y * 320) * 3, 320 * 3);
	}
	snprintf(pathBuf, sizeof(pathBuf), "/pk/snapshots/%s_dual.bmp", filenamePrefix);
	writeBmp24(pathBuf, 400, 480, bothImg, 400 * 3);

	free(topImg);
	free(botImg);
	free(bothImg);

	return 0;
}
