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
#include "campaign.h" // RA2: CampaignClass::From_Name, Campaigns
#include "voc.h" // RA2: VocClass::PlayGlobal
#include "audioctl.h" // RA2: AudioController
#include "queue.h" // RA2: DoList and OutList
#include "_tooltip.h" // RA2: ToolTips
#include "cctooltip.h" // RA2: CCToolTip::Activate
#include "movie.h" // RA2: Movies, Play_Movie
#include "enviro.h" // RA2: Environment
#include "misc.h" // RA2: Set_Palette
#include "skirmish.h" // RA2: Skirmish_Mode_Dialog
#include "modemhst.h" // RA2: ModemHost
#include "modemgst.h" // RA2: ModemGuest
#include "wspipx.h" // RA2: IPXInterfaceClass
#include "netdlg2.h" // RA2: Net2Init_Network
#include "_timer.h" // RA2: TickCount
#include "mainopt.h" // RA2: Main_Options_Dialog
#include "vox.h" // RA2: Is_Speaking
#include "planning.h" // RA2: Planning_Reset_State
#include "vqoption.h" // RA2: Anim_Init sets the VQ option
#include "loaddlg.h" // RA2: LoadOptionsClass::Files_Present
#include "wspudp.h" // RA2: Start_Network_Session creates the UDP interface
#include "_wsproto.h" // RA2: PacketTransport
#include "ownertalk.h" // RA2: OwnerTalk.Reset
#include "netdlg.h" // RA2: Init_Network
#include "_rules.h" // RA2: ConfigINI, RuleINI
#include "houstype.h" // RA2: HouseTypeClass::Read_INI
#include "winstub.h"
#include "worlddom.h"
#include "ipxmgr.h" // RA2: Ipx.Set_Socket
#include "obscure.h" // RA2: Obfuscate
#include "wsproto.h" // RA2: Set_Packet_Loss_Percent, for -DROP=


#include <conio.h>
#include <ctime>
#include <dos.h>



#define	PARM_INSTALL		0xD95C68A2		//	"FROMINSTALL"

extern void __fastcall Set_Jabber_Option(int value); // RA2: 0x006F9FB0, the -jabber switch: stores the value and tells the text system
extern void __fastcall Set_Str_Option(int value); // RA2: 0x006F9FE0, the -str and -nostr switches, the same for a second text option
extern int AudioAllowed; // RA2: 0x007F4CC8, 0 with -noaudio
extern int RecordPlayRequest; // RA2: 0x00A41664, 1 with -record, 2 with -play; WinMain opens session.ipb for it
extern int RouterType; // RA2: 0x00B2AC30, 1 with -DLINK1, 2 with -NETGEAR
extern bool GameControlsShown; // RA2: 0x00A40D84, set by -SPEEDCONTROL
extern bool ExtraSyncInfo; // RA2: 0x00AB62E8, set by -MPDEBUG

/*
**	RA2: the values Main_Menu and the menu dialogs store for Select_Game. The numbers are what the
**	executable compares; the names say what the code around them does.
*/
enum MainMenuChoice { // RA2: as above
	MENU_TIMEOUT = -1,			// RA2: Main_Menu ran out of time: the attract mode may play a recording
	MENU_SINGLE_PLAYER = 1,		// RA2: the button that opens the single player dialog 0x100
	MENU_INTERNET = 2,			// RA2: Session.Type = GAME_INTERNET
	MENU_NETWORK = 3,			// RA2: IDC_NETWORK, Session.Type = GAME_IPX
	MENU_EXTRAS = 4,			// RA2: the button that opens the movies and credits dialog 0x101
	MENU_OPTIONS = 5,			// RA2: Main_Options_Dialog
	MENU_EXIT_ASK = 6,			// RA2: "GUI:ExitAreYouSure"
	MENU_EXIT = 7, // RA2: as above
	MENU_NEW_CAMPAIGN = 8,		// RA2: the single player dialog's first button: Choose_Campaign
	MENU_LOAD_GAME = 9,			// RA2: its second button, enabled while saved games exist
	MENU_SINGLE_AGAIN = 10,		// RA2: its third button; Select_Game shows the single player dialog again
	MENU_SKIRMISH = 11,			// RA2: its fourth button: Session.Type = GAME_SKIRMISH
	MENU_TRAILER = 13,			// RA2: movies dialog: plays the trailer
	MENU_MOVIES = 14,			// RA2: movies dialog: the list of movies
	MENU_CREDITS = 15,			// RA2: movies dialog: Show_Credits
	MENU_MULTIPLAYER = 0x10,	// RA2: back from a multiplayer game
	MENU_INTERNET_RETURN = 0x11,	// RA2: back from an Internet game
	MENU_NONE = 0x12			// RA2: nothing chosen yet
};

/*
**	RA2: an entry of the movie list that the movies dialog shows (OptionsClass::Fill_Movie_List fills it).
*/
struct MovieListEntry { // RA2: as above
	char const * Name;		// RA2: +0x00, the movie file
	char const * Label;		// RA2: +0x04, the string table entry shown in the list
	int CD;					// RA2: +0x08, the disc the movie is on
};

int MovieListChoice = -1; // RA2: 0x007DB558, the list entry that was chosen last

/***********************************************************************************************
 * Single_Player_Dialog_Proc -- Message handler of the single player dialog.                   *
 *                                                                                             *
 *    RA2's main menu has a button that opens this dialog; its buttons start a campaign,       *
 *    load a game or start a skirmish. TibSun has no such dialog.                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   RA2 has this dialog; it is not in TibSun.                                                 *
 *=============================================================================================*/
BOOL CALLBACK Single_Player_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) // RA2: 0x00513290, not in TibSun
{
	int rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam); // RA2: as above
	if (rc) { // RA2: as above
		return(rc); // RA2: as above
	} // RA2: as above

	int * choice = (int *)GetWindowLong(window, DWL_USER); // RA2: as above

	switch (message) { // RA2: as above
		case WM_PAINT: // RA2: as above
		{
			HWND control = GetDlgItem(window, 0x71A); // RA2: as above
			SendMessage(control, WM_USER + 240, 0, 0); // RA2: as above
			break; // RA2: as above
		}

		case WM_COMMAND: // RA2: as above
			switch (LOWORD(wparam)) { // RA2: as above
				case 0x688: // RA2: as above
					*choice = MENU_NEW_CAMPAIGN; // RA2: as above
					return(FALSE); // RA2: as above

				case 0x689: // RA2: as above
					*choice = MENU_LOAD_GAME; // RA2: as above
					return(FALSE); // RA2: as above

				case 0x68A: // RA2: as above
					*choice = MENU_SINGLE_AGAIN; // RA2: as above
					return(FALSE); // RA2: as above

				case 0x579: // RA2: as above
					*choice = MENU_SKIRMISH; // RA2: as above
					return(FALSE); // RA2: as above

				case 0x686: // RA2: as above
					*choice = MENU_NONE; // RA2: as above
					return(FALSE); // RA2: as above
			}
			break; // RA2: as above

		case WM_USER + 151: // RA2: as above
		{
			HWND control = GetDlgItem(window, 0x689); // RA2: as above
			if (control) { // RA2: as above
				bool saved = LoadOptionsClass().Files_Present(); // RA2: as above
				EnableWindow(control, saved); // RA2: as above
			} // RA2: as above
			break; // RA2: as above
		}
	}

	return(FALSE); // RA2: as above
}


/***********************************************************************************************
 * Extras_Dialog_Proc -- Message handler of the movies and credits dialog.                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   RA2 has this dialog; it is not in TibSun.                                                 *
 *=============================================================================================*/
BOOL CALLBACK Extras_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) // RA2: 0x005133E0, not in TibSun
{
	int rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam); // RA2: as above
	if (rc) { // RA2: as above
		return(rc); // RA2: as above
	} // RA2: as above

	int * choice = (int *)GetWindowLong(window, DWL_USER); // RA2: as above

	switch (message) { // RA2: as above
		case WM_PAINT: // RA2: as above
		{
			HWND control = GetDlgItem(window, 0x71A); // RA2: as above
			SendMessage(control, WM_USER + 240, 0, 0); // RA2: as above
			break; // RA2: as above
		}

		case WM_COMMAND: // RA2: as above
			switch (LOWORD(wparam)) { // RA2: as above
				case 0x68D: // RA2: as above
					*choice = MENU_TRAILER; // RA2: as above
					return(FALSE); // RA2: as above

				case 0x68E: // RA2: as above
					*choice = MENU_MOVIES; // RA2: as above
					return(FALSE); // RA2: as above

				case 0x68F: // RA2: as above
					*choice = MENU_CREDITS; // RA2: as above
					return(FALSE); // RA2: as above

				case 0x686: // RA2: as above
					*choice = MENU_NONE; // RA2: as above
					return(FALSE); // RA2: as above
			}
			break; // RA2: as above
	}

	return(FALSE); // RA2: as above
}


/***********************************************************************************************
 * Movie_List_Dialog_Proc -- Message handler of the list of movies.                            *
 *                                                                                             *
 *    The dialog lists the movies the player has unlocked; the choice is the list entry the    *
 *    player picked, or -1.                                                                    *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   RA2 has this dialog; it is not in TibSun.                                                 *
 *=============================================================================================*/
BOOL CALLBACK Movie_List_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) // RA2: 0x005134C0, not in TibSun
{
	bool chosen = false; // RA2: as above
	HWND list = GetDlgItem(window, 0x744); // RA2: as above

	int rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam); // RA2: as above
	if (rc) { // RA2: as above
		return(rc); // RA2: as above
	} // RA2: as above

	int * choice = (int *)GetWindowLong(window, DWL_USER); // RA2: as above

	switch (message) { // RA2: as above
		case WM_USER + 151: // RA2: as above
			if (list != NULL) { // RA2: as above
				Options.Fill_Movie_List(list); // RA2: as above
				if (SendMessage(list, LB_SETCURSEL, MovieListChoice, 0) == LB_ERR) { // RA2: as above
					SendMessage(list, LB_SETCURSEL, 0, 0); // RA2: as above
				} // RA2: as above
			} else { // RA2: as above
				if (choice != NULL) { // RA2: as above
					*choice = -1; // RA2: as above
				} // RA2: as above
				return(FALSE); // RA2: as above
			} // RA2: as above
			break; // RA2: as above

		case WM_COMMAND: // RA2: as above
			switch (LOWORD(wparam)) { // RA2: as above
				case 0x686: // RA2: as above
					if (choice != NULL) { // RA2: as above
						*choice = -1; // RA2: as above
					} // RA2: as above
					break; // RA2: as above

				case 0x744: // RA2: as above
					if (HIWORD(wparam) == 0) { // RA2: as above
						chosen = true; // RA2: as above
					} // RA2: as above
					break; // RA2: as above

				case 0x745: // RA2: as above
					chosen = true; // RA2: as above
					break; // RA2: as above
			}
			break; // RA2: as above
	}

	if (list != NULL && chosen) { // RA2: as above
		int index = SendMessage(list, LB_GETCURSEL, 0, 0); // RA2: as above
		MovieListChoice = index; // RA2: as above
		if (index != LB_ERR) { // RA2: as above
			int movie = SendMessage(list, LB_GETITEMDATA, index, 0); // RA2: as above
			if (movie != 0 && choice != NULL) { // RA2: as above
				*choice = movie; // RA2: as above
			} // RA2: as above
		} // RA2: as above
	} // RA2: as above

	return(FALSE); // RA2: as above
}


struct ChooseCampaignStruct {
	CampaignType ChosenCampaign;
	bool ChoiceMade;
};

extern char const * const Difficulty_Labels[3]; // RA2: 0x007D8404, the string table names of the three difficulty levels (TXT_EASY, TXT_NORMAL, TXT_HARD)
extern bool ScenarioSaveNameSet; // RA2: 0x00A6EBD0, as in mainloop.cpp
extern AudioController SideSound; // RA2: 0x00A411E8, the sound of the side that was chosen
extern TimeStamp SideSoundTime; // RA2: 0x00A41680, when the hovered side's sound may play
extern int HoveredSideButton; // RA2: 0x00A41688, the highlighted side button
extern bool CampaignListShown; // RA2: 0x00A4168D, the campaign list and its button are shown
extern HWND PressedSideButton; // RA2: 0x00A41690, the side button the mouse went down on
int PendingSideVoc = -1; // RA2: 0x007DB4F8, the side sound to play when the choice is made (-1 none)

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
extern char CampaignAlliedStart[]; // RA2: 0x007DB578
extern char CampaignSovietStart[]; // RA2: 0x007DB570
extern char CampaignBootCampStart[]; // RA2: 0x007DB568
char * CampaignStartNames[3] = {CampaignAlliedStart, CampaignSovietStart, CampaignBootCampStart}; // RA2: 0x007DB540, the first scenario of the Allied, Soviet and boot camp campaigns
int SovietSelectVoc = -1; // RA2: 0x007DB54C
int AlliedSelectVoc = -1; // RA2: 0x007DB550
int BootCampSelectVoc = -1; // RA2: 0x007DB554
char CampaignBootCampStart[] = "tut1"; // RA2: as above
char CampaignSovietStart[] = "sov1"; // RA2: as above
char CampaignAlliedStart[] = "all1"; // RA2: as above

BOOL CALLBACK Campaign_Choice_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x005147A0, after Select_Game
extern int ObserverMode; // RA2: 0x00A72C68
void Darken_Surface(Surface * surface, bool update); // RA2: 0x005159B0, defined below
void Init_Random(void); // RA2: 0x00515710, defined below
extern bool Net_Join_Dialog(void); // RA2: 0x005BD070 (netdlg2.cpp)
bool Load_Recording_Values(CCFileClass & file); // RA2: 0x00516BA0, defined below
bool Save_Recording_Values(CCFileClass & file); // RA2: as above
bool Start_Network_Session(void); // RA2: 0x00514EE0, defined below
extern void __fastcall Load_Side_Animation_Art(void); // RA2: 0x006F2C40, loads the animation the side buttons of the campaign dialog show
extern void __fastcall Free_Side_Animation_Art(void); // RA2: 0x006F2D00, frees it again
extern void __fastcall Show_Credits(void); // RA2: 0x004B37C0, scrolls CREDITS.TXT over the screen
extern int __fastcall Wonline_Main(bool returning); // RA2: 0x0073A300, the Internet game dialogs; returns -2 or 0 (back), -1 (quit), 1 (a game is ready), 2 (skirmish)
extern int __fastcall Dialog_Box(int resource, int (__stdcall * proc)(struct HWND__ *, unsigned int, unsigned int, long), bool show); // RA2: 0x005EAB60 (ownrdraw.cpp)
extern bool MovieListShown; // RA2: 0x00A40D10, the trailer button plays the movies of the list in turn
extern int TrailerMovieIndex; // RA2: 0x00A41698, the movie the trailer button plays next
int WolGameResult = -1; // RA2: 0x007DB55C, 2 when a game is ready after the Internet dialogs, -1 when not
extern void Play_Movie(char const * name, ThemeType theme, bool clrscrn_after, bool stretch, bool clrscrn_before, bool fullscreen); // RA2: 0x005A02D0, the overload with the fullscreen flag (movie.cpp)

/***********************************************************************************************
 * Cheat_Disable -- Clears the cheats that are not allowed in a multiplayer game.              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   RA2 has this as a routine of its own; TibSun walks the table where it needs it.           *
 *=============================================================================================*/
static void Cheat_Disable(void)
{
	for (int code = 0; code < ARRAY_SIZE(CheatEntries); code++) { // RA2: as above
		if (!CheatEntries[code].IsAllowedInMP) { // RA2: as above
			*CheatEntries[code].State = false;
		}
	}
}


/// <summary>
/// Asks the player which campaign to play.
/// This routine reads the campaign list first if that has not already happened, and then
/// runs the campaign dialog until the player either commits or backs out.
/// </summary>
/// <returns>Returns with the campaign chosen, or CAMPAIGN_NONE if the player backed out.</returns>
static CampaignType Choose_Campaign(void)
{
	HWND dialog;
	struct ChooseCampaignStruct state;

	state.ChoiceMade = false;
	state.ChosenCampaign = CAMPAIGN_NONE;

	if (Campaigns.Count() == 0) {
		Init_Campaigns();

		if (Campaigns.Count() == 0) {
			return(CAMPAIGN_NONE);
		}
	}

	Load_Side_Animation_Art(); // RA2: not in TibSun: the animation the side buttons show
	SideSound.Init(); // RA2: as above
	SideSoundTime = 0; // RA2: as above
	HoveredSideButton = 0; // RA2: as above

	dialog = OwnerDraw::Begin_Dialog(0x94, Campaign_Choice_Dialog_Proc, 0); // RA2: the campaign dialog (IDD_CAMPAIGN in TibSun)
	if (dialog != NULL) {
		SetWindowLong(dialog, DWL_USER, (LONG)&state); // RA2: as above
		Center_Window_Within_Window(dialog, MainWindow); // RA2: as above
		OwnerDraw::Display_Dialog(dialog);

		while (!state.ChoiceMade && OwnerDraw::Dialog_Message_Handler() != true) { // RA2: as above
			if (PendingSideVoc != -1 && SideSoundTime < AudioGetTime()) { // RA2: the hovered side's sound plays once the mouse has rested on the button
				SideSound.Stop(); // RA2: as above
				VocClass::PlayGlobal(PendingSideVoc, 1.0f, 0x2000, &SideSound); // RA2: as above
				PendingSideVoc = -1; // RA2: as above
			}
			if (SurfacesRestored) { // RA2: as above
				SurfacesRestored = false; // RA2: as above
				Darken_Surface(NULL, true); // RA2: as above
				Update_Visible_Surface(false, HiddenSurface, NULL); // RA2: as above
			}
		}
		OwnerDraw::End_Dialog(dialog);
	}

	SideSoundTime = AudioGetTime() + 3000; // RA2: lets the side's sound play out
	while (AudioControllerGetEvent(&SideSound) != NULL && SideSoundTime > AudioGetTime()) { // RA2: as above
		Call_Back(); // RA2: as above
	}
	Free_Side_Animation_Art(); // RA2: as above

	return(state.ChosenCampaign);
}


#define Max(a, b) ((a) < (b) ? (b) : (a)) // RA2: the executable tests the first value against the second, not as the MAX macro does

/***********************************************************************************************
 * Select_Game -- The game's main menu                                                         *
 *                                                                                             *
 *    This routine runs the main menu and its dialogs until a game has been chosen, then       *
 *    starts the scenario. A failed start returns to the menu.                                 *
 *                                                                                             *
 * INPUT:   fade  -- unused                                                                    *
 *                                                                                             *
 * OUTPUT:  bool; Is a game going to be played?                                                *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   RA2's form of TibSun's routine.                                                           *
 *=============================================================================================*/
bool Select_Game(bool) // RA2: as above
{
	bool gameloaded;				// Has the game been loaded from the menu?
	int selection;					// the default selection
	bool process;					// false = break out of while loop

	Theme.Play_Song(Theme.From_Name("INTRO")); // RA2: the main menu theme is looked up by name; TibSun has this commented out

restart:
	gameloaded = false;
	process = true;

	/*
	**	[Re]set any globals that need it, in preparation for a new scenario
	*/
	GameActive = true;
	PlayerPtr = NULL;
	DoList.Init();
	OutList.Init();
	Frame = 0;
	Scen->MissionTimer = 0;
	Scen->MissionTimer.Stop();
	Scen->CDifficulty = DIFF_NORMAL;
	Scen->Difficulty = DIFF_NORMAL;
	PlayerWins = false;
	PlayerLoses = false;
	PlayerRestarts = false;
	PlayerAborts = false;
	Session.MultiplayerObserver = false; // RA2: as above
	Map.Set_Cursor_Shape(NULL);
	Map.PendingObjectPtr = 0;
	Map.PendingObject = 0;
	Map.PendingHouse = HOUSE_NONE;

	Session.ProcessTicks = 0;
	Session.ProcessFrames = 0;
	Session.DesiredFrameRate = 30;
	NewMaxAheadFrame1 = 0;
	NewMaxAheadFrame2 = 0;

	/*
	**	Init multiplayer game scores.  Let Wins accumulate; just init the current
	**	Kills for this game.  Kills of -1 means this player didn't play this round.
	*/
	for (int i = 0; i < MAX_MULTI_GAMES; i++) {
		Session.Score[i].Kills[Session.CurGame] = -1;
	}

	/*
	**	Set default mouse shape
	*/
	Map.Set_Default_Mouse(MOUSE_NORMAL, false);

	if (ToolTips != NULL) {
		ToolTips->Activate(false);
	}

	/*
	**	If the last game we played was a multiplayer game, jump right to that
	**	menu by pre-setting 'selection'.
	*/
	if (Session.Type == GAME_NORMAL) {
		selection = MENU_NONE; // RA2: as above
	} else if (Session.Type == GAME_INTERNET) {
		selection = MENU_INTERNET_RETURN; // RA2: as above
	} else {
		selection = MENU_MULTIPLAYER; // RA2: as above
	}

	/*
	**	Main menu processing; only do this if we're not in editor mode.
	*/
	if (!Debug_Map) {

		/*
		**	If we're playing back a recording, load all pertinent values & skip
		**	the menu loop.  Hide the now-useless mouse pointer.
		*/
		if (Session.Play && Session.RecordFile.Is_Available()) {
			if (Session.RecordFile.Open(FileClass::READ)) {
				Load_Recording_Values(Session.RecordFile);
				process = false;
				Theme.Stop(true);
			} else {
				Session.Play = false;
			}
		}

		while (process) {

			/*
			**	Display menu and fetch selection from player.
			*/
			if (PacketTransport != NULL) {
				if (Session.ViaPacketRouter) { // RA2: as above
					if (PacketRouterConnection != NULL) { // RA2: as above
						OwnerTalk.Send_Shutdown(); // RA2: as above
					}
					Session.ViaPacketRouter = false; // RA2: as above
				}
				delete PacketTransport;
				PacketTransport = NULL;
				Start_Network_Session(); // RA2: as above
			}

			if ((selection == MENU_NONE) && !Debug_ForceScenario) { // RA2: as above
				selection = Main_Menu(3600); // RA2: as above
			}

			if (Debug_ForceScenario) {
				selection = MENU_NEW_CAMPAIGN; // RA2: as above
			}

			if (selection != MENU_EXIT) { // RA2: as above
				Theme.Play_Song(Theme.From_Name("INTRO")); // RA2: as above
			}

			Call_Back();

			switch (selection) {

				case MENU_TIMEOUT: // RA2: as above
					if (Session.Attract && Session.RecordFile.Is_Available()) {
						Session.Play = true;
						if (Session.RecordFile.Open(FileClass::READ)) {
							Load_Recording_Values(Session.RecordFile);
							process = false;
							Theme.Stop(true);
							break;
						}
						Session.Play = false;
						selection = MENU_NONE; // RA2: as above
					} else {
						selection = MENU_NONE; // RA2: as above
					}
					break;

				case MENU_SINGLE_PLAYER: // RA2: as above
					ObserverMode = 0; // RA2: as above
					selection = Dialog_Box(0x100, Single_Player_Dialog_Proc, true); // RA2: as above
					break;

				case MENU_INTERNET: // RA2: as above
					selection = MENU_MULTIPLAYER; // RA2: as above
					Session.Type = GAME_INTERNET;
					Set_Address_Protocol(2); // RA2: as above
					break;

				case MENU_NETWORK: // RA2: as above
					selection = MENU_MULTIPLAYER; // RA2: as above
					Session.Type = GAME_IPX;
					Set_Address_Protocol(1); // RA2: as above
					break;

				case MENU_EXTRAS: // RA2: as above
					selection = Dialog_Box(0x101, Extras_Dialog_Proc, true); // RA2: as above
					break;

				case MENU_OPTIONS: // RA2: as above
					selection = MENU_NONE; // RA2: as above
					Main_Options_Dialog();
					break;

				case MENU_EXIT_ASK: { // RA2: as above
					wchar_t const * text = StringTable::LoadString("GUI:ExitAreYouSure", NULL, "D:\\Ra2\\Init.CPP", 2411); // RA2: as above
					wchar_t const * ok = StringTable::LoadString("GUI:Ok", NULL, "D:\\Ra2\\Init.CPP", 2412); // RA2: as above
					wchar_t const * cancel = StringTable::LoadString("GUI:Cancel", NULL, "D:\\Ra2\\Init.CPP", 2413); // RA2: as above
					Darken_Surface(NULL, true); // RA2: as above
					if (WWMessageBox(L" ").Process(text, ok, cancel, NULL, false) == 0) { // RA2: as above
						selection = MENU_EXIT; // RA2: as above
						Options.Save_Settings(); // RA2: as above
					} else {
						Darken_Surface(NULL, true); // RA2: as above
						selection = MENU_NONE; // RA2: as above
					}
					break;
				}

				case MENU_EXIT: { // RA2: as above
					Theme.Stop(true);
					long timeout = TickCount + 3000; // RA2: as above
					while (Theme.Still_Playing() && Is_Speaking() && timeout > TickCount) { // RA2: as above
						Call_Back();
					}
					Theme.Stop(false); // RA2: as above
					if (!HicolorFlag) {
						Set_Palette(BlackPalette, 30, NULL); // RA2: as above
					}
					return(false);
				}

				case MENU_TRAILER: // RA2: as above
					selection = MENU_EXTRAS; // RA2: as above
					if (MovieListShown) { // RA2: as above
						if (TrailerMovieIndex <= 0) { // RA2: as above
							TrailerMovieIndex = Movies.Count(); // RA2: as above
						}
						if (TrailerMovieIndex == Movies.Count()) { // RA2: as above
							TrailerMovieIndex--; // RA2: as above
						}
						Play_Movie((VQType)TrailerMovieIndex--, THEME_NONE, true, true, true); // RA2: as above
						Darken_Surface(NULL, true); // RA2: as above
					} else {
						Play_Movie("EMPRTRLR.BIK", THEME_NONE, true, true, true, false); // RA2: as above
						Darken_Surface(NULL, true); // RA2: as above
					}
					break;

				case MENU_MOVIES: { // RA2: as above
					MovieListEntry * movie = (MovieListEntry *)Dialog_Box(0x129, Movie_List_Dialog_Proc, true); // RA2: as above
					if (movie != (MovieListEntry *)-1 && movie != NULL) { // RA2: as above
						if (!CD().Force_Available((DiskID)movie->CD)) { // RA2: as above
							break;
						}
						Play_Movie(movie->Name, THEME_NONE, true, true, true, false); // RA2: as above
						Darken_Surface(NULL, true); // RA2: as above
					} else {
						selection = MENU_EXTRAS; // RA2: as above
					}
					break;
				}

				case MENU_CREDITS: // RA2: as above
					selection = MENU_EXTRAS; // RA2: as above
					Show_Credits(); // RA2: as above
					Theme.Queue_Song(Theme.From_Name("INTRO")); // RA2: as above
					Darken_Surface(NULL, true); // RA2: as above
					break;

				case MENU_NEW_CAMPAIGN: { // RA2: as above
					new (&Environment) EnvironmentClass;

					Scen->Campaign = Choose_Campaign();
					if (Scen->Campaign == CAMPAIGN_NONE) {
						process = true;
						selection = MENU_SINGLE_PLAYER; // RA2: as above
						break;
					}

					switch (Options.Difficulty) {
						case 0:
							Scen->CDifficulty = DIFF_HARD;
							Scen->Difficulty = DIFF_EASY;
							break;

						case 1:
							Scen->CDifficulty = DIFF_HARD;
							Scen->Difficulty = DIFF_NORMAL;
							break;

						case 2:
							Scen->CDifficulty = DIFF_NORMAL;
							Scen->Difficulty = DIFF_NORMAL;
							break;

						case 3:
							Scen->CDifficulty = DIFF_EASY;
							Scen->Difficulty = DIFF_NORMAL;
							break;

						case 4:
							Scen->CDifficulty = DIFF_EASY;
							Scen->Difficulty = DIFF_HARD;
							break;
					}

					Session.Type = GAME_NORMAL;
					process = false;
					break;
				}

				/*
				**	Load a saved game.
				*/
				case MENU_LOAD_GAME: // RA2: as above
					ScenarioInit++;
					Init_Campaigns();
					ScenarioInit--;
					if (LoadOptionsClass().Load()) {
						gameloaded = true;
						process = false;
					} else {
						selection = MENU_SINGLE_PLAYER; // RA2: as above
					}
					break;

				case MENU_SINGLE_AGAIN: // RA2: as above
					selection = MENU_SINGLE_PLAYER; // RA2: as above
					break;

				case MENU_SKIRMISH: // RA2: as above
					Session.Type = GAME_SKIRMISH;
					// fall through

				/*
				**	MENU_MULTIPLAYER: set 'Session.Type' to NULL-modem, modem, or
				**	network play.
				*/
				case MENU_MULTIPLAYER: // RA2: as above
				case MENU_INTERNET_RETURN: { // RA2: as above
					Session.Read_MultiPlayer_Settings();

					for (int house = 0; house < HouseTypes.Count(); house++) {
						HouseTypes[house]->Read_INI(*RuleINI);
					}

					Session.Suspended = 0;

					switch (Session.Type) {
						case GAME_NORMAL:
							selection = MENU_NONE; // RA2: as above
							continue;

						case GAME_SKIRMISH:
							if (!Skirmish_Mode_Dialog()) {
								Session.Type = GAME_NORMAL;
								selection = MENU_SINGLE_PLAYER; // RA2: as above
							}
							break;

						case GAME_NULL_MODEM:
						case GAME_MODEM: { // RA2: as above
							Cheat_Disable();
							Session.NetOpen = true;

							if (NullModem.Num_Connections()) {
								NullModem.Init_Send_Queue();
								ModemGuest guest; // RA2: as above

								if ((Session.Type == GAME_NULL_MODEM && Session.ModemType == MODEM_NULL_HOST) || (Session.Type == GAME_MODEM && Session.ModemType == MODEM_DIALER)) {
									if (!ModemHost().Dialog()) {
										Session.Type = GAME_NORMAL;
										selection = MENU_NONE; // RA2: as above
									}
								} else {
									if (!guest.Dialog()) { // RA2: as above
										Session.Type = GAME_NORMAL;
										selection = MENU_NONE; // RA2: as above
									}
								}
							} else {
								selection = MENU_NONE; // RA2: as above
								Session.Type = GAME_NORMAL;
								continue;
							}
							break;
						}

						/*
						**	Handle being spawned from WChat. Internet play based on IPX code.
						*/
						case GAME_INTERNET: {
							Cheat_Disable();
							DebugString("Init.cpp - GAME_INTERNET\n"); // RA2: as above
							memset(Session.WolPlayerState, 0, sizeof(Session.WolPlayerState)); // RA2: as above
							Session.RouterProbePort2 = 0; // RA2: as above
							bool success = Start_Network_Session(); // RA2: as above
							int result = Wonline_Main(selection == MENU_INTERNET_RETURN); // RA2: as above
							selection = MENU_MULTIPLAYER; // RA2: as above
							Session.NetOpen = false;

							switch (result) { // RA2: as above
								case -2: // RA2: as above
								case 0:
									delete PacketTransport;
									PacketTransport = NULL;
									selection = MENU_NONE; // RA2: as above
									Session.Type = GAME_NORMAL;
									Theme.Play_Song(Theme.From_Name("INTRO")); // RA2: as above
									break;

								case -1: // RA2: as above
									Theme.Stop(true);
									if (!HicolorFlag) {
										Set_Palette(BlackPalette, 30, NULL); // RA2: as above
									}
									delete PacketTransport;
									PacketTransport = NULL;
									return(false);

								case 1:
									DebugString("Return from WOL to play a %d player game\n", Session.Players.Count());
									if (success) {
										process = false;
										WolGameResult = 2; // RA2: as above
									} else {
										Session.Type = GAME_NORMAL;
										selection = MENU_NONE; // RA2: as above
										delete PacketTransport;
										PacketTransport = NULL;
										WolGameResult = -1; // RA2: as above
									}
									break;

								case 2:
									delete PacketTransport;
									PacketTransport = NULL;
									Session.Type = GAME_NORMAL;
									selection = MENU_SKIRMISH; // RA2: as above
									break;
							}
							break;
						}
					}

					switch (Session.Type) {
						case GAME_IPX: {
							Cheat_Disable();
							Session.Read_MultiPlayer_Settings();
							if (PacketTransport != NULL) {
								delete PacketTransport;
								PacketTransport = NULL;
							}

							for (int house = 0; house < HouseTypes.Count(); house++) {
								HouseTypes[house]->Read_INI(*RuleINI);
							}

							Session.Type = GAME_IPX;
							Session.CommProtocol = COMM_PROTOCOL_MULTI_E_COMP;
							if (PacketTransport == NULL) {
								PacketTransport = new IPXInterfaceClass;
								PacketTransport->Init(); // RA2: as above
								Darken_Surface(NULL, true); // RA2: as above
								if (Net2Init_Network() && Net_Join_Dialog()) { // RA2: as above
									process = false;
								} else {
									Session.Type = GAME_NORMAL;
									selection = MENU_NONE; // RA2: as above
									delete PacketTransport;
									PacketTransport = NULL;
								}
							}
							break;
						}

						case GAME_MODEM:
						case GAME_NULL_MODEM:
						case GAME_INTERNET:
						case GAME_SKIRMISH:
							process = false;
							break;
					}
					break;
				}
			}
		}
	}

	Keyboard->Clear();
	Init_Random();
	Planning_Reset_State(); // RA2: as above

	if (!gameloaded && !Session.LoadGame) {
		DebugString("About to load a %d player game.\n", Session.Players.Count()); // RA2: as above
		MouseCursor->Hide_Mouse(); // RA2: as above
		if (selection != MENU_NEW_CAMPAIGN) { // RA2: as above
			if (!HicolorFlag) {
				Set_Palette(BlackPalette, 15, Call_Back); // RA2: as above
			}
			HiddenSurface->Fill(0);
			Update_Visible_Surface(true, HiddenSurface, NULL); // RA2: as above
		}
		MouseCursor->Show_Mouse(); // RA2: as above

		if (Session.Type == GAME_INTERNET) {
			for (int player = 0; player < 8; player++) { // RA2: as above
				Session.PlayerLastTime[player] = TickCount; // RA2: as above
			}
		}

		if (Session.Type == GAME_NORMAL && !Debug_ForceScenario && !Session.Play) { // RA2: as above
			if (!Start_Scenario(Campaigns[Scen->Campaign]->ScenarioName, true, Scen->Campaign)) {
				if (Debug_Map) {
					return(false);
				}
				goto restart;
			}
		} else if (!Start_Scenario(Scen->ScenarioName, true, CAMPAIGN_NONE)) { // RA2: as above
			if (Debug_Map) {
				return(false);
			}
			goto restart;
		}

		if (Session.Record) {
			if (Session.RecordFile.Open(FileClass::WRITE)) {
				Save_Recording_Values(Session.RecordFile);
			} else {
				Session.Record = false;
			}
		}
		Special.IsFromInstall = false;
	}

	DebugString("IsTGrowth = %d\n", Special.IsTGrowth);
	DebugString("IsTSpread = %d\n", Special.IsTSpread);

	if (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH && !Session.Play) {
		Session.Create_Connections();
		if (Session.Type == GAME_IPX) {
			Ipx.Set_Timing(Max(Ipx.Global_Response_Time() + 2, 15), (unsigned int)-1, 600, true); // RA2: as above
			Ipx.Set_External_Timing(Max(Ipx.Global_Response_Time() + 2, 60), (unsigned int)-1, 600); // RA2: as above
		} else if (Session.Type == GAME_INTERNET && !Session.PrecalcDesiredFrameRate) { // RA2: as above
			Ipx.Set_Timing(Max(Ipx.Global_Response_Time() + 2, 60), (unsigned int)-1, 600, true); // RA2: as above
		}
	}

	Call_Back();
	MouseCursor->Hide_Mouse(); // RA2: as above
	if (!HicolorFlag) {
		Set_Palette(BlackPalette, 15, Call_Back); // RA2: as above
	}
	HiddenSurface->Fill(0);
	Update_Visible_Surface(true, HiddenSurface, NULL); // RA2: as above
	MouseCursor->Show_Mouse(); // RA2: as above
	LogicalSurface = HiddenSurface;
	Map.Override_Mouse_Shape(MOUSE_NO_MOVE, false); // RA2: as above
	Map.Revert_Mouse_Shape();
	if (!Debug_Map) {
		Map.Activate(1);
	}
	Map.Flag_To_Redraw();
	Call_Back();
	MouseCursor->Hide_Mouse(); // RA2: as above
	return(true);
}

#undef Max // RA2: as above


/***********************************************************************************************
 * Campaign_Choice_Dialog_Proc -- Message handler of the campaign choice dialog.               *
 *                                                                                             *
 *    The dialog has a button for each side's campaign (Allied 0x6EA, Soviet 0x6EC and boot    *
 *    camp 0x6EB), which highlights under the mouse and plays the side's sound when chosen;    *
 *    the list control 0x455 holds the campaigns and the slider 0x50F the difficulty.          *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   RA2 has the side buttons; TibSun's dialog only has the list.                              *
 *=============================================================================================*/
BOOL CALLBACK Campaign_Choice_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) // RA2: 0x005147A0
{
	HWND item;
	struct ChooseCampaignStruct * state;

	int rc;
	rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);

	if (rc) {
		return(rc);
	}

	switch (message) {

		case WM_HSCROLL: // RA2: the difficulty label follows the slider
			if ((HWND)lparam == GetDlgItem(window, 0x50F)) { // RA2: as above
				char const * label = Difficulty_Labels[wparam >> 16]; // RA2: as above
				item = GetDlgItem(window, 0x670); // RA2: as above
				SendMessage(item, WM_USER + 178, 0, (LPARAM)StringTable::LoadString(label, NULL, "D:\\Ra2\\Init.CPP", 1193)); // RA2: as above
				return(FALSE); // RA2: as above
			} // RA2: as above
			break; // RA2: as above

		case WM_COMMAND:
			switch (LOWORD(wparam)) {
				case 0x40E: // RA2: the OK button
					if (HIWORD(wparam) == BN_CLICKED) {
						state = (ChooseCampaignStruct *)GetWindowLong(window, DWL_USER);

						if (state != NULL) {
							item = GetDlgItem(window, 0x455); // RA2: the campaign list
							if (item != NULL) {
								state->ChosenCampaign = (CampaignType)SendMessage(item, LB_GETCURSEL, 0, 0); // RA2: the list holds the available campaigns in order
								state->ChoiceMade = true;
							}
						}

						item = GetDlgItem(window, 0x50F); // RA2: the difficulty slider

						if (item != NULL) {
							Options.Difficulty = SendMessage(item, TBM_GETPOS, 0, 0); // RA2: as above
							return(FALSE); // RA2: as above
						}
					}
					break;

				case 0x686: // RA2: back
					state = (ChooseCampaignStruct *)GetWindowLong(window, DWL_USER);

					if (state != NULL) {
						state->ChosenCampaign = CAMPAIGN_NONE;
						state->ChoiceMade = true;
						return(FALSE); // RA2: as above
					}
					break;
			}
			break;

		case WM_NCHITTEST: // RA2: highlights the side button under the mouse; not in TibSun
			if (PressedSideButton == NULL) { // RA2: as above
				POINT point; // RA2: as above
				point.x = LOWORD(lparam); // RA2: as above
				point.y = HIWORD(lparam); // RA2: as above
				RECT rect; // RA2: as above
				GetWindowRect(window, &rect); // RA2: as above
				point.x -= rect.left; // RA2: as above
				point.y -= rect.top; // RA2: as above
				HWND child = ChildWindowFromPointEx(window, point, CWP_SKIPINVISIBLE); // RA2: as above
				if (child != NULL) { // RA2: as above
					int id = GetDlgCtrlID(child); // RA2: as above
					if (HoveredSideButton != id) { // RA2: as above
						SendDlgItemMessage(window, HoveredSideButton, WM_USER + 213, 0, 0); // RA2: as above
						SendDlgItemMessage(window, HoveredSideButton, WM_USER + 212, 0, 0); // RA2: as above
						SideSoundTime = AudioGetTime() + 500; // RA2: as above
						PendingSideVoc = -1; // RA2: as above
						HoveredSideButton = 0; // RA2: as above
					} // RA2: as above
					if (id == 0x6EA) { // RA2: as above
						if (HoveredSideButton != id) { // RA2: as above
							PendingSideVoc = AlliedSelectVoc; // RA2: as above
						} // RA2: as above
						SendDlgItemMessage(window, 0x6EA, WM_USER + 211, 0, 0); // RA2: as above
						HoveredSideButton = 0x6EA; // RA2: as above
						return(FALSE); // RA2: as above
					} // RA2: as above
					if (id == 0x6EC) { // RA2: as above
						if (HoveredSideButton != id) { // RA2: as above
							PendingSideVoc = SovietSelectVoc; // RA2: as above
						} // RA2: as above
						SendDlgItemMessage(window, 0x6EC, WM_USER + 211, 0, 0); // RA2: as above
						HoveredSideButton = 0x6EC; // RA2: as above
						return(FALSE); // RA2: as above
					} // RA2: as above
					if (id == 0x6EB) { // RA2: as above
						if (HoveredSideButton != id) { // RA2: as above
							PendingSideVoc = BootCampSelectVoc; // RA2: as above
						} // RA2: as above
						SendDlgItemMessage(window, 0x6EB, WM_USER + 211, 0, 0); // RA2: as above
						HoveredSideButton = 0x6EB; // RA2: as above
						return(FALSE); // RA2: as above
					} // RA2: as above
				} else if (HoveredSideButton != 0) { // RA2: as above
					SendDlgItemMessage(window, HoveredSideButton, WM_USER + 213, 0, 0); // RA2: as above
					SendDlgItemMessage(window, HoveredSideButton, WM_USER + 212, 0, 0); // RA2: as above
					return(FALSE); // RA2: as above
				} // RA2: as above
			} // RA2: as above
			break; // RA2: as above

		case WM_LBUTTONDOWN: // RA2: remembers the side button the mouse went down on
		{
			POINT point; // RA2: as above
			point.x = LOWORD(lparam); // RA2: as above
			point.y = HIWORD(lparam); // RA2: as above
			RECT rect; // RA2: as above
			GetWindowRect(window, &rect); // RA2: as above
			point.x -= rect.left; // RA2: as above
			point.y -= rect.top; // RA2: as above
			HWND child = ChildWindowFromPointEx(window, point, CWP_SKIPINVISIBLE); // RA2: as above
			if (child != NULL) { // RA2: as above
				int id = GetDlgCtrlID(child); // RA2: as above
				if (id == 0x6EA || id == 0x6EC || id == 0x6EB) { // RA2: as above
					PressedSideButton = child; // RA2: as above
					SetCapture(window); // RA2: as above
					return(FALSE); // RA2: as above
				} // RA2: as above
			} // RA2: as above
			break; // RA2: as above
		}

		case WM_LBUTTONUP: // RA2: chooses the side's first campaign when the button is released over the one that was pressed
		{
			POINT point; // RA2: as above
			point.x = LOWORD(lparam); // RA2: as above
			point.y = HIWORD(lparam); // RA2: as above
			RECT rect; // RA2: as above
			GetWindowRect(window, &rect); // RA2: as above
			point.x -= rect.left; // RA2: as above
			point.y -= rect.top; // RA2: as above
			HWND child = ChildWindowFromPointEx(window, point, CWP_SKIPINVISIBLE); // RA2: as above
			if (child != NULL) { // RA2: as above
				int id = GetDlgCtrlID(child); // RA2: as above
				if (child == PressedSideButton) { // RA2: as above
					state = (ChooseCampaignStruct *)GetWindowLong(window, DWL_USER); // RA2: as above
					if (state != NULL) { // RA2: as above
						if (id == 0x6EA) { // RA2: as above
							state->ChosenCampaign = CampaignClass::From_Name(CampaignStartNames[0]); // RA2: as above
							state->ChoiceMade = true; // RA2: as above
						} else if (id == 0x6EC) { // RA2: as above
							state->ChosenCampaign = CampaignClass::From_Name(CampaignStartNames[1]); // RA2: as above
							state->ChoiceMade = true; // RA2: as above
						} else if (id == 0x6EB) { // RA2: as above
							state->ChosenCampaign = CampaignClass::From_Name(CampaignStartNames[2]); // RA2: as above
							state->ChoiceMade = true; // RA2: as above
						} // RA2: as above
						if (state->ChoiceMade) { // RA2: as above
							ScenarioSaveNameSet = false; // RA2: as above
							SendDlgItemMessage(window, HoveredSideButton, WM_USER + 213, 0, 0); // RA2: as above
							SendDlgItemMessage(window, HoveredSideButton, WM_USER + 212, 0, 0); // RA2: as above
							item = GetDlgItem(window, 0x50F); // RA2: as above
							if (item != NULL) { // RA2: as above
								Options.Difficulty = SendMessage(item, TBM_GETPOS, 0, 0); // RA2: as above
							} // RA2: as above
							if (PendingSideVoc != -1) { // RA2: as above
								SideSound.Stop(); // RA2: as above
								VocClass::PlayGlobal(PendingSideVoc, 1.0f, 0x2000, &SideSound); // RA2: as above
								PendingSideVoc = -1; // RA2: as above
							} // RA2: as above
						} // RA2: as above
					} // RA2: as above
				} else if (id == 0x6EA || id == 0x6EC || id == 0x6EB) { // RA2: as above
					if (HoveredSideButton != 0) { // RA2: as above
						SendDlgItemMessage(window, HoveredSideButton, WM_USER + 213, 0, 0); // RA2: as above
						SendDlgItemMessage(window, HoveredSideButton, WM_USER + 212, 0, 0); // RA2: as above
					} // RA2: as above
					SendDlgItemMessage(window, id, WM_USER + 213, 0, 0); // RA2: as above
					SendDlgItemMessage(window, id, WM_USER + 212, 0, 0); // RA2: as above
					HoveredSideButton = id; // RA2: as above
				} else if (HoveredSideButton != 0) { // RA2: as above
					SendDlgItemMessage(window, HoveredSideButton, WM_USER + 213, 0, 0); // RA2: as above
					SendDlgItemMessage(window, HoveredSideButton, WM_USER + 212, 0, 0); // RA2: as above
					HoveredSideButton = 0; // RA2: as above
				} // RA2: as above
			} else if (HoveredSideButton != 0) { // RA2: as above
				SendDlgItemMessage(window, HoveredSideButton, WM_USER + 213, 0, 0); // RA2: as above
				SendDlgItemMessage(window, HoveredSideButton, WM_USER + 212, 0, 0); // RA2: as above
				HoveredSideButton = 0; // RA2: as above
			} // RA2: as above
			if (GetCapture() == window) { // RA2: as above
				ReleaseCapture(); // RA2: as above
			} // RA2: as above
			PressedSideButton = NULL; // RA2: as above
			break; // RA2: as above
		}

		case WM_USER + 151: // RA2: the dialog is ready
		{
			SovietSelectVoc = VocClass::From_Name("SovietCampaignSelect"); // RA2: as above
			AlliedSelectVoc = VocClass::From_Name("AlliedCampaignSelect"); // RA2: as above
			BootCampSelectVoc = VocClass::From_Name("BootCampSelect"); // RA2: as above
			item = GetDlgItem(window, 0x455); // RA2: the campaign list
			if (CampaignListShown) { // RA2: as above
				if (item != NULL) {
					for (int index = 0; index < Campaigns.Count(); index++) {
						SendMessage(item, WM_USER + 205, 0, (LPARAM)Campaigns[index]->Description); // RA2: wide text
					}
					SendMessage(item, LB_SETCURSEL, 0, 0); // RA2: as above
				}
			} else { // RA2: as above
				ShowWindow(item, SW_HIDE); // RA2: as above
				HWND button = GetDlgItem(window, 0x40E); // RA2: as above
				ShowWindow(button, SW_HIDE); // RA2: as above
				EnableWindow(button, FALSE); // RA2: as above
			} // RA2: as above

			item = GetDlgItem(window, 0x50F); // RA2: the difficulty slider
			if (item != NULL) {
				PostMessage(item, OD_TRACKNUMBERS, 0, 0); // RA2: as above
				PostMessage(item, TBM_SETRANGE, TRUE, MAKELONG(0, 2)); // RA2: as above
				PostMessage(item, TBM_SETPOS, TRUE, Options.Difficulty); // RA2: as above
			}

			SendDlgItemMessage(window, 0x6EA, WM_USER + 213, 0, 0); // RA2: as above
			SendDlgItemMessage(window, 0x6EA, WM_USER + 212, 0, 0); // RA2: as above
			SendDlgItemMessage(window, 0x6EC, WM_USER + 213, 0, 0); // RA2: as above
			SendDlgItemMessage(window, 0x6EC, WM_USER + 212, 0, 0); // RA2: as above
			SendDlgItemMessage(window, 0x6EB, WM_USER + 213, 0, 0); // RA2: as above
			SendDlgItemMessage(window, 0x6EB, WM_USER + 212, 0, 0); // RA2: as above
			return(FALSE); // RA2: as above
		}
	}

	return(FALSE);
}


extern int PortPool; // RA2: 0x00A71B28, the written PortPool value of RA2.INI's [MultiPlayer] section

/***********************************************************************************************
 * Start_Network_Session -- Opens the UDP network for a multiplayer game.                      *
 *                                                                                             *
 *    RA2 has this routine, TibSun does not. It reads every house's rules, creates the UDP     *
 *    interface and its socket, initializes the network code, clears the per-player port and   *
 *    connection state of the session, picks the base port for the game and writes the port    *
 *    base and pool to RA2.INI.                                                                *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Did Init_Network succeed?                                                    *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *=============================================================================================*/
bool Start_Network_Session(void) // RA2: 0x00514EE0, not in TibSun
{
	if (PacketTransport != NULL) { // RA2: as above
		delete PacketTransport; // RA2: as above
		PacketTransport = NULL; // RA2: as above
	}

	for (int house = 0; house < HouseTypes.Count(); house++) { // RA2: as above
		HouseTypes[house]->Read_INI(*RuleINI); // RA2: as above
	}

	if (PacketTransport == NULL) { // RA2: as above
		PacketTransport = new UDPInterfaceClass; // RA2: as above
	}

	Session.CommProtocol = COMM_PROTOCOL_MULTI_E_COMP; // RA2: as above
	PacketTransport->Open_Socket(0); // RA2: as above
	Ipx.Set_Timing(30, (unsigned int)-1, 600, true); // RA2: as above
	DebugString("About to initialise the network code\n"); // RA2: as above
	bool success = Init_Network(); // RA2: as above
	PacketTransport->Start_Listening(); // RA2: as above

	memset(Session.PlayerPorts, 0, sizeof(Session.PlayerPorts)); // RA2: as above
	Session.RouterProbeActive = false; // RA2: as above
	Session.PortProbeDue = false; // RA2: as above
	Session.PacketRouterPort = 0; // RA2: as above
	Session.RouterProbePort = 0; // RA2: as above
	Session.SlowestResponse = 0; // RA2: as above
	Session.LastTimingID = -1; // RA2: as above
	Session.RouterResponse = 20; // RA2: as above
	Session.ExitEventPending = 0; // RA2: as above
	Session.ExitEventFrame = 0; // RA2: as above
	Session.ExitEventPlayer = -1; // RA2: as above
	memset(Session.PeerProbeData, 0, sizeof(Session.PeerProbeData)); // RA2: as above
	memset(Session.ConnectionResults, -1, sizeof(Session.ConnectionResults)); // RA2: as above
	memset(Session.PeerPorts, 0, sizeof(Session.PeerPorts)); // RA2: as above
	memset(Session.PlayerPortBase, 0, sizeof(Session.PlayerPortBase)); // RA2: as above
	Session.ConnectionResultCount = 0; // RA2: as above

	if (Session.ForcePortBase) { // RA2: as above
		Session.PortBase = Session.ForcePortBase; // RA2: as above
	} else if (Session.RotatePortBase) { // RA2: as above
		Session.PortBase += 5; // RA2: as above
		if (Session.PortBase >= 1750) { // RA2: as above
			Session.PortBase = 1250; // RA2: as above
		} // RA2: as above
		Session.RotatePortBase = false; // RA2: as above
	} // RA2: as above

	RawFileClass file("RA2.INI"); // RA2: as above
	ConfigINI.Put_Int("MultiPlayer", "PortBase", Session.PortBase); // RA2: as above
	ConfigINI.Put_Int("MultiPlayer", "PortPool", PortPool); // RA2: as above
	ConfigINI.Save(file, false); // RA2: as above
	OwnerTalk.Reset(); // RA2: as above
	Session.NetOpen = true; // RA2: as above
	return(success); // RA2: as above
}


/***********************************************************************************************
 * Anim_Init -- Initialize the VQ animation control structure.                                 *
 *                                                                                             *
 *    VQ animations are controlled by a structure passed to the VQ player. This routine        *
 *    initializes the structure to values required by C&C.                                     *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Only need to call this routine once at the beginning of the game.               *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/20/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void Anim_Init(void)
{
	Initialize_Options();
	Set_Option(OPTION_PLAY_FROM_MIXFILE);
}



/***********************************************************************************************
 * Parse_Command_Line -- Parses the command line parameters.                                   *
 *                                                                                             *
 *    This routine should be called before the graphic mode is initialized. It examines the    *
 *    command line parameters and sets the appropriate globals. If there is an error, then     *
 *    it outputs a command summary and then returns false.                                     *
 *                                                                                             *
 * INPUT:   argc  -- The number of command line arguments.                                     *
 *                                                                                             *
 *          argv  -- Pointer to character string array that holds the individual arguments.    *
 *                                                                                             *
 * OUTPUT:  bool; Was the command line parsed successfully?                                    *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/18/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
bool Parse_Command_Line(int argc, char * argv[])
{
	/*
	**	Parse the command line and set globals to reflect the parameters
	**	passed in.
	*/
	//Whom = HOUSE_GOOD;
	Special.Init();

	Debug_Map = false;

	for (int index = 1; index < argc; index++) {
		char * string;		// Pointer to argument.
		string = strupr(argv[index]);

		if (stricmp("-jabber", string) == 0) { // RA2: -jabber, -str, -nostr, -noaudio, -record and -play are not in TibSun; the executable tests them first
			Set_Jabber_Option(1); // RA2: as above
			continue; // RA2: as above
		}
		if (stricmp("-str", string) == 0) { // RA2: as above
			Set_Str_Option(1); // RA2: as above
			continue; // RA2: as above
		}
		if (stricmp("-nostr", string) == 0) { // RA2: as above
			Set_Str_Option(0); // RA2: as above
			continue; // RA2: as above
		}
		if (stricmp("-noaudio", string) == 0) { // RA2: as above
			AudioAllowed = 0; // RA2: as above
			continue; // RA2: as above
		}
		if (stricmp("-record", string) == 0) { // RA2: as above
			RecordPlayRequest = 1; // RA2: as above
			continue; // RA2: as above
		}
		if (stricmp("-play", string) == 0) { // RA2: as above
			RecordPlayRequest = 2; // RA2: as above
			continue; // RA2: as above
		}

		/*
		**	Print usage text only if requested.
		*/
		if (stricmp("/?", string) == 0 || stricmp("-?", string) == 0 || stricmp("-h", string) == 0 || stricmp("/h", string) == 0) {
			/*
			**	Unrecognized command line parameter... Display usage
			**	and then exit.
			*/
			_putws(StringTable::LoadString("TXT_OPTION_HELP_01", NULL, "D:\\Ra2\\Init.CPP", 3342)); // RA2: wide text, with the executable's file name and line
			_putws(StringTable::LoadString("TXT_OPTION_HELP_02", NULL, "D:\\Ra2\\Init.CPP", 3343)); // RA2: as above
			_putws(StringTable::LoadString("TXT_OPTION_HELP_03", NULL, "D:\\Ra2\\Init.CPP", 3344)); // RA2: as above
			_putws(StringTable::LoadString("TXT_OPTION_HELP_04", NULL, "D:\\Ra2\\Init.CPP", 3345)); // RA2: as above
			_putws(StringTable::LoadString("TXT_OPTION_HELP_05", NULL, "D:\\Ra2\\Init.CPP", 3346)); // RA2: as above
			_putws(StringTable::LoadString("TXT_OPTION_HELP_06", NULL, "D:\\Ra2\\Init.CPP", 3347)); // RA2: as above
			return(false);
		}


		bool processed = true;
		int ob = Obfuscate(string);

		switch (ob) {

			/*
			**	Special flag - is C&C being run from the install program?
			*/
			case PARM_INSTALL:
				Special.IsFromInstall = true;
// If uncommented, will disable the <ESC> key during the first movie run.
//				BreakoutAllowed = false;
				break;

			default:
				processed = false;
				break;
		}
		if (processed) continue;

		/*
		**	File search path override.
		*/
		if (strstr(string, "-CD")) { // RA2: in every build, not only the debug ones
			CD::Override_Swap(true); // RA2: as above
			CDFileClass::Set_Search_Drives(&string[3]); // RA2: the executable calls CDFileClass's
			continue; // RA2: as above
		}

		/*
		**	Specify destination connection for network play
		*/
		if (strstr(string, "-DESTNET")) {
			NetNumType net;
			NetNodeType node;

			/*
			**	Scan the command-line string, pulling off each address piece
			*/
			int i = 0;
			char * p = strtok(string + 8, ".");

			while (p) {
				int x;

				sscanf(p, "%x", &x);			// convert from hex string to int

				if (i < 4) {
					net[i] = (char)x;			// fill NetNum
				} else {
					node[i-4] = (char)x;		// fill NetNode
				}

				i++;
				p = strtok(NULL, ".");
			}

			/*
			**	If all the address components were successfully read, fill in the
			**	BridgeNet with a broadcast address to the network across the bridge.
			*/
			if (i >= 4) {
				Session.IsBridge = 1;
				memset(node, 0xff, 6);
				Session.BridgeNet = IPXAddressClass(net, node, 0); // RA2: IPXAddressClass has only the (net, node, port) constructor
			}
			continue;
		}

		/*
		**	Specify socket ID, as an offset from 0x4000.
		*/
		if (strstr(string, "-SOCKET")) {
			unsigned short socket;

			socket = (unsigned short)(atoi(string + strlen("SOCKET")));
			socket += (unsigned short)0x4000;
			if (socket >= 0x4000 && socket < 0x8000) {
				Ipx.Set_Socket(socket);
			}
			continue;
		}

		Set_Packet_Loss_Percent(0); // RA2: not in TibSun (it has -TIME= here): every other argument first resets the packet loss

		if (strstr(string, "-MPDEBUG")) {
			ExtraSyncInfo = true; // RA2: TibSun sets Session.ShowInternetDebug
			Session.EnableMultiplayerDebug = true; // RA2: as above
		}

		if (strstr(string, "-DROP=")) { // RA2: not in TibSun: drop that percentage of received packets
			Set_Packet_Loss_Percent((unsigned short)atoi(string + strlen("-DROP="))); // RA2: as above
			continue; // RA2: as above
		}

		if (strstr(string, "-SPEEDCONTROL")) { // RA2: not in TibSun: keeps the game speed sliders visible
			GameControlsShown = true; // RA2: as above
		}
		if (strstr(string, "-DLINK1")) { // RA2: not in TibSun: router type 1
			RouterType = 1; // RA2: as above
		}
		if (strstr(string, "-NETGEAR")) { // RA2: not in TibSun: router type 2
			RouterType = 2; // RA2: as above
		}
		if (strstr(string, "-NOROUTER")) { // RA2: not in TibSun: do not allow for a router
			Session.UseRouter = false; // RA2: as above
		}

		/*
		**	Set the Net Stealth option
		*/
		if (strstr(string, "-STEALTH")) {
			Session.NetStealth = true;
			continue;
		}

		/*
		**	Set the Net Protection option
		*/
		if (strstr(string, "-MESSAGES")) {
			Session.NetProtect = false;
			continue;
		}

		/*
		**	Allow "attract" mode
		*/
		if (strstr(string, "-ATTRACT")) {
			Session.Attract = true;
			continue;
		}

		if (isdigit(string[1])) {
			sscanf(string, "-%dX%d", &Options.ScreenWidth, &Options.ScreenHeight);
			continue;
		}

		/*
		**	Set screen to 640x480 instead of 640x400
		*/
		if (strcmp(string, "-480") == 0) {
			Options.ScreenHeight = 480;
			continue;
		}

		if (strcmp(string, "-16") == 0) {
			HicolorFlag = true;
			continue;
		}

		if (stricmp(string, "-WIN") == 0) {
			WindowedMode = true;
			continue;
		}

		/*
		**	Special command line control parsing.
		*/
		if (strnicmp(string, "-X", strlen("-O")) == 0) {
			string += strlen("-X");
			while (*string) {
				char code = *string++;
				switch (toupper(code)) {

					/*
					**	Quiet mode
					*/
					case 'Q':
						Debug_Quiet = true;
						break;

					default:
						_putws(StringTable::LoadString("TXT_INVALID", NULL, "D:\\Ra2\\Init.CPP", 3660)); // RA2: wide text, with the executable's file name and line
						return(false);
				}
			}
			continue;
		}
	}

	return(true);
}

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
					*res = MENU_SINGLE_PLAYER; // RA2: as above
					return(FALSE);

				case 0x686: // RA2: as above
					*res = MENU_EXTRAS; // RA2: as above
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
