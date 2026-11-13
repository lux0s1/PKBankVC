#include "am.h"
#include "fs.h"

#include <3ds/result.h>
#include <stdlib.h>
#include <string.h>

static const u32 pokemonTitleCount = 37;
static const u64 pokemonTitleIDs[] = {
	0x0004000000170C00ULL, ///< Pokémon Red     (JPN) (JP)
	0x0004000000170D00ULL, ///< Pokémon Green   (JPN) (JP)
	0x0004000000170E00ULL, ///< Pokémon Blue    (JPN) (JP)
	0x0004000000170F00ULL, ///< Pokémon Yellow  (JPN) (JP)
	0x0004000000171000ULL, ///< Pokémon Red     (FREE) (EN)
	0x0004000000171100ULL, ///< Pokémon Blue    (FREE) (EN)
	0x0004000000171200ULL, ///< Pokémon Yellow  (FREE) (EN)
	0x0004000000171300ULL, ///< Pokémon Red     (EUR) (DE)
	0x0004000000171400ULL, ///< Pokémon Blue    (EUR) (DE)
	0x0004000000171500ULL, ///< Pokémon Yellow  (EUR) (DE)
	0x0004000000171600ULL, ///< Pokémon Red     (EUR) (FR)
	0x0004000000171700ULL, ///< Pokémon Blue    (EUR) (FR)
	0x0004000000171800ULL, ///< Pokémon Yellow  (EUR) (FR)
	0x0004000000171900ULL, ///< Pokémon Red     (EUR) (ES)
	0x0004000000171A00ULL, ///< Pokémon Blue    (EUR) (ES)
	0x0004000000171B00ULL, ///< Pokémon Yellow  (EUR) (ES)
	0x0004000000171C00ULL, ///< Pokémon Red     (EUR) (IT)
	0x0004000000171D00ULL, ///< Pokémon Blue    (EUR) (IT)
	0x0004000000171E00ULL, ///< Pokémon Yellow  (EUR) (IT)
	0x0004000000172300ULL, ///< Pokémon Gold    (JPN) (JP)
	0x0004000000172400ULL, ///< Pokémon Silver  (JPN) (JP)
	0x0004000000172500ULL, ///< Pokémon Crystal (JPN) (JP)
	0x0004000000172600ULL, ///< Pokémon Gold    (FREE) (EN)
	0x0004000000172700ULL, ///< Pokémon Silver  (FREE) (EN)
	0x0004000000172800ULL, ///< Pokémon Crystal (FREE) (EN)
	0x0004000000172900ULL, ///< Pokémon Gold    (EUR) (DE)
	0x0004000000172A00ULL, ///< Pokémon Silver  (EUR) (DE)
	0x0004000000172B00ULL, ///< Pokémon Crystal (EUR) (DE)
	0x0004000000172C00ULL, ///< Pokémon Gold    (EUR) (FR)
	0x0004000000172D00ULL, ///< Pokémon Silver  (EUR) (FR)
	0x0004000000172E00ULL, ///< Pokémon Crystal (EUR) (FR)
	0x0004000000172F00ULL, ///< Pokémon Gold    (EUR) (ES)
	0x0004000000173000ULL, ///< Pokémon Silver  (EUR) (ES)
	0x0004000000173100ULL, ///< Pokémon Crystal (EUR) (ES)
	0x0004000000173200ULL, ///< Pokémon Gold    (EUR) (IT)
	0x0004000000173300ULL, ///< Pokémon Silver  (EUR) (IT)
	0x0004000000173400ULL, ///< Pokémon Crystal (EUR) (IT)
	0x0000000000000000ULL, ///< Not Pokémon
};

static Result AM_GetSmdh(AM_TitleMediaEntry* title)
{
	if (!title || title->smdh) return -1;

	Result ret;
	u32 bytesRead = 0;
	Handle fileHandle = 0;

	u32 archivePath[] = { (u32)(title->titleid & 0xFFFFFFFF), (u32)((title->titleid >> 32) & 0xFFFFFFFF), title->mediatype, 0x00000000 };
	u32 filePath[] = { 0x00000000, 0x00000000, 0x00000002, 0x6E6F6369, 0x00000000 };

	ret = FSUSER_OpenFileDirectly(&fileHandle, ARCHIVE_SAVEDATA_AND_CONTENT, (FS_Path) { PATH_BINARY, 16, archivePath }, (FS_Path) { PATH_BINARY, 20, filePath }, FS_OPEN_READ, FS_ATTRIBUTE_NONE);

	if (R_SUCCEEDED(ret))
	{
		title->smdh = (smdh_s*) malloc(sizeof(smdh_s));
		if (title->smdh)
		{
			ret = FSFILE_Read(fileHandle, &bytesRead, 0LL, title->smdh, sizeof(smdh_s));
			if (R_FAILED(ret))
			{
				free(title->smdh);
				title->smdh = NULL;
			}
		}
		FSFILE_Close(fileHandle);
	}
	else
	{
		title->smdh = NULL;
	}

	return ret;
}

Result AM_GetPokemonTitleEntryList(AM_TitleMediaEntry** titleList, u32* count)
{
	if (!titleList) return -1;
	*titleList = NULL;
	if (count) *count = 0;

	Result ret = 0;
	u32 ccount = 0;
	u32 count_SD = 0;
	u32 count_Card = 0;
	u32 read_SD = 0;
	u32 read_Card = 0;
	u32 pkmCount_SD = 0;
	u32 pkmCount_Card = 0;
	u32 pkmCount_Total = 0;
	u64* titleIDs_SD = NULL;
	u64* titleIDs_Card = NULL;

	// Method 1: Try AM service (works when running as installed .cia or with am:net/am:u permissions)
	Result amRet = AM_GetTitleCount(MEDIATYPE_SD, &count_SD);
	if (R_SUCCEEDED(amRet) && count_SD > 0)
	{
		titleIDs_SD = (u64*) calloc(count_SD, sizeof(u64));
		if (titleIDs_SD)
		{
			if (R_FAILED(AM_GetTitleList(&read_SD, MEDIATYPE_SD, count_SD, titleIDs_SD)))
				read_SD = 0;
		}
	}

	if (R_SUCCEEDED(AM_GetTitleCount(MEDIATYPE_GAME_CARD, &count_Card)) && count_Card > 0)
	{
		titleIDs_Card = (u64*) calloc(count_Card, sizeof(u64));
		if (titleIDs_Card)
		{
			if (R_FAILED(AM_GetTitleList(&read_Card, MEDIATYPE_GAME_CARD, count_Card, titleIDs_Card)))
				read_Card = 0;
		}
	}

	for (u32 i = 0; i < read_SD; i++)
		for (u32 ii = 0; ii < pokemonTitleCount; ii++)
			if (titleIDs_SD[i] == pokemonTitleIDs[ii])
				pkmCount_SD++;

	for (u32 i = 0; i < read_Card; i++)
		for (u32 ii = 0; ii < pokemonTitleCount; ii++)
			if (titleIDs_Card[i] == pokemonTitleIDs[ii])
				pkmCount_Card++;

	pkmCount_Total = pkmCount_SD + pkmCount_Card;

	if (pkmCount_Total > 0)
	{
		AM_TitleMediaEntry* titleIDs = (AM_TitleMediaEntry*) calloc(pkmCount_Total, sizeof(AM_TitleMediaEntry));
		if (titleIDs)
		{
			for (u32 i = 0; i < read_Card; i++)
				for (u32 ii = 0; ii < pokemonTitleCount; ii++)
					if (titleIDs_Card[i] == pokemonTitleIDs[ii])
					{
						titleIDs[ccount] = (AM_TitleMediaEntry) { pokemonTitleIDs[ii], NULL, MEDIATYPE_GAME_CARD };
						AM_GetSmdh(&(titleIDs[ccount]));
						ccount++;
					}

			for (u32 i = 0; i < read_SD; i++)
				for (u32 ii = 0; ii < pokemonTitleCount; ii++)
					if (titleIDs_SD[i] == pokemonTitleIDs[ii])
					{
						titleIDs[ccount] = (AM_TitleMediaEntry) { pokemonTitleIDs[ii], NULL, MEDIATYPE_SD };
						AM_GetSmdh(&(titleIDs[ccount]));
						ccount++;
					}

			*titleList = titleIDs;
			if (count) *count = ccount;
		}
	}

	if (titleIDs_SD) free(titleIDs_SD);
	if (titleIDs_Card) free(titleIDs_Card);

	// Method 2 (Fallback for Homebrew Launcher .3dsx without AM access):
	// Directly probe each of the 37 Pokémon VC TitleIDs via FSUSER_OpenFileDirectly (SMDH icon) or Save Archive!
	if (ccount == 0)
	{
		AM_TitleMediaEntry tempEntries[37];
		u32 found = 0;

		for (u32 ii = 0; ii < pokemonTitleCount; ii++)
		{
			u64 tid = pokemonTitleIDs[ii];
			if (tid == 0) continue;

			AM_TitleMediaEntry entry = { tid, NULL, MEDIATYPE_SD };
			if (R_SUCCEEDED(AM_GetSmdh(&entry)))
			{
				tempEntries[found++] = entry;
			}
			else if (R_SUCCEEDED(FSCIA_InitSaveOnly(tid, MEDIATYPE_SD)))
			{
				FSCIA_Exit();
				tempEntries[found++] = entry;
			}
		}

		if (found > 0)
		{
			AM_TitleMediaEntry* titleIDs = (AM_TitleMediaEntry*) calloc(found, sizeof(AM_TitleMediaEntry));
			if (titleIDs)
			{
				memcpy(titleIDs, tempEntries, found * sizeof(AM_TitleMediaEntry));
				*titleList = titleIDs;
				if (count) *count = found;
				ret = 0;
			}
		}
	}

	return ret;
}

Result AM_FreePokemonTitleEntryList(AM_TitleMediaEntry* titleList, u32 count)
{
	if (!titleList) return -1;

	for (u32 i = 0; i < count; i++)
		if (titleList[i].smdh)
			free(titleList[i].smdh);
	free(titleList);

	return 0;
}
