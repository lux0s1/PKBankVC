#include "save.h"
#include "bank.h"
#include "pkdir.h"
#include "pokedex.h"
#include "personal.h"
#include "fs.h"
#include "items.h"

#include <3ds/result.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dirent.h>

/// 
typedef enum
{
	/* Game (Gen I) */
	OFFSET_PARTY_FSAV = 0x2F2C,			///< Party PokÃ©mon list
	OFFSET_PARTY_JSAV = 0x2ED5,			///< Party PokÃ©mon list
	OFFSET_CURRENT_IDX_FSAV = 0x284C,	///< Current Box index PokÃ©mon list
	OFFSET_CURRENT_IDX_JSAV = 0x2842,	///< Current Box index PokÃ©mon list
	OFFSET_CURRENT_FSAV = 0x30C0,		///< Current Box PokÃ©mon list
	OFFSET_CURRENT_JSAV = 0x302D,		///< Current Box PokÃ©mon list
	OFFSET_BOX_1 = 0x4000,				///< First group of PokÃ©mon list
	OFFSET_BOX_2 = 0x6000,				///< Second group of PokÃ©mon list

	/* Game (Gen II) */
	OFFSET_CURRENT_IDX_GS_FSAV = 0x2724,
	OFFSET_CURRENT_IDX_GS_JSAV = 0x2705,
	OFFSET_CURRENT_IDX_C_FSAV  = 0x2700,
	OFFSET_CURRENT_IDX_C_JSAV  = 0x26E2,
	OFFSET_CURRENT_GS_FSAV     = 0x2D6C,
	OFFSET_CURRENT_GSC_OTHER   = 0x2D10,

	/* Bank */
	OFFSET_BBOX_1 = 0x100,			///< First bank group of PokÃ©mon list
} SAV_PokemonListOffset;

/// 
typedef enum
{
	/* Game */
	OFFSET_POCKET_FSAV = 0x25C9,		///< Pocket item list
	OFFSET_POCKET_JSAV = 0x25C4,		///< Pocket item list
	OFFSET_STORAGE_FSAV = 0x27E6,	///< Storage item list
	OFFSET_STORAGE_JSAV = 0x27DC,	///< Storage item list

	OFFSET_BSTORAGE = 0x000,		///< Bank storage item list
} SAV_ItemListOffset;

///
typedef enum
{
	/* Gen I */
	OFFSET_OT_NAME_FSAV = 0x2598,		//< Trainer name
	OFFSET_OT_NAME_JSAV = 0x2598,		//< Trainer name
	OFFSET_RIVAL_NAME_FSAV = 0x25F6,		//< Rival name
	OFFSET_RIVAL_NAME_JSAV = 0x25F1,		//< Rival name
	OFFSET_POKEDEX_OWNED_FSAV = 0x25A3,	//< PokÃ©dex owned
	OFFSET_POKEDEX_OWNED_JSAV = 0x259E,	//< PokÃ©dex owned
	OFFSET_POKEDEX_SEEN_FSAV = 0x25B6,	//< PokÃ©dex seen
	OFFSET_POKEDEX_SEEN_JSAV = 0x25B1,	//< PokÃ©dex seen
	OFFSET_CHECKSUM_FSAV = 0x3523,		//< Checksum
	OFFSET_CHECKSUM_JSAV = 0x3594,		//< Checksum

	/* Gen II */
	OFFSET_OT_NAME_GSC = 0x200B,
	OFFSET_RIVAL_NAME_GSC_FSAV = 0x2021,
	OFFSET_RIVAL_NAME_GSC_JSAV = 0x2017,
	OFFSET_POKEDEX_OWNED_GS_FSAV = 0x2A4C,
	OFFSET_POKEDEX_SEEN_GS_FSAV  = 0x2A6C,
	OFFSET_POKEDEX_OWNED_C_FSAV  = 0x2A27,
	OFFSET_POKEDEX_SEEN_C_FSAV   = 0x2A47,
	OFFSET_POKEDEX_OWNED_GS_JSAV = 0x29CE,
	OFFSET_POKEDEX_SEEN_GS_JSAV  = 0x29EE,
	OFFSET_POKEDEX_OWNED_C_JSAV  = 0x29AA,
	OFFSET_POKEDEX_SEEN_C_JSAV   = 0x29CA,
} SAV_Offset;

uint64_t titleid;
uint8_t save[SAVE_SIZE];
uint8_t bank[BANK_SIZE];
SAV_Game sgame;
SAV_Bank sbank;
SAV_GameVersion vgame;
SAV_GameLang lgame;
static uint32_t saveFileSize = 0x8000;

Result saveLoad(const char* path)
{
	Result ret;

	memset(&sgame, 0, sizeof(SAV_Game));
	memset(save, 0, SAVE_SIZE);

	vgame = saveGetGameVersion(titleid);
	lgame = saveGetGameLang(titleid);
	if (vgame == NOT_POKEMON || lgame == NOT_POKEMON) return -2;

	ret = saveReadFile(save, path ? path : ROOT_FOLDER SAVEGB_FILE);
	if (R_FAILED(ret)) return ret;

	saveReadData(save, &sgame, lgame);
	bankReadData(bank, &sbank, bankReadFile(bank, BANK_FOLDER BANKGB_FILE));
	
	return (true && true ? 0 : -5);
}

void saveSave(const char* path)
{
	saveWriteData(save, &sgame);
	saveWriteFile(save, path ? path : ROOT_FOLDER SAVEGB_FILE);

	mkdir(BASE_FOLDER, 0700);
	mkdir(BANK_FOLDER, 0700);

	bankWriteData(bank, &sbank);
	bankWriteFile(bank, BANK_FOLDER BANKGB_FILE);
	bankStoreSave();

	memset(&sgame, 0, sizeof(SAV_Game));
	memset(save, 0, SAVE_SIZE);
}

SAV_GameVersion saveGetGameVersion(uint64_t titleid)
{
	switch (titleid)
	{
		case 0x0004000000170C00: ///< PokÃ©mon Red (JPN) (JP)
		case 0x0004000000171000: ///< PokÃ©mon Red (FREE) (EN)
		case 0x0004000000171300: ///< PokÃ©mon Red (EUR) (DE)
		case 0x0004000000171600: ///< PokÃ©mon Red (EUR) (FR)
		case 0x0004000000171900: ///< PokÃ©mon Red (EUR) (ES)
		case 0x0004000000171C00: ///< PokÃ©mon Red (EUR) (IT)
			return POKEMON_RED;
		case 0x0004000000170D00: ///< PokÃ©mon Green (JPN) (JP)
			return POKEMON_GREEN;
		case 0x0004000000170E00: ///< PokÃ©mon Blue (JPN) (JP)
		case 0x0004000000171100: ///< PokÃ©mon Blue (FREE) (EN)
		case 0x0004000000171400: ///< PokÃ©mon Blue (EUR) (DE)
		case 0x0004000000171700: ///< PokÃ©mon Blue (EUR) (FR)
		case 0x0004000000171A00: ///< PokÃ©mon Blue (EUR) (ES)
		case 0x0004000000171D00: ///< PokÃ©mon Blue (EUR) (IT)
			return POKEMON_BLUE;
		case 0x0004000000170F00: ///< PokÃ©mon Yellow (JPN) (JP)
		case 0x0004000000171500: ///< PokÃ©mon Yellow (EUR) (DE)
		case 0x0004000000171200: ///< PokÃ©mon Yellow (FREE) (EN)
		case 0x0004000000171800: ///< PokÃ©mon Yellow (EUR) (FR)
		case 0x0004000000171B00: ///< PokÃ©mon Yellow (EUR) (ES)
		case 0x0004000000171E00: ///< PokÃ©mon Yellow (EUR) (IT)
			return POKEMON_YELLOW;
		case 0x0004000000172300: ///< PokÃ©mon Gold (JPN) (JP)
		case 0x0004000000172600: ///< PokÃ©mon Gold (FREE) (EN)
		case 0x0004000000172900: ///< PokÃ©mon Gold (EUR) (DE)
		case 0x0004000000172C00: ///< PokÃ©mon Gold (EUR) (FR)
		case 0x0004000000172F00: ///< PokÃ©mon Gold (EUR) (ES)
		case 0x0004000000173200: ///< PokÃ©mon Gold (EUR) (IT)
			return POKEMON_GOLD;
		case 0x0004000000172400: ///< PokÃ©mon Silver (JPN) (JP)
		case 0x0004000000172700: ///< PokÃ©mon Silver (FREE) (EN)
		case 0x0004000000172A00: ///< PokÃ©mon Silver (EUR) (DE)
		case 0x0004000000172D00: ///< PokÃ©mon Silver (EUR) (FR)
		case 0x0004000000173000: ///< PokÃ©mon Silver (EUR) (ES)
		case 0x0004000000173300: ///< PokÃ©mon Silver (EUR) (IT)
			return POKEMON_SILVER;
		case 0x0004000000172500: ///< PokÃ©mon Crystal (JPN) (JP)
		case 0x0004000000172800: ///< PokÃ©mon Crystal (FREE) (EN)
		case 0x0004000000172B00: ///< PokÃ©mon Crystal (EUR) (DE)
		case 0x0004000000172E00: ///< PokÃ©mon Crystal (EUR) (FR)
		case 0x0004000000173100: ///< PokÃ©mon Crystal (EUR) (ES)
		case 0x0004000000173400: ///< PokÃ©mon Crystal (EUR) (IT)
			return POKEMON_CRYSTAL;
		default:
			return NOT_POKEMON;
	}
}

SAV_GameLang saveGetGameLang(uint64_t titleid)
{
	switch (titleid)
	{
		case 0x0004000000170C00: ///< PokÃ©mon Red (JPN) (JP)
		case 0x0004000000170D00: ///< PokÃ©mon Green (JPN) (JP)
		case 0x0004000000170E00: ///< PokÃ©mon Blue (JPN) (JP)
		case 0x0004000000170F00: ///< PokÃ©mon Yellow (JPN) (JP)
		case 0x0004000000172300: ///< PokÃ©mon Gold (JPN) (JP)
		case 0x0004000000172400: ///< PokÃ©mon Silver (JPN) (JP)
		case 0x0004000000172500: ///< PokÃ©mon Crystal (JPN) (JP)
			return POKEMON_JP;
		case 0x0004000000171000: ///< PokÃ©mon Red (FREE) (EN)
		case 0x0004000000171100: ///< PokÃ©mon Blue (FREE) (EN)
		case 0x0004000000171200: ///< PokÃ©mon Yellow (FREE) (EN)
		case 0x0004000000172600: ///< PokÃ©mon Gold (FREE) (EN)
		case 0x0004000000172700: ///< PokÃ©mon Silver (FREE) (EN)
		case 0x0004000000172800: ///< PokÃ©mon Crystal (FREE) (EN)
			return POKEMON_EN;
		case 0x0004000000171600: ///< PokÃ©mon Red (EUR) (FR)
		case 0x0004000000171700: ///< PokÃ©mon Blue (EUR) (FR)
		case 0x0004000000171800: ///< PokÃ©mon Yellow (EUR) (FR)
		case 0x0004000000172C00: ///< PokÃ©mon Gold (EUR) (FR)
		case 0x0004000000172D00: ///< PokÃ©mon Silver (EUR) (FR)
		case 0x0004000000172E00: ///< PokÃ©mon Crystal (EUR) (FR)
			return POKEMON_FR;
		case 0x0004000000171300: ///< PokÃ©mon Red (EUR) (DE)
		case 0x0004000000171400: ///< PokÃ©mon Blue (EUR) (DE)
		case 0x0004000000171500: ///< PokÃ©mon Yellow (EUR) (DE)
		case 0x0004000000172900: ///< PokÃ©mon Gold (EUR) (DE)
		case 0x0004000000172A00: ///< PokÃ©mon Silver (EUR) (DE)
		case 0x0004000000172B00: ///< PokÃ©mon Crystal (EUR) (DE)
			return POKEMON_DE;
		case 0x0004000000171C00: ///< PokÃ©mon Red (EUR) (IT)
		case 0x0004000000171D00: ///< PokÃ©mon Blue (EUR) (IT)
		case 0x0004000000171E00: ///< PokÃ©mon Yellow (EUR) (IT)
		case 0x0004000000173200: ///< PokÃ©mon Gold (EUR) (IT)
		case 0x0004000000173300: ///< PokÃ©mon Silver (EUR) (IT)
		case 0x0004000000173400: ///< PokÃ©mon Crystal (EUR) (IT)
			return POKEMON_IT;
		case 0x0004000000171B00: ///< PokÃ©mon Yellow (EUR) (ES)
		case 0x0004000000171900: ///< PokÃ©mon Red (EUR) (ES)
		case 0x0004000000171A00: ///< PokÃ©mon Blue (EUR) (ES)
		case 0x0004000000172F00: ///< PokÃ©mon Gold (EUR) (ES)
		case 0x0004000000173000: ///< PokÃ©mon Silver (EUR) (ES)
		case 0x0004000000173100: ///< PokÃ©mon Crystal (EUR) (ES)
			return POKEMON_ES;
		default:
			return NOT_POKEMON;
	}
}

/**
 * @brief Reads a PokÃ©mon from the savedata buffer.
 * @param[in] save The savedata buffer to the list offset.
 * @param[out] pkm The PokÃ©mon.
 * @param index The index in the list.
 * @param sizePkm The size of a PokÃ©mon struct (0x20 Gen II, 0x21 Gen I / Bank, 0x2C Party).
 * @param sizeName The size of a name.
 * @param capacity The capacity of the list.
 */
static void saveExtractPokemon(const uint8_t* save, SAV_Pokemon* pkm, uint8_t index, uint8_t sizePkm, uint8_t sizeName, uint8_t capacity)
{
	const uint8_t* pkmbuf;
	memset(pkm, 0, sizeof(SAV_Pokemon));

	if (sizePkm == 0x20)
	{
		// Gen II Box PokÃ©mon (32 bytes)
		uint8_t listSpecies = (save + 1 + index)[0];
		pkmbuf = save + 2 + capacity + index * sizePkm;

		if (listSpecies == 0x00 || listSpecies == 0xFF || (listSpecies > PKM_COUNT && listSpecies != 0xFD))
		{
			pkm->species = 0xFF;
			pkm->speciesIndex = 0xFF;
			pkm->nationalDex = 0;
			pkm->isEgg = false;
			pkm->moved = false;
			return;
		}

		pkm->isEgg = (listSpecies == 0xFD);
		pkm->nationalDex = pkmbuf[0x00];
		if (pkm->nationalDex == 0 || pkm->nationalDex > PKM_COUNT)
		{
			pkm->nationalDex = (listSpecies <= PKM_COUNT ? listSpecies : 0);
		}
		pkm->species = pokedexGetIndex(pkm->nationalDex);
		pkm->speciesIndex = pkm->species;

		pkm->heldItem = pkmbuf[0x01];
		pkm->moves[0] = pkmbuf[0x02];
		pkm->moves[1] = pkmbuf[0x03];
		pkm->moves[2] = pkmbuf[0x04];
		pkm->moves[3] = pkmbuf[0x05];
		pkm->originalTID = pkmbuf[0x06] << 8 | pkmbuf[0x07];
		pkm->experience = pkmbuf[0x08] << 16 | pkmbuf[0x09] << 8 | pkmbuf[0x0A];
		pkm->EVs[0] = pkmbuf[0x0B] << 8 | pkmbuf[0x0C];
		pkm->EVs[1] = pkmbuf[0x0D] << 8 | pkmbuf[0x0E];
		pkm->EVs[2] = pkmbuf[0x0F] << 8 | pkmbuf[0x10];
		pkm->EVs[3] = pkmbuf[0x11] << 8 | pkmbuf[0x12];
		pkm->EVs[4] = pkmbuf[0x13] << 8 | pkmbuf[0x14];
		pkm->IVs[1] = pkmbuf[0x15] >> 4;
		pkm->IVs[2] = pkmbuf[0x15] & 0x0F;
		pkm->IVs[3] = pkmbuf[0x16] >> 4;
		pkm->IVs[4] = pkmbuf[0x16] & 0x0F;
		pkm->PPUps[0] = pkmbuf[0x17] >> 6;
		pkm->PPUps[1] = pkmbuf[0x18] >> 6;
		pkm->PPUps[2] = pkmbuf[0x19] >> 6;
		pkm->PPUps[3] = pkmbuf[0x1A] >> 6;
		pkm->PPs[0] = pkmbuf[0x17] & 0x3F;
		pkm->PPs[1] = pkmbuf[0x18] & 0x3F;
		pkm->PPs[2] = pkmbuf[0x19] & 0x3F;
		pkm->PPs[3] = pkmbuf[0x1A] & 0x3F;
		pkm->friendship = pkmbuf[0x1B];
		pkm->pokerus = pkmbuf[0x1C];
		pkm->caughtData = pkmbuf[0x1D] << 8 | pkmbuf[0x1E];
		pkm->currentLevel = pkmbuf[0x1F];
		pkm->level = pkm->currentLevel;
		pkm->status = STATUS_OK;

		pkm->IVs[STAT_HP] = ((pkm->IVs[STAT_ATK] & 0x1) << 3) | ((pkm->IVs[STAT_DEF] & 0x1) << 2) | ((pkm->IVs[STAT_SPE] & 0x1) << 1) | ((pkm->IVs[STAT_SPC] & 0x1) << 0);

		const PersonalInfo* pInfo = Personal(pkm->nationalDex);
		pkm->types[0] = pInfo->type1;
		pkm->types[1] = pInfo->type2;
		pkm->catchRate = (pkm->heldItem != 0 ? pkm->heldItem : pInfo->catchRate);

		pkm->maxHP = ((((pInfo->HP + pkm->IVs[STAT_HP]) * 2 + sqrt(pkm->EVs[STAT_HP]) / 4) * pkm->level) / 100) + pkm->level + 10;
		pkm->currentHP = pkm->maxHP;
		pkm->ATK = ((((pInfo->ATK + pkm->IVs[STAT_ATK]) * 2 + sqrt(pkm->EVs[STAT_ATK]) / 4) * pkm->level) / 100) + 5;
		pkm->DEF = ((((pInfo->DEF + pkm->IVs[STAT_DEF]) * 2 + sqrt(pkm->EVs[STAT_DEF]) / 4) * pkm->level) / 100) + 5;
		pkm->SPE = ((((pInfo->SPE + pkm->IVs[STAT_SPE]) * 2 + sqrt(pkm->EVs[STAT_SPE]) / 4) * pkm->level) / 100) + 5;
		pkm->SPC = ((((pInfo->SPC + pkm->IVs[STAT_SPC]) * 2 + sqrt(pkm->EVs[STAT_SPC]) / 4) * pkm->level) / 100) + 5;
		pkm->SPD = ((((pInfo->SPD + pkm->IVs[STAT_SPC]) * 2 + sqrt(pkm->EVs[STAT_SPC]) / 4) * pkm->level) / 100) + 5;
	}
	else
	{
		// Gen I / Bank Box PokÃ©mon (33 bytes) or Party (44 bytes)
		pkmbuf = save + 1 + index;

		pkm->species = pkmbuf[0x00];
		pkm->nationalDex = pokedexGetNational(pkm->species);
		pkm->isEgg = false;

		if (pkm->species == 0x00 || pkm->species == 0xFF || pkm->nationalDex == 0)
		{
			pkm->species = 0xFF;
			pkm->speciesIndex = 0xFF;
			pkm->nationalDex = 0;
			return;
		}

		pkmbuf = save + 2 + capacity + index * sizePkm;

		pkm->speciesIndex = pkmbuf[0x00];
		pkm->currentHP = pkmbuf[0x01] << 8 | pkmbuf[0x02];
		pkm->currentLevel = pkmbuf[0x03];
		pkm->status = pkmbuf[0x04];
		pkm->types[0] = pkmbuf[0x05];
		pkm->types[1] = pkmbuf[0x06];
		pkm->catchRate = pkmbuf[0x07];
		pkm->moves[0] = pkmbuf[0x08];
		pkm->moves[1] = pkmbuf[0x09];
		pkm->moves[2] = pkmbuf[0x0A];
		pkm->moves[3] = pkmbuf[0x0B];
		pkm->originalTID = pkmbuf[0x0C] << 8 | pkmbuf[0x0D];
		pkm->experience = pkmbuf[0x0E] << 16 | pkmbuf[0x0F] << 8 | pkmbuf[0x10];
		pkm->EVs[0] = pkmbuf[0x11] << 8 | pkmbuf[0x12];
		pkm->EVs[1] = pkmbuf[0x13] << 8 | pkmbuf[0x14];
		pkm->EVs[2] = pkmbuf[0x15] << 8 | pkmbuf[0x16];
		pkm->EVs[3] = pkmbuf[0x17] << 8 | pkmbuf[0x18];
		pkm->EVs[4] = pkmbuf[0x19] << 8 | pkmbuf[0x1A];
		pkm->IVs[1] = pkmbuf[0x1B] >> 4;
		pkm->IVs[2] = pkmbuf[0x1B] & 0x0F;
		pkm->IVs[3] = pkmbuf[0x1C] >> 4;
		pkm->IVs[4] = pkmbuf[0x1C] & 0x0F;
		pkm->PPUps[0] = pkmbuf[0x1D] >> 6;
		pkm->PPUps[1] = pkmbuf[0x1E] >> 6;
		pkm->PPUps[2] = pkmbuf[0x1F] >> 6;
		pkm->PPUps[3] = pkmbuf[0x20] >> 6;
		pkm->PPs[0] = pkmbuf[0x1D] & 0x3F;
		pkm->PPs[1] = pkmbuf[0x1E] & 0x3F;
		pkm->PPs[2] = pkmbuf[0x1F] & 0x3F;
		pkm->PPs[3] = pkmbuf[0x20] & 0x3F;

		pkm->IVs[STAT_HP] = ((pkm->IVs[STAT_ATK] & 0x1) << 3) | ((pkm->IVs[STAT_DEF] & 0x1) << 2) | ((pkm->IVs[STAT_SPE] & 0x1) << 1) | ((pkm->IVs[STAT_SPC] & 0x1) << 0);

		const PersonalInfo* pInfo = Personal(pkm->nationalDex);
		pkm->heldItem = 0;
		pkm->friendship = 70;
		pkm->pokerus = 0;
		pkm->caughtData = 0;

		if (sizePkm == 0x2C)
		{
			pkm->level = pkmbuf[0x21];
			pkm->maxHP = pkmbuf[0x22] << 8 | pkmbuf[0x23];
			pkm->ATK = pkmbuf[0x24] << 8 | pkmbuf[0x25];
			pkm->DEF = pkmbuf[0x26] << 8 | pkmbuf[0x27];
			pkm->SPE = pkmbuf[0x28] << 8 | pkmbuf[0x29];
			pkm->SPC = pkmbuf[0x2A] << 8 | pkmbuf[0x2B];
			pkm->SPD = ((((pInfo->SPD + pkm->IVs[STAT_SPC]) * 2 + sqrt(pkm->EVs[STAT_SPC]) / 4) * pkm->level) / 100) + 5;
		}
		else
		{
			pkm->level = pkm->currentLevel;
			pkm->maxHP = ((((pInfo->HP + pkm->IVs[STAT_HP]) * 2 + sqrt(pkm->EVs[STAT_HP]) / 4) * pkm->level) / 100) + pkm->level + 10;
			pkm->ATK = ((((pInfo->ATK + pkm->IVs[STAT_ATK]) * 2 + sqrt(pkm->EVs[STAT_ATK]) / 4) * pkm->level) / 100) + 5;
			pkm->DEF = ((((pInfo->DEF + pkm->IVs[STAT_DEF]) * 2 + sqrt(pkm->EVs[STAT_DEF]) / 4) * pkm->level) / 100) + 5;
			pkm->SPE = ((((pInfo->SPE + pkm->IVs[STAT_SPE]) * 2 + sqrt(pkm->EVs[STAT_SPE]) / 4) * pkm->level) / 100) + 5;
			pkm->SPC = ((((pInfo->SPC + pkm->IVs[STAT_SPC]) * 2 + sqrt(pkm->EVs[STAT_SPC]) / 4) * pkm->level) / 100) + 5;
			pkm->SPD = ((((pInfo->SPD + pkm->IVs[STAT_SPC]) * 2 + sqrt(pkm->EVs[STAT_SPC]) / 4) * pkm->level) / 100) + 5;
		}

		if (pkm->nationalDex > SPECIES_MEW)
		{
			pkm->types[0] = pInfo->type1;
			pkm->types[1] = pInfo->type2;
			if (pkm->currentHP == 0 || pkm->currentHP > pkm->maxHP)
				pkm->currentHP = pkm->maxHP;
		}
	}

	// Count (1) + Species (capacity+1) + PokÃ©mon (capacity*sizePkm) + OT_Name.index (index*sizeName)
	pkmbuf = save + 2 + capacity * (sizePkm + 1) + (index) * sizeName;

	for (uint8_t i = 0; i < sizeName; i++)
		pkm->nameOT[i] = pkmbuf[i];
	if (sizeName > 0 && sizeName <= sizeof(pkm->nameOT)) pkm->nameOT[sizeName-1] = 0x50;

	// Count (1) + Species (capacity+1) + PokÃ©mon (capacity*sizePkm) + OT_Name (capacity*sizeName) + NK_Name.index (index*sizeName)
	pkmbuf = save + 2 + capacity * (sizePkm + 1) + (capacity + index) * sizeName;

	for (uint8_t i = 0; i < sizeName; i++)
		pkm->nameNK[i] = pkmbuf[i];
	if (sizeName > 0 && sizeName <= sizeof(pkm->nameNK)) pkm->nameNK[sizeName-1] = 0x50;

	pkm->moved = false;
}

/**
 * @brief Writes a PokÃ©mon to the savedata buffer.
 * @param[in/out] save The savedata buffer to the list offset.
 * @param[in] pkm The PokÃ©mon.
 * @param index The index in the list.
 * @param sizePkm The size of a PokÃ©mon struct (0x20 Gen II, 0x21 Gen I / Bank, 0x2C Party).
 * @param sizeName The size of a name.
 * @param capacity The capacity of the list.
 */
static void saveInjectPokemon(uint8_t* save, const SAV_Pokemon* pkm, uint8_t index, uint8_t sizePkm, uint8_t sizeName, uint8_t capacity)
{
	if (!pkm->moved) return;

	uint8_t* pkmbuf;

	if (sizePkm == 0x20)
	{
		// Gen II Box PokÃ©mon (32 bytes)
		pkmbuf = save + 1 + index;
		if (saveIsPkmEmpty(pkm))
		{
			pkmbuf[0x00] = 0xFF;
			pkmbuf = save + 2 + capacity + index * sizePkm;
			memset(pkmbuf, 0, sizePkm);
			return;
		}

		pkmbuf[0x00] = pkm->isEgg ? 0xFD : pkm->nationalDex;

		pkmbuf = save + 2 + capacity + index * sizePkm;
		pkmbuf[0x00] = pkm->nationalDex;
		pkmbuf[0x01] = pkm->heldItem;
		pkmbuf[0x02] = pkm->moves[0];
		pkmbuf[0x03] = pkm->moves[1];
		pkmbuf[0x04] = pkm->moves[2];
		pkmbuf[0x05] = pkm->moves[3];
		pkmbuf[0x06] = pkm->originalTID >> 8;
		pkmbuf[0x07] = pkm->originalTID & 0xFF;
		pkmbuf[0x08] = pkm->experience >> 16 & 0xFF;
		pkmbuf[0x09] = pkm->experience >> 8 & 0xFF;
		pkmbuf[0x0A] = pkm->experience & 0xFF;
		pkmbuf[0x0B] = pkm->EVs[0] >> 8;
		pkmbuf[0x0C] = pkm->EVs[0] & 0xFF;
		pkmbuf[0x0D] = pkm->EVs[1] >> 8;
		pkmbuf[0x0E] = pkm->EVs[1] & 0xFF;
		pkmbuf[0x0F] = pkm->EVs[2] >> 8;
		pkmbuf[0x10] = pkm->EVs[2] & 0xFF;
		pkmbuf[0x11] = pkm->EVs[3] >> 8;
		pkmbuf[0x12] = pkm->EVs[3] & 0xFF;
		pkmbuf[0x13] = pkm->EVs[4] >> 8;
		pkmbuf[0x14] = pkm->EVs[4] & 0xFF;
		pkmbuf[0x15] = (pkm->IVs[1] & 0x0F) << 4 | (pkm->IVs[2] & 0x0F);
		pkmbuf[0x16] = (pkm->IVs[3] & 0x0F) << 4 | (pkm->IVs[4] & 0x0F);
		pkmbuf[0x17] = (pkm->PPUps[0] & 0x3) << 6 | (pkm->PPs[0] & 0x3F);
		pkmbuf[0x18] = (pkm->PPUps[1] & 0x3) << 6 | (pkm->PPs[1] & 0x3F);
		pkmbuf[0x19] = (pkm->PPUps[2] & 0x3) << 6 | (pkm->PPs[2] & 0x3F);
		pkmbuf[0x1A] = (pkm->PPUps[3] & 0x3) << 6 | (pkm->PPs[3] & 0x3F);
		pkmbuf[0x1B] = (pkm->friendship ? pkm->friendship : 70);
		pkmbuf[0x1C] = pkm->pokerus;
		pkmbuf[0x1D] = pkm->caughtData >> 8;
		pkmbuf[0x1E] = pkm->caughtData & 0xFF;
		pkmbuf[0x1F] = pkm->currentLevel;
	}
	else
	{
		// Gen I / Bank Box PokÃ©mon (33 bytes) or Party (44 bytes)
		pkmbuf = save + 1 + index;
		if (saveIsPkmEmpty(pkm))
		{
			pkmbuf[0x00] = 0xFF;
			pkmbuf = save + 2 + capacity + index * sizePkm;
			memset(pkmbuf, 0, sizePkm);
			return;
		}

		uint8_t idx = pokedexGetIndex(pkm->nationalDex);
		pkmbuf[0x00] = idx;

		pkmbuf = save + 2 + capacity + index * sizePkm;

		uint8_t type0 = pkm->types[0];
		uint8_t type1 = pkm->types[1];
		if (vgame <= POKEMON_YELLOW && (type0 == TYPE_STEEL || type0 == TYPE_DARK || type1 == TYPE_STEEL || type1 == TYPE_DARK))
		{
			// Convert Magnemite/Magneton Steel secondary type back to Electric if transferred to Gen I
			if (pkm->nationalDex == SPECIES_MAGNEMITE || pkm->nationalDex == SPECIES_MAGNETON)
			{
				type0 = TYPE_ELECTRIC;
				type1 = TYPE_ELECTRIC;
			}
		}

		pkmbuf[0x00] = idx;
		pkmbuf[0x01] = pkm->currentHP >> 8;
		pkmbuf[0x02] = pkm->currentHP & 0xFF;
		pkmbuf[0x03] = pkm->currentLevel;
		pkmbuf[0x04] = pkm->status;
		pkmbuf[0x05] = type0;
		pkmbuf[0x06] = type1;
		pkmbuf[0x07] = (pkm->catchRate ? pkm->catchRate : Personal(pkm->nationalDex)->catchRate);
		pkmbuf[0x08] = pkm->moves[0];
		pkmbuf[0x09] = pkm->moves[1];
		pkmbuf[0x0A] = pkm->moves[2];
		pkmbuf[0x0B] = pkm->moves[3];
		pkmbuf[0x0C] = pkm->originalTID >> 8;
		pkmbuf[0x0D] = pkm->originalTID & 0xFF;
		pkmbuf[0x0E] = pkm->experience >> 16 & 0xFF;
		pkmbuf[0x0F] = pkm->experience >> 8 & 0xFF;
		pkmbuf[0x10] = pkm->experience & 0xFF;
		pkmbuf[0x11] = pkm->EVs[0] >> 8;
		pkmbuf[0x12] = pkm->EVs[0] & 0xFF;
		pkmbuf[0x13] = pkm->EVs[1] >> 8;
		pkmbuf[0x14] = pkm->EVs[1] & 0xFF;
		pkmbuf[0x15] = pkm->EVs[2] >> 8;
		pkmbuf[0x16] = pkm->EVs[2] & 0xFF;
		pkmbuf[0x17] = pkm->EVs[3] >> 8;
		pkmbuf[0x18] = pkm->EVs[3] & 0xFF;
		pkmbuf[0x19] = pkm->EVs[4] >> 8;
		pkmbuf[0x1A] = pkm->EVs[4] & 0xFF;
		pkmbuf[0x1B] = (pkm->IVs[1] & 0x0F) << 4 | (pkm->IVs[2] & 0x0F);
		pkmbuf[0x1C] = (pkm->IVs[3] & 0x0F) << 4 | (pkm->IVs[4] & 0x0F);
		pkmbuf[0x1D] = (pkm->PPUps[0] & 0x3) << 6 | (pkm->PPs[0] & 0x3F);
		pkmbuf[0x1E] = (pkm->PPUps[1] & 0x3) << 6 | (pkm->PPs[1] & 0x3F);
		pkmbuf[0x1F] = (pkm->PPUps[2] & 0x3) << 6 | (pkm->PPs[2] & 0x3F);
		pkmbuf[0x20] = (pkm->PPUps[3] & 0x3) << 6 | (pkm->PPs[3] & 0x3F);

		if (sizePkm == 0x2C)
		{
			pkmbuf[0x21] = pkm->level;
			pkmbuf[0x22] = pkm->maxHP >> 8;
			pkmbuf[0x23] = pkm->maxHP & 0xFF;
			pkmbuf[0x24] = pkm->ATK >> 8;
			pkmbuf[0x25] = pkm->ATK & 0xFF;
			pkmbuf[0x26] = pkm->DEF >> 8;
			pkmbuf[0x27] = pkm->DEF & 0xFF;
			pkmbuf[0x28] = pkm->SPE >> 8;
			pkmbuf[0x29] = pkm->SPE & 0xFF;
			pkmbuf[0x2A] = pkm->SPC >> 8;
			pkmbuf[0x2B] = pkm->SPC & 0xFF;
		}
	}

	// Count (1) + Species (capacity+1) + PokÃ©mon (capacity*sizePkm) + OT_Name.index (index*sizeName)
	pkmbuf = save + 2 + capacity * (sizePkm + 1) + (index) * sizeName;

	for (uint8_t i = 0; i < sizeName; i++)
		pkmbuf[i] = pkm->nameOT[i];
	if (sizeName > 0) pkmbuf[sizeName-1] = 0x50;

	// Count (1) + Species (capacity+1) + PokÃ©mon (capacity*sizePkm) + OT_Name (capacity*sizeName) + NK_Name.index (index*sizeName)
	pkmbuf = save + 2 + capacity * (sizePkm + 1) + (capacity + index) * sizeName;

	for (uint8_t i = 0; i < sizeName; i++)
		pkmbuf[i] = pkm->nameNK[i];
	if (sizeName > 0) pkmbuf[sizeName-1] = 0x50;
}

/**
 * @brief Reads the data from a save to a structure.
 * @param[in] save The savedata buffer.
 * @param[out] pkmList The PokÃ©mon list to populate.
 * @param listOffset The offset of the list.
 * @param sizePkm The size of a PokÃ©mon struct (pk1).
 * @param sizeName The size of a name.
 * @param capacity The capacity of the list.
 */
static void saveExtractPokemonList(const uint8_t* save, SAV_PokemonList* pkmList, uint32_t listOffset, uint8_t sizePkm, uint8_t sizeName, uint8_t capacity)
{
	pkmList->count = save[listOffset];
	if (pkmList->count > capacity || save[listOffset + 1] == 0xFF || save[listOffset + 1] == 0x00)
		pkmList->count = 0;
	pkmList->sizePkm = sizePkm;
	pkmList->sizeName = sizeName;
	pkmList->capacity = capacity;

	for (uint8_t i = 0; i < pkmList->capacity; i++)
	{
		if (i >= pkmList->count)
		{
			memset(&pkmList->slots[i], 0, sizeof(SAV_Pokemon));
			pkmList->slots[i].species = 0xFF;
			pkmList->slots[i].speciesIndex = 0xFF;
		}
		else
		{
			saveExtractPokemon(save + listOffset, &pkmList->slots[i], i, pkmList->sizePkm, pkmList->sizeName, pkmList->capacity);
		}
	}
}

/**
 * @brief Writes the data from a structure to a save.
 * @param[in/out] save The savedata buffer.
 * @param[in] pkmList The PokÃ©mon list to inject.
 * @param listOffset The offset of the list.
 */
static void saveInjectPokemonList(uint8_t* save, const SAV_PokemonList* pkmList, uint32_t listOffset)
{
	save[listOffset] = pkmList->count;
	save[listOffset+pkmList->count+1] = 0xFF;

	for (uint8_t i = 0; i < pkmList->capacity; i++)
		saveInjectPokemon(save + listOffset, &pkmList->slots[i], i, pkmList->sizePkm, pkmList->sizeName, pkmList->capacity);
}

uint8_t saveGetCurrentBox(const uint8_t* save)
{
	uint8_t maxBoxes;
	uint8_t boxIdx;

	if (vgame >= POKEMON_GOLD)
	{
		maxBoxes = (lgame == POKEMON_JP ? 9 : 14);
		if (vgame == POKEMON_CRYSTAL)
			boxIdx = save[lgame == POKEMON_JP ? OFFSET_CURRENT_IDX_C_JSAV : OFFSET_CURRENT_IDX_C_FSAV] & 0x7F;
		else
			boxIdx = save[lgame == POKEMON_JP ? OFFSET_CURRENT_IDX_GS_JSAV : OFFSET_CURRENT_IDX_GS_FSAV] & 0x7F;
	}
	else
	{
		maxBoxes = (lgame == POKEMON_JP ? 8 : 12);
		boxIdx = (lgame == POKEMON_JP ? save[OFFSET_CURRENT_IDX_JSAV] : save[OFFSET_CURRENT_IDX_FSAV]) & 0x7F;
	}

	return (boxIdx < maxBoxes ? boxIdx : 0);
}

SAV_PokemonList* gameGetBox(uint8_t box)
{
	return &sgame.boxes[box];
}

SAV_PokemonList* bankGetBox(uint8_t box)
{
	return &sbank.boxes[box];
}

SAV_PokemonList* saveGetBox(uint8_t box, bool inBank)
{
	return (inBank ? bankGetBox(box) : gameGetBox(box));
}

SAV_Pokemon* saveGetPkm(uint8_t box, uint8_t slot, bool inBank)
{
	return &saveGetBox(box, inBank)->slots[slot];
}

/**
 * @brief Checks if a PokÃ©mon is compatible with the target destination.
 */
static bool canPlacePokemon(const SAV_Pokemon* pkm, bool inBank)
{
	if (saveIsPkmEmpty(pkm)) return true;

	// Prevent moving Eggs out of Gen II PC boxes into Bank or Gen I
	if (pkm->isEgg && (inBank || vgame <= POKEMON_YELLOW))
		return false;

	// Prevent moving Gen II species or Gen II moves or held items into a Gen I game box
	if (!inBank && vgame <= POKEMON_YELLOW)
	{
		if (pkm->heldItem != 0)
			return false;
		if (pkm->nationalDex > SPECIES_MEW)
			return false;
		for (uint8_t i = 0; i < 4; i++)
		{
			if (pkm->moves[i] > 165)
				return false;
		}
	}

	return true;
}

/**
 * @brief Swaps two PokÃ©mon.
 * @param[out] dst The PokÃ©mon destination.
 * @param[in] src The PokÃ©mon source.
 * @return Whether the PokÃ©mon have been swapped.
 */
static bool _saveMovePkm(SAV_Pokemon* dst, SAV_Pokemon* src)
{
	// We copy the pokemon data
	SAV_Pokemon tmp;
	memcpy(&tmp, dst, sizeof(SAV_Pokemon));
	memcpy(dst, src, sizeof(SAV_Pokemon));
	memcpy(src, &tmp, sizeof(SAV_Pokemon));

	// We valid the PokÃ©mon move'd
	src->moved = true;
	dst->moved = true;

	return true;
}

bool saveMovePkm(SAV_Pokemon* dst, SAV_Pokemon* src, bool srcBanked, bool dstBanked)
{
	if (!canPlacePokemon(src, dstBanked) || !canPlacePokemon(dst, srcBanked))
		return false;

	return _saveMovePkm(dst, src);
}

/**
 * @brief Pastes a PokÃ©mon over another.
 * @param[out] dst The PokÃ©mon destination.
 * @param[in] src The PokÃ©mon source.
 * @return Whether the PokÃ©mon has been pasted.
 */
static bool _savePastePkm(SAV_Pokemon* dst, SAV_Pokemon* src)
{
	// We copy the pokemon data
	memcpy(dst, src, sizeof(SAV_Pokemon));

	// We valid the PokÃ©mon move'd
	dst->moved = true;

	return true;
}

bool savePastePkm(SAV_Pokemon* dst, SAV_Pokemon* src, bool srcBanked, bool dstBanked)
{
	if (!canPlacePokemon(src, dstBanked))
		return false;

	return _savePastePkm(dst, src);
}

bool saveIsPkmEmpty(const SAV_Pokemon* pkm)
{
	return pkm->nationalDex == 0 || pkm->nationalDex > PKM_COUNT;
}

const char8_t* saveGetTrainerName(void)
{
	if (vgame >= POKEMON_GOLD)
		return save + OFFSET_OT_NAME_GSC;
	return save + (lgame == POKEMON_JP ? OFFSET_OT_NAME_JSAV : OFFSET_OT_NAME_FSAV);
}

const char8_t* saveGetRivalName(void)
{
	if (vgame >= POKEMON_GOLD)
		return save + (lgame == POKEMON_JP ? OFFSET_RIVAL_NAME_GSC_JSAV : OFFSET_RIVAL_NAME_GSC_FSAV);
	return save + (lgame == POKEMON_JP ? OFFSET_RIVAL_NAME_JSAV : OFFSET_RIVAL_NAME_FSAV);
}

uint8_t* saveGetPokedexOwned(void)
{
	if (vgame >= POKEMON_GOLD)
	{
		if (vgame == POKEMON_CRYSTAL)
			return save + (lgame == POKEMON_JP ? OFFSET_POKEDEX_OWNED_C_JSAV : OFFSET_POKEDEX_OWNED_C_FSAV);
		return save + (lgame == POKEMON_JP ? OFFSET_POKEDEX_OWNED_GS_JSAV : OFFSET_POKEDEX_OWNED_GS_FSAV);
	}
	return save + (lgame == POKEMON_JP ? OFFSET_POKEDEX_OWNED_JSAV : OFFSET_POKEDEX_OWNED_FSAV);
}

uint8_t* saveGetPokedexSeen(void)
{
	if (vgame >= POKEMON_GOLD)
	{
		if (vgame == POKEMON_CRYSTAL)
			return save + (lgame == POKEMON_JP ? OFFSET_POKEDEX_SEEN_C_JSAV : OFFSET_POKEDEX_SEEN_C_FSAV);
		return save + (lgame == POKEMON_JP ? OFFSET_POKEDEX_SEEN_GS_JSAV : OFFSET_POKEDEX_SEEN_GS_FSAV);
	}
	return save + (lgame == POKEMON_JP ? OFFSET_POKEDEX_SEEN_JSAV : OFFSET_POKEDEX_SEEN_FSAV);
}

uint16_t saveGetGamePokedexOwnedCount(void)
{
	const uint8_t* owned = saveGetPokedexOwned();
	if (!owned) return 0;
	uint16_t maxSp = (vgame >= POKEMON_GOLD ? 251 : 151);
	uint16_t count = 0;
	for (uint16_t i = 1; i <= maxSp; i++)
	{
		uint8_t bit = (uint8_t)(i - 1);
		if ((owned[bit / 8] >> (bit % 8)) & 1)
			count++;
	}
	return count;
}

bool saveIsGamePokedexComplete(void)
{
	const uint8_t* owned = saveGetPokedexOwned();
	if (!owned) return false;
	uint16_t reqMax = (vgame >= POKEMON_GOLD ? 250 : 150);
	uint16_t count = 0;
	for (uint16_t i = 1; i <= reqMax; i++)
	{
		uint8_t bit = (uint8_t)(i - 1);
		if ((owned[bit / 8] >> (bit % 8)) & 1)
			count++;
	}
	return (count >= reqMax);
}

Result saveReadFile(uint8_t* save, const char* path)
{
	Result ret;
	uint32_t bytesRead = 0;

	printf("Reading save file...");
	ret = FS_ReadFile(path, save, SAVE_SIZE, saveArchive, &bytesRead);

	if (R_FAILED(ret)) printf(" ERROR\n");
	else
	{
		printf(" OK\n");
		saveFileSize = (bytesRead >= 0x8000 && bytesRead <= SAVE_SIZE ? bytesRead : 0x8000);
	}
	printf("  Read %ld/%d bytes\n", bytesRead, SAVE_SIZE);

	return ret;
}

Result saveWriteFile(const uint8_t* save, const char* path)
{
	Result ret;
	uint32_t bytesWritten = 0;

	printf("Deleting old save file...");
	ret = FS_DeleteFile(path, saveArchive);
	if (R_FAILED(ret)) printf(" ERROR\n");
	else printf(" OK\n");

	printf("Writing save file...");
	ret = FS_WriteFile(path, save, saveFileSize, saveArchive, &bytesWritten);
	if (R_FAILED(ret)) printf(" ERROR\n");
	else printf(" OK\n  Written %ld/%ld bytes\n", bytesWritten, saveFileSize);

	return bytesWritten;
}

static uint16_t saveGetBoxOffset(uint8_t i, uint16_t boxSize)
{
	if (vgame >= POKEMON_GOLD)
	{
		uint8_t split = (lgame == POKEMON_JP ? 6 : 7);
		uint16_t stride = boxSize + 2;
		return (i < split) ? (OFFSET_BOX_1 + i * stride) : (OFFSET_BOX_2 + (i - split) * stride);
	}
	else
	{
		uint8_t split = sgame.boxCount / 2;
		return (i < split ? OFFSET_BOX_1 : OFFSET_BOX_2 - split * boxSize) + i * boxSize;
	}
}

static uint16_t saveGetCurrentBoxOffset(void)
{
	if (vgame >= POKEMON_GOLD)
	{
		return (vgame != POKEMON_CRYSTAL && lgame != POKEMON_JP) ? OFFSET_CURRENT_GS_FSAV : OFFSET_CURRENT_GSC_OTHER;
	}
	return (lgame == POKEMON_JP ? OFFSET_CURRENT_JSAV : OFFSET_CURRENT_FSAV);
}

void saveReadData(const uint8_t* save, SAV_Game* sgame, SAV_GameLang lgame)
{
	char title[11];
	uint8_t sizePkm = (vgame >= POKEMON_GOLD ? 0x20 : 0x21);

	memset(sgame, 0, sizeof(SAV_Game));

	if (vgame >= POKEMON_GOLD)
		sgame->boxCount = (lgame == POKEMON_JP ? 9 : 14);
	else
		sgame->boxCount = (lgame == POKEMON_JP ? 8 : 12);

	sgame->boxCapacity = (lgame == POKEMON_JP ? 30 : 20);
	sgame->nameSize = (lgame == POKEMON_JP ? 0x6 : 0xB);

	uint16_t offsetCurrent = saveGetCurrentBoxOffset();
	uint16_t boxSize = BOX_SIZE(sgame->boxCapacity, sizePkm, sgame->nameSize);
	uint8_t currentBox = saveGetCurrentBox(save);

	// In Gen I, bit 7 (0x80) of wCurrentBoxNum indicates whether inactive boxes in SRAM Banks 2 & 3
	// have been initialized since starting a New Game. If 0, only currentBox is valid.
	bool gen1BoxesInitialized = true;
	if (vgame <= POKEMON_YELLOW)
	{
		uint16_t idxOff = (lgame == POKEMON_JP ? OFFSET_CURRENT_IDX_JSAV : OFFSET_CURRENT_IDX_FSAV);
		gen1BoxesInitialized = ((save[idxOff] & 0x80) != 0);
	}

	for (uint8_t i = 0; i < sgame->boxCount; i++)
	{
		sgame->boxes[i].index = i;
		sprintf(title, "CAJA %u", i+1);
		fontConvertString(sgame->boxes[i].title, title);

		uint16_t sramBoxOff = saveGetBoxOffset(i, boxSize);
		uint16_t offset = (i == currentBox ? offsetCurrent : sramBoxOff);

		if (vgame <= POKEMON_YELLOW && !gen1BoxesInitialized && i != currentBox)
		{
			// Uninitialized SRAM bank from previous overwritten save: treat as empty
			((uint8_t*)save)[sramBoxOff] = 0;
			((uint8_t*)save)[sramBoxOff + 1] = 0xFF;
		}
		else if (vgame >= POKEMON_GOLD && i == currentBox)
		{
			// In Gen II, EmptyAllSRAMBoxes on New Game zeroes sBox1..14 (sramBoxOff) but leaves old data in sBox (offsetCurrent)
			if (save[sramBoxOff] == 0 || save[sramBoxOff + 1] == 0xFF || save[sramBoxOff + 1] == 0x00)
			{
				((uint8_t*)save)[offsetCurrent] = 0;
				((uint8_t*)save)[offsetCurrent + 1] = 0xFF;
			}
		}

		saveExtractPokemonList(save, &sgame->boxes[i], offset, sizePkm, sgame->nameSize, sgame->boxCapacity);
	}

	// Purge leftover ghost Pokémon on brand-new 0-hour saves (New Game saved in bedroom)
	uint8_t hours = (vgame >= POKEMON_GOLD) ? save[0x2054] : save[lgame == POKEMON_JP ? 0x2CA0 : 0x2CED];
	uint8_t minutes = (vgame >= POKEMON_GOLD) ? save[0x2055] : save[lgame == POKEMON_JP ? 0x2CA2 : 0x2CEF];
	if (hours == 0 && minutes <= 10)
	{
		uint16_t pTid = (vgame >= POKEMON_GOLD) ? ((save[0x2009] << 8) | save[0x200A]) : ((save[0x2605] << 8) | save[0x2606]);
		for (uint8_t b = 0; b < sgame->boxCount; b++)
		{
			if (b == currentBox) continue;
			bool boxCleaned = false;
			for (uint8_t s = 0; s < sgame->boxes[b].capacity; s++)
			{
				SAV_Pokemon* pkm = &sgame->boxes[b].slots[s];
				if (!saveIsPkmEmpty(pkm) && pkm->originalTID != pTid)
				{
					memset(pkm, 0, sizeof(SAV_Pokemon));
					pkm->species = 0xFF;
					pkm->speciesIndex = 0xFF;
					pkm->moved = false;
					boxCleaned = true;
				}
			}
			if (boxCleaned)
			{
				sgame->boxes[b].count = 0;
				for (uint8_t s = 0; s < sgame->boxes[b].capacity; s++)
				{
					if (!saveIsPkmEmpty(&sgame->boxes[b].slots[s]))
						sgame->boxes[b].count++;
				}
			}
		}
	}
}

void saveWriteData(uint8_t* save, SAV_Game* sgame)
{
	uint8_t sizePkm = (vgame >= POKEMON_GOLD ? 0x20 : 0x21);
	uint16_t offsetCurrent = saveGetCurrentBoxOffset();
	uint16_t boxSize = BOX_SIZE(sgame->boxCapacity, sizePkm, sgame->nameSize);
	uint8_t currentBox = saveGetCurrentBox(save);

	for (uint8_t iB = 0; iB < sgame->boxCount; iB++)
	{
		// Iterate to find the empty slots
		for (uint8_t iP = 0, iPmin = 0; iP < sgame->boxes[iB].capacity; iP++)
		{
			// If looking for the next empty slot
			if (iPmin == 0)
			{
				if (saveIsPkmEmpty(&sgame->boxes[iB].slots[iP]))
				{
					iPmin = iP+1;
				}
			}
			// If looking for the next occupied slot
			else
			{
				if (!saveIsPkmEmpty(&sgame->boxes[iB].slots[iP]))
				{
					printf("Moving %03u -> %03u\n",
						sgame->boxes[iB].slots[iP].nationalDex,
						sgame->boxes[iB].slots[iPmin-1].nationalDex);
					_saveMovePkm(&sgame->boxes[iB].slots[iP], &sgame->boxes[iB].slots[iPmin-1]);
					iP = iPmin-1;
					iPmin = 0;
				}
			}
		}

		// Count the Pokémon
		sgame->boxes[iB].count = 0;
		for (uint8_t iP = 0; iP < sgame->boxes[iB].capacity; iP++)
		{
			if (!saveIsPkmEmpty(&sgame->boxes[iB].slots[iP]))
			{
				sgame->boxes[iB].count++;
			}
		}

		uint16_t boxOffset = saveGetBoxOffset(iB, boxSize);
		if (vgame >= POKEMON_GOLD)
		{
			// In Gen II, write to SRAM box slot AND mirror to active box if iB is the active box
			saveInjectPokemonList(save, &sgame->boxes[iB], boxOffset);
			if (iB == currentBox)
			{
				saveInjectPokemonList(save, &sgame->boxes[iB], offsetCurrent);
			}
		}
		else
		{
			if (iB != currentBox && sgame->boxes[iB].count > 0)
			{
				uint16_t idxOff = (lgame == POKEMON_JP ? OFFSET_CURRENT_IDX_JSAV : OFFSET_CURRENT_IDX_FSAV);
				save[idxOff] |= 0x80;
			}
			saveInjectPokemonList(save, &sgame->boxes[iB], (iB == currentBox ? offsetCurrent : boxOffset));
		}
	}

	saveFixChecksum(save);
}

void saveFixChecksum(uint8_t* save)
{
	if (vgame >= POKEMON_GOLD)
	{
		uint16_t chk = 0;
		uint16_t chkEnd;
		uint16_t chkOff1;
		uint16_t chkOff2;

		if (vgame != POKEMON_CRYSTAL && lgame != POKEMON_JP)
		{
			// Gold / Silver International
			chkEnd = 0x2D68;
			chkOff1 = 0x2D69;
			chkOff2 = 0x7E6D;
		}
		else if (vgame == POKEMON_CRYSTAL && lgame != POKEMON_JP)
		{
			// Crystal International
			chkEnd = 0x2B82;
			chkOff1 = 0x2D0D;
			chkOff2 = 0x1F0D;
		}
		else if (vgame != POKEMON_CRYSTAL && lgame == POKEMON_JP)
		{
			// Gold / Silver Japanese
			chkEnd = 0x2C8B;
			chkOff1 = 0x2D0D;
			chkOff2 = 0x7F0D;
		}
		else
		{
			// Crystal Japanese
			chkEnd = 0x2AE2;
			chkOff1 = 0x2D0D;
			chkOff2 = 0x7F0D;
		}

		for (uint16_t off = 0x2009; off <= chkEnd; off++)
		{
			chk += save[off];
		}

		save[chkOff1]     = chk & 0xFF;
		save[chkOff1 + 1] = chk >> 8;

		// Mirror main save data into backup save area and write Checksum 2
		if (vgame != POKEMON_CRYSTAL && lgame != POKEMON_JP)
		{
			memcpy(save + 0x15C7, save + 0x2009, 0x222F - 0x2009);
			memcpy(save + 0x3D69, save + 0x222F, 0x23D9 - 0x222F);
			memcpy(save + 0x0C6B, save + 0x23D9, 0x2856 - 0x23D9);
			memcpy(save + 0x7E39, save + 0x2856, 0x288A - 0x2856);
			memcpy(save + 0x10E8, save + 0x288A, 0x2D69 - 0x288A);
		}
		else if (vgame == POKEMON_CRYSTAL && lgame != POKEMON_JP)
		{
			memcpy(save + 0x1209, save + 0x2009, 0xB7A);
		}
		else if (vgame != POKEMON_CRYSTAL && lgame == POKEMON_JP)
		{
			memcpy(save + 0x7209, save + 0x2009, 0xC83);
		}
		else
		{
			memcpy(save + 0x7209, save + 0x2009, 0xADA);
		}

		save[chkOff2]     = chk & 0xFF;
		save[chkOff2 + 1] = chk >> 8;
	}
	else
	{
		uint8_t chk = 0;
		uint16_t chk_offset = (lgame == POKEMON_JP ? OFFSET_CHECKSUM_JSAV : OFFSET_CHECKSUM_FSAV);

		// [0x2598 -> checksum_offset]
		for (uint16_t off = 0x2598; off < chk_offset; off++)
		{
			chk += save[off];
		}

		save[chk_offset] = ~chk;
	}
}

uint32_t bankReadFile(uint8_t* bank, const char* path)
{
	uint32_t bytesRead = 0;

	printf("Opening bank file...\n");
	FILE* fp = fopen(path, "rb");
	if (fp)
	{
		printf("Reading bank file...");
		bytesRead = fread(bank, 1, BANK_SIZE, fp);

		if (ferror(fp)) printf(" ERROR\n");
		else printf(" OK\n");
		printf("  Read %lu/%u bytes\n", (unsigned long)bytesRead, BANK_SIZE);

		if (!ferror(fp))
		{
			if (bytesRead < BANK_SIZE)
			{
				memset(bank + bytesRead, 0x00, BANK_SIZE - bytesRead);
				uint32_t boxSize = BOX_SIZE(POKEMON_LIST_MAX_COUNT, 0x21, 11);
				for (uint32_t i = 0; i < BANK_BOX_MAX_COUNT; i++)
				{
					uint32_t bOffset = OFFSET_BBOX_1 + i * boxSize;
					if (bOffset >= bytesRead)
					{
						bank[bOffset] = 0; // count = 0
						bank[bOffset + 1] = 0xFF; // species terminator
					}
				}
			}
			bankUpdate(bank, bytesRead);
		}

		fclose(fp);
	}
	else
	{
		printf(" Creating...");
		memset(bank, 0x00, BANK_SIZE);
		memset(bank+0x100, 0xFF, BANK_BOX_MAX_COUNT * BOX_SIZE(POKEMON_LIST_MAX_COUNT,0x21,11));
		bankUpdate(bank, BANK_SIZE);

		printf(" OK\n  Created %d bytes\n", BANK_SIZE);
	}

	return bytesRead;
}

uint32_t bankWriteFile(const uint8_t* bank, const char* path)
{
	uint32_t bytesWritten = 0;

	printf("Opening bank file...\n");
	FILE* fp = fopen(path, "wb");
	if (fp)
	{
		printf("Writing bank file...");
		bytesWritten = fwrite(bank, 1, BANK_SIZE, fp);

		if (ferror(fp)) printf(" ERROR\n");
		else printf(" OK\n");
		printf("  Written %lu/%u bytes\n", (unsigned long)bytesWritten, BANK_SIZE);

		fclose(fp);
	}

	return bytesWritten;
}

void bankReadData(uint8_t* bank, SAV_Bank* sbank, uint32_t bytesRead)
{
	char title[16];
	sbank->magic = *(uint32_t*)(bank + 0x00);
	sbank->version = *(uint32_t*)(bank + 0x04);
	sbank->boxCount = BANK_BOX_MAX_COUNT;
	sbank->boxCapacity = POKEMON_LIST_MAX_COUNT;

	uint8_t nameSize = 0xB;
	uint32_t boxSize = BOX_SIZE(sbank->boxCapacity, 0x21, nameSize);

	for (uint8_t i = 0; i < sbank->boxCount; i++)
	{
		sbank->boxes[i].index = i;
		sprintf(title, "CAJA %u", (unsigned int)(i + 1));
		fontConvertString(sbank->boxes[i].title, title);
		saveExtractPokemonList(bank, &sbank->boxes[i], OFFSET_BBOX_1 + (uint32_t)i * boxSize, 0x21, nameSize, sbank->boxCapacity);
	}
}

void bankWriteData(uint8_t* bank, SAV_Bank* sbank)
{
	*(uint32_t*)(bank + 0x00) = sbank->magic;
	*(uint32_t*)(bank + 0x04) = sbank->version;

	uint8_t nameSize = 0xB;
	uint32_t boxSize = BOX_SIZE(sbank->boxCapacity, 0x21, nameSize);

	for (uint8_t iB = 0; iB < sbank->boxCount; iB++)
	{
		// Iterate to find the empty slots
		for (uint8_t iP = 0, iPmin = 0; iP < sbank->boxes[iB].capacity; iP++)
		{
			// If looking for the next empty slot
			if (iPmin == 0)
			{
				if (saveIsPkmEmpty(&sbank->boxes[iB].slots[iP]))
				{
					iPmin = iP+1;
				}
			}
			// If looking for the next occupied slot
			else
			{
				if (!saveIsPkmEmpty(&sbank->boxes[iB].slots[iP]))
				{
					printf("Moving %03u -> %03u\n",
						sbank->boxes[iB].slots[iP].nationalDex,
						sbank->boxes[iB].slots[iPmin-1].nationalDex);
					_saveMovePkm(&sbank->boxes[iB].slots[iP], &sbank->boxes[iB].slots[iPmin-1]);
					iP = iPmin-1;
					iPmin = 0;
				}
			}
		}

		// Count the Pokemon
		sbank->boxes[iB].count = 0;
		for (uint8_t iP = 0; iP < sbank->boxes[iB].capacity; iP++)
		{
			if (!saveIsPkmEmpty(&sbank->boxes[iB].slots[iP]))
				sbank->boxes[iB].count++;
		}

		saveInjectPokemonList(bank, &sbank->boxes[iB], OFFSET_BBOX_1 + (uint32_t)iB * boxSize);
	}
}

bool saveIsPkmShiny(const SAV_Pokemon* pkm)
{
	if (!pkm || saveIsPkmEmpty(pkm) || pkm->isEgg) return false;
	return (pkm->IVs[STAT_DEF] == 10 &&
	        pkm->IVs[STAT_SPE] == 10 &&
	        pkm->IVs[STAT_SPC] == 10 &&
	        ((pkm->IVs[STAT_ATK] & 2) == 2));
}

void saveRecalculateStats(SAV_Pokemon* pkm)
{
	if (!pkm || saveIsPkmEmpty(pkm) || pkm->isEgg) return;

	// In Gen 1 and 2, HP DV is calculated from the lowest bits of ATK, DEF, SPE, SPC:
	pkm->IVs[STAT_HP] = ((pkm->IVs[STAT_ATK] & 0x1) << 3) |
	                    ((pkm->IVs[STAT_DEF] & 0x1) << 2) |
	                    ((pkm->IVs[STAT_SPE] & 0x1) << 1) |
	                    ((pkm->IVs[STAT_SPC] & 0x1) << 0);

	const PersonalInfo* pInfo = Personal(pkm->nationalDex);
	if (!pInfo) return;

	pkm->types[0] = pInfo->type1;
	pkm->types[1] = pInfo->type2;

	uint16_t oldMax = pkm->maxHP;
	pkm->maxHP = ((((pInfo->HP + pkm->IVs[STAT_HP]) * 2 + sqrt(pkm->EVs[STAT_HP]) / 4) * pkm->level) / 100) + pkm->level + 10;
	pkm->ATK   = ((((pInfo->ATK + pkm->IVs[STAT_ATK]) * 2 + sqrt(pkm->EVs[STAT_ATK]) / 4) * pkm->level) / 100) + 5;
	pkm->DEF   = ((((pInfo->DEF + pkm->IVs[STAT_DEF]) * 2 + sqrt(pkm->EVs[STAT_DEF]) / 4) * pkm->level) / 100) + 5;
	pkm->SPE   = ((((pInfo->SPE + pkm->IVs[STAT_SPE]) * 2 + sqrt(pkm->EVs[STAT_SPE]) / 4) * pkm->level) / 100) + 5;
	pkm->SPC   = ((((pInfo->SPC + pkm->IVs[STAT_SPC]) * 2 + sqrt(pkm->EVs[STAT_SPC]) / 4) * pkm->level) / 100) + 5;
	pkm->SPD   = ((((pInfo->SPD + pkm->IVs[STAT_SPC]) * 2 + sqrt(pkm->EVs[STAT_SPC]) / 4) * pkm->level) / 100) + 5;

	if (oldMax == 0 || pkm->currentHP >= oldMax || pkm->currentHP > pkm->maxHP)
	{
		pkm->currentHP = pkm->maxHP;
	}
}

void saveSetPkmShiny(SAV_Pokemon* pkm, bool shiny)
{
	if (!pkm || saveIsPkmEmpty(pkm) || pkm->isEgg) return;
	if (shiny)
	{
		// Optimal Gen 1 & 2 Shiny DVs: Atk 15 (odd gives HP DV 8), Def 10, Spe 10, Spc 10
		pkm->IVs[STAT_ATK] = 15;
		pkm->IVs[STAT_DEF] = 10;
		pkm->IVs[STAT_SPE] = 10;
		pkm->IVs[STAT_SPC] = 10;
	}
	else
	{
		// Standard competitive max non-shiny DVs (15 all)
		pkm->IVs[STAT_ATK] = 15;
		pkm->IVs[STAT_DEF] = 15;
		pkm->IVs[STAT_SPE] = 15;
		pkm->IVs[STAT_SPC] = 15;
	}
	saveRecalculateStats(pkm);
}

void saveSetPkmLevel(SAV_Pokemon* pkm, uint8_t newLevel)
{
	if (!pkm || saveIsPkmEmpty(pkm) || pkm->isEgg) return;
	if (newLevel < 1) newLevel = 1;
	if (newLevel > 100) newLevel = 100;
	pkm->currentLevel = newLevel;
	pkm->level = newLevel;
	uint32_t exp = (uint32_t)newLevel * (uint32_t)newLevel * (uint32_t)newLevel;
	if (exp > 1000000) exp = 1000000;
	pkm->experience = exp;
	saveRecalculateStats(pkm);
}
