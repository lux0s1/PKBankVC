#include "render.h"

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

void renderDrawImagePart(C2D_Image img, float x, float y, int sx, int sy, int sw, int sh)
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
	C2D_DrawImageAt(partImg, x, y, RENDER_DEPTH, NULL, 1.0f, 1.0f);
}

void renderDrawRect(float x, float y, float w, float h, u32 color)
{
	C2D_DrawRectSolid(x, y, RENDER_DEPTH, w, h, color);
}
