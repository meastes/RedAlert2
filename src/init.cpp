/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/* $Header: /CounterStrike/INIT.CPP 8     3/14/97 5:15p Joe_b $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : INIT.CPP                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : January 20, 1992                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Anim_Init -- Initialize the VQ animation control structure.                               *
 *   Bootstrap -- Perform the initial bootstrap procedure.                                     *
 *   Calculate_CRC -- Calculates a one-way hash from a data block.                             *
 *   Init_Authorization -- Verifies that the player is authorized to play the game.            *
 *   Init_Bootstrap_Mixfiles -- Registers and caches any mixfiles needed for bootstrapping.    *
 *   Init_Bulk_Data -- Initialize the time-consuming mixfile caching.                          *
 *   Init_CDROM_Access -- Initialize the CD-ROM access handler.                                *
 *   Init_Color_Remaps -- Initialize the text remap tables.                                    *
 *   Init_Expansion_Files -- Fetch any override expansion mixfiles.                            *
 *   Init_Fonts -- Initialize all the game font pointers.                                      *
 *   Init_Game -- Main game initialization routine.                                            *
 *   Init_Heaps -- Initialize the game heaps and buffers.                                      *
 *   Init_Keys -- Initialize the cryptographic keys.                                           *
 *   Init_Mouse -- Initialize the mouse system.                                                *
 *   Init_One_Time_Systems -- Initialize internal pointers to the bulk data.                   *
 *   Init_Random -- Initializes the random-number generator                                    *
 *   Init_Secondary_Mixfiles -- Register and cache secondary mixfiles.                         *
 *   Load_Recording_Values -- Loads recording values from recording file                       *
 *   Load_Title_Page -- Load the background art for the title page.                            *
 *   Obfuscate -- Sufficiently transform parameter to thwart casual hackers.                   *
 *   Parse_Command_Line -- Parses the command line parameters.                                 *
 *   Parse_INI_File -- Parses CONQUER.INI for special options                                  *
 *   Play_Intro -- plays the introduction & logo movies                                        *
 *   Save_Recording_Values -- Saves recording values to a recording file                       *
 *   Select_Game -- The game's main menu                                                       *
 *   Load_Prolog_Page -- Loads the special pre-prolog "please wait" page.                      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"
#include "init.h"
#include "bitfont.h" // RA2: Draw_Loading_Screen measures its text with a BitFont
#include "_keyboar.h"
#include "_palette.h"
#include "_pk.h"
#include "_rand.h"
#include "_surface.h"
#include "airctype.h"
#include "anim.h"
#include "builtype.h"
#include "bullettype.h"
#include "cd.h"
#include "cell.h"
#include "conquer.h"
#include "dbgprint.h"
#include "audtimer.h" // RA2: AudioGetTime, for the loading screen hold time
#include "draw.h" // RA2: Draw_Shape, for the loading screen picture
#include "msgloop.h" // RA2: Get_Input_Record_Mode, which Init_Random asks
#include "shapeset.h" // RA2: the loading screen picture is a ShapeSet
#include "stringtable.h" // RA2: StringTable::LoadString, with RA2's source file and line
#include "dialog.h"
#include "dsurface.h"
#include "gamedlg.h"
#include "getcpu.h"
#include "infatype.h"
#include "ionblast.h"
#include "lightcon.h"
#include "mixfile.h"
#include "msgbox.h"
#include "newmenu.h"
#include "nullmgr.h"
#include "overtype.h"
#include "ovrlight.h"
#include "ownrdraw.h"
#include "pcx.h"
#include "revent.h"
#include "rndstraw.h"
#include "rules.h"
#include "scenario.h"
#include "smudtype.h"
#include "theme.h"
#include "unittype.h"
#include "vein.h"
#include "winstub.h"
#include "worlddom.h"


#include <conio.h>
#include <ctime>
#include <dos.h>



/***************************************************************************
 * Init_Random -- Initializes the random-number generator                  *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/04/1995 BRR : Created.                                             *
 *=========================================================================*/
void Init_Random(void)
{
	DebugString("Init random number\n");
	/*
	**	Do nothing if we've loaded a multiplayer game, or we're playing back
	**	a recording; the random number generator is initialized by loading
	**	the game.
	*/
	if (Session.LoadGame || Session.Play || Get_Input_Record_Mode()) { // RA2: a recording that is being made or played back (0x00522E80 Get_Input_Record_Mode) also keeps the seed as it is
		Scen->RandomNumber = Seed;
		NonCriticalRandomNumber = Seed;
		DebugString("Seed is %08x\n", Seed);
		return;
	}

	/*
	**	Initialize the random number Seed.  For multiplayer, this will have been done
	**	in the connection dialogs.  For single-player games, AND if we're not playing
	**	back a recording, init the Seed to a random value.
	*/
	if (Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH) {

		/*
		**	Gather some "random" bits from the system timer. Actually, only the
		**	low order millisecond bits are secure. The other bits could be
		**	easily guessed from the system clock (most clocks are fairly accurate
		**	and thus predictable).
		*/
		SYSTEMTIME t;
		GetSystemTime(&t);
		CryptRandom.Seed_Byte(t.wMilliseconds);
		CryptRandom.Seed_Bit(t.wSecond);
		CryptRandom.Seed_Bit(t.wSecond>>1);
		CryptRandom.Seed_Bit(t.wSecond>>2);
		CryptRandom.Seed_Bit(t.wSecond>>3);
		CryptRandom.Seed_Bit(t.wSecond>>4);
		CryptRandom.Seed_Bit(t.wMinute);
		CryptRandom.Seed_Bit(t.wMinute>>1);
		CryptRandom.Seed_Bit(t.wMinute>>2);
		CryptRandom.Seed_Bit(t.wMinute>>3);
		CryptRandom.Seed_Bit(t.wMinute>>4);
		CryptRandom.Seed_Bit(t.wHour);
		CryptRandom.Seed_Bit(t.wDay);
		CryptRandom.Seed_Bit(t.wDayOfWeek);
		CryptRandom.Seed_Bit(t.wMonth);
		CryptRandom.Seed_Bit(t.wYear);

		/*
		**	Set the optional user-specified seed
		*/
		if (CustomSeed != 0) {
			Seed = CustomSeed;
		} else {
			CryptRandom.Get(&Seed, sizeof(Seed));
			Seed = GetTickCount();
			//srand(time(NULL));
			//Seed = rand();
		}
	}

	/*
	**	Initialize the random-number generators
	*/
	DebugString("Seed is %08x\n", Seed);
	Scen->RandomNumber = Seed;
	NonCriticalRandomNumber = Seed;
}

extern bool __fastcall Fill_Surface_Rect(Surface * surface, Rect const & rect, int color); // RA2: 0x006F3A20, fills a rectangle of a surface with a colour

/// <summary>
/// Dims a surface and shows it.
/// This routine is used to darken the screen behind a dialog box. A null surface means the
/// hidden surface.
/// </summary>
/// <param name="surface">The surface to darken, or NULL for the hidden surface.</param>
/// <param name="update">Should the result be copied to the visible surface?</param>
void Darken_Surface(Surface * surface, bool update) // RA2: 0x005159B0, not in TibSun: dims the screen behind a dialog; update copies the result to the visible surface
{
	if (surface == NULL) { // RA2: as above
		surface = HiddenSurface; // RA2: as above
	}
	surface->Fill(0); // RA2: as above
	int width = surface->Get_Width(); // RA2: as above
	int height = surface->Get_Height(); // RA2: as above
	Rect rect(0, 0, width, height); // RA2: as above
	BSurface * work = new BSurface(width, height, 2); // RA2: as above
	Fill_Surface_Rect(work, rect, false); // RA2: as above
	surface->Blit_From(*work, false, true); // RA2: as above
	delete work; // RA2: as above
	if (update) { // RA2: as above
		Update_Visible_Surface(false, surface, NULL); // RA2: as above
	}
}



/***********************************************************************************************
 * Load_Title_Page -- Load the background art for the title page.                              *
 *                                                                                             *
 *    This routine will load the background art in a machine independent format. There is      *
 *    different art required for the hi-res and lo-res versions of the game.                   *
 *                                                                                             *
 * INPUT:   visible  -- Should the title page art be copied to the visible page by this        *
 *                      routine?                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Be sure the mouse is hidden if the image is to be copied to the visible page.   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void Load_Title_Page(const char * name, bool visible)
{
	HiddenSurface->Fill(0);
	Load_Title_Screen(name, HiddenSurface, &CCPalette);
	if (visible)
		Update_Visible_Surface(true, HiddenSurface);
}

/// <summary>
/// Clears the hidden surface to black.
/// </summary>
/// <param name="visible">Should the cleared surface be copied to the visible one as well?</param>
void Blank_Hidden_Surface(bool visible) // RA2: 0x00515AC0, not in TibSun: clears the hidden surface and, if visible, copies it to the screen (YR: Blank_Hidden_Surface)
{
	HiddenSurface->Fill(0); // RA2: as above
	if (visible) { // RA2: as above
		Update_Visible_Surface(true, HiddenSurface); // RA2: as above
	}
}



/***********************************************************************************************
 * Init_Expansion_Files -- Fetch any override expansion mixfiles.                              *
 *                                                                                             *
 *    This routine will search for and register/cache any override mixfiles found.             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool __cdecl Init_Expansion_Files(void) // RA2: 0x00515AF0 searches ECACHE*.MIX and ELOCAL*.MIX in every build (TibSun only in the demo); a plain C function, not static, as Init_Game is not part of this file yet
{
	WIN32_FIND_DATA data; // RA2: as above
	HANDLE handle = FindFirstFile("ECACHE*.MIX", &data); // RA2: as above
	if (handle != INVALID_HANDLE_VALUE) {
		do { // RA2: as above
			if ((data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY)) == 0) { // RA2: as above
				MFCD * mix = new MFCD(data.cFileName, &FastKey); // RA2: as above
				if (mix != NULL) { // RA2: as above
					ExpandMix.Add(mix); // RA2: as above
					mix->Cache(); // RA2: as above
				}
			}
		} while (FindNextFile(handle, &data)); // RA2: as above
		FindClose(handle);
	}
	handle = FindFirstFile("ELOCAL*.MIX", &data); // RA2: as above
	if (handle != INVALID_HANDLE_VALUE) {
		do { // RA2: as above
			if ((data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY)) == 0) { // RA2: as above
				MFCD * mix = new MFCD(data.cFileName, &FastKey); // RA2: as above
				if (mix != NULL) { // RA2: as above
					ExpandMix.Add(mix); // RA2: as above
				}
			}
		} while (FindNextFile(handle, &data)); // RA2: as above
		FindClose(handle);
	}
	return(true);
}



/***********************************************************************************************
 * Init_Bootstrap_Mixfiles -- Registers and caches any mixfiles needed for bootstrapping.      *
 *                                                                                             *
 *    This routine will register the initial mixfiles that are required to display error       *
 *    messages and get input from the player.                                                  *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Be sure to call this routine before any dialogs would be displayed to the       *
 *             player.                                                                         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool Init_Bootstrap_Mixfiles(void) // RA2: 0x00515C90 is not static (Init_Game, 0x005116A0, is not in this part); RA2 registers EXPAND99..00.MIX, RA2.MIX, CACHE.MIX and LOCAL.MIX only
{
	DiskID temp = CD::Get_Required_Disk(); // RA2: as above
	CD::Set_Required_Disk(DISK_LOCAL);

	for (int index = 99; index >= 0; index--) { // RA2: as above
		char name[64]; // RA2: as above
		sprintf(name, "EXPAND%02d.MIX", index); // RA2: as above
		if (RawFileClass(name).Is_Available()) { // RA2: as above
			MFCD * expand = new MFCD(name, &FastKey); // RA2: as above
			if (expand != NULL) { // RA2: as above
				ExpandMix.Add(expand); // RA2: as above
				DebugStringNoPrefix(" %s", name); // RA2: as above
			}
		}
	}

	GameMix = new MFCD("RA2.MIX", &FastKey); // RA2: as above
	if (GameMix == NULL) {
		return(false);
	}

	DebugStringNoPrefix(" CACHE.MIX");

	CacheMix = new MFCD("CACHE.MIX", &FastKey);
	if ((CacheMix == NULL) || (MFCD::Cache("CACHE.MIX") == false)) {
		return(false);
	}

	DebugStringNoPrefix(" CACHE.MIX");

	LocalMix = new MFCD("LOCAL.MIX", &FastKey);
	if (LocalMix == NULL) {
		return(false);
	}

	DebugStringNoPrefix(" LOCAL.MIX");

	CD::Set_Required_Disk(temp);

	return(true);
}



/***********************************************************************************************
 * Init_Secondary_Mixfiles -- Register and cache secondary mixfiles.                           *
 *                                                                                             *
 *    This routine is used to register the mixfiles that are needed for main menu processing.  *
 *    Call this routine before the main menu is display and processed.                         *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool Init_Secondary_Mixfiles(void) // RA2: 0x00515E70 is not static; RA2 registers CONQUER.MIX, CAMEO.MIX, MAPS, MULTI, THEME (ScoresMix) and MOVIES, with the further MAPS*/MOVIES* files kept in every build
{
	char name[_MAX_PATH];

	if (CCFileClass("CONQUER.MIX").Is_Available()) {
		ConquerMix = new MFCD("CONQUER.MIX", &FastKey);
	}
	DebugStringNoPrefix(" CONQUER.MIX");
	if (ConquerMix == NULL) {
		return(false);
	}

	if (CCFileClass("CAMEO.MIX").Is_Available()) { // RA2: as above
		CameoMix = new MFCD("CAMEO.MIX", &FastKey); // RA2: as above
	}
	DebugStringNoPrefix(" CAMEO.MIX"); // RA2: as above
	if (CameoMix == NULL) { // RA2: as above
		return(false);
	}

	int id = ((int)CD::Get_Current_Disk(60) + 1); // RA2: as above
	if (CD::Is_Override_Swap() == true) {
		strcpy(name, "MAPS*.MIX");
		if (CDFileClass::Find_First_File(name) == true) {
			DebugStringNoPrefix(" %s", name);
			MapsMix = new MFCD(name, &FastKey);
			while (CDFileClass::Find_Next_File(name) == true) {
				DebugStringNoPrefix(" %s", name);
				MFCD * mix = new MFCD(name, &FastKey); // RA2: as above
				if (mix != NULL) {
					MapsMixLocal.Add(mix);
				}
			}
			CDFileClass::Find_Close();
		}
	} else {
		sprintf(name, "MAPS%02d.MIX", id);
		if (CCFileClass(name).Is_Available()) {
			MapsMix = new MFCD(name, &FastKey);
		}
		DebugStringNoPrefix(" %s", name);
	}
	if (MapsMix == NULL) {
		return(false);
	}

	if (CCFileClass("MULTI.MIX").Is_Available()) {
		MultiMix = new MFCD("MULTI.MIX", &FastKey);
	}
	DebugStringNoPrefix(" MULTI.MIX");
	if (MultiMix == NULL) {
		return(false);
	}

	if (CCFileClass("THEME.MIX").Is_Available()) { // RA2: as above
		ScoresMix = new MFCD("THEME.MIX", &FastKey); // RA2: as above
	}
	DebugStringNoPrefix(" THEME.MIX"); // RA2: as above
	ScoresPresent = true;
	Theme.Scan();

	if (CD::Is_Override_Swap() == true) {
		strcpy(name, "u:\\mixfiles\\MOVIES*.MIX"); // RA2: as above
		if (RawFileClass("MOVIES01.MIX").Is_Available()) { // RA2: as above
			strcpy(name, "MOVIES*.MIX");
		}
		if (RawFileClass("MIXFILES\\MOVIES01.MIX").Is_Available()) { // RA2: as above
			strcpy(name, "MIXFILES\\MOVIES*.MIX"); // RA2: as above
		}
		if (CDFileClass::Find_First_File(name) == true) {
			DebugStringNoPrefix(" %s", name);
			MoviesMix = new MFCD(name, &FastKey);
			while (CDFileClass::Find_Next_File(name) == true) {
				DebugStringNoPrefix(" %s", name);
				MFCD * mix = new MFCD(name, &FastKey); // RA2: as above
				if (mix != NULL) {
					MoviesMixLocal.Add(mix);
				}
			}
			CDFileClass::Find_Close();
		}
	} else {
		sprintf(name, "MOVIES%02d.MIX", id);
		if (CCFileClass(name).Is_Available()) {
			MoviesMix = new MFCD(name, &FastKey);
		}
		DebugStringNoPrefix(" %s", name);
	}
	if (MoviesMix == NULL) {
		return(false);
	}
	return(true);
}

extern VoxelDataStruct DropPodVoxel;



/***********************************************************************************************
 * Init_One_Time_Systems -- Initialize internal pointers to the bulk data.                     *
 *                                                                                             *
 *    This performs the one-time processing required after the bulk data has been cached but   *
 *    before the game actually starts. Typically, this routine extracts pointers to all the    *
 *    embedded data sub-files within the main game data mixfile. This routine must be called   *
 *    AFTER the bulk data has been cached.                                                     *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Call this routine AFTER the bulk data has been cached.                          *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static bool Init_One_Time_Systems(void)
{
	Call_Back();

	Map.One_Time();
	Options.One_Time();
	Session.One_Time();
	ObjectTypeClass::One_Time();
	BuildingTypeClass::One_Time();
	BulletTypeClass::One_Time();
	OverlayTypeClass::One_Time();
	SmudgeTypeClass::One_Time();
	UnitTypeClass::One_Time();
	InfantryTypeClass::One_Time();
	AnimTypeClass::One_Time();
	AircraftTypeClass::One_Time();
	HouseClass::One_Time();
	SpotLightClass::One_Time();
	IonBlastClass::One_Time();

	CCFileClass dropvxl("DPOD.VXL");
	DropPodVoxel.VoxLib = new VoxelLibrary(dropvxl);

	if (DropPodVoxel.VoxLib == NULL) {
		DebugString("Failed to create VoxLib!\n");
		return(false);
	}

	CCFileClass dropmot("DPOD.HVA");
	DropPodVoxel.MotLib = new MotionLibrary(dropmot);

	if (DropPodVoxel.MotLib == NULL) {
		DebugString("Failed to create MotLib!\n");
		return(false);
	}

	return(true);
}

extern TimeStamp LoadingScreenHoldUntil; // RA2: 0x00A41668, the time before which Init_Game keeps the loading screen up

/// <summary>
/// Draws the loading screen.
/// This routine puts the loading picture, the copyright text and the loading text up on the
/// screen, and notes the time before which Init_Game keeps it there.
/// </summary>
void Draw_Loading_Screen(void) // RA2: 0x00516560, not in TibSun: GLS.PAL, GLSS.SHP or GLSL.SHP and the copyright and loading texts on the hidden surface; Init_Game waits until LoadingScreenHoldUntil
{
	CCFileClass file("GLS.PAL"); // RA2: as above
	ShapeSet * shape = NULL; // RA2: as above
	PaletteClass * palette = NULL; // RA2: as above
	ConvertClass * drawer = NULL; // RA2: as above
	if (file.Is_Available()) { // RA2: as above
		palette = (PaletteClass *)Load_Alloc_Data(file); // RA2: as above
		file.Close(); // RA2: as above
		for (int index = 0; index < PaletteClass::COLOR_COUNT; index++) { // RA2: as above
			RGBClass & color = (*palette)[index]; // RA2: as above
			color = RGBClass(color.Get_Red() << 2, color.Get_Green() << 2, color.Get_Blue() << 2); // RA2: as above
		}
		if (palette != NULL) { // RA2: as above
			drawer = new ConvertClass(*palette, GamePalette, *VisibleSurface, 1, false); // RA2: as above
			if (drawer != NULL) { // RA2: as above
				CCFileClass shapefile(VideoModeWidth == 640 ? "GLSS.SHP" : "GLSL.SHP"); // RA2: as above
				if (shapefile.Is_Available()) { // RA2: as above
					shape = (ShapeSet *)Load_Alloc_Data(shapefile); // RA2: as above
					shapefile.Close(); // RA2: as above
				}
			}
		}
	}

	RECT client; // RA2: as above
	GetClientRect(MainWindow, &client); // RA2: as above
	Rect area(0, 0, client.right - client.left, client.bottom - client.top); // RA2: as above
	if (shape != NULL) { // RA2: as above
		Point2D xy((area.Width - shape->Get_Width()) / 2, (area.Height - shape->Get_Height()) / 2); // RA2: as above
		Draw_Shape(*HiddenSurface, *drawer, shape, 0, xy, area, SHAPE_NORMAL, NULL, 0, ZGRAD_GROUND, 1000, NULL, 0, Point2D(0, 0)); // RA2: as above
	}

	wchar_t const * text = StringTable::LoadString("TXT_COPYRIGHT", NULL, "D:\\Ra2\\Init.CPP", 4700); // RA2: as above
	BitFont * font = Font_From_TPF(TPF_TEXT); // RA2: as above
	Point2D xy(0, area.Height - 60); // RA2: as above
	if (font != NULL) { // RA2: as above
		xy.X = area.Width - font->String_Pixel_Width(text, 0) - 50; // RA2: as above
	}
	Fancy_Text_Print(text, *HiddenSurface, area, xy, 0xFFFFU, 0U, TPF_TEXT); // RA2: as above

	text = StringTable::LoadString("GUI:WWBrand", NULL, "D:\\Ra2\\Init.CPP", 4711); // RA2: as above
	if (font != NULL) { // RA2: as above
		xy.X = area.Width - font->String_Pixel_Width(text, 0) - 50; // RA2: as above
		xy.Y += font->LineHeight + 3; // RA2: as above
	}
	Fancy_Text_Print(text, *HiddenSurface, area, xy, 0xFFFFU, 0U, TPF_TEXT); // RA2: as above

	text = StringTable::LoadString("GUI:LoadingEx", NULL, "D:\\Ra2\\Init.CPP", 4721); // RA2: as above
	xy.X = 10; // RA2: as above
	xy.Y = 10; // RA2: as above
	Fancy_Text_Print(text, *HiddenSurface, area, xy, 0xFFFFU, 0U, TPF_TEXT); // RA2: as above

	Update_Visible_Surface(true, HiddenSurface); // RA2: as above
	LoadingScreenHoldUntil = AudioGetTime() + 5000; // RA2: as above
	if (palette != NULL) { // RA2: as above
		operator delete(palette); // RA2: as above
	}
	if (shape != NULL) { // RA2: as above
		operator delete(shape); // RA2: as above
	}
	delete drawer; // RA2: as above
}



/***********************************************************************************************
 * Init_Bulk_Data -- Initialize the time-consuming mixfile caching.                            *
 *                                                                                             *
 *    This routine is called to handle the time consuming process of game initialization.      *
 *    The title page will be displayed when this routine is called.                            *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   This routine will take a very long time.                                        *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/03/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool Init_Bulk_Data(void) // RA2: 0x00516900 is not static; RA2 caches CONQUER.MIX and CAMEO.MIX only, there is no sound cache and no TUTORIAL.INI
{
	if (ConquerMix == NULL || !ConquerMix->Cache()) {
		return(false);
	}
	if (CameoMix == NULL || !CameoMix->Cache()) { // RA2: as above
		return(false);
	}
	Call_Back();
	return(Init_One_Time_Systems());
}



/***************************************************************************
 * Save_Recording_Values -- Saves multiplayer-specific values              *
 *                                                                         *
 * This routine saves multiplayer values that need to be restored for a    *
 * save game.  In addition to saving the random # seed for this scenario,  *
 * it saves the contents of the actual random number generator; this       *
 * ensures that the random # sequencer will pick up where it left off when *
 * the game was saved.                                                     *
 * This routine also saves the header for a Recording file, so it must     *
 * save some data not needed specifically by a save-game file (ie Seed).   *
 *                                                                         *
 * INPUT:                                                                  *
 *      file      file to save to                                          *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      true = success, false = failure                                    *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   09/28/1995 BRR : Created.                                             *
 *=========================================================================*/
bool Save_Recording_Values(CCFileClass & file)
{
	DebugString("Saving recording values for scenario : %s\n", Scen->ScenarioName);
	file.Write(&BuildLevel, sizeof(BuildLevel));
	file.Write(&Seed, sizeof(Seed));
	file.Write(&Scen->Scenario, sizeof(Scen->Scenario));
	file.Write(Scen->ScenarioName, sizeof(Scen->ScenarioName));
	file.Write(&Whom, sizeof(Whom));
	file.Write(&Special, sizeof(SpecialClass));
	file.Write(&Options, sizeof(OptionsClass));
	return(true);
}



/***************************************************************************
 * Load_Recording_Values -- Loads multiplayer-specific values              *
 *                                                                         *
 * INPUT:                                                                  *
 *      file         file to load from                                     *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      true = success, false = failure                                    *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   09/28/1995 BRR : Created.                                             *
 *=========================================================================*/
bool Load_Recording_Values(CCFileClass & file)
{
	file.Read(&BuildLevel, sizeof(BuildLevel));
	file.Read(&Seed, sizeof(Seed));
	file.Read(&Scen->Scenario, sizeof(Scen->Scenario));
	file.Read(Scen->ScenarioName, sizeof(Scen->ScenarioName));
	file.Read(&Whom, sizeof(Whom));
	file.Read(&Special, sizeof(SpecialClass));
	file.Read(&Options, sizeof(OptionsClass));
	DebugString("Loaded recording values for scenario : %s\n", Scen->ScenarioName);
	return(true);
}

BOOL CALLBACK Main_Menu_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: Main_Menu names its dialog procedure

extern wchar_t * __fastcall Narrow_To_Wide(char const * narrow); // RA2: 0x006FA180, narrow text to wide text in a static buffer

/*
**	RA2: the values Main_Menu returns and Main_Menu_Dialog_Proc stores. The numbers are what the
**	executable compares; the names are given only where the code around them shows what they do.
*/
enum MainMenuChoice {
	MENU_DIALOG_100 = 1,	// RA2: the button that opens dialog 0x100 (Select_Game runs it through the dialog runner)
	MENU_INTERNET = 2,		// RA2: Session.Type = GAME_INTERNET
	MENU_NETWORK = 3,		// RA2: IDC_NETWORK, Session.Type = GAME_IPX
	MENU_DIALOG_101 = 4,	// RA2: the button that opens dialog 0x101
	MENU_OPTIONS = 5,		// RA2: Main_Options_Dialog
	MENU_EXIT_ASK = 6,		// RA2: "GUI:ExitAreYouSure"
	MENU_EXIT = 7,
	MENU_NONE = 0x12		// RA2: nothing chosen yet
};

struct CheatEntryStruct {
	bool * State;
	char const * CheatString;
	char const * VersionSuffix;	/// Suffix appended to version string
	bool IsAllowedInMP;
};

static CheatEntryStruct CheatEntries[] = {
	{ &VisceroidsAsSnoBees,    "PENGO",    " PG", false },
	{ &Just4Fun,               "THETEAM",  NULL,  false },
	{ &Options.AllowHiResModes,"HIRES",    " RS", false },
	{ &Debug_IngameModeChange, "TOGGLE",   NULL,  true  },
};



/// <summary>
/// Feeds a typed character to the cheat code recognizer.
/// This routine gathers up alphanumeric keystrokes and toggles a cheat the moment its code
/// appears in what has been typed so far. Anything else the player types wipes the slate.
/// </summary>
/// <param name="chr">The character that the player just typed.</param>
/// <returns>bool; Was a cheat toggled by this character?</returns>
static bool Cheat_Key_Process(char chr) // RA2: static: RA2 has no out-of-line copy, the code is inlined into Main_Menu (0x00517081-0x00517164)
{
	static char _buffer[32] = "";

	if (!isalnum(chr) || chr == '~') {
		memset(_buffer, 0, sizeof(_buffer));
		return(false);
	}

	char tmp[2];
	tmp[0] = chr;
	tmp[1] = 0;
	DebugStringNoPrefix(tmp);

	int len = strlen(_buffer);
	if (len >= ARRAY_SIZE(_buffer)-1) {
		len = 0;
		_buffer[0] = 0;
	}
	_buffer[len] = toupper(chr);

	for (int c = 0; c < ARRAY_SIZE(CheatEntries); c++) {
		if (strstr(_buffer, CheatEntries[c].CheatString) != NULL) {
			*CheatEntries[c].State = !(*CheatEntries[c].State);
			memset(_buffer, 0, sizeof(_buffer));
			return(true);
		}
	}
	return(false);
}

/// <summary>
/// Places the flag picture of a menu dialog.
/// The flag is moved to the corner of the dialog area on screens larger than 800 by 600,
/// and is given the small flag picture on a 640 pixel wide screen.
/// </summary>
/// <param name="dialog">The dialog whose flag control is to be placed.</param>
inline void Show_Dialog_With_Flag(HWND dialog) // RA2: 0x005115F0, not in TibSun: puts the flag picture (control 0x71A) at the dialog corner of a 800x600 or larger screen; inline here, the out-of-line copy lies before this part
{
	HWND item = GetDlgItem(dialog, 0x71A); // RA2: as above
	if (item != NULL) { // RA2: as above
		int x = VideoModeWidth <= 800 ? 0 : (VideoModeWidth - 800) / 2; // RA2: as above
		int y = VideoModeHeight <= 600 ? 0 : (VideoModeHeight - 600) / 2; // RA2: as above
		SetWindowPos(item, NULL, x, y, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW); // RA2: as above
		SendMessage(item, WM_USER + 227, 1, 0); // RA2: as above
		if (VideoModeWidth == 640) { // RA2: as above
			SendMessage(item, WM_USER + 228, 0, (LPARAM)"Ra2ts_s"); // RA2: as above
		} else { // RA2: as above
			SendMessage(item, WM_USER + 228, 0, (LPARAM)"Ra2ts_l"); // RA2: as above
		}
	}
}

extern char ProfileText[128]; // RA2: 0x00A4127C, the summary text Get_Performance_Profile builds
extern char ProfileCPUText[128]; // RA2: 0x00A411FC
extern char ProfileRAMText[128]; // RA2: 0x00A41388
extern char ProfileVRAMText[128]; // RA2: 0x00A41308
extern char ProfileBlitText[128]; // RA2: 0x00A41408



/// <summary>
/// Handles the messages for the version information dialog.
/// This routine fills the list box with the game's title, its version numbers, the build
/// stamp, and a description of the processor it finds itself running upon. It is the
/// first thing to ask for when a player reports a problem.
/// </summary>
BOOL CALLBACK Version_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	char buffer[256];
	char time[128]; // RA2: 0x00516C40 uses RA2's dialog messages (WM_USER + 151 after creation, WM_USER + 200 adds a line to the list, + 205 sets its title) and its profile texts
	char machine[128]; // RA2: as above
	char user[128]; // RA2: as above
	int result = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam); // RA2: as above
	if (result != 0) { // RA2: as above
		return(result); // RA2: as above
	}
	int * choice = (int *)GetWindowLong(window, DWL_USER); // RA2: as above
	switch (message) {
		case WM_USER + 151: // RA2: as above
		{
			HWND item = GetDlgItem(window, IDC_VERSION_INFO); // RA2: as above
			SendMessage(item, WM_USER + 205, 0, (LPARAM)StringTable::LoadString("TXT_SHORT_TITLE", NULL, "D:\\Ra2\\Init.CPP", 5428)); // RA2: as above
			sprintf(buffer, "Version %s", Version_Name());
			SendMessage(item, WM_USER + 200, 0, (LPARAM)buffer); // RA2: as above
			sprintf(buffer, "Internal Version %s", VerNum.Version_Name());
			SendMessage(item, WM_USER + 200, 0, (LPARAM)buffer); // RA2: as above
			time[0] = 0; // RA2: as above
			Build_Date_String(time, sizeof(time)); // RA2: as above
			sprintf(buffer, "Release Build: %s by %s - %s", Build_Number_String(machine, sizeof(machine)), // RA2: as above
				Build_By_String(user, sizeof(user)), time); // RA2: as above
			SendMessage(item, WM_USER + 200, 0, (LPARAM)buffer); // RA2: as above
			int family = 5; // RA2: as above
			bool mmx = false; // RA2: as above
			SendMessage(item, WM_USER + 200, 0, (LPARAM)ProfileCPUText); // RA2: as above
			SendMessage(item, WM_USER + 200, 0, (LPARAM)ProfileRAMText); // RA2: as above
			SendMessage(item, WM_USER + 200, 0, (LPARAM)ProfileVRAMText); // RA2: as above
			SendMessage(item, WM_USER + 200, 0, (LPARAM)ProfileBlitText); // RA2: as above
			SendMessage(item, WM_USER + 200, 0, (LPARAM)ProfileText); // RA2: as above
			Get_CPU_Type(family, mmx, NULL, 0); // RA2: as above
			sprintf(buffer, "MMX %s, Vendor: %s", mmx ? "Yes" : "No", &VendorID); // RA2: as above
			SendMessage(item, WM_USER + 200, 0, (LPARAM)buffer); // RA2: as above
			Get_Language_Version(buffer);
			SendMessage(item, WM_USER + 200, 0, (LPARAM)buffer); // RA2: as above
		}
		return(FALSE);
	case WM_COMMAND:
		if ((int)(wparam & 0xFFFF) > 0 && (int)(wparam & 0xFFFF) <= 2) { // RA2: as above
			*choice = wparam & 0xFFFF; // RA2: as above
		}
		break;
	}
	return(FALSE);
}



/// <summary>
/// Displays the version information dialog.
/// This routine does not return until the player dismisses the dialog, and keeps the
/// title screen alive behind it while it waits.
/// </summary>
void Version_Dialog(void)
{
	int res = 0;

	HWND dialog = OwnerDraw::Begin_Dialog(IDD_VERSION, (DLGPROC)Version_Dialog_Proc); // RA2: 0x00516E70 is not static; opens dialog 212 with the procedure above
	if (dialog != NULL) {
		SetWindowLong(dialog, DWL_USER, (LONG)&res);
		OwnerDraw::Display_Dialog(dialog);
		while (res == 0) {
			if (OwnerDraw::Dialog_Message_Handler() == true) {
				break;
			}
			Title_Screen_Restore();
		}
		OwnerDraw::End_Dialog(dialog);
	}
}



/***************************************************************************
 * Main_Menu -- Menu processing                                            *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      index of item selected, -1 if time out                             *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/17/1995 BRR : Created.                                             *
 *=========================================================================*/
int Main_Menu(unsigned int timeout)
{
	int retval = MENU_NONE; // RA2: 0x00516F00: RA2's menu returns MENU_* values, centres its dialog on the main window and shows the flag; the cheat check is inlined

	CD::Set_Required_Disk(DISK_ANY);
	if (CD::Force_Available() == false) {
		return(MENU_EXIT); // RA2: as above
	}

	HWND dialog = OwnerDraw::Begin_Dialog(IDD_MAIN_MENU, (DLGPROC)Main_Menu_Dialog_Proc); // RA2: as above
	if (dialog != NULL) {
		SetWindowLong(dialog, DWL_USER, (LONG)&retval);
		Center_Window_Within_Window(dialog, MainWindow); // RA2: as above
		OwnerDraw::Display_Dialog(dialog);
		SetFocus(MainWindow);
		Show_Dialog_With_Flag(dialog); // RA2: as above

		do {
			if (OwnerDraw::Dialog_Message_Handler()) { // RA2: as above
				retval = MENU_EXIT; // RA2: as above
			}
			Title_Screen_Restore();

			if (Keyboard->Check()) {
				int input = Keyboard->Get(); // RA2: as above
				if ((input & 0x800) == 0) { // RA2: as above
					if (Cheat_Key_Process((char)input) == true) {
						VocClass::PlayGlobal(Rule->OptionsChanged, 1.0f, 0x2000); // RA2: as above
						Title_Screen_Restore(true);
					}
				}
			}
		} while (retval == MENU_NONE); // RA2: as above

		OwnerDraw::End_Dialog(dialog);

		SYSTEMTIME t;
		GetSystemTime(&t);
		CryptRandom.Seed_Byte(t.wMilliseconds);
	} else {
		retval = MENU_EXIT; // RA2: as above
	}

	SetFocus(MainWindow);
	return(retval);
}



/// <summary>
/// Handles the messages for the main menu dialog.
/// This routine records the button the player pressed into the result that Main_Menu is
/// waiting upon, and greys out the load button when there is nothing to load.
/// </summary>
BOOL CALLBACK Main_Menu_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	int rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (rc) {
		return(rc);
	}

	int * res = (int *)GetWindowLong(window, DWL_USER); // RA2: 0x005171D0: RA2's buttons are 0x578 (IDC_NETWORK), 0x55C, IDC_EXIT_GAME, 0x683, 0x686 and 0x684; control 0x71D shows the version

	switch (message) {
		case WM_USER + 151: // RA2: as above
		{
			HWND control = GetDlgItem(window, 0x71D); // RA2: as above
			wchar_t text[128]; // RA2: as above
			swprintf(text, L"%s %s", StringTable::LoadString("GUI:Version", NULL, "D:\\Ra2\\Init.CPP", 5723), Narrow_To_Wide(VerNum.Version_Name())); // RA2: as above
			SendMessage(control, WM_USER + 178, 0, (LPARAM)text); // RA2: as above
			return(FALSE);
		}

		case WM_COMMAND: // RA2: as above
			switch (LOWORD(wparam)) {
				case 0x578: // RA2: as above
					*res = MENU_NETWORK; // RA2: as above
					return(FALSE);

				case 0x55C: // RA2: as above
					*res = MENU_OPTIONS; // RA2: as above
					return(FALSE);

				case IDC_EXIT_GAME:
					*res = MENU_EXIT_ASK; // RA2: as above
					return(FALSE);

				case 0x683: // RA2: as above
					*res = MENU_DIALOG_100; // RA2: as above
					return(FALSE);

				case 0x686: // RA2: as above
					*res = MENU_DIALOG_101; // RA2: as above
					return(FALSE);

				case 0x684: // RA2: as above
					*res = MENU_INTERNET; // RA2: as above
					return(FALSE);
			}
			break;

		case WM_PAINT: // RA2: as above
		{
			HWND control = GetDlgItem(window, 0x71A); // RA2: as above
			SendMessage(control, WM_USER + 240, 0, 0); // RA2: as above
			break;
		}
	}

	return(FALSE);
}



/// <summary>
/// Redraws the title screen if the display surfaces have been lost.
/// The menu dialogs call this routine from their message loops, so that the background
/// comes back after the display has been taken away and handed back to the game.
/// </summary>
/// <param name="force">Should the title screen be redrawn even if nothing was lost?</param>
void Title_Screen_Restore(bool force)
{
	if (SurfacesRestored || force) { // RA2: 0x00517370 darkens the title screen with Darken_Surface instead of loading the menu background
		SurfacesRestored = false;
		Darken_Surface(NULL, true); // RA2: as above
		Update_Visible_Surface(false, HiddenSurface);
	}
}

/// <summary>
/// Does nothing.
/// Leave_Game calls this routine first.
/// </summary>
void Leave_Game_Hook(void) // RA2: 0x005173A0, not in TibSun: an empty function that Leave_Game calls first (session.cpp)
{
}
