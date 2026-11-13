#include <3ds.h>
#include <stdio.h>

#include "fs.h"
#include "key.h"
#include "gfx.h"
#include "font.h"
#include "save.h"
#include "data.h"
#include "error.h"
#include "version.h"
#include "personal.h"
#include "box_viewer.h"
#include "render.h"
#include "ts.h"

int main(int argc, char* argv[])
{
	Result ret = 0, error = 0;

	ret = romfsInit();
	if (R_FAILED(ret))
	{
		error |= ERR_FILESYSTEM;
	}

	ret = renderInit();
	if (R_FAILED(ret))
	{
		error |= ERR_GRAPHICS;
	}

	ret = gfxLoad();
	if (R_FAILED(ret))
	{
		error |= ERR_GRAPHICS;
	}

	ret = fontLoad();
	if (R_FAILED(ret))
	{
		error |= ERR_FONT;
	}

	ret = PersonalLoad();
	if (R_FAILED(ret))
	{
		error |= ERR_DATA;
	}

	// Detect whether we were launched under a specific Pokémon VC title (*hax target)
	// or as a standalone app (Luma3DS Rosalina .3dsx or installed .cia).
	uint64_t launchedProgramID = 0;
	APT_GetProgramID(&launchedProgramID);
	bool hasTargetTitle = (saveGetGameVersion(launchedProgramID) != NOT_POKEMON);

	if (hasTargetTitle)
	{
		titleid = launchedProgramID;

		ret = FS_Init();
		if (R_FAILED(ret))
		{
			error |= ERR_FILESYSTEM;
		}

		ret = gfxLoadFrame(titleid);
		if (R_FAILED(ret))
		{
			error |= ERR_GRAPHICS;
		}

		ret = saveLoad(argc > 1 ? argv[1] : NULL);
		if (R_FAILED(ret))
		{
			error |= ERR_SAVE;
		}

		if (!error)
		{
			boxViewerInitialize();

			while (aptMainLoop())
			{
				hidScanInput();

				boxViewerUpdate();

				if (boxViewerWantsExit()) break;

				boxViewerDraw();
			}

			saveSave(argc > 1 ? argv[1] : NULL);
		}

		gfxFreeFrame();
		FS_Exit();
	}
	else
	{
		while (!error && TS_Loop())
		{
			// If a custom path argument was provided, allow SDMC fallback; otherwise require the title's save archive.
			ret = (argc > 1)
				? FSCIA_Init(titleEntry.titleid, titleEntry.mediatype)
				: FSCIA_InitSaveOnly(titleEntry.titleid, titleEntry.mediatype);

			if (R_FAILED(ret))
			{
				// Title has never been launched / has no save archive yet: show message and return to title selector.
				TS_DrawNoSaveMessage();
				continue;
			}

			titleid = titleEntry.titleid;

			ret = gfxLoadFrame(titleid);
			if (R_FAILED(ret))
			{
				error |= ERR_GRAPHICS;
			}

			ret = saveLoad(argc > 1 ? argv[1] : NULL);
			if (R_FAILED(ret))
			{
				FSCIA_Exit();
				gfxFreeFrame();
				TS_DrawNoSaveMessage();
				continue;
			}

			if (!error)
			{
				boxViewerInitialize();

				while (aptMainLoop())
				{
					hidScanInput();

					boxViewerUpdate();

					if (boxViewerWantsExit()) break;

					boxViewerDraw();
				}

				saveSave(argc > 1 ? argv[1] : NULL);
			}

			gfxFreeFrame();
			FSCIA_Exit();
		}
	}

	fontFree();
	gfxFree();
	renderExit();

	if (error)
	{
		gfxInitDefault();
		consoleInit(GFX_TOP, NULL);

		printf("\nProblem happened: 0x%lx\n", error);
		if (error & ERR_SAVE) printf(" \a Save\n");
		if (error & ERR_DATA) printf(" \a Data\n");
		if (error & ERR_FONT) printf(" \a Font\n");
		if (error & ERR_GRAPHICS) printf(" \a Graphics\n");
		if (error & ERR_FILESYSTEM) printf(" \a Filesystem\n");
		printf("PKBankVC version: %08x\n", VERSION);
		printf("Can't start the viewer.\n");
		printf("Press any key to exit\n");
		waitKey(KEY_ANY);

		gfxExit();
	}

	romfsExit();
	return 0;
}
