#pragma once

#include "save.h"

typedef enum
{
	EVENT_CAT_GEN1 = 0,
	EVENT_CAT_GEN2 = 1,
	EVENT_CAT_PCNY = 2,
	EVENT_CAT_EGGS = 3,
} EventCategory;

typedef enum
{
	EVENT_TID_RANDOM = 0,
	EVENT_TID_FIXED  = 1,
	EVENT_TID_PLAYER = 2,
} EventTidMode;

typedef enum
{
	EVENT_DV_FIXED = 0,
	EVENT_DV_RANDOM = 1,
	EVENT_DV_SHINY_GEN2 = 2,
	EVENT_DV_RANDOM_NON_SHINY_GEN2 = 3,
} EventDvMode;

typedef enum
{
	EVENT_DEST_PC_BOX = 0,
	EVENT_DEST_PARTY  = 1,
	EVENT_DEST_BANK   = 2,
} EventInjectDest;

typedef struct
{
	const char* region_label;
	const char* year_label;
	const char* method_label;
	const char* badge;
	const char* home_note;
} EventUiMeta;

typedef struct
{
	const char* id;
	const char* title;
	const char* short_name;
	uint8_t gen;          // 1 or 2
	EventCategory category;
	uint8_t species_id;   // National Dex 1..251
	uint8_t level;
	uint8_t moves[4];
	uint8_t ot_count;     // 0 = inherits active save's trainer OT
	const char* ot_options[8];
	EventTidMode tid_mode;
	uint16_t fixed_tid;
	EventDvMode dv_mode;
	uint8_t fixed_dvs[4]; // {atk, def, spe, spc}
	uint8_t gen1_cr_or_g2_item;
	bool is_egg;
	EventUiMeta ui_meta;
} HistoricalEvent;

uint16_t eventsGetCount(void);
const HistoricalEvent* eventsGet(uint16_t index);
uint16_t eventsGetByCategory(EventCategory cat, uint16_t* outIndices, uint16_t maxOut);
bool eventAllowsManualShiny(const HistoricalEvent* ev);
void eventBuildPokemon(const HistoricalEvent* ev, uint8_t otIdx, bool forceShiny, SAV_Pokemon* outPkm);
int16_t eventInject(const HistoricalEvent* ev, uint8_t otIdx, bool forceShiny, EventInjectDest dest, char* outMsg1, char* outMsg2);
const char* eventGetDvLabel(const HistoricalEvent* ev, bool forceShiny);
const char* eventGetItemOrCrLabel(const HistoricalEvent* ev);
