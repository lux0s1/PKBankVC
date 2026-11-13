#pragma once
/**
 * @file ts.h
 * @brief Title Selector Module for Pokémon titles
 */
#ifndef TS_H
#define TS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "am.h"

extern AM_TitleMediaEntry titleEntry;

/**
 * @brief Starts the loop of the title selector.
 * @return Whether the title selector has selected a title.
 */
bool TS_Loop(void);

/**
 * @brief Displays a message box when the selected game has no save data.
 */
void TS_DrawNoSaveMessage(void);

#ifdef __cplusplus
}
#endif

#endif // TS_H
