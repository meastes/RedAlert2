/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

/*
**	RA2: the skirmish setup dialog (Skirmish.cpp in the executable's debug text, resource 0x102): seven
**	computer player rows with a type, side and color box each, the game options, the map list and the preview.
**	TibSun's skirmish.cpp of the same name has the same entry point and dialog procedure, but its dialog is
**	another one (sliders for the number of computer players); the code here is the executable's from 0x00682600.
*/

#define RA2_GEOMETRY_H // RA2: geometry.h follows the rectangle below, as the executable initializes the rectangle first (0x00682600) and the geometry constants after it (0x00682620 to 0x006827D0)
#include "always.h"
#include <string.h> // RA2: the includes this file needs
#include <wchar.h> // RA2: the includes this file needs
#include "_rules.h"
#include "goptions.h"
#include "init.h"
#include "mplayer.h"
#include "msgbox.h"
#include "netshare.h"
#include "multiplayer.h" // RA2: the includes this file needs
#include "ownrdraw.h"
#include "rules.h"
#include "session.h" // RA2: the includes this file needs
#include "houstype.h"
#undef LoadString
#include "stringtable.h" // RA2: the includes this file needs
#include "skirmish.h"


#include "geometry.h" // RA2: this module holds its own copy of the geometry constants (initialized after the rectangle)

struct ToolTipTextRequest { // RA2: what a combo box asks for with message 0x4E9 (the same layout as netdlg2.cpp reads)
	HWND Window; // RA2: as above
	int Item; // RA2: as above
	WideWstring Text; // RA2: as above
};

HWND SkirmishDialog; // RA2: 0x00ABCDFC; the dialog while it runs (Skirmish_Mode_Dialog sets and clears it, Skirmish_Has_Computer_After reads it)

int __fastcall Skirmish_Player_Combo_ID(int index); // RA2: 0x006827F0, defined below
int __fastcall Skirmish_Player_Index(int id); // RA2: 0x00682850, defined below
wchar_t const * __fastcall Skirmish_Player_Type_Name(int type); // RA2: 0x006828B0, defined below
bool __fastcall Skirmish_Has_Computer_After(int player); // RA2: 0x00682930, defined below
void __fastcall Skirmish_On_WM_COMMAND(HWND dialog, int id, HWND control, int code); // RA2: 0x006829F0, defined below
BOOL __fastcall Skirmish_Is_Computer(HWND dialog, int id); // RA2: 0x006836B0, defined below after its first callers (they expand it)
void __fastcall Skirmish_Show_Players(HWND dialog, int first, int count); // RA2: 0x006836F0, defined below
void __fastcall Skirmish_Hide_Players(HWND dialog, int first); // RA2: 0x00683920, defined below
int __stdcall Skirmish_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x00683D10, defined below (TibSun returns BOOL here)
int __fastcall Skirmish_On_WM_INITDIALOG(HWND dialog, WPARAM wparam, LPARAM lparam); // RA2: 0x00683FC0, defined below

int __fastcall Player_Control_ID(int player); // RA2: 0x004D0EE0, the id of the flag control of a player (netshare.cpp declares it the same)
int __fastcall Player_Side_Combo_ID(int player); // RA2: 0x004D1350, the id of the side combo box of a player
int __fastcall Player_Color_Combo_ID(int player); // RA2: 0x004D1CB0, the id of the color combo box of a player
void __fastcall Select_Color_Combo(HWND dialog, int id, int color); // RA2: 0x004D2480, selects the color with this item data in a color combo box (-2 for none)
void __fastcall Select_Side_Combo(HWND dialog, int id, int side); // RA2: 0x004D1A90, selects the side with this item data in a side combo box (-2 for none)
int __fastcall Side_Combo_Player(int id); // RA2: 0x004D13B0, the player a side combo box belongs to, or -1
int __fastcall Color_Combo_Player(int id); // RA2: 0x004D1D10, the player a color combo box belongs to, or -1
int __fastcall Side_Combo_Data(HWND dialog, int id, int index); // RA2: 0x004D1C70, the item data of a side combo box item (-1 for the selected one)
int __fastcall Color_Combo_Data(HWND dialog, int id, int index); // RA2: 0x004D2900, the item data of a color combo box item (-1 for the selected one)
wchar_t const * __fastcall Color_Name(int color); // RA2: 0x004D1D80, the text of a color
wchar_t const * __fastcall Side_Name(int side); // RA2: 0x004D1420, the text of a side
void __fastcall Fill_Side_Combos(HWND dialog); // RA2: 0x004D16D0, fills the side combo boxes of all players
void __fastcall Init_Color_Entries(void); // RA2: 0x004D1EA0, prepares the color table the color combo boxes list
void __fastcall Fill_Color_Combos(HWND dialog); // RA2: 0x004D2300, fills the color combo boxes of all players
void __fastcall Select_Player_Side(HWND dialog, int player); // RA2: 0x004D1820, selects the side of a player in its combo box
void __fastcall Select_Player_Color(HWND dialog, int player); // RA2: 0x004D23C0, selects the color of a player in its combo box
void __fastcall Show_Player_Flag(HWND dialog, int player); // RA2: 0x004D1230, shows the flag of the side a player has picked
void __fastcall Color_Combo_Changed(HWND dialog, int id); // RA2: 0x004D2700, a color combo box was changed
void __fastcall Set_Item_Text(HWND dialog, MultiplayerGameMode * mode); // RA2: 0x005C3450, puts the name of a game mode into the dialog
void __fastcall Set_Skirmish_Dialog_Text(HWND dialog); // RA2: 0x005C34B0, puts the description of the chosen map into the dialog
MultiplayerGameMode * Get_Current_Game_Mode(void); // RA2: 0x005C34D0, the game mode the session options name
void __fastcall Darken_Surface(Surface * surface, bool update); // RA2: 0x005159B0, dims the screen behind a dialog (init.cpp)
namespace OwnerDraw { bool __fastcall WS_Save_Dialog_Values(HWND dialog); bool __fastcall Is_Control_Disabled(HWND control); void __fastcall WideToCharString(char * dest, wchar_t const * src, int count); } // RA2: ownrdraw.cpp's 0x005E5FC0, 0x005E4720 and 0x006FA0F0 (narrows a wide string)
wchar_t * __fastcall Narrow_To_Wide(char const * narrow); // RA2: 0x006FA180, widens a string into a static buffer
MultiplayerGameMode * First_Game_Mode(void); // RA2: 0x005B6EC0, the first game mode of the list

/// <summary>
/// The id of the player type combo box of a computer player.
/// </summary>
int __fastcall Skirmish_Player_Combo_ID(int index) // RA2: 0x006827F0
{
	if (index == 0) { // RA2: as above
		return(0x50B); // RA2: as above
	}
	if (index == 1) { // RA2: as above
		return(0x50E); // RA2: as above
	}
	if (index == 2) { // RA2: as above
		return(0x516); // RA2: as above
	}
	if (index == 3) { // RA2: as above
		return(0x51A); // RA2: as above
	}
	if (index == 4) { // RA2: as above
		return(0x51B); // RA2: as above
	}
	if (index == 5) { // RA2: as above
		return(0x51C); // RA2: as above
	}
	if (index == 6) { // RA2: as above
		return(0x51D); // RA2: as above
	}
	return(-1); // RA2: as above
}

/// <summary>
/// The index of the computer player a player type combo box belongs to.
/// </summary>
int __fastcall Skirmish_Player_Index(int id) // RA2: 0x00682850
{
	if (id == 0x50B) { // RA2: as above
		return(0); // RA2: as above
	}
	if (id == 0x50E) { // RA2: as above
		return(1); // RA2: as above
	}
	if (id == 0x516) { // RA2: as above
		return(2); // RA2: as above
	}
	if (id == 0x51A) { // RA2: as above
		return(3); // RA2: as above
	}
	if (id == 0x51B) { // RA2: as above
		return(4); // RA2: as above
	}
	if (id == 0x51C) { // RA2: as above
		return(5); // RA2: as above
	}
	if (id == 0x51D) { // RA2: as above
		return(6); // RA2: as above
	}
	return(-1); // RA2: as above
}

/// <summary>
/// The text of a player type.
/// </summary>
wchar_t const * __fastcall Skirmish_Player_Type_Name(int type) // RA2: 0x006828B0
{
	if (type == -1) { // RA2: as above
		return(StringTable::LoadString("STT:PlayerNone", NULL, "D:\\Ra2\\Skirmish.cpp", 0x85)); // RA2: the tool tip of a type: none; line numbers 0x85 to 0x8B are the executable's
	}
	if (type == 2) { // RA2: as above
		return(StringTable::LoadString("STT:PlayerDumbAI", NULL, "D:\\Ra2\\Skirmish.cpp", 0x87)); // RA2: as above
	}
	if (type == 1) { // RA2: as above
		return(StringTable::LoadString("STT:PlayerSmartAI", NULL, "D:\\Ra2\\Skirmish.cpp", 0x89)); // RA2: as above
	}
	if (type == 0) { // RA2: as above
		return(StringTable::LoadString("STT:PlayerGeniusAI", NULL, "D:\\Ra2\\Skirmish.cpp", 0x8B)); // RA2: as above
	}
	return(NULL); // RA2: as above
}

/// <summary>
/// Whether a player from this one on is a computer player.
/// </summary>
bool __fastcall Skirmish_Has_Computer_After(int player) // RA2: 0x00682930; the players are numbered from 1, so the combo box of player p is Skirmish_Player_Combo_ID(p - 1)
{
	HWND dialog = SkirmishDialog; // RA2: the dialog is not passed in, it is the global
	for (int index = player; index < 8; index++) { // RA2: as above
		if (Skirmish_Is_Computer(dialog, Skirmish_Player_Combo_ID(index - 1))) { // RA2: as above
			return(true);
		}
	}
	return(false);
}

/// <summary>
/// The commands of the skirmish dialog.
/// </summary>
void __fastcall Skirmish_On_WM_COMMAND(HWND dialog, int id, HWND control, int code) // RA2: 0x006829F0; TibSun's Skirmish_On_WM_COMMAND (the same arguments: the id of the control and the notification code)
{
	int * result = (int *)GetWindowLong(dialog, DWL_USER); // RA2: the dialog keeps the address of its result at offset 8 (Skirmish_Mode_Dialog stores it with SetWindowLong), not DWL_USER (4)
	switch (id) { // RA2: as above
	case 0x50B: // RA2: the player type combo boxes of the seven computer players (Skirmish_Player_Combo_ID)
	case 0x50E: // RA2: as above
	case 0x516: // RA2: as above
	case 0x51A: // RA2: as above
	case 0x51B: // RA2: as above
	case 0x51C: // RA2: as above
	case 0x51D: // RA2: as above
		{
			int row = Skirmish_Player_Index(id) + 1; // RA2: a type was picked: the side and color boxes of the row are cleared and disabled unless a computer player is chosen
			int row_color = Player_Color_Combo_ID(row); // RA2: as above
			int row_side = Player_Side_Combo_ID(row); // RA2: as above
			bool enable = Skirmish_Is_Computer(dialog, id); // RA2: as above
			if (!enable) { // RA2: as above
				Select_Color_Combo(dialog, row_color, -2); // RA2: as above
				Select_Side_Combo(dialog, row_side, -2); // RA2: as above
			}
			HWND color_box = GetDlgItem(dialog, row_color); // RA2: as above
			HWND side_box = GetDlgItem(dialog, row_side); // RA2: as above
			EnableWindow(color_box, enable); // RA2: as above
			EnableWindow(side_box, enable); // RA2: as above
		}
		return;
	case 0x5AA: { // RA2: the button that opens the map list (Scenario_Dialog)
		int old_mode = Session.Options.MPModeIndex; // RA2: the game mode and map are kept to be put back when the list is cancelled
		int old_scenario = Session.Options.ScenarioIndex; // RA2: as above
		char old_file[0x200]; // RA2: the file name and the description are copied too, and not read again
		strcpy(old_file, Session.ScenarioFileName); // RA2: as above
		wchar_t old_name[0x100]; // RA2: as above
		wcscpy(old_name, Session.Options.ScenarioDescription); // RA2: as above
		OwnerDraw::WS_Save_Dialog_Values(dialog); // RA2: saves the values of the dialog before it is hidden (0x005E5FC0)
		ShowWindow(dialog, SW_HIDE); // RA2: as above
		if (Scenario_Dialog(MainWindow) == IDCANCEL) {
			Session.Options.MPModeIndex = old_mode; // RA2: as above
			Session.Options.ScenarioIndex = old_scenario; // RA2: as above
			Set_Scenario_Info_From_Index(old_scenario); // RA2: as above
			Update_Network_Dialog_Preview(dialog); // RA2: as above
			ShowWindow(dialog, SW_SHOW); // RA2: as above
			if (Session.Scenarios[Session.Options.ScenarioIndex]->Is_Random_Map()) { // RA2: as above
				if (MultiplayerMapPreview != NULL) {
					delete MultiplayerMapPreview;
				}
				MultiplayerMapPreview = new MapPreviewClass;
				MultiplayerMapPreview->Read_PCX_Preview("RandMap.img");
				if (MultiplayerMapPreview->Get_Preview_Surface() == NULL) {
					Update_Network_Dialog_Preview(dialog); // RA2: as above
				}
				InvalidateRect(dialog, NULL, FALSE); // RA2: as above
			} else {
				Update_Network_Dialog_Preview(dialog); // RA2: as above
			}
			InvalidateRect(dialog, NULL, FALSE); // RA2: as above
			return;
		}
		int scenario = Session.Options.ScenarioIndex; // RA2: as above
		int old_players; // RA2: the number of rows the old map showed: 8 for the random map (-1)
		if (old_scenario == -1) { // RA2: as above
			old_players = 8; // RA2: as above
		} else {
			old_players = RandomMapWaypointCount(old_scenario); // RA2: as above
		}
		int players = RandomMapWaypointCount(scenario); // RA2: the rows a map shows depend on its starting positions
		int extra = players - old_players; // RA2: as above
		if (old_scenario == scenario) { // RA2: the same map again (the list was left with a different game mode): all rows are redone
			Skirmish_Hide_Players(dialog, 1); // RA2: as above
			Skirmish_Show_Players(dialog, 1, players - 1); // RA2: as above
		}
		if (extra > 0) { // RA2: as above
			Skirmish_Show_Players(dialog, old_players, extra); // RA2: as above
		} else if (extra < 0) { // RA2: as above
			Skirmish_Hide_Players(dialog, players); // RA2: as above
		}
		ShowWindow(dialog, SW_SHOW); // RA2: as above
		if (Set_Scenario_Info_From_Index(Session.Options.ScenarioIndex)) { // RA2: as above
			Set_Item_Text(dialog, MultiplayerGameMode::Get(Session.Options.MPModeIndex)); // RA2: as above
			Set_Skirmish_Dialog_Text(dialog); // RA2: as above
			if (Session.Scenarios[Session.Options.ScenarioIndex]->Is_Random_Map()) { // RA2: as above
				if (MultiplayerMapPreview != NULL) {
					delete MultiplayerMapPreview;
					MultiplayerMapPreview = new MapPreviewClass;
					MultiplayerMapPreview->Read_PCX_Preview("RandMap.img");
				}
				if (MultiplayerMapPreview->Get_Preview_Surface() == NULL) {
					Update_Network_Dialog_Preview(dialog); // RA2: as above
				}
				InvalidateRect(dialog, NULL, FALSE); // RA2: as above
			} else {
				Update_Network_Dialog_Preview(dialog); // RA2: as above
			}
		} else {
			Session.Options.ScenarioIndex = old_scenario; // RA2: as above
			Session.Options.MPModeIndex = old_mode; // RA2: as above
		}
		return;
	}
	case 0x5C0: // RA2: 0x5C0 ends the dialog as Back (Skirmish_Mode_Dialog darkens the screen for it) and 0x617 as Start (it returns true for it); both store the settings, only Start checks them
	case 0x617: { // RA2: as above
		if (code != 0) { // RA2: as above
			break;
		}
		if (id == 0x617) { // RA2: as above
			EnableWindow(GetDlgItem(dialog, 0x617), FALSE); // RA2: as above
		}
		int players = RandomMapWaypointCount(Session.Options.ScenarioIndex); // RA2: as above
		int computers = 0; // RA2: as above
		int index; // RA2: as above
		for (index = 0; index < 7; index++) { // RA2: as above
			if (Skirmish_Is_Computer(dialog, Skirmish_Player_Combo_ID(index))) { // RA2: as above
				computers++; // RA2: as above
			}
		}
		int total = computers + 1; // RA2: the human player counts too
		Session.Options.AIPlayers = computers; // RA2: the number of computer players the rows ask for
		if (id == 0x617) { // RA2: as above
			wchar_t text[0x100]; // RA2: as above
			if (players < total) { // RA2: as above
				swprintf(text, StringTable::LoadString("TXT_SCENARIO_TOO_SMALL", NULL, "D:\\Ra2\\Skirmish.cpp", 0x334), players); // RA2: the map has fewer starting positions than players
				WWMessageBox(L" ").Process(text, StringTable::LoadString("TXT_OK", NULL, "D:\\Ra2\\Skirmish.cpp", 0x335), NULL, NULL, false); // RA2: as above
				EnableWindow(GetDlgItem(dialog, 0x617), TRUE); // RA2: as above
				return;
			}
			if (total <= 1) { // RA2: as above
				swprintf(text, StringTable::LoadString("TXT_NEED_AT_LEAST_TWO_PLAYERS", NULL, "D:\\Ra2\\Skirmish.cpp", 0x33C), players); // RA2: as above
				WWMessageBox(L" ").Process(text, StringTable::LoadString("TXT_OK", NULL, "D:\\Ra2\\Skirmish.cpp", 0x33D), NULL, NULL, false); // RA2: as above
				EnableWindow(GetDlgItem(dialog, 0x617), TRUE); // RA2: as above
				return;
			}
		}
		MultiplayerGameMode * mode = Session.MPGameMode; // RA2: as above
		WideWstring message; // RA2: the game mode may refuse the setup with a message (CheckStartConditions, slot 5 of its vtable)
		if (!mode->CheckStartConditions(&message)) { // RA2: as above
			if (id == 0x617) { // RA2: as above
				WWMessageBox(L" ").Process(message.get(), StringTable::LoadString("TXT_OK", NULL, "D:\\Ra2\\Skirmish.cpp", 0x34B), NULL, NULL, false); // RA2: as above
				EnableWindow(GetDlgItem(dialog, 0x617), TRUE); // RA2: as above
				return;
			}
			First_Game_Mode(); // RA2: as above
		}
		Session.Skirmish.MPGameMode = Session.Options.MPModeIndex; // RA2: as above
		Session.Skirmish.ScenIndex = Session.Options.ScenarioIndex; // RA2: as above
		if (Session.Options.ScenarioIndex >= Session.Scenarios.Count()) { // RA2: as above
			Session.Skirmish.ScenIndex = 0; // RA2: as above
		}
		wchar_t name[20]; // RA2: the name edit box takes 19 characters (Skirmish_On_WM_INITDIALOG limits it so)
		HWND edit = GetDlgItem(dialog, 0x6A0); // RA2: as above
		SendMessage(edit, 0x4B3, 20, (LPARAM)name); // RA2: as above
		OwnerDraw::WideToCharString(Session.Handle, name, -1); // RA2: as above
		int local_side = Side_Combo_Data(dialog, 0x6A1, -1); // RA2: as above
		Session.Set_Side(local_side, true); // RA2: the side of the local player (random is allowed: Assign_Random_Choices resolves it below)
		for (index = 0; index < 7; index++) { // RA2: as above
			int player = index + 1; // RA2: as above
			int combo = Skirmish_Player_Combo_ID(index); // RA2: as above
			int side_id = Player_Side_Combo_ID(player); // RA2: as above
			int color_id = Player_Color_Combo_ID(player); // RA2: as above
			if (Skirmish_Is_Computer(dialog, combo)) { // RA2: as above
				int difficulty = SendDlgItemMessage(dialog, combo, CB_GETCURSEL, 0, 0); // RA2: as above
				Session.Options.AISlots.Difficulties[player] = SendDlgItemMessage(dialog, combo, CB_GETITEMDATA, difficulty, 0); // RA2: the item data of a type box is the computer player's difficulty: 0 hard, 1 normal, 2 easy
				Session.Options.AISlots.Countries[player] = Side_Combo_Data(dialog, side_id, -1); // RA2: as above
				Session.Options.AISlots.Colors[player] = Color_Combo_Data(dialog, color_id, -1); // RA2: as above
			}
		}
		for (int player = 1; player < 8; player++) { // RA2: as above
			int combo = Skirmish_Player_Combo_ID(player - 1); // RA2: as above
			int side_id = Player_Side_Combo_ID(player); // RA2: as above
			int color_id = Player_Color_Combo_ID(player); // RA2: as above
			int selected = SendDlgItemMessage(dialog, combo, CB_GETCURSEL, 0, 0); // RA2: as above
			int type = SendDlgItemMessage(dialog, combo, CB_GETITEMDATA, selected, 0); // RA2: as above
			int side = Side_Combo_Data(dialog, side_id, -1); // RA2: as above
			int color = Color_Combo_Data(dialog, color_id, -1); // RA2: as above
			int value; // RA2: as above
			if (type == -1) { // RA2: as above
				value = 1; // RA2: as above
			} else if (type == 0) { // RA2: as above
				value = 4; // RA2: as above
			} else if (type == 1) { // RA2: as above
				value = 5; // RA2: as above
			} else {
				value = type == 2 ? 6 : 0; // RA2: as above
			}
			Session.Skirmish.SlotData[player][0] = value; // RA2: the saved slot: the type (1 none, 4 hard, 5 normal, 6 easy computer), the side and the color
			Session.Skirmish.SlotData[player][1] = side; // RA2: as above
			Session.Skirmish.SlotData[player][2] = color; // RA2: as above
		}
		int local_color = Color_Combo_Data(dialog, 0x6A2, -1); // RA2: as above
		Session.Set_Color(local_color, true); // RA2: as above
		NodeNameType * node = new NodeNameType; // RA2: as above
		if (node != NULL) { // RA2: as above
			wcscpy(node->Name, Narrow_To_Wide(Session.Handle)); // RA2: as above
			node->Slot.Country = Session.House; // RA2: the local player's entry in the player list (TibSun keeps house and color in Player instead)
			node->Slot.ColorIdx = Session.PrefColor; // RA2: as above
			node->Slot.Time = -1; // RA2: as above
			Session.Players.Add(node); // RA2: as above
		}
		Session.Assign_Random_Choices(); // RA2: turns the random sides and colors into real ones (0x00672BB0)
		HWND item = GetDlgItem(dialog, 0x529); // RA2: as above
		if (item != NULL) { // RA2: as above
			Session.Options.GameSpeed = 6 - SendMessage(item, TBM_GETPOS, 0, 0); // RA2: the speed slider counts the other way round
			Options.GameSpeed = Session.Options.GameSpeed;
		}
		item = GetDlgItem(dialog, 0x511); // RA2: as above
		if (item != NULL) { // RA2: as above
			Session.Options.Credits = SendMessage(item, TBM_GETPOS, 0, 0); // RA2: as above
		}
		item = GetDlgItem(dialog, 0x50C); // RA2: as above
		if (item != NULL) { // RA2: as above
			Session.Options.UnitCount = SendMessage(item, TBM_GETPOS, 0, 0); // RA2: as above
		}
		Session.Skirmish.GameSpeed = Session.Options.GameSpeed; // RA2: as above
		Session.Skirmish.Credits = Session.Options.Credits; // RA2: as above
		Session.Skirmish.UnitCount = Session.Options.UnitCount; // RA2: as above
		item = GetDlgItem(dialog, 0x54E); // RA2: as above
		if (item != NULL) { // RA2: as above
			Session.Options.ShortGame = SendMessage(item, BM_GETCHECK, 0, 0) == 1; // RA2: as above
		}
		item = GetDlgItem(dialog, 0x69A); // RA2: as above
		if (item != NULL) { // RA2: as above
			Session.Options.SWAllowed = SendMessage(item, BM_GETCHECK, 0, 0) == 1; // RA2: as above
		}
		item = GetDlgItem(dialog, 0x69D); // RA2: as above
		if (item != NULL) { // RA2: as above
			Session.Options.BuildOffAlly = SendMessage(item, BM_GETCHECK, 0, 0) == 1; // RA2: as above
		}
		item = GetDlgItem(dialog, 0x693); // RA2: as above
		if (item != NULL) { // RA2: as above
			Session.Options.MCVRedeploy = SendMessage(item, BM_GETCHECK, 0, 0) == 1; // RA2: as above
		}
		item = GetDlgItem(dialog, 0x696); // RA2: as above
		if (item != NULL) { // RA2: as above
			Session.Options.Goodies = SendMessage(item, BM_GETCHECK, 0, 0) == 1; // RA2: as above
		}
		Session.Skirmish.ShortGame = Session.Options.ShortGame; // RA2: as above
		Session.Skirmish.SuperWeaponsAllowed = Session.Options.SWAllowed; // RA2: as above
		Session.Skirmish.BuildOffAlly = Session.Options.BuildOffAlly; // RA2: as above
		Session.Skirmish.MCVRepacks = Session.Options.MCVRedeploy; // RA2: as above
		Session.Skirmish.CratesAppear = Session.Options.Goodies; // RA2: as above
		Session.Options.FogOfWar = false; // RA2: the options a skirmish does not offer
		Session.Options.BridgeDestruction = true; // RA2: as above
		Session.Options.HarvTruce = false;
		Session.Options.CrapEngineers = false; // RA2: as above
		if (MultiplayerMapPreview != NULL) {
			delete MultiplayerMapPreview;
			MultiplayerMapPreview = NULL;
		}
		*result = id;
		return;
	}
	case 0x510: // RA2: the side combo boxes (of the local player and of the computer players): the flag of the side is shown
	case 0x513: // RA2: as above
	case 0x514: // RA2: as above
	case 0x51E: // RA2: as above
	case 0x51F: // RA2: as above
	case 0x520: // RA2: as above
	case 0x521: // RA2: as above
	case 0x6A1: // RA2: as above
		Show_Player_Flag(dialog, Side_Combo_Player(id)); // RA2: as above
		break;
	case 0x522: // RA2: the color combo boxes: a change is passed on when it was a selection (code 1, CBN_SELCHANGE)
	case 0x523: // RA2: as above
	case 0x524: // RA2: as above
	case 0x525: // RA2: as above
	case 0x526: // RA2: as above
	case 0x527: // RA2: as above
	case 0x528: // RA2: as above
	case 0x6A2: // RA2: as above
		if (code == 1) { // RA2: as above
			Color_Combo_Changed(dialog, id); // RA2: as above
		}
		break;
	}
}

/// <summary>
/// Whether the player type combo box selects a computer player.
/// </summary>
BOOL __fastcall Skirmish_Is_Computer(HWND dialog, int id) // RA2: as above
{
	int selection = SendDlgItemMessage(dialog, id, CB_GETCURSEL, 0, 0); // RA2: as above
	int type = SendDlgItemMessage(dialog, id, CB_GETITEMDATA, selection, 0); // RA2: as above
	if (type != 0 && type != 1 && type != 2) { // RA2: as above
		return(FALSE);
	}
	return(TRUE);
}

/// <summary>
/// Shows the rows of some players.
/// </summary>
void __fastcall Skirmish_Show_Players(HWND dialog, int first, int count) // RA2: as above
{
	for (int player = first; player < first + count; player++) { // RA2: as above
		int combo = Skirmish_Player_Combo_ID(player - 1); // RA2: as above
		int row_id = Player_Control_ID(player); // RA2: as above
		int color_id = Player_Color_Combo_ID(player); // RA2: as above
		int side_id = Player_Side_Combo_ID(player); // RA2: as above
		HWND combo_window = GetDlgItem(dialog, combo); // RA2: as above
		HWND row_window = GetDlgItem(dialog, row_id); // RA2: as above
		HWND color_window = GetDlgItem(dialog, color_id); // RA2: as above
		HWND side_window = GetDlgItem(dialog, side_id); // RA2: as above
		ShowWindow(combo_window, SW_SHOW); // RA2: as above
		ShowWindow(row_window, SW_SHOW); // RA2: as above
		ShowWindow(color_window, SW_SHOW); // RA2: as above
		ShowWindow(side_window, SW_SHOW); // RA2: as above
		int row = Skirmish_Player_Index(combo) + 1; // RA2: as above
		int row_color = Player_Color_Combo_ID(row); // RA2: as above
		int row_side = Player_Side_Combo_ID(row); // RA2: as above
		int selection = SendDlgItemMessage(dialog, combo, CB_GETCURSEL, 0, 0); // RA2: as above
		int type = SendDlgItemMessage(dialog, combo, CB_GETITEMDATA, selection, 0); // RA2: as above
		bool enable = type == 0 || type == 1 || type == 2; // RA2: as above
		if (!enable) { // RA2: as above
			Select_Color_Combo(dialog, row_color, -2); // RA2: as above
			Select_Side_Combo(dialog, row_side, -2); // RA2: as above
		}
		HWND color_box = GetDlgItem(dialog, row_color); // RA2: as above
		HWND side_box = GetDlgItem(dialog, row_side); // RA2: as above
		EnableWindow(color_box, enable); // RA2: as above
		EnableWindow(side_box, enable); // RA2: as above
	}
}

/// <summary>
/// Resets and hides the rows of the players from this one on.
/// </summary>
void __fastcall Skirmish_Hide_Players(HWND dialog, int first) // RA2: as above
{
	int player; // RA2: as above
	for (player = first; player < 8; player++) { // RA2: as above
		int combo = Skirmish_Player_Combo_ID(player - 1); // RA2: as above
		int count = SendDlgItemMessage(dialog, combo, CB_GETCOUNT, 0, 0); // RA2: as above
		for (int item = 0; item < count; item++) { // RA2: as above
			if (SendDlgItemMessage(dialog, combo, CB_GETITEMDATA, item, 0) == -1) { // RA2: as above
				SendDlgItemMessage(dialog, combo, CB_SETCURSEL, item, 0); // RA2: as above
				break;
			}
		}
		int row = Skirmish_Player_Index(combo) + 1; // RA2: as above
		int row_color = Player_Color_Combo_ID(row); // RA2: as above
		int row_side = Player_Side_Combo_ID(row); // RA2: as above
		int selection = SendDlgItemMessage(dialog, combo, CB_GETCURSEL, 0, 0); // RA2: as above
		int type = SendDlgItemMessage(dialog, combo, CB_GETITEMDATA, selection, 0); // RA2: as above
		bool enable = type == 0 || type == 1 || type == 2; // RA2: as above
		if (!enable) { // RA2: as above
			Select_Color_Combo(dialog, row_color, -2); // RA2: as above
			Select_Side_Combo(dialog, row_side, -2); // RA2: as above
		}
		HWND color_box = GetDlgItem(dialog, row_color); // RA2: as above
		HWND side_box = GetDlgItem(dialog, row_side); // RA2: as above
		EnableWindow(color_box, enable); // RA2: as above
		EnableWindow(side_box, enable); // RA2: as above
	}
	for (player = first; player < 8; player++) { // RA2: as above
		int combo = Skirmish_Player_Combo_ID(player - 1); // RA2: as above
		int row_id = Player_Control_ID(player); // RA2: as above
		int side_id = Player_Side_Combo_ID(player); // RA2: as above
		int color_id = Player_Color_Combo_ID(player); // RA2: as above
		HWND combo_window = GetDlgItem(dialog, combo); // RA2: as above
		HWND row_window = GetDlgItem(dialog, row_id); // RA2: as above
		HWND side_window = GetDlgItem(dialog, side_id); // RA2: as above
		HWND color_window = GetDlgItem(dialog, color_id); // RA2: as above
		ShowWindow(combo_window, SW_HIDE); // RA2: as above
		ShowWindow(row_window, SW_HIDE); // RA2: as above
		ShowWindow(side_window, SW_HIDE); // RA2: as above
		ShowWindow(color_window, SW_HIDE); // RA2: as above
	}
}

/// <summary>
/// Runs the skirmish dialog.
/// </summary>
bool Skirmish_Mode_Dialog(void)
{
	int result = -1; // RA2: as above
	Rule->Do_HouseTypes(*RuleINI);
	Rule->Do_Sides(*RuleINI);
	for (int index = 0; index < HouseTypes.Count(); index++) { // RA2: as above
		HouseTypes[index]->Read_INI(*RuleINI); // RA2: as above
	}
	HWND dialog = OwnerDraw::Begin_Dialog(0x102, Skirmish_Dialog_Proc, 0); // RA2: as above
	if (dialog != NULL) { // RA2: as above
		SkirmishDialog = dialog; // RA2: as above
		SetWindowLong(dialog, 8, (LONG)&result); // RA2: as above
		OwnerDraw::Display_Dialog(dialog);
		while (result != 0x617 && result != 0x5C0 && OwnerDraw::Dialog_Message_Handler() != true) { // RA2: as above
			Title_Screen_Restore(false); // RA2: as above
		}
		OwnerDraw::End_Dialog(dialog);
		SkirmishDialog = NULL; // RA2: as above
	}
	if (MultiplayerMapPreview != NULL) {
		delete MultiplayerMapPreview;
		MultiplayerMapPreview = NULL;
	}
	Session.Write_MultiPlayer_Settings();
	if (result == 0x5C0) { // RA2: as above
		Darken_Surface(NULL, true); // RA2: as above
	}
	if (result == 0x617) { // RA2: as above
		return(true);
	}
	return(false);
}

/// <summary>
/// The dialog procedure of the skirmish dialog.
/// </summary>
int __stdcall Skirmish_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) // RA2: 0x00683D10
{
	int result = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam); // RA2: the owner draw handler (0x005FFB00) has the first say
	if (result != 0) { // RA2: as above
		return(result); // RA2: as above
	}
	switch (message) {
	case WM_PAINT:
		if (MultiplayerMapPreview != NULL && !OwnerDraw::Is_Control_Disabled(GetDlgItem(window, 0x468))) { // RA2: the preview is drawn only while its control is enabled
			MultiplayerMapPreview->Blit_Preview(window);
		}
		ValidateRect(window, NULL);
		return(0); // RA2: as above
	case WM_COMMAND:
		Skirmish_On_WM_COMMAND(window, wparam & 0xFFFF, (HWND)lparam, wparam >> 16); // RA2: as above
		return(1); // RA2: as above
	case 0x497: // RA2: RA2's owner draw dialogs send 0x497 to set themselves up (the counterpart of WM_INITDIALOG)
		return(Skirmish_On_WM_INITDIALOG(window, wparam, lparam));
	case 0x4E9: { // RA2: the tool tip text of a combo box item: a player type, a side or a color
		ToolTipTextRequest * request = (ToolTipTextRequest *)lparam; // RA2: as above
		request->Text = NULL; // RA2: as above
		if (request->Window == NULL || request->Item == -1) { // RA2: as above
			return(1); // RA2: as above
		}
		int id = GetDlgCtrlID(request->Window); // RA2: as above
		if (Skirmish_Player_Index(id) == -1) { // RA2: as above
			if (Side_Combo_Player(id) == -1) { // RA2: as above
				if (Color_Combo_Player(id) == -1) { // RA2: as above
					return(1); // RA2: as above
				}
				request->Text = Color_Name(Color_Combo_Data(window, id, request->Item)); // RA2: as above
				return(1); // RA2: as above
			}
			int side = Side_Combo_Data(window, id, request->Item); // RA2: as above
			if (side == -1) { // RA2: as above
				return(1); // RA2: as above
			}
			request->Text = Side_Name(side); // RA2: as above
			return(1); // RA2: as above
		}
		int item = request->Item; // RA2: as above
		if (item == -1) { // RA2: as above
			item = SendDlgItemMessage(window, id, CB_GETCURSEL, 0, 0); // RA2: as above
		}
		request->Text = Skirmish_Player_Type_Name(SendDlgItemMessage(window, id, CB_GETITEMDATA, item, 0)); // RA2: as above
		return(1); // RA2: as above
	}
	}
	return(0); // RA2: as above
}

/// <summary>
/// Fills the skirmish dialog from the saved settings.
/// </summary>
int __fastcall Skirmish_On_WM_INITDIALOG(HWND dialog, WPARAM wparam, LPARAM lparam) // RA2: 0x00683FC0; TibSun's Skirmish_On_WM_INITDIALOG
{
	Session.Options.Reset_AI(); // RA2: no computer players and no countries or colors chosen yet (0x00672810)
	HWND item = GetDlgItem(dialog, 0x6A0); // RA2: as above
	if (item != NULL) { // RA2: as above
		SendMessage(item, 0xC5, 19, 0); // RA2: EM_SETLIMITTEXT: the name has 19 characters at most
		SendMessage(item, 0x4B2, 0, (LPARAM)Narrow_To_Wide(Session.Handle)); // RA2: sets the text of the owner draw edit control (0x4B2) to the saved name
		SendMessage(item, 0x4D1, 1, 0); // RA2: as above
	}
	for (int i = 0; i < 7; i++) { // RA2: as above
		item = GetDlgItem(dialog, Skirmish_Player_Combo_ID(i)); // RA2: as above
		SendMessage(item, CB_RESETCONTENT, 0, 0); // RA2: as above
		int index = SendMessage(item, 0x4C2, 0, (LPARAM)StringTable::LoadString("GUI:None", NULL, "D:\\Ra2\\Skirmish.cpp", 0x9C)); // RA2: adds an item to the owner draw combo box (0x4C2) and gives it the player type as its data: -1 none, 2 easy, 1 normal, 0 hard computer
		SendMessage(item, CB_SETITEMDATA, index, -1); // RA2: as above
		index = SendMessage(item, 0x4C2, 0, (LPARAM)StringTable::LoadString("GUI:AIEasy", NULL, "D:\\Ra2\\Skirmish.cpp", 0x9F)); // RA2: as above
		SendMessage(item, CB_SETITEMDATA, index, 2); // RA2: as above
		index = SendMessage(item, 0x4C2, 0, (LPARAM)StringTable::LoadString("GUI:AINormal", NULL, "D:\\Ra2\\Skirmish.cpp", 0xA2)); // RA2: as above
		SendMessage(item, CB_SETITEMDATA, index, 1); // RA2: as above
		index = SendMessage(item, 0x4C2, 0, (LPARAM)StringTable::LoadString("GUI:AIHard", NULL, "D:\\Ra2\\Skirmish.cpp", 0xA5)); // RA2: as above
		SendMessage(item, CB_SETITEMDATA, index, 0); // RA2: as above
		SendMessage(item, CB_SETCURSEL, 0, 0); // RA2: as above
	}
	Fill_Side_Combos(dialog); // RA2: as above
	Init_Color_Entries(); // RA2: as above
	Fill_Color_Combos(dialog); // RA2: as above
	Select_Player_Side(dialog, 0); // RA2: as above
	Select_Player_Color(dialog, 0); // RA2: as above
	for (int player = 1; player < 8; player++) { // RA2: as above
		int combo = Skirmish_Player_Combo_ID(player - 1); // RA2: as above
		int type = Session.Skirmish.SlotData[player][0]; // RA2: the saved type of the slot, turned into the item data the boxes use
		int selected; // RA2: as above
		if (type == 1) { // RA2: as above
			selected = -1; // RA2: as above
		} else if (type == 4) { // RA2: as above
			selected = 0; // RA2: as above
		} else if (type == 5) { // RA2: as above
			selected = 1; // RA2: as above
		} else if (type == 6) { // RA2: as above
			selected = 2; // RA2: as above
		} else {
			selected = -1; // RA2: as above
		}
		int count = SendDlgItemMessage(dialog, combo, CB_GETCOUNT, 0, 0); // RA2: as above
		for (int row = 0; row < count; row++) { // RA2: as above
			if (SendDlgItemMessage(dialog, combo, CB_GETITEMDATA, row, 0) == selected) { // RA2: as above
				SendDlgItemMessage(dialog, combo, CB_SETCURSEL, row, 0); // RA2: as above
				break;
			}
		}
		int side = Player_Side_Combo_ID(player); // RA2: as above
		int color = Player_Color_Combo_ID(player); // RA2: as above
		if (selected == -1) { // RA2: as above
			Select_Side_Combo(dialog, side, -2); // RA2: as above
			Select_Color_Combo(dialog, color, -2); // RA2: as above
			HWND side_window = GetDlgItem(dialog, side); // RA2: as above
			HWND color_window = GetDlgItem(dialog, color); // RA2: as above
			EnableWindow(side_window, FALSE); // RA2: as above
			EnableWindow(color_window, FALSE); // RA2: as above
		} else {
			Select_Side_Combo(dialog, side, Session.Skirmish.SlotData[player][1]); // RA2: as above
			Select_Color_Combo(dialog, color, Session.Skirmish.SlotData[player][2]); // RA2: as above
		}
	}
	if (Session.Skirmish.MPGameMode < 0 || Session.Skirmish.MPGameMode >= MultiplayerGameMode::GetModeCount()) { // RA2: as above
		Session.Skirmish.MPGameMode = First_Game_Mode()->MPModeIndex; // RA2: a saved game mode that does not exist: the first one
	}
	Session.Options.MPModeIndex = Session.Skirmish.MPGameMode; // RA2: as above
	MultiplayerGameMode * mode = Get_Current_Game_Mode(); // RA2: as above
	Session.Skirmish.MPGameMode = mode->MPModeIndex; // RA2: as above
	Session.Options.MPModeIndex = mode->MPModeIndex; // RA2: as above
	if (Session.Skirmish.ScenIndex < 0 || Session.Skirmish.ScenIndex >= Session.Scenarios.Count()) { // RA2: as above
		Session.Skirmish.ScenIndex = 0; // RA2: as above
	}
	Session.Options.ScenarioIndex = Set_Scenario_Info_From_Index(Session.Skirmish.ScenIndex) ? Session.Skirmish.ScenIndex : 0; // RA2: the saved map, or the first one when it cannot be used
	MultiMission * map = Session.Scenarios[Session.Options.ScenarioIndex]; // RA2: as above
	if (!mode->IsMapAllowed(map)) { // RA2: the saved map does not suit the game mode: its first valid map is taken
		int selection = mode->FirstValidMapIndex(); // RA2: as above
		if (selection == -1) { // RA2: as above
			mode = First_Game_Mode(); // RA2: as above
			Session.Skirmish.MPGameMode = mode->MPModeIndex; // RA2: as above
			Session.Options.MPModeIndex = mode->MPModeIndex; // RA2: as above
			selection = mode->FirstValidMapIndex(); // RA2: as above
		}
		map = Session.Scenarios[selection]; // RA2: as above
		Session.Skirmish.ScenIndex = selection; // RA2: as above
		Session.Options.ScenarioIndex = selection; // RA2: as above
	}
	int scenario = Session.Options.ScenarioIndex; // RA2: as above
	int players = RandomMapWaypointCount(scenario); // RA2: as above
	if (scenario == -1) { // RA2: as above
		Skirmish_Hide_Players(dialog, 1); // RA2: as above
		Skirmish_Show_Players(dialog, 1, players - 1); // RA2: as above
	}
	int extra = players - 8; // RA2: as above
	if (extra > 0) { // RA2: as above
		Skirmish_Show_Players(dialog, 8, extra); // RA2: as above
	} else if (extra < 0) { // RA2: as above
		Skirmish_Hide_Players(dialog, players); // RA2: as above
	}
	Session.MPGameMode = mode; // RA2: as above
	mode->vt_entry_20(); // RA2: tells the game mode that it is the chosen one (slot 8 of its vtable)
	Set_Item_Text(dialog, mode); // RA2: as above
	Set_Skirmish_Dialog_Text(dialog); // RA2: as above
	Clear_Vector(&Session.Players);
	Session.Options.GameSpeed = Session.Skirmish.GameSpeed; // RA2: as above
	Session.Options.Credits = Session.Skirmish.Credits; // RA2: as above
	Session.Options.UnitCount = Session.Skirmish.UnitCount; // RA2: as above
	item = GetDlgItem(dialog, 0x529); // RA2: as above
	if (item != NULL) { // RA2: as above
		int speed = 6 - Session.Options.GameSpeed; // RA2: as above
		SendMessage(item, TBM_SETRANGE, 1, 0x60000); // RA2: the speed slider runs from 0 to 6
		SendMessage(item, TBM_SETPOS, 1, speed); // RA2: as above
	}
	item = GetDlgItem(dialog, 0x511); // RA2: as above
	if (item != NULL) { // RA2: as above
		SendMessage(item, TBM_SETRANGE, 1, ((unsigned int)(unsigned short)Rule->MPMaxMoney << 16) | (unsigned short)Rule->MinMoney); // RA2: as above
		SendMessage(item, TBM_SETPOS, 1, Session.Options.Credits); // RA2: as above
		SendMessage(item, 0x4AB, 0, Rule->MoneyIncrement); // RA2: sets the step of the owner draw credits slider (0x4AB)
	}
	item = GetDlgItem(dialog, 0x50C); // RA2: as above
	if (item != NULL) { // RA2: as above
		SendMessage(item, TBM_SETRANGE, 1, ((unsigned int)(unsigned short)Rule->MaxUnitCount << 16) | (unsigned short)Rule->MinUnitCount); // RA2: as above
		SendMessage(item, TBM_SETPOS, 1, Session.Options.UnitCount); // RA2: as above
	}
	Session.Options.ShortGame = Session.Skirmish.ShortGame; // RA2: as above
	Session.Options.SWAllowed = Session.Skirmish.SuperWeaponsAllowed; // RA2: as above
	Session.Options.BuildOffAlly = Session.Skirmish.BuildOffAlly; // RA2: as above
	Session.Options.MCVRedeploy = Session.Skirmish.MCVRepacks; // RA2: as above
	Session.Options.Goodies = Session.Skirmish.CratesAppear; // RA2: as above
	item = GetDlgItem(dialog, 0x54E); // RA2: as above
	if (item != NULL) { // RA2: as above
		SendMessage(item, BM_SETCHECK, Session.Options.ShortGame != false, 0); // RA2: as above
	}
	item = GetDlgItem(dialog, 0x69A); // RA2: as above
	if (item != NULL) { // RA2: as above
		SendMessage(item, BM_SETCHECK, Session.Options.SWAllowed != false, 0); // RA2: as above
	}
	item = GetDlgItem(dialog, 0x69D); // RA2: as above
	if (item != NULL) { // RA2: as above
		SendMessage(item, BM_SETCHECK, Session.Options.BuildOffAlly != false, 0); // RA2: as above
	}
	item = GetDlgItem(dialog, 0x693); // RA2: as above
	if (item != NULL) { // RA2: as above
		SendMessage(item, BM_SETCHECK, Session.Options.MCVRedeploy != false, 0); // RA2: as above
	}
	item = GetDlgItem(dialog, 0x696); // RA2: as above
	if (item != NULL) { // RA2: as above
		SendMessage(item, BM_SETCHECK, Session.Options.Goodies != false, 0); // RA2: as above
	}
	if (map->Is_Random_Map()) { // RA2: as above
		if (MultiplayerMapPreview != NULL) {
			delete MultiplayerMapPreview;
		}
		MultiplayerMapPreview = new MapPreviewClass;
		MultiplayerMapPreview->Read_PCX_Preview("RandMap.img");
		if (MultiplayerMapPreview->Get_Preview_Surface() == NULL) {
			Update_Network_Dialog_Preview(dialog); // RA2: as above
		}
	} else {
		Update_Network_Dialog_Preview(dialog); // RA2: as above
	}
	return(0); // RA2: as above
}
