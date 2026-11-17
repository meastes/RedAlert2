/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/*
**	RA2: the loading screen of a scenario (LdPrgMgr.cpp in the executable's debug text): its
**	areas, the art it draws once into a surface of its own, and the progress screen's hook that
**	hands that surface back. TibSun has no such file; the code is the executable's from 0x00535720.
*/

#define RA2_GEOMETRY_H // RA2: geometry.h follows the includes and the rectangle below, as the executable initializes the rectangle first (0x00535720)
#include "always.h" // RA2: the includes this file needs
#include <string.h> // RA2: as above
#include <stdio.h> // RA2: as above
#include <wchar.h> // RA2: as above
#include "abstract.h" // RA2: as above
#include "_pk.h" // RA2: as above
#include "_surface.h" // RA2: as above
#include "bitfont.h" // RA2: as above
#include "bsurface.h" // RA2: as above
#include "convert.h" // RA2: as above
#include "data.h" // RA2: as above
#include "dialog.h" // RA2: as above
#include "draw.h" // RA2: as above
#include "dsurface.h" // RA2: as above
#include "mixfile.h" // RA2: as above
#include "misc.h" // RA2: as above
#include "progress.h" // RA2: as above
#include "rgb.h" // RA2: as above
#include "scheme.h" // RA2: as above
#include "rect.h" // RA2: as above
#include "scenario.h" // RA2: as above
#include "session.h" // RA2: as above
#undef LoadString // RA2: as above
#include "stringtable.h" // RA2: as above
#include "ldprgmgr.h" // RA2: as above


#include "geometry.h" // RA2: this module holds its own copy of the geometry constants (initialized after the rectangle, 0x00535740 to 0x005358F0)

extern int const ScreenWidths[]; // RA2: {640, 800, 1024} at 0x007ADD68
extern int const ScreenHeights[]; // RA2: {480, 600, 768} at 0x007ADD74
char const * __fastcall Get_Title_Bar_File(int width, int height, int side); // RA2: 0x006F2400 (txtlabel2.cpp)
char const * __fastcall Get_Splash_Bar_File(int width, int height, int side); // RA2: 0x006F2420 (txtlabel2.cpp)
ConvertClass * __cdecl Get_Load_Screen_Drawer(void); // RA2: 0x006F2450 (txtlabel2.cpp)
ConvertClass * __cdecl Get_Load_Screen_Side_Drawer(void); // RA2: 0x006F2470 (txtlabel2.cpp)
ConvertClass * __cdecl Get_Splash_Bar_Drawer(void); // RA2: 0x006F2490 (txtlabel2.cpp)
ConvertClass * __cdecl Get_MP_Load_Drawer(void); // RA2: 0x006F25B0 (txtlabel2.cpp)
void __fastcall Inflate_Rect(Rect & rect, int amount); // RA2: 0x006F1B00 (txtlabel2.cpp)
void __fastcall Blend_Rect(Rect * area, Surface * surface, int color, int alpha); // RA2: 0x005FEB60 (ownrdraw.cpp), blends a color over the rectangle
namespace OwnerDraw { // RA2: as above
	int __fastcall Draw_Text_In_Rect_Ex(Surface * surface, wchar_t const * text, RECT * area, BitFont * font, unsigned long color, int flags, int unknown7, int unknown8, int arg9, int arg10); // RA2: 0x005FE020 (ownrdraw.cpp)
}
using OwnerDraw::Draw_Text_In_Rect_Ex; // RA2: as above
Rect __fastcall Get_Briefing_Area(void); // RA2: 0x00536B80, defined below
Rect __fastcall Get_Loading_Text_Area(void); // RA2: 0x00536C00, defined below
void __fastcall Shade_Text_Area(wchar_t const * text, Surface * surface, int x, int y, int width, int flags, bool fill); // RA2: 0x00536C70, defined below
void __fastcall Draw_Area_Text(wchar_t const * text, Surface * surface, Rect const & area, unsigned short color, int flags); // RA2: 0x00536D30, defined below

char const * UKShapeName = "ls%sukingdom.shp"; // RA2: 0x007DF370, the picture of the British
char const * CubaShapeName = "ls%scuba.shp"; // RA2: 0x007DF374
char const * FranceShapeName = "ls%sfrance.shp"; // RA2: 0x007DF378
char const * GermanyShapeName = "ls%sgermany.shp"; // RA2: 0x007DF37C
char const * IraqShapeName = "ls%siraq.shp"; // RA2: 0x007DF380
char const * KoreaShapeName = "ls%skorea.shp"; // RA2: 0x007DF384
char const * LibyaShapeName = "ls%slibya.shp"; // RA2: 0x007DF388
char const * RussiaShapeName = "ls%srussia.shp"; // RA2: 0x007DF38C
char const * USShapeName = "ls%sustates.shp"; // RA2: 0x007DF390
char const * ObserverShapeName = "ls%sobs.shp"; // RA2: 0x007DF394, the picture of an observer
char const * Suffix640 = "640"; // RA2: 0x007DF398, the picture's size for 640 wide screens
char const * Suffix800 = "800"; // RA2: 0x007DF39C, the picture's size for larger screens

void __fastcall Load_Loading_Screen_Palettes(int side); // RA2: 0x006F24A0 (txtlabel2.cpp)
void __fastcall Free_Loading_Screen_Palettes(void); // RA2: 0x006F2510 (txtlabel2.cpp)
void __fastcall Load_MP_Load_Palette(void); // RA2: 0x006F25C0 (txtlabel2.cpp)
void __fastcall Free_MP_Load_Palette(void); // RA2: 0x006F26D0 (txtlabel2.cpp)

LoadProgressManager * LoadProgressManager::Instance; // RA2: 0x00A6E7C4

/// <summary>
/// The loading screen, made on first use.
/// </summary>
LoadProgressManager * LoadProgressManager::Create(void) // RA2: 0x00535910
{
	if (Instance == NULL) { // RA2: the object is made once
		Instance = new LoadProgressManager; // RA2: as above
	}
	return Instance; // RA2: as above
}

/// <summary>
/// Releases and deletes the loading screen.
/// </summary>
void LoadProgressManager::Destroy(void) // RA2: 0x00535970
{
	if (Instance != NULL) { // RA2: nothing to do when the screen was never made
		Instance->Release(); // RA2: as above
		delete Instance; // RA2: as above
	}
	Instance = NULL; // RA2: as above
}

/// <summary>
/// Starts with nothing loaded.
/// </summary>
LoadProgressManager::LoadProgressManager(void) // RA2: 0x005359A0
{
	LoadMix = NULL; // RA2: nothing is loaded yet
	LoadMessage = NULL; // RA2: as above
	LoadBriefing = NULL; // RA2: as above
	TitleBarShape = NULL; // RA2: as above
	PictureShape = NULL; // RA2: as above
	TextBarShape = NULL; // RA2: as above
	TitleBarShapeLoaded = false; // RA2: as above
	PictureShapeLoaded = false; // RA2: as above
	TextBarShapeLoaded = false; // RA2: as above
	Field50 = 0; // RA2: as above
	Field54 = 0; // RA2: as above
	Field58 = 0; // RA2: as above
	ProgressSurface = NULL; // RA2: as above
}

/// <summary>
/// Places the areas by the screen size: a campaign stacks the title bar, the picture and the bar
/// below it, centred; a multiplayer game has the picture only, centred.
/// </summary>
void LoadProgressManager::Compute_Rects(void) // RA2: 0x005359E0
{
	int render_width = VideoModeWidth; // RA2: the screen size decides everything
	bool low_res = render_width == 640; // RA2: 640 wide uses the first size of each table, anything else the second
	int width = low_res ? ScreenWidths[0] : ScreenWidths[1]; // RA2: as above
	if (Session.Type == GAME_NORMAL) { // RA2: a campaign
		int height = (low_res ? ScreenHeights[0] : ScreenHeights[1]) - 80; // RA2: the picture leaves room for the two bars of 40
		int x = (render_width - width) / 2; // RA2: centred
		int y = (VideoModeHeight - height - 80) / 2; // RA2: as above
		TextBarRect.Width = PictureRect.Width = TitleBarRect.Width = width; // RA2: the three areas are as wide as the screen size
		PictureRect.X = TitleBarRect.X = x; // RA2: as above
		TitleBarRect.Y = y; // RA2: the title bar is on top
		TitleBarRect.Height = 40; // RA2: as above
		y += 40; // RA2: the picture follows
		PictureRect.Y = y; // RA2: as above
		TextBarRect.X = x; // RA2: as above
		PictureRect.Height = height; // RA2: as above
		y += height; // RA2: the bar below follows
		TextBarRect.Y = y; // RA2: as above
		TextBarRect.Height = 40; // RA2: as above
	} else {
		int height = low_res ? ScreenHeights[0] : ScreenHeights[1]; // RA2: a multiplayer game has the picture only
		PictureRect.Set((render_width - width) / 2, (VideoModeHeight - height) / 2, width, height); // RA2: as above
	}
}

Point2D __fastcall Copy_Point(Point2D const * source); // RA2: 0x006F1AE0 (txtlabel2.cpp)

/// <summary>
/// Where the progress bar's text starts: in a campaign inside the bar below the picture, in a
/// multiplayer game at an offset from the picture's corner.
/// </summary>
Point2D LoadProgressManager::Text_Origin(void) // RA2: 0x00535AB0
{
	Point2D p; // RA2: the result
	if (Session.Type == GAME_NORMAL) { // RA2: a campaign
		p.X = TextBarRect.X; // RA2: inside the bar below the picture
		if (VideoModeWidth == ScreenWidths[0]) { // RA2: the offset depends on the screen size
			p.X += 0x54; // RA2: as above
		} else {
			p.X += 0xA4; // RA2: as above
		}
		p.Y = TextBarRect.Y + 7; // RA2: as above
	} else {
		p = Copy_Point((Point2D const *)&PictureRect); // RA2: the picture's corner
		if (VideoModeWidth == ScreenWidths[0]) { // RA2: the offset depends on the screen size
			p.X += 0xC; // RA2: as above
			p.Y += 0x100; // RA2: as above
		} else {
			p.X += 0x10; // RA2: as above
			p.Y += 0x141; // RA2: as above
		}
	}
	return p; // RA2: as above
}

/// <summary>
/// The width of the names beside the progress bars: -1 in a campaign, 326 or 406 otherwise.
/// </summary>
int LoadProgressManager::Get_Row_Width(void) // RA2: 0x00535B60
{
	int width = -1; // RA2: a campaign has no names
	if (Session.Type != GAME_NORMAL) { // RA2: as above
		width = VideoModeWidth == ScreenWidths[0] ? 326 : 406; // RA2: the width follows the screen size
	}
	return width; // RA2: as above
}

/// <summary>
/// Opens LOAD.MIX once and loads the palettes of the art: the campaign's by the player's side,
/// the multiplayer one otherwise.
/// </summary>
void LoadProgressManager::Load_Mixes(void) // RA2: 0x00535B90
{
	if (LoadMix == NULL) { // RA2: opened once
		LoadMix = new MixFileClass("LOAD.MIX", &FastKey); // RA2: as above
		if (LoadMix == NULL) { // RA2: nothing to load from
			return; // RA2: as above
		}
	}
	if (Session.Type == GAME_NORMAL) { // RA2: a campaign
		Load_Loading_Screen_Palettes(Scen->IsGDI == 0); // RA2: the art of the player's side
	} else {
		Load_MP_Load_Palette(); // RA2: as above
	}
}

/// <summary>
/// Whether the local player watches the game instead of playing it.
/// </summary>
inline bool Is_Observer(void) // RA2: the observer's slot has -1 as its team
{
	return (Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) && Session.Players[0]->Slot.SpectatorFlag == -1; // RA2: only games over a network have observers
}

/// <summary>
/// The name of the picture file of a country, or of the observer, for a screen size.
/// </summary>
inline void Loading_Filename(char * filename, int side, char const * suffix) // RA2: as above
{
	if (Is_Observer()) { // RA2: the observer has a picture of its own
		sprintf(filename, ObserverShapeName, suffix); // RA2: as above
	} else if (side == 0) { // RA2: the others by country
		sprintf(filename, USShapeName, suffix); // RA2: as above
	} else if (side == 1) { // RA2: as above
		sprintf(filename, KoreaShapeName, suffix); // RA2: as above
	} else if (side == 2) { // RA2: as above
		sprintf(filename, FranceShapeName, suffix); // RA2: as above
	} else if (side == 3) { // RA2: as above
		sprintf(filename, GermanyShapeName, suffix); // RA2: as above
	} else if (side == 4) { // RA2: as above
		sprintf(filename, UKShapeName, suffix); // RA2: as above
	} else if (side == 5) { // RA2: as above
		sprintf(filename, LibyaShapeName, suffix); // RA2: as above
	} else if (side == 6) { // RA2: as above
		sprintf(filename, IraqShapeName, suffix); // RA2: as above
	} else if (side == 7) { // RA2: as above
		sprintf(filename, CubaShapeName, suffix); // RA2: as above
	} else if (side == 8) { // RA2: as above
		sprintf(filename, RussiaShapeName, suffix); // RA2: as above
	}
}

/// <summary>
/// The area of the country's name, 200 by 20 at the bottom, centred on screens past 800 by 600.
/// </summary>
inline void Name_Area(Rect & area) // RA2: as above
{
	if (VideoModeWidth == ScreenWidths[0]) { // RA2: the area depends on the screen size
		area.X = 385; // RA2: as above
		area.Y = 436; // RA2: as above
	} else {
		area.X = 533; // RA2: as above
		area.Y = 545; // RA2: as above
	}
	area.Width = 200; // RA2: as above
	area.Height = 20; // RA2: as above
	if (VideoModeWidth >= ScreenWidths[1]) { // RA2: centred on screens past 800 by 600
		area.X += (VideoModeWidth - ScreenWidths[1]) / 2; // RA2: as above
		area.Y += (VideoModeHeight - ScreenHeights[1]) / 2; // RA2: as above
	}
}

/// <summary>
/// The area of the name of the country's unit, 200 by 20 at the top, centred on screens past 800 by 600.
/// </summary>
inline Rect Unit_Area(void) // RA2: as above
{
	Rect area; // RA2: the result
	if (VideoModeWidth == ScreenWidths[0]) { // RA2: the area depends on the screen size
		area.X = 16; // RA2: as above
		area.Y = 72; // RA2: as above
	} else {
		area.X = 20; // RA2: as above
		area.Y = 90; // RA2: as above
	}
	area.Width = 200; // RA2: as above
	area.Height = 20; // RA2: as above
	if (VideoModeWidth >= ScreenWidths[1]) { // RA2: centred on screens past 800 by 600
		area.X += (VideoModeWidth - ScreenWidths[1]) / 2; // RA2: as above
		area.Y += (VideoModeHeight - ScreenHeights[1]) / 2; // RA2: as above
	}
	return area; // RA2: as above
}

/// <summary>
/// Changes the lower case letters of a text to capitals, those of the Latin-1 letters from 0xE0 to 0xFE too.
/// </summary>
inline void Upper_Case(wchar_t * string) // RA2: as above
{
	for (wchar_t * p = string; *p != 0; p++) { // RA2: every character of the text
		if ((*p >= 'a' && *p <= 'z') || (*p >= 0xE0 && *p <= 0xFE)) { // RA2: a lower case letter
			*p -= 0x20; // RA2: its capital is 32 below
		}
	}
}

/// <summary>
/// The name of a country, or of the observer, from the string table.
/// </summary>
inline wchar_t const * Country_Name(int side) // RA2: as above
{
	if (Is_Observer()) { // RA2: the observer has a name of its own
		return StringTable::LoadString("Name:Observer", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 162); // RA2: as above
	}
	if (side == 0) { // RA2: the others by country
		return StringTable::LoadString("Name:Americans", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 167); // RA2: as above
	}
	if (side == 1) { // RA2: as above
		return StringTable::LoadString("Name:Alliance", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 169); // RA2: as above
	}
	if (side == 2) { // RA2: as above
		return StringTable::LoadString("Name:French", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 171); // RA2: as above
	}
	if (side == 3) { // RA2: as above
		return StringTable::LoadString("Name:Germans", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 173); // RA2: as above
	}
	if (side == 4) { // RA2: as above
		return StringTable::LoadString("Name:British", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 175); // RA2: as above
	}
	if (side == 5) { // RA2: as above
		return StringTable::LoadString("Name:Africans", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 177); // RA2: as above
	}
	if (side == 6) { // RA2: as above
		return StringTable::LoadString("Name:Arabs", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 179); // RA2: as above
	}
	if (side == 7) { // RA2: as above
		return StringTable::LoadString("Name:Confederation", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 181); // RA2: as above
	}
	if (side == 8) { // RA2: as above
		return StringTable::LoadString("Name:Russians", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 183); // RA2: as above
	}
	return NULL; // RA2: as above
}

/// <summary>
/// The name of a country's unit from the string table.
/// </summary>
inline wchar_t const * Unit_Name(int side) // RA2: as above
{
	if (side == 0) { // RA2: by country
		return StringTable::LoadString("Name:Para", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 229); // RA2: as above
	}
	if (side == 1) { // RA2: as above
		return StringTable::LoadString("Name:BEAGLE", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 231); // RA2: as above
	}
	if (side == 2) { // RA2: as above
		return StringTable::LoadString("Name:GTGCAN", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 233); // RA2: as above
	}
	if (side == 3) { // RA2: as above
		return StringTable::LoadString("Name:TNKD", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 235); // RA2: as above
	}
	if (side == 4) { // RA2: as above
		return StringTable::LoadString("Name:SNIPE", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 237); // RA2: as above
	}
	if (side == 5) { // RA2: as above
		return StringTable::LoadString("Name:DTRUCK", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 239); // RA2: as above
	}
	if (side == 6) { // RA2: as above
		return StringTable::LoadString("Name:DESO", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 241); // RA2: as above
	}
	if (side == 7) { // RA2: as above
		return StringTable::LoadString("Name:TERROR", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 243); // RA2: as above
	}
	if (side == 8) { // RA2: as above
		return StringTable::LoadString("Name:TTNK", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 245); // RA2: as above
	}
	return NULL; // RA2: as above
}

/// <summary>
/// The briefing of a country from the string table.
/// </summary>
inline wchar_t const * Briefing(int side) // RA2: as above
{
	if (side == 0) { // RA2: by country
		return StringTable::LoadString("LoadBrief:USA", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 278); // RA2: as above
	}
	if (side == 1) { // RA2: as above
		return StringTable::LoadString("LoadBrief:Korea", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 280); // RA2: as above
	}
	if (side == 2) { // RA2: as above
		return StringTable::LoadString("LoadBrief:French", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 282); // RA2: as above
	}
	if (side == 3) { // RA2: as above
		return StringTable::LoadString("LoadBrief:Germans", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 284); // RA2: as above
	}
	if (side == 4) { // RA2: as above
		return StringTable::LoadString("LoadBrief:British", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 286); // RA2: as above
	}
	if (side == 5) { // RA2: as above
		return StringTable::LoadString("LoadBrief:Lybia", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 288); // RA2: as above
	}
	if (side == 6) { // RA2: as above
		return StringTable::LoadString("LoadBrief:Iraq", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 290); // RA2: as above
	}
	if (side == 7) { // RA2: as above
		return StringTable::LoadString("LoadBrief:Cuba", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 292); // RA2: as above
	}
	if (side == 8) { // RA2: as above
		return StringTable::LoadString("LoadBrief:Russia", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 294); // RA2: as above
	}
	return NULL; // RA2: as above
}

/// <summary>
/// Draws the whole screen into a surface of its own: in a campaign the title bar, the picture and the bar
/// below it with the scenario's texts; in a multiplayer game the picture of the player's country with its
/// name, the name of its unit, a briefing and the word for loading.
/// </summary>
void LoadProgressManager::Draw(void) // RA2: 0x00535BF0
{
	wchar_t text[2048]; // RA2: the briefing, copied for drawing
	wchar_t unit_name[128]; // RA2: the name of the country's unit, in capitals
	char filename[32]; // RA2: the picture's file name

	bool low_res = VideoModeWidth == ScreenWidths[0]; // RA2: 640 wide uses the first size of each table
	ProgressSurface = new BSurface(VideoModeWidth, VideoModeHeight, 2); // RA2: a surface the size of the screen, two bytes a pixel
	if (ProgressSurface == NULL) { // RA2: nothing to draw into
		return; // RA2: as above
	}
	Rect screen(0, 0, 0, 0); // RA2: the whole surface
	int background = DSurface::Build_Hicolor_Pixel(BlackColor); // RA2: the screen is cleared to black
	screen.Width = VideoModeWidth; // RA2: as above
	screen.Height = VideoModeHeight; // RA2: as above
	ProgressSurface->Fill_Rect(screen, screen, background); // RA2: as above

	if (Session.Type == GAME_NORMAL) { // RA2: a campaign
		char const * name = low_res ? Scen->LS640BkgdName : Scen->LS800BkgdName; // RA2: the picture the scenario names
		if (*name != '\0') { // RA2: no picture, no screen
			LoadMessage = NULL; // RA2: the texts the scenario names
			if (Scen->LSLoadMessage[0] != '\0') { // RA2: as above
				LoadMessage = StringTable::LoadString(Scen->LSLoadMessage, NULL, "D:\\Ra2\\LdPrgMgr.cpp", 591); // RA2: as above
			}
			LoadBriefing = NULL; // RA2: as above
			if (Scen->LSBrief[0] != '\0') { // RA2: as above
				LoadBriefing = StringTable::LoadString(Scen->LSBrief, NULL, "D:\\Ra2\\LdPrgMgr.cpp", 595); // RA2: as above
			}
			int side = Progress.Get_Side(); // RA2: the art follows the player's side
			char const * title_name = Get_Title_Bar_File(VideoModeWidth, VideoModeHeight, side); // RA2: as above
			char const * bar_name = Get_Splash_Bar_File(VideoModeWidth, VideoModeHeight, side); // RA2: as above
			TitleBarShape = Load_Alloc_Data(title_name, TitleBarShapeLoaded); // RA2: the three shapes
			TextBarShape = Load_Alloc_Data(bar_name, TextBarShapeLoaded); // RA2: as above
			PictureShape = Load_Alloc_Data(name, PictureShapeLoaded); // RA2: as above
			ConvertClass * title_drawer = Get_Load_Screen_Drawer(); // RA2: their drawers
			ConvertClass * picture_drawer = Get_Load_Screen_Side_Drawer(); // RA2: as above
			ConvertClass * bar_drawer = Get_Splash_Bar_Drawer(); // RA2: as above

			Point2D point(TitleBarRect.X, TitleBarRect.Y); // RA2: the title bar's corner
			if (TitleBarShape != NULL) { // RA2: a shape when the file has one, black otherwise
				Draw_Shape(*ProgressSurface, *title_drawer, (ShapeSet const *)TitleBarShape, 0, point, screen, SHAPE_WIN_REL, NULL, 0, ZGRAD_GROUND, 1000, NULL, 0, Point2D(0, 0)); // RA2: as above
			} else {
				ProgressSurface->Fill_Rect(screen, TitleBarRect, background); // RA2: as above
			}
			point.Y = PictureRect.Y; // RA2: the picture
			Draw_Shape(*ProgressSurface, *picture_drawer, (ShapeSet const *)PictureShape, 0, point, screen, SHAPE_WIN_REL, NULL, 0, ZGRAD_GROUND, 1000, NULL, 0, Point2D(0, 0)); // RA2: as above
			point.Y = TextBarRect.Y; // RA2: the bar below
			if (TextBarShape != NULL) { // RA2: as above
				Draw_Shape(*ProgressSurface, *bar_drawer, (ShapeSet const *)TextBarShape, 0, point, screen, SHAPE_WIN_REL, NULL, 0, ZGRAD_GROUND, 1000, NULL, 0, Point2D(0, 0)); // RA2: as above
			} else {
				ProgressSurface->Fill_Rect(screen, TextBarRect, background); // RA2: as above
			}

			point.X = TitleBarRect.X + 10; // RA2: the message sits in the title bar, ten pixels in
			point.Y = TitleBarRect.Y + 10; // RA2: as above
			ColorScheme * scheme = Progress.Side_Scheme(); // RA2: in the side's color
			if (LoadMessage != NULL) { // RA2: as above
				Fancy_Text_Print(LoadMessage, *ProgressSurface, screen, point, scheme, 0, TextPrintType(TPF_EFNT | TPF_NOSHADOW)); // RA2: as above
			}
			if (LoadBriefing != NULL) { // RA2: the briefing sits in the picture
				if (low_res) { // RA2: as above
					point.X = Scen->LS640BriefLocX + PictureRect.X; // RA2: the scenario names where
					point.Y = Scen->LS640BriefLocY + PictureRect.Y; // RA2: as above
				} else {
					point.X = Scen->LS800BriefLocX + PictureRect.X; // RA2: as above
					point.Y = Scen->LS800BriefLocY + PictureRect.Y; // RA2: as above
				}
				wcscpy(text, LoadBriefing); // RA2: as above
				RGBClass pixel_color = scheme->HSV; // RA2: the scheme's base color
				int pixel = DSurface::Build_Hicolor_Pixel(pixel_color); // RA2: as a pixel
				int width = 0; // RA2: as above
				int height = 0; // RA2: as above
				BitFont::Instance->Text_Dimensions(text, &width, &height, 400); // RA2: as above
				Rect area(point.X, point.Y, width, height); // RA2: as above
				Shade_Text_Area(text, ProgressSurface, point.X, point.Y, 400, 0, false); // RA2: as above
				Draw_Area_Text(text, ProgressSurface, area, pixel, 0); // RA2: as above
			}
		}
		return; // RA2: as above
	}

	{
		int side = Session.Players[0]->Get_Country(); // RA2: a multiplayer game shows the player's country
		char const * suffix = VideoModeWidth == ScreenWidths[0] ? Suffix640 : Suffix800; // RA2: the picture follows the screen size
		Loading_Filename(filename, side, suffix); // RA2: as above
		PictureShape = Load_Alloc_Data(filename, PictureShapeLoaded); // RA2: the picture
		ConvertClass * drawer = Get_MP_Load_Drawer(); // RA2: as above
		Point2D point = Copy_Point((Point2D const *)&PictureRect); // RA2: drawn at the picture's corner
		Draw_Shape(*ProgressSurface, *drawer, (ShapeSet const *)PictureShape, 0, point, screen, SHAPE_WIN_REL, NULL, 0, ZGRAD_GROUND, 1000, NULL, 0, Point2D(0, 0)); // RA2: as above

		ColorScheme * scheme = Progress.Text_Scheme(); // RA2: the texts are in the text color
		RGBClass pixel_color = scheme->HSV; // RA2: as above
		int pixel = DSurface::Build_Hicolor_Pixel(pixel_color); // RA2: as above

		wchar_t const * country_name = Country_Name(side); // RA2: the country's name sits at the bottom
		Rect area; // RA2: as above
		Name_Area(area); // RA2: as above
		point = Copy_Point((Point2D const *)&area); // RA2: as above
		Shade_Text_Area(country_name, ProgressSurface, point.X, point.Y, area.Width, 2, false); // RA2: as above
		Draw_Area_Text(country_name, ProgressSurface, area, pixel, 2); // RA2: as above

		if (!Is_Observer()) { // RA2: an observer has no unit and no briefing
			wcscpy(unit_name, Unit_Name(side)); // RA2: the unit's name, in capitals, at the top left in black
			Upper_Case(unit_name); // RA2: as above
			area = Unit_Area(); // RA2: as above
			int unit_pixel = DSurface::Build_Hicolor_Pixel(BlackColor); // RA2: as above
			Draw_Area_Text(unit_name, ProgressSurface, area, unit_pixel, 0); // RA2: as above

			wchar_t const * brief = Briefing(side); // RA2: the country's briefing is drawn in an area of its own
			area = Get_Briefing_Area(); // RA2: as above
			point = Copy_Point((Point2D const *)&area); // RA2: as above
			Shade_Text_Area(brief, ProgressSurface, point.X, point.Y, area.Width, 0, true); // RA2: as above
			Draw_Area_Text(brief, ProgressSurface, area, pixel, 0); // RA2: as above
		}

		wchar_t const * loading = StringTable::LoadString("GUI:LoadingEx", NULL, "D:\\Ra2\\LdPrgMgr.cpp", 749); // RA2: the word for loading
		area = Get_Loading_Text_Area(); // RA2: as above
		point = Copy_Point((Point2D const *)&area); // RA2: as above
		Shade_Text_Area(loading, ProgressSurface, point.X, point.Y, area.Width, 0, false); // RA2: as above
		Draw_Area_Text(loading, ProgressSurface, area, pixel, 0); // RA2: as above
	}
}

/// <summary>
/// The area of the briefing text, 318 by 104 at 640 wide and 398 by 130 otherwise, centred on larger screens.
/// </summary>
Rect __fastcall Get_Briefing_Area(void) // RA2: 0x00536B80
{
	Rect area; // RA2: the result
	if (VideoModeWidth == ScreenWidths[0]) { // RA2: the area depends on the screen size
		area = Rect(16, 126, 318, 104); // RA2: as above
	} else {
		area = Rect(20, 158, 398, 130); // RA2: as above
	}
	if (VideoModeWidth >= ScreenWidths[1]) { // RA2: centred on screens past 800 by 600
		area.X += (VideoModeWidth - ScreenWidths[1]) / 2; // RA2: as above
		area.Y += (VideoModeHeight - ScreenHeights[1]) / 2; // RA2: as above
	}
	return area; // RA2: as above
}

/// <summary>
/// The area of the word for loading, 200 by 20, centred on larger screens.
/// </summary>
Rect __fastcall Get_Loading_Text_Area(void) // RA2: 0x00536C00
{
	Rect area; // RA2: the result
	if (VideoModeWidth == ScreenWidths[0]) { // RA2: the area depends on the screen size
		area = Rect(16, 235, 200, 20); // RA2: as above
	} else {
		area = Rect(20, 300, 200, 20); // RA2: as above
	}
	if (VideoModeWidth >= ScreenWidths[1]) { // RA2: centred on screens past 800 by 600
		area.X += (VideoModeWidth - ScreenWidths[1]) / 2; // RA2: as above
		area.Y += (VideoModeHeight - ScreenHeights[1]) / 2; // RA2: as above
	}
	return area; // RA2: as above
}

/// <summary>
/// Darkens the area behind a text: the area the text takes from a corner and a width (flags 1 centres it, 2
/// aligns it to the right, fill makes it as wide as the width), moved in by four pixels.
/// </summary>
void __fastcall Shade_Text_Area(wchar_t const * text, Surface * surface, int x, int y, int width, int flags, bool fill) // RA2: 0x00536C70
{
	int text_width = 0; // RA2: the size of the text
	int text_height = 0; // RA2: as above
	BitFont::Instance->Text_Dimensions(text, &text_width, &text_height, width); // RA2: as above
	if (fill) { // RA2: the whole width
		text_width = width; // RA2: as above
	}
	Rect area; // RA2: as above
	area = Rect(x, y, text_width, text_height); // RA2: as above
	if (flags & 1) { // RA2: centred in the width
		area.X = (width - text_width) / 2 + x; // RA2: as above
	} else if (flags & 2) { // RA2: aligned to the right
		area.X = x - text_width + width; // RA2: as above
	}
	Inflate_Rect(area, -4); // RA2: as above
	Blend_Rect(&area, surface, 0, 159); // RA2: a veil of black, 159 of 255
}

/// <summary>
/// Draws a text in an area in a pixel color through the game font.
/// </summary>
void __fastcall Draw_Area_Text(wchar_t const * text, Surface * surface, Rect const & area, unsigned short color, int flags) // RA2: 0x00536D30
{
	RECT rect; // RA2: the area as a window rectangle
	rect.left = area.X; // RA2: as above
	rect.top = area.Y; // RA2: as above
	rect.right = area.X + area.Width; // RA2: as above
	rect.bottom = area.Y + area.Height; // RA2: as above
	RGBClass rgb = DSurface::Deconstruct_Hicolor_Pixel(color); // RA2: the drawing function takes a 24 bit color
	Draw_Text_In_Rect_Ex(surface, text, &rect, BitFont::Instance, rgb.Get_Red() | (rgb.Get_Green() << 8) | (rgb.Get_Blue() << 16), flags, 12, 0, 0, 0); // RA2: as above
}

/// <summary>
/// Copies the drawn screen onto a surface.
/// </summary>
void LoadProgressManager::Copy_Surface(Surface * target) // RA2: 0x00536E00
{
	if (ProgressSurface != NULL) { // RA2: only when the screen has been drawn
		target->Blit_From(*ProgressSurface, false, true); // RA2: as above
	}
}

/// <summary>
/// Frees the shapes the screen loaded itself, the palettes, LOAD.MIX and the surface.
/// </summary>
void LoadProgressManager::Release(void) // RA2: 0x00536E20
{
	if (TitleBarShapeLoaded && TitleBarShape != NULL) { // RA2: only the shapes read from a file are ours
		operator delete(TitleBarShape); // RA2: as above
	}
	TitleBarShape = NULL; // RA2: as above
	if (PictureShapeLoaded && PictureShape != NULL) { // RA2: as above
		operator delete(PictureShape); // RA2: as above
	}
	PictureShape = NULL; // RA2: as above
	if (TextBarShapeLoaded && TextBarShape != NULL) { // RA2: as above
		operator delete(TextBarShape); // RA2: as above
	}
	TextBarShape = NULL; // RA2: as above
	Free_Loading_Screen_Palettes(); // RA2: the palettes of both kinds of art
	Free_MP_Load_Palette(); // RA2: as above
	if (LoadMix != NULL) { // RA2: closes LOAD.MIX
		delete LoadMix; // RA2: as above
		LoadMix = NULL; // RA2: as above
	}
	delete ProgressSurface; // RA2: the surface the screen was drawn into
	ProgressSurface = NULL; // RA2: as above
}

/// <summary>
/// Answers the progress screen's redraw request (message 0x11AE) by copying the screen onto the
/// hidden surface.
/// </summary>
bool LoadProgressManager::Unknown_Virtual_0(int message, int data) // RA2: 0x00536EA0
{
	if (message == 0x11AE) { // RA2: the progress bar is redrawn
		Copy_Surface(HiddenSurface); // RA2: as above
		return true; // RA2: as above
	}
	return false; // RA2: as above
}
