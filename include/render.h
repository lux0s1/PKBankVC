#pragma once
/**
 * @file render.h
 * @brief Thin rendering layer over citro2d (replaces the old sf2d/sfil usage).
 */
#ifndef RENDER_H
#define RENDER_H

#include <citro2d.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Builds a RGBA color (same argument order as the old sf2d RGBA8 macro).
#define RENDER_RGBA(r,g,b,a) C2D_Color32((r),(g),(b),(a))

/**
 * @brief Initializes gfx, citro3d, citro2d and the screen targets.
 */
Result renderInit(void);

/**
 * @brief Deinitializes everything initialized by renderInit.
 */
void renderExit(void);

/**
 * @brief Sets the clear color used by the next frames.
 */
void renderSetClearColor(u32 color);

/**
 * @brief Starts a new frame and clears both screens.
 */
void renderFrameBegin(void);

/**
 * @brief Selects the top screen as drawing target.
 */
void renderSceneTop(void);

/**
 * @brief Selects the bottom screen as drawing target.
 */
void renderSceneBottom(void);

/**
 * @brief Ends the current frame and presents it.
 */
void renderFrameEnd(void);

/**
 * @brief Loads a sprite sheet from a t3x file (with nearest filtering).
 * @return The sprite sheet or NULL on failure.
 */
C2D_SpriteSheet renderLoadSheet(const char* path);

/**
 * @brief Gets an image of a sprite sheet.
 */
C2D_Image renderSheetImage(C2D_SpriteSheet sheet, size_t index);

/**
 * @brief Draws a whole image.
 */
void renderDrawImage(C2D_Image img, float x, float y, float scaleX, float scaleY);

/**
 * @brief Draws a part of an image (in pixels of the source image).
 */
void renderDrawImagePart(C2D_Image img, float x, float y, int sx, int sy, int sw, int sh);

/**
 * @brief Draws a solid rectangle.
 */
void renderDrawRect(float x, float y, float w, float h, u32 color);

#ifdef __cplusplus
}
#endif

#endif // RENDER_H
