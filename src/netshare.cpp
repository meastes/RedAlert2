/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "netshare.h"
#include "bitfont.h" // RA2: the lobby messages are measured with BitFont
#include "multiplayer.h" // RA2: MultiplayerGameMode

#include "_rules.h"
#include "cd.h"
#include "conquer.h"
#include "data.h"
#include "dbgprint.h"
#include "globals.h"
#include "goptions.h"
#include "ipxmgr.h"
#include "language\language.h"
#include "lzopipe.h"
#include "lzostraw.h"
#include "mapgen.h"
#include "modemhst.h"
#include "msgbox.h"
#include "netdlg.h"
#include "netdlg2.h"
#include "newmenu.h"
#include "nulldlg.h"
#include "nullmgr.h"
#include "ownrdraw.h"
#include "rules.h"
#include "scenario.h"
#include "sendfile.h"
#include "session.h"
#include "stimer.h"
#include "wdtnet.h"
#include "windlg.h"
#include "wolapi.h" // RA2: main has no include/wolapi/; the part of the WOL API this file uses is one flat header
#include "wonline.h"
#include "worlddom.h"
#include "wstring.h"
#include "xpipe.h"
#include "xstraw.h"

#include <ctime>
#include <wchar.h> // RA2: the wide string functions the lobby messages use
#undef LoadString // RA2: windows.h maps it to LoadStringA
#include "stringtable.h" // RA2: the messages are loaded from the string table

// RA2: callees that are not in a header yet
LRESULT __fastcall Get_List_Item_Wide_Text(HWND window, WPARAM index, WideWstring & text); // RA2: 0x00600F10, the text of a list box item
void __fastcall Dialog_On_WM_PAINT(HWND dialog); // RA2: 0x005FEE70, defined in ownrdraw.cpp
void __fastcall Dialog_On_WM_DRAWITEM(long item); // RA2: as above
void __fastcall Show_Dialog(HWND dialog, int kind); // RA2: as above
void __fastcall Move_Default_Button_Owner(HWND window); // RA2: as above
void __fastcall Slide_Pending_Dialog(HWND window); // RA2: 0x005E5F20, defined in ownrdraw.cpp
MultiplayerGameMode * First_Game_Mode(void); // RA2: 0x005B6EC0, defined in the game mode list
int Net2SetHouseAndColor(char const * who, int house, int color); // RA2: 0x005BCCF0, defined in netdlg2.cpp
rc_ptr<WDTTerritory> Find_WDT_Territory(int index); // RA2: 0x00769690, the tour's territory with this index (defined in worlddom.cpp)
char * __fastcall Wide_To_Narrow(wchar_t const * text); // RA2: 0x006FA120, static buffer
wchar_t * __fastcall Narrow_To_Wide(char const * narrow); // RA2: 0x006FA180, static buffer
unsigned int __fastcall Compute_Name_CRC(wchar_t const * name); // RA2: 0x005BBA50, the wide form of netdlg.h's
void Join_Lobby_Start_Reminder(void); // RA2: 0x005C28E0, defined in netdlg2.cpp
void __cdecl Join_Lobby_Reset_Ready(void); // RA2: 0x005C2940, defined in netdlg2.cpp
void Wol_Send_Start_Reminder(void); // RA2: 0x0075C960, sends the game option "G" through the chat service (the counterpart of Send_Start_Reminder)
void Wol_Handle_Start_Reminder(void); // RA2: 0x0075C980, the joiner is told to accept: makes the ready button available and prints GUI:HostGameStartJoiner
void Wol_Reset_Ready(void); // RA2: 0x0075CA00, sends message 0x4DC to the ready button (the counterpart of Join_Lobby_Reset_Ready)

struct IChat * g_pChat;
struct INetUtil * g_pNetUtil;
struct IDownload * g_pDownload;

const COLORREF ColorSystem     = RGB(255, 255, 255)|(255<<24);  /// 0xFFFFFFFF
const COLORREF ColorUser       = RGB(240, 240, 0);              /// 0x0000F0F0
const COLORREF ColorPriv       = RGB(128, 128, 255);            /// 0x00FF8080
const COLORREF ColorPrivAction = RGB(255, 0, 255);              /// 0x00FF00FF
const COLORREF ColorAction     = RGB(255, 80, 48);              /// 0x003050FF
const COLORREF ColorOp         = RGB(0, 255, 255);              /// 0x00FFFF00
const COLORREF ColorPaged      = RGB(255, 255, 255);            /// 0x00FFFFFF
const COLORREF ColorMe         = RGB(255, 255, 64);             /// 0x0040FFFF
const COLORREF ColorNoJoin     = RGB(128, 128, 128);            /// 0x00808080

/*
 * If the host is forcing a color reassignment, then this flag will be true. It is set when
 * the host accepts the game options and cleared once the assignment has been processed, and
 * it suppresses the "color in use" message that would otherwise appear.
 */
int IsColorChangePending;

COLORREF PlayerColorTable[MAX_PLAYERS] = {
	RGB(255, 223, 94),	/// 0x005EDFFF
	RGB(255, 26, 20),	/// 0x00141AFF
	RGB(39, 60, 179),	/// 0x00B33C27
	RGB(11, 148, 11),	/// 0x000B940B
	RGB(218, 137, 26),	/// 0x001A89DA
	RGB(20, 177, 255),	/// 0x00FFB114
	RGB(185, 20, 255),	/// 0x00FF14B9
	RGB(255, 70, 173)	/// 0x00AD46FF
};


bool IsRandomMap = true;

MapPreviewClass *MultiplayerMapPreview;


/// <summary>
/// Computes a hash value for a string.
/// This routine supplies the bucket value for the dictionaries that the multiplayer dialogs
/// key by name.
/// </summary>
/// <param name="string">The string to hash.</param>
/// <returns>Returns with the hash value of the string.</returns>
unsigned int Wstring_Hash(Wstring & string)
{
	unsigned int hash = 0;

	hash = string.length();

	for (unsigned int i = 0; i < string.length(); i++) {
		hash += *(string.get() + i);
		hash += i;
		hash = (hash << 8) ^ (hash >> 24);
	}
	return(hash);
}

/// <summary>
/// Computes a hash value for a wide string.
/// This routine supplies the bucket value for the dictionaries that key by a wide name.
/// </summary>
/// <param name="string">The string to hash.</param>
/// <returns>Returns with the hash value of the string.</returns>
unsigned int Wstring_Hash(WideWstring & string) // RA2: 0x005C2BF0, the wide string class has a hash of its own; the character and the index are added in one expression
{
	unsigned int hash = 0;

	hash = string.length();

	for (unsigned int i = 0; i < string.length(); i++) {
		hash += *(string.get() + i) + i; // RA2: as above
		hash = (hash << 8) ^ (hash >> 24);
	}
	return(hash);
}



/// <summary>
/// Fetches the game options dialog that is currently up.
/// The same options are presented by four different dialogs depending on how the game was
/// started. Use this routine rather than trying to remember which one the player is looking
/// at.
/// </summary>
/// <returns>Returns with the handle of the open game options dialog. NULL is returned if
/// none of them is up.</returns>
HWND GameoptWindow(void)
{
	HWND dialog;

	dialog = WS_Find_Dialog(IDD_WOL_GAMEOPT);
	if (dialog) {
		return(dialog);
	}
	dialog = WS_Find_Dialog(IDD_WOL_GUEST);
	if (dialog) {
		return(dialog);
	}
	dialog = WS_Find_Dialog(IDD_MPLAYER_HOST);
	if (dialog) {
		return(dialog);
	}
	dialog = WS_Find_Dialog(IDD_MPLAYER_GUEST);
	if (dialog) {
		return(dialog);
	}
	return(0);
}


/// <summary>
/// Prints a formatted chat message to the player.
/// This routine hunts down the topmost dialog that has somewhere to show public and private
/// messages and puts the text there, so the caller does not have to know which dialog the
/// player is looking at. If no dialog wants chat messages, the message is quietly dropped.
/// </summary>
/// <param name="color">Color to display the message in, or -1 for the default.</param>
/// <param name="fmt">Printf style format string for the message.</param>
void __cdecl Lobby_Chat_Printf(int color, const wchar_t * fmt, ...) // RA2: 0x005C2C80, the line is built from a wide format into a wide buffer; main names it Lobby_Chat_Printf (netdlg2.cpp)
{
	va_list va;
	wchar_t buffer[1024]; // RA2: as above
	memset(buffer, 0, sizeof(buffer));

	va_start(va, fmt);
	vswprintf(buffer, fmt, va); // RA2: as above
	va_end(va);

	if (WS_Top_Window() != 0) {
		HWND top = WS_Top_Window();
		HWND msg = GetDlgItem(top, IDC_PMESSAGES);
		while (msg == 0) {
			top = WS_Next_Lower_Dialog(top);
			if (top == 0) {
				msg = 0;
				break;
			}
			msg = GetDlgItem(top, IDC_PMESSAGES);
		}

		if (msg != 0) {
			_DrawMessage(color, buffer, msg);
		}
	}
}



/// <summary>
/// Prints a formatted system message to the player.
/// This routine hunts down the topmost dialog that has somewhere to show system messages and
/// puts the text there, so the caller does not have to know which dialog the player is
/// looking at. If no dialog wants system messages, the message is quietly dropped.
/// </summary>
/// <param name="color">Color to display the message in, or -1 for the default.</param>
/// <param name="fmt">Printf style format string for the message.</param>
void __cdecl SMessagePrintf(int color, const wchar_t * fmt, ...) // RA2: 0x005C2D10, the line is built from a wide format into a wide buffer
{
	va_list va;
	wchar_t buffer[1024]; // RA2: as above
	memset(buffer, 0, sizeof(buffer));

	va_start(va, fmt);
	vswprintf(buffer, fmt, va); // RA2: as above
	va_end(va);

	if (WS_Top_Window() != 0) {
		HWND top = WS_Top_Window();
		HWND msg = GetDlgItem(top, IDC_SMESSAGES);
		while (msg == 0) {
			top = WS_Next_Lower_Dialog(top);
			if (top == 0) {
				msg = 0;
				break;
			}
			msg = GetDlgItem(top, IDC_SMESSAGES);
		}

		if (msg != 0) {
			_DrawMessage(color, buffer, msg);
		}
	}
}



/// <summary>
/// Draws a message into a message list box.
/// This routine word wraps the message to the width of the list box and adds each resulting
/// line as its own entry, so that a long chat message stays readable. Embedded newlines
/// break the text as well.
/// </summary>
/// <param name="color">Color to display the message in, or -1 for the list box
/// default.</param>
/// <param name="message">The text to display.</param>
/// <param name="window">The message list box to display the text in.</param>
void _DrawMessage(int color, const wchar_t * message, HWND window) // RA2: 0x005C2DA0, the text is wide and measured with the BitFont, and the leading blanks of a wrapped line are skipped
{
	RECT rect;
	struct { HDC hdc; SIZE size; } l; // RA2: as above

	int length = wcslen(message); // RA2: as above

	int offset = 18;
	Get_Display_Rect(window, &rect);
	if (SendMessage(window, OD_HASATTACHED, 0, 0)) {
		offset = 1;
	}

	l.hdc = GetDC(window); // RA2: as above
	SendMessage(window, OD_RESTOREDC, 0, (LPARAM)l.hdc); // RA2: as above

	while (length) {
		if (message != NULL) {
			wchar_t const * newline = wcschr(message, L'\n'); // RA2: as above
			if (newline != NULL) {
				int linelen = newline - message + 1;
				if (length >= linelen) {
					length = linelen;
				}
			}
		}

		wchar_t * copy = _wcsdup(message); // RA2: as above
		copy[length] = 0; // RA2: as above
		BitFont::Instance->Text_Dimensions(copy, (int *)&l.size.cx, (int *)&l.size.cy, 0); // RA2: as above
		free(copy); // RA2: as above
		int maxWidth = rect.right - rect.left - offset;

		if (l.size.cx >= maxWidth - 4) { // RA2: as above
			int reduceBy;
			if (l.size.cx / 2 > maxWidth) { // RA2: as above
				reduceBy = 10;
				length -= reduceBy;
			} else {
				reduceBy = 1;
			}

			int found = -1;
			int idx = length - 1;

			while (idx > 0) {
				if (!iswgraph(message[idx])) { // RA2: as above
					found = idx;
					break;
				}
				idx--;
			}

			if (found == -1) {
				length -= reduceBy;
				found = length;
			}
			length = found;
		} else {
			_SetMessageString(window, message, length, color);
			message += length;
			while (*message == L' ') { // RA2: as above
				message++; // RA2: as above
			}
			length = wcslen(message); // RA2: as above
		}
	}

	ReleaseDC(window, l.hdc); // RA2: as above
}



/// <summary>
/// Adds a single line of text to a message list box.
/// This is the low level routine that _DrawMessage uses once it has decided where the text
/// should break. The list box is capped, so a long game does not pile up messages without
/// limit.
/// </summary>
/// <param name="window">The message list box to add the line to.</param>
/// <param name="message">The text to add; only the leading characters are taken.</param>
/// <param name="length">Number of characters of the message to add.</param>
/// <param name="color">Color to display the line in, or -1 for the list box default.</param>
void _SetMessageString(HWND window, const wchar_t * message, int length, int color) // RA2: 0x005C2F20, the text is wide; the list only scrolls to the new line when it was showing its end
{
	static wchar_t buffer[512]; // RA2: as above
	memset(buffer, 0, sizeof(buffer));
	wcsncpy(buffer, message, length); // RA2: as above
	wchar_t * line_end = wcschr(buffer, L'\r'); // RA2: as above
	if (line_end != NULL) {
		line_end[0] = L'\0'; // RA2: as above
	} else {
		line_end = wcschr(buffer, L'\n'); // RA2: as above
		if (line_end != NULL) {
			line_end[0] = L'\0'; // RA2: as above
		}
	}

	int old = SendMessage(window, OD_DISABLEPAINT, 0, 1);
	int topindex = SendMessage(window, LB_GETTOPINDEX, 0, 0); // RA2: as above
	int count = SendMessage(window, LB_GETCOUNT, 0, 0); // RA2: as above
	int itemheight = SendMessage(window, LB_GETITEMHEIGHT, topindex, 0); // RA2: as above
	RECT rect;
	GetWindowRect(window, &rect); // RA2: as above
	bool atbottom = (count - topindex) <= (rect.bottom - rect.top) / itemheight; // RA2: as above
	if (count > 500) { // RA2: as above
		SendMessage(window, LB_DELETESTRING, 0, 0); // RA2: as above
		count--; // RA2: as above
	}

	int index = SendMessage(window, OD_ADDSTRINGW, 0, (LPARAM)buffer); // RA2: as above
	if (color != -1) {
		SendMessage(window, OD_SETCOLOR, index, color);
	}

	if (atbottom) { // RA2: as above
		SendMessage(window, LB_SETTOPINDEX, count, 0); // RA2: as above
	}
	SendMessage(window, OD_DISABLEPAINT, 0, old);
}


/// <summary>
/// Adds a dot to the last chat line.
/// This routine finds the chat list of the topmost dialog and appends a dot to its last line. Once
/// that line is long enough the dot starts a new line, so a long wait shows progress.
/// </summary>
void Add_Chat_Dot(void) // RA2: 0x005C3040, TibSun has no such routine
{
	if (WS_Top_Window() != 0) {
		HWND top = WS_Top_Window();
		HWND msg = GetDlgItem(top, IDC_PMESSAGES);
		while (msg == 0) {
			top = WS_Next_Lower_Dialog(top);
			if (top == 0) {
				return;
			}
			msg = GetDlgItem(top, IDC_PMESSAGES);
		}

		int old = SendMessage(msg, OD_DISABLEPAINT, 0, 1); // RA2: as above
		int count = SendMessage(msg, LB_GETCOUNT, 0, 0); // RA2: as above
		WideWstring text; // RA2: as above
		if (count > 0) { // RA2: as above
			Get_List_Item_Wide_Text(msg, count - 1, text); // RA2: as above
		}
		if (text.length() < 0x24) { // RA2: as above
			SendMessage(msg, LB_DELETESTRING, count - 1, 0); // RA2: as above
			text += L"."; // RA2: as above
		} else {
			text = L"."; // RA2: as above
			count++;
		}
		SendMessage(msg, OD_ADDSTRINGW, 0, (LPARAM)text.text()); // RA2: as above
		while (count > 500) { // RA2: as above
			SendMessage(msg, LB_DELETESTRING, 0, 0); // RA2: as above
		}
		SendMessage(msg, OD_DISABLEPAINT, 0, old); // RA2: as above
	}
}



/// <summary>
/// Counts the human teams still in the game.
/// A group of allied players counts as one team. This routine is used to recognize the point
/// where only one side is left standing, which is why the house being examined is left out
/// of the tally.
/// </summary>
/// <param name="house">The house to leave out of the count.</param>
/// <returns>Returns with the number of opposing teams still alive.</returns>
int CountAliveTeams(HouseClass * house)
{
	int count = 0;
	for (int i = 0; i < Houses.Count(); i++) {
		HouseClass * house1 = Houses[i];
		if (house1 != NULL && !house1->IsDefeated && house1->IsHuman && house1 != house) {
			bool has_ally = false;
			for (int j = 0; j < Houses.Count(); j++) {
				if (j != i) {
					HouseClass * house2 = Houses[j];
					if (house2 != NULL && !house2->IsDefeated && house2->IsHuman) {
						if (house2 != house && house1->Is_Ally(Houses[j]) && house2->Is_Ally(house1)) {
							if (j > i && !has_ally) {
								count++;
							}
							has_ally = true;
						}
					}
				}
			}
			if (!has_ally) {
				count++;
			}
		}
	}
	return(count);
}


/// <summary>
/// Displays an owner drawn message box and waits for an answer.
/// Use this routine in place of the Windows message box so that the prompt matches the
/// game's artwork. The box holds up the caller, but the supplied callback is polled while it
/// is on screen so that network traffic keeps flowing.
/// </summary>
/// <param name="text">The message to display. Nothing at all happens for an empty
/// message.</param>
/// <param name="type">The button layout to use; MB_OK, MB_OKCANCEL or MB_YESNO.</param>
/// <param name="callback">Idle routine to poll while the box is up.</param>
/// <param name="large">Should the large version of the box be used?</param>
/// <returns>Returns with the control ID of the button the player pressed. Zero is returned
/// if there was nothing to display.</returns>
int ODMessageBox(const wchar_t * text, int type, bool (*callback)(void), bool large) // RA2: 0x005C3230, the text is wide and the box has RA2's dialogs: 0xD0 for OK, 0xFC for OK and Cancel, 0x126 for Yes and No, 0xFB for the large box
{
	if (text != NULL && wcslen(text) != 0) { // RA2: as above
		int id; // RA2: as above
		if (large) {
			id = 0xfb; // RA2: as above
		} else if (type == MB_OK) { // RA2: as above
			id = IDD_MSGBOX_3_SMALL; // RA2: as above
		} else if (type == MB_OKCANCEL) { // RA2: as above
			id = 0xfc; // RA2: as above
		} else {
			id = (type != MB_YESNO) ? 0xfb : 0x126; // RA2: as above
		}
		HWND dialog = WS_Create_Dialog(ProgramInstance, id, MainWindow, ODMessageBox_Proc, false); // RA2: as above
		Show_Dialog(dialog, id); // RA2: as above
		if (type == MB_OK) {
			HWND ok = GetDlgItem(dialog, IDOK);
			SetWindowLong(ok, GWL_STYLE, GetWindowLong(ok, GWL_STYLE) | WS_VISIBLE);
			if (large) {
				Move_Default_Button_Owner(ok); // RA2: as above
			}
		} else if (type == MB_OKCANCEL) { // RA2: as above
			HWND ok = GetDlgItem(dialog, IDOK);
			SetWindowLong(ok, GWL_STYLE, GetWindowLong(ok, GWL_STYLE) | WS_VISIBLE);
			HWND cancel = GetDlgItem(dialog, IDCANCEL); // RA2: as above
			SetWindowLong(cancel, GWL_STYLE, GetWindowLong(cancel, GWL_STYLE) | WS_VISIBLE); // RA2: as above
		} else if (type == MB_YESNO) { // RA2: as above
			HWND yes = GetDlgItem(dialog, IDYES);
			SetWindowLong(yes, GWL_STYLE, GetWindowLong(yes, GWL_STYLE) | WS_VISIBLE);
			HWND no = GetDlgItem(dialog, IDNO);
			SetWindowLong(no, GWL_STYLE, GetWindowLong(no, GWL_STYLE) | WS_VISIBLE);
		}
		HWND label = GetDlgItem(dialog, IDC_MSGBOX_TEXT); // RA2: as above
		SendMessage(label, OD_SETTEXTW, 0, (LPARAM)text); // RA2: as above
		ShowWindow(dialog, SW_NORMAL);
		return(WS_Wait_Dialog(dialog, callback, false, true, true)); // RA2: as above
	}
	return(0);
}



/// <summary>
/// Handles the messages for the owner drawn message box.
/// This routine paints the box through the owner draw system and tears it down with
/// whichever of the buttons the player pressed.
/// </summary>
/// <returns>Returns with TRUE if the message was dealt with here, FALSE otherwise.</returns>
int CALLBACK ODMessageBox_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message) {
		case WM_ERASEBKGND: // RA2: the cases come in this order in the executable
			return(TRUE);

		case WM_PAINT:
			Dialog_On_WM_PAINT(window); // RA2: the dialog is painted by the owner draw paint (0x005FEE70), not OwnerDraw::Draw_Dialog_Back
			Slide_Pending_Dialog(window); // RA2: a dialog waiting to slide in does so now (0x005E5F20)
			ValidateRect(window, NULL);
			return(TRUE);

		case WM_DRAWITEM:
			Dialog_On_WM_DRAWITEM(lparam); // RA2: as above
			return(TRUE);

		case WM_COMMAND:
			switch (LOWORD(wparam)) {
				case IDOK:
				case IDCANCEL:
				case IDYES:
				case IDNO:
					WS_Destroy_Dialog(window, LOWORD(wparam));
					return(TRUE);
			}
			break;
	}
	return(FALSE);
}


/// <summary>
/// Shows the name of a game mode in a lobby dialog.
/// The name is put into the game mode text control of the dialog.
/// </summary>
/// <param name="dialog">The lobby dialog.</param>
/// <param name="mode">The game mode whose name is shown.</param>
void __fastcall Set_Item_Text(HWND dialog, MultiplayerGameMode * mode) // RA2: 0x005C3450, TibSun has no such routine
{
	HWND item = GetDlgItem(dialog, 0x6ec); // RA2: as above
	SendMessage(item, OD_SETTEXTW, 0, (LPARAM)mode->UIName.text()); // RA2: as above
}

/// <summary>
/// Shows a scenario in a lobby dialog.
/// The scenario at the given index of the session's scenario list is handed to the scenario text
/// control of the dialog.
/// </summary>
/// <param name="dialog">The lobby dialog.</param>
/// <param name="index">Index of the scenario in the session's list.</param>
void __fastcall Set_Scenario_Item_Text(HWND dialog, int index) // RA2: 0x005C3480, TibSun has no such routine
{
	MultiMission * map = Session.Scenarios[index]; // RA2: as above
	HWND item = GetDlgItem(dialog, 0x5a8); // RA2: as above
	SendMessage(item, OD_SETTEXTW, 0, (LPARAM)map); // RA2: as above
}

/// <summary>
/// Shows the description of the chosen scenario in a lobby dialog.
/// </summary>
/// <param name="dialog">The lobby dialog.</param>
void __fastcall Set_Skirmish_Dialog_Text(HWND dialog) // RA2: 0x005C34B0, TibSun has no such routine
{
	HWND item = GetDlgItem(dialog, 0x5a8); // RA2: as above
	SendMessage(item, OD_SETTEXTW, 0, (LPARAM)Session.Options.ScenarioDescription); // RA2: as above
}

/// <summary>
/// Fetches the game mode the session options name.
/// The first game mode of the list is used when the options name none that exists.
/// </summary>
/// <returns>Returns with the current game mode.</returns>
MultiplayerGameMode * Get_Current_Game_Mode(void) // RA2: 0x005C34D0, TibSun has no such routine
{
	MultiplayerGameMode * mode = MultiplayerGameMode::Get(Session.Options.MPModeIndex); // RA2: as above
	if (mode == NULL) { // RA2: as above
		mode = First_Game_Mode(); // RA2: as above
	}
	return(mode); // RA2: as above
}

/// <summary>
/// Passes a change of the game mode on to the game mode itself.
/// In the game mode dialog the mode picked in its list is told; elsewhere the session's own mode is.
/// </summary>
void __fastcall Handle_Mode_Selection(void) // RA2: 0x005C34F0, TibSun has no such routine; slot 31 (offset 0x7C) of the game mode is called
{
	if (WS_Top_Window_ID() == 0x6b) { // RA2: as above
		HWND dialog = WS_Top_Window(); // RA2: as above
		int selected = SendDlgItemMessage(dialog, 0x6eb, LB_GETCURSEL, 0, 0); // RA2: as above
		if (selected != -1) { // RA2: as above
			MultiplayerGameMode * mode = (MultiplayerGameMode *)SendDlgItemMessage(dialog, 0x6eb, LB_GETITEMDATA, selected, 0); // RA2: as above
			if (mode != NULL) { // RA2: as above
				mode->vt_entry_7C(); // RA2: as above
			}
		}
	} else if (Session.MPGameMode != NULL) { // RA2: as above
		Session.MPGameMode->vt_entry_7C(); // RA2: as above
	}
}







/// <summary>
/// Displays the current game options in the setup dialog.
/// This routine pushes the session options out to the sliders and check boxes. On the
/// initializing pass it also establishes the slider ranges and greys out whichever options a
/// World Domination Tour territory refuses to let the players meddle with.
/// </summary>
/// <param name="window">The game options dialog to update.</param>
/// <param name="initialize">Is this the first call for a freshly created dialog?</param>
void __fastcall Fill_Options_Dialog(HWND window, int initialize) // RA2: 0x005C3550, RA2 has the options of its own: only the credits, unit count, short game, MCV and crates controls are limited by a tour territory
{
	int unit_count = Session.Options.UnitCount; // RA2: as above
	int credits = Session.Options.Credits; // RA2: as above
	int game_speed = Session.Options.GameSpeed; // RA2: as above

	if (initialize) {
		rc_ptr<WDTTerritory> territory; // RA2: as above
		if (Session.Type == GAME_INTERNET && Session.TournamentState == 2) { // RA2: as above
			territory = Find_WDT_Territory(Session.TournamentTerritory); // RA2: as above
		}
		if (territory.Get() != NULL) { // RA2: as above
			EnableWindow(GetDlgItem(window, IDC_YOURSIDE), FALSE);
			EnableWindow(GetDlgItem(window, 0x6d4), FALSE); // RA2: as above
			if (!(territory->UserModBooleans & 0x20000)) { // RA2: as above
				EnableWindow(GetDlgItem(window, IDC_CREDITS), FALSE);
			}
			if (!(territory->UserModBooleans & 0x8000)) { // RA2: as above
				EnableWindow(GetDlgItem(window, IDC_UNITCOUNT), FALSE);
			}
			if (!(territory->UserModBooleans & 0x400)) { // RA2: as above
					EnableWindow(GetDlgItem(window, IDC_SHORT_GAME), FALSE); /// Short Game
			}
			if (!(territory->UserModBooleans & 0x20)) { // RA2: as above
					EnableWindow(GetDlgItem(window, IDC_REDEPLOY_MCV), FALSE); /// Re-Deployable MCV
			}
			if (!(territory->UserModBooleans & 0x100)) { // RA2: as above
				EnableWindow(GetDlgItem(window, IDC_CRATES), FALSE);
			}
		}
		SendDlgItemMessage(window, IDC_GAME_SPEED_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(0, 6)); /// Game Speed
		SendDlgItemMessage(window, IDC_CREDITS, TBM_SETRANGE, TRUE, MAKELONG(Rule->MinMoney, Rule->MPMaxMoney)); // RA2: as above
		SendDlgItemMessage(window, IDC_CREDITS, OD_SETTRACKSTEP, 0, Rule->MoneyIncrement); // RA2: as above
		SendDlgItemMessage(window, IDC_UNITCOUNT, TBM_SETRANGE, TRUE, MAKELONG(Rule->MinUnitCount, Rule->MaxUnitCount)); // RA2: as above
	}

	SendDlgItemMessage(window, IDC_GAME_SPEED_SLIDER, TBM_SETPOS, TRUE, 6 - game_speed); // RA2: as above
	SendDlgItemMessage(window, IDC_CREDITS, TBM_SETPOS, TRUE, credits); // RA2: as above
	SendDlgItemMessage(window, IDC_UNITCOUNT, TBM_SETPOS, TRUE, unit_count); // RA2: as above

	int button_state[] = { BST_UNCHECKED, BST_CHECKED };
	SendDlgItemMessage(window, IDC_SHORT_GAME, BM_SETCHECK, button_state[Session.Options.ShortGame], 0);
	SendDlgItemMessage(window, 0x69a, BM_SETCHECK, button_state[Session.Options.SWAllowed], 0); // RA2: as above
	SendDlgItemMessage(window, 0x69d, BM_SETCHECK, button_state[Session.Options.BuildOffAlly], 0); // RA2: as above
	SendDlgItemMessage(window, IDC_REDEPLOY_MCV, BM_SETCHECK, button_state[Session.Options.MCVRedeploy], 0);
	SendDlgItemMessage(window, IDC_CRATES, BM_SETCHECK, button_state[Session.Options.Goodies], 0);
}



void Encode_Game_Options(char *out);
void Net2EncodeGameopt(char *out);


/// <summary>
/// Broadcasts the host's game options if they need broadcasting.
/// This routine is called from the game setup dialog's idle loop. It only puts a packet on
/// the wire when an option has actually changed or when the caller insists, so that dragging
/// a slider about does not flood the other players.
/// </summary>
/// <param name="force">Should the options be sent even though nothing has changed?</param>
/// <param name="now">Should the send happen immediately rather than waiting for the next
/// opportunity?</param>
void __fastcall Send_Lobby_Options(bool force, bool now) // RA2: 0x005C3810, RA2 sends fewer options than TibSun (no tech level, AI players, difficulty, allies, truce, flag, fog or bases)
{
	static int _last_pump_time; // RA2: as above
	static bool _need_to_pump; // RA2: as above
	static bool _last_short_game; // RA2: as above
	static bool _last_sw_allowed; // RA2: as above
	static bool _last_build_off_ally; // RA2: as above
	static bool _last_mcv_redeploy; // RA2: as above
	static bool _last_crates; // RA2: as above
	static int _last_game_speed = -1;
	static int _last_credits = -1;
	static int _last_unit_count = -1;
	static bool _last_scenario_is_official = true;
	static unsigned int _last_scenario_file_length = -1;
	static wchar_t _last_scenario_description[45]; // RA2: as above
	static char _last_scenario_file_name[14];
	static char _last_scenario_digest[33];

	bool do_pump = false;
	bool it_is_time = false;

	/// if we didn't pump this second, we can consider pumping
	if (time(NULL) - _last_pump_time != 0) {
		it_is_time = true;
	}

	/// if it isn't time to pump, but we were told to pump, remember to do
	/// so when time comes, regardless of if anything changed
	if (!it_is_time) {
		if (force) {
			_need_to_pump = true;
		}
	}

	/// if it is time to pump, or we were told to pump, do so
	if (it_is_time || now) {
		if (_need_to_pump) {
			force = true;
		}
		_need_to_pump = false;

		if (g_pChat != NULL) {
			if (time(NULL) - _last_pump_time >= 5) {
				do_pump = true;
			}
		} else {
			if (time(NULL) - _last_pump_time >= 2) {
				do_pump = true;
			}
		}

		if (force) {
			do_pump = true;
		}

		if (_last_game_speed != Session.Options.GameSpeed) do_pump = true;
		if (_last_credits != Session.Options.Credits) do_pump = true;
		if (_last_unit_count != Session.Options.UnitCount) do_pump = true;
		if (_last_short_game != Session.Options.ShortGame) do_pump = true;
		if (_last_sw_allowed != Session.Options.SWAllowed) do_pump = true; // RA2: as above
		if (_last_build_off_ally != Session.Options.BuildOffAlly) do_pump = true; // RA2: as above
		if (_last_mcv_redeploy != Session.Options.MCVRedeploy) do_pump = true;
		if (_last_crates != Session.Options.Goodies) do_pump = true; // RA2: as above

		_last_scenario_description[43] = 0;
		if (wcsicmp(_last_scenario_description, Session.Options.ScenarioDescription)) do_pump = true; // RA2: as above

		_last_scenario_file_name[13] = 0;
		if (stricmp(_last_scenario_file_name, Session.ScenarioFileName)) do_pump = true;

		_last_scenario_digest[32] = 0;
		if (stricmp(_last_scenario_digest, Session.ScenarioDigest)) do_pump = true;

		if (_last_scenario_file_length != Session.ScenarioFileLength) do_pump = true;
		if (_last_scenario_is_official != Session.ScenarioIsOfficial) do_pump = true;

		if (do_pump) {
			_last_pump_time = time(0);
			_last_game_speed = Session.Options.GameSpeed;
			_last_credits = Session.Options.Credits;
			_last_unit_count = Session.Options.UnitCount;
			_last_short_game = Session.Options.ShortGame;
			_last_sw_allowed = Session.Options.SWAllowed; // RA2: as above
			_last_build_off_ally = Session.Options.BuildOffAlly; // RA2: as above
			_last_mcv_redeploy = Session.Options.MCVRedeploy;
			_last_crates = Session.Options.Goodies; // RA2: as above
			wcscpy(_last_scenario_description, Session.Options.ScenarioDescription); // RA2: as above
			strcpy(_last_scenario_digest, Session.ScenarioDigest);
			strcpy(_last_scenario_file_name, Session.ScenarioFileName);
			_last_scenario_file_length = Session.ScenarioFileLength;
			_last_scenario_is_official = Session.ScenarioIsOfficial;

			char buffer[520]; // RA2: as above
			memset(buffer, '\0', sizeof(buffer));

			if (g_pChat != NULL) {
				Encode_Game_Options(buffer);
				Send_Host_Option(buffer); // RA2: as above
			} else {
				Net2EncodeGameopt(buffer);
				Send_Host_Option(buffer); // RA2: as above
			}
		}
	}
}


/// <summary>
/// Reminds the guests that the host is about to start the game.
/// A reminder packet goes to every other player of the lobby over the network layer.
/// </summary>
void Send_Start_Reminder(void) // RA2: 0x005C3AE0, TibSun has no such routine
{
	GlobalPacketType packet;
	memset(&packet, 0, sizeof(packet));
	packet.Command = NET_COMMAND_28; // RA2: as above
	strcpy(packet.Name, Session.Handle);
	int count = Session.Players.Count(); // RA2: as above
	for (int i = 1; i < count; i++) { // RA2: as above
		Ipx.Send_Global_Message(&packet, sizeof(packet), 1, &Session.Players[i]->Address, 0, 0); // RA2: as above
		Call_Back();
	}
}

/// <summary>
/// Sends the start reminder through whichever layer the game is played over.
/// </summary>
void __fastcall Dispatch_Game_Mode(void) // RA2: 0x005C3B70, TibSun has no such routine
{
	switch (Session.Type) {
		case GAME_IPX:
			Send_Start_Reminder(); // RA2: as above
			break;

		case GAME_INTERNET:
			Wol_Send_Start_Reminder(); // RA2: as above
			break;
	}
}

/// <summary>
/// Handles the host's start reminder in the guest's lobby.
/// The guest is marked as reminded and told, through the chat service or the network layer.
/// </summary>
void __fastcall Handle_Start_Reminder(void) // RA2: 0x005C3C20, TibSun has no such routine
{
	Session.IsStartReminded = true; // RA2: as above
	if (g_pChat != NULL) {
		Wol_Handle_Start_Reminder(); // RA2: as above
	} else {
		Join_Lobby_Start_Reminder(); // RA2: as above
	}
}

/// <summary>
/// Handles the host taking the start reminder back in the guest's lobby.
/// The reminded mark is cleared and the ready button is reset, through the chat service or the network layer.
/// </summary>
void __fastcall Handle_Reset_Ready(void) // RA2: 0x005C3C40, TibSun has no such routine
{
	Session.IsStartReminded = false; // RA2: as above
	if (g_pChat != NULL) {
		Wol_Reset_Ready(); // RA2: as above
	} else {
		Join_Lobby_Reset_Ready(); // RA2: as above
	}
}






/// <summary>
/// Sends an encoded game options string to every player.
/// This routine hides the difference between the internet chat service and the direct
/// network layer, so the game options only have to be encoded once.
/// </summary>
/// <param name="options">The encoded option string to send.</param>
void __fastcall Send_Host_Option(char * options) // RA2: 0x005C3C60, the options go out as a plain string and the player color is the preferred one; main names it Send_Host_Option (netdlg2.cpp)
{
	if (g_pChat != NULL) {
		char buffer[0x400]; // RA2: as above
		strncpy(buffer, options, 0x3ff); // RA2: as above
		g_pChat->RequestPublicGameOptions((LPCSTR)buffer); // RA2: as above
	} else {
		GlobalPacketType packet;
		memset(&packet, 0, sizeof(packet));
		packet.Command = NET_PUB_GAMEOPT;
		strcpy(packet.Name, Session.Handle);
		strcpy(packet.Options.Buf, options);
		packet.Options.Color = Session.PrefColor; // RA2: as above
		packet.Options.NameCRC = Compute_Name_CRC(Session.GameName);
		for (int i = 1; i < Session.Players.Count(); i++) {
			NodeNameType * node = Session.Players[i]; // RA2: as above
			DebugString("Sending public game options to %s\n", Wide_To_Narrow(node->Name)); // RA2: as above
			Ipx.Send_Global_Message(&packet, sizeof(packet), 1, &node->Address, 0, 0); // RA2: as above
			Call_Back();
		}
	}
}



/// <summary>
/// Sends an encoded game options string to one player.
/// This routine hides the difference between the internet chat service and the direct
/// network layer, so the caller only has to name the player who is to receive the options.
/// </summary>
/// <param name="player">Name of the player to send the options to.</param>
/// <param name="options">The encoded option string to send.</param>
void __fastcall Send_Player_Choice(char const * player, char const * options) // RA2: 0x005C3D90, the packet carries the chosen color and the wide host name; main names it Send_Player_Choice (netdlg2.cpp)
{
	if (g_pChat != NULL) {
		User user;
		memset(&user, '\0', sizeof(user));
		strcpy((char *)user.name, player);
		g_pChat->RequestPrivateGameOptions(&user, (LPCSTR)options);
	} else {
		GlobalPacketType packet;
		memset(&packet, 0, sizeof(packet));
		packet.Command = NET_PRIV_GAMEOPT; // RA2: as above
		strcpy(packet.Name, Session.Handle);
		strcpy(packet.Options.Buf, options);
		Session.GPacket.Options.Color = Session.Chosen_Color(); // RA2: as above
		Session.GPacket.Options.NameCRC = Compute_Name_CRC(Session.GameName);
		for (int i = 1; i < Session.Players.Count(); i++) {
			if (stricmp(Wide_To_Narrow(Session.Players[i]->Name), player) == 0) { // RA2: as above
				DebugString("Sending private game options to %ls\n", Session.Players[i]->Name); // RA2: as above
				Ipx.Send_Global_Message(&packet, sizeof(packet), 1, &Session.Players[i]->Address, 0, 0); // RA2: as above
				Call_Back();
			}
		}
	}
}


/// <summary>
/// Records whether a player has the host's map.
/// The state is kept with the player; when a player lacks the map or is being sent it, the lobby
/// chat says so. The host always has the map.
/// </summary>
/// <param name="name">Name of the player, or NULL for the local player.</param>
/// <param name="state">The map state: 0 for none, 1 for present, 2 for being transferred.</param>
void __fastcall Set_Player_Map_State(char const * name, int state) // RA2: 0x005C3F00, TibSun has no such routine
{
	if (name == NULL) { // RA2: as above
		name = Session.Handle; // RA2: as above
	}
	if (wcscmp(Narrow_To_Wide(name), Session.GameName) == 0) { // RA2: as above
		state = 1; // RA2: as above
	}
	int found = -1;
	for (int i = 0; i < Session.Players.Count(); i++) { // RA2: as above
		if (stricmp(Wide_To_Narrow(Session.Players[i]->Name), name) == 0) { // RA2: as above
			found = i; // RA2: as above
			break;
		}
	}
	if (found != -1) { // RA2: as above
		Session.Players[found]->Slot.Unknown7F = state; // RA2: as above
		if (state == 0) { // RA2: as above
			Lobby_Chat_Printf(-1, StringTable::LoadString("GUI:HostNoMap", NULL, "D:\\Ra2\\netshare.cpp", 1115), Session.Players[found]->Name, Session.Options.ScenarioDescription); // RA2: as above
		} else if (state == 2) { // RA2: as above
			Lobby_Chat_Printf(-1, StringTable::LoadString("GUI:HostMapTransfer", NULL, "D:\\Ra2\\netshare.cpp", 1119), Session.Players[found]->Name, Session.Options.ScenarioDescription); // RA2: as above
		}
	}
}



/// <summary>
/// Decodes a public game options string sent by the host.
/// This routine applies the host's settings to the local session and refreshes the game
/// options dialog to suit. When something material has changed, everybody's accept flag is
/// cleared so that the players have to agree to the new terms. A player whose preferred
/// color has been taken is quietly moved to the one the host assigned.
/// </summary>
/// <param name="options">The encoded option string as received.</param>
/// <param name="name">Name of the player the options came from.</param>
/// <returns>bool; Did the scenario change?</returns>
bool DecodePubGameopt(char * options, char * name)
{
	static int _last_unit_count = -1;
	static int _last_tech_level = -1;
	static int _last_credits = -1;
	static int _last_ai_players = -1;
	static int _last_ai_difficulty = 1;
	static int _last_game_speed = -1;
	static bool _last_mcv_redeploy = false;
	static bool _last_allies_allowed = false;
	static bool _last_harvester_truce = false;
	static bool _last_capture_the_flag = false;
	static bool _last_fog_of_war = false;
	static bool _last_bases = false;
	static bool _last_bridge_destruction = false;
	static bool _last_goodies = false;
	static bool _last_short_game = false;
	static bool _last_crap_engineers = false;

	bool do_decode = false;

	char *string = strdup(options);
	char *token = string;
	if (token[0] == 'A') {
		int status = atol(&token[1]);
		if (g_pChat != NULL) {
			SetPlayerAccepted(name, status);
		} else {
			Net2SetAccept(name, status);
		}
		return(false);
	}

	SendDlgItemMessage(GameoptWindow(), IDC_USERS, OD_DISABLEPAINT, 0, 1);
	DebugString("Decoding game options %s\n", options);

	token = strtok(token, ",");
	if (token != NULL) {
		Session.Options.UnitCount = atol(token);
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		BuildLevel = atol(token);
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.Credits = atol(token);
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.FogOfWar = atol(token) != false;
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.BridgeDestruction = atoi(token) != false;
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.Goodies = atoi(token) != false;
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.MCVRedeploy = atoi(token) != false;
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.AlliesAllowed = atoi(token) != false;
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.HarvTruce = atoi(token) != false;
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.Bases = atoi(token) != false;
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.CTF = atoi(token) != false;
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Seed = atol(token);
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Options.GameSpeed = atol(token);
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.AIPlayers = atol(token);
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.AIDifficulty = (DiffType)atol(token);
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.ShortGame = atol(token) != false;;
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.CrapEngineers = atol(token) != false;;
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		Session.Options.GameSpeed = atol(token);
	}

	bool same_scenario = true;
	char * scenario_description = strtok(NULL, ",");

	token = strtok(NULL, ",");
	bool old_official = Session.ScenarioIsOfficial;
	if (token != NULL) {
		Session.ScenarioIsOfficial = atoi(token) != false;;
	}

	token = strtok(NULL, ",");
	unsigned int old_scenario_file_length = Session.ScenarioFileLength;
	if (token != NULL) {
		Session.ScenarioFileLength = atoi(token);
	}

	token = strtok(NULL, ",");
	if (token != NULL) {
		if (stricmp(Session.ScenarioFileName, token) != 0) {
			same_scenario = false;
		}
		strncpy(Session.ScenarioFileName, token, sizeof(Session.ScenarioFileName));
		strcpy(Scen->ScenarioName, Session.ScenarioFileName);
	}

	char * digest = strtok(NULL, ":");
	if (digest != NULL) {
		if (strcmp(Session.ScenarioDigest, digest) != 0) {
			same_scenario = false;
		}
		strncpy(Session.ScenarioDigest, digest, sizeof(Session.ScenarioDigest)-1);
	}

	if (!same_scenario || wcslen(Session.Options.ScenarioDescription) == 0) { // RA2: the scenario description is wide
		same_scenario = false;
		bool found = false;
		for (int i = 0; i < Session.Scenarios.Count(); i++) {
			MultiMission * scenario = Session.Scenarios[i];
			if (stricmp(Session.ScenarioFileName, scenario->Get_Filename()) == 0 && stricmp(Session.ScenarioFileName, RANDOM_MAP_FILE_NAME) != 0) {
				wcscpy(Session.Options.ScenarioDescription, scenario->Description()); // RA2: the scenario description is wide
				found = true;
				break;
			}
		}
		if (!found && scenario_description != NULL) {
			wcscpy(Session.Options.ScenarioDescription, Narrow_To_Wide(scenario_description)); // RA2: the scenario description is wide
		}
		if (stricmp(Session.ScenarioFileName, RANDOM_MAP_FILE_NAME) == 0) {
			wcscpy(Session.Options.ScenarioDescription, Narrow_To_Wide(Fetch_String(TXT_RANDOM_MAP_DESCRIPTION))); // RA2: the scenario description is wide
		}
	}

	SendDlgItemMessage(GameoptWindow(), IDC_SCENARIONAME, WM_SETTEXT, 0, (LPARAM)Session.Options.ScenarioDescription);

	Scen->Scenario = -1;
	Frame = 0;
	Session.CommProtocol = DEFAULT_COMM_PROTOCOL;

	if (old_official != Session.ScenarioIsOfficial || old_scenario_file_length != Session.ScenarioFileLength) {
		same_scenario = false;
	}

	if (!same_scenario) {
		DebugString("Not same scenario...");
		Update_Network_Dialog_Preview(GameoptWindow());
	}

	if (digest == NULL) {
		Lobby_Chat_Printf(-1, L"Gameopt parse failed"); // RA2: the chat lines are wide
	}

	for (int i = 0; i < MAX_PLAYERS; i++) {
		token = strtok(NULL, ",:");
		if (token == NULL) break;
		char * handle = token;

		token = strtok(NULL, ",:");
		if (token == NULL) break;
		int house = atol(token);

		token = strtok(NULL, ",:");
		if (token == NULL) break;
		int color = atol(token);

		if (stricmp(handle, Session.Handle) == 0 && color != Session.PrefColor && !IsColorChangePending) {
			Lobby_Chat_Printf(-1, Narrow_To_Wide(Fetch_String(TXT_COLOR_IN_USE))); // RA2: the chat lines are wide
			Session.PrefColor = color;
		}

		if (stricmp(handle, Session.Handle) == 0) {
			IsColorChangePending = false;
		}

		if (g_pChat != NULL && Assign_House_And_Color(handle, house, color) ||
			g_pChat == NULL && Net2SetHouseAndColor(handle, house, color)) {

			do_decode = true;
		}
	}

	free(string);

	Fill_Options_Dialog(GameoptWindow(), false); // RA2: main names the function at 0x005C3550 Fill_Options_Dialog

	if (_last_unit_count != Session.Options.UnitCount) do_decode = true;
	if (_last_tech_level != BuildLevel) do_decode = true;
	if (_last_credits != Session.Options.Credits) do_decode = true;
	if (_last_ai_players != Session.Options.AIPlayers) do_decode = true;
	if (_last_ai_difficulty != Session.Options.AIDifficulty) do_decode = true;
	if (_last_bases != Session.Options.Bases) do_decode = true;
	if (_last_bridge_destruction != Session.Options.BridgeDestruction) do_decode = true;
	if (_last_goodies != Session.Options.Goodies) do_decode = true;
	if (_last_mcv_redeploy != Session.Options.MCVRedeploy) do_decode = true;
	if (_last_allies_allowed != Session.Options.AlliesAllowed) do_decode = true;
	if (_last_harvester_truce != Session.Options.HarvTruce) do_decode = true;
	if (_last_capture_the_flag != Session.Options.CTF) do_decode = true;
	if (_last_fog_of_war != Session.Options.FogOfWar) do_decode = true;
	if (_last_short_game != Session.Options.ShortGame) do_decode = true;
	if (_last_game_speed != Session.Options.GameSpeed) do_decode = true;
	if (_last_crap_engineers != Session.Options.CrapEngineers) do_decode = true;

	if (!same_scenario || do_decode) {
		int accept;
		if (g_pChat != NULL) {
			accept = GetPlayerAccepted(NULL);
		} else {
			accept = Net2GetAccept(NULL);
		}

		if (accept == 1) {
			if (g_pChat != NULL) {
				SetPlayerAccepted(NULL, 0);
			} else {
				Net2SetAccept(NULL, 0);
			}

			Lobby_Chat_Printf(-1, Narrow_To_Wide(Fetch_String(TXT_HOST_CHANGED_OPTIONS))); // RA2: the chat lines are wide

			char buffer[64];
			sprintf(buffer, "A0");
			Send_Host_Option(buffer); // RA2: main names the function at 0x005C3C60 Send_Host_Option

			EnableWindow(GetDlgItem(GameoptWindow(), IDC_ACCEPT), TRUE);
			InvalidateRect(GetDlgItem(GameoptWindow(), IDC_ACCEPT), NULL, FALSE);
		} else {
			if (!IsWindowEnabled(GetDlgItem(GameoptWindow(), IDC_ACCEPT))) {
				EnableWindow(GetDlgItem(GameoptWindow(), IDC_ACCEPT), TRUE);
				InvalidateRect(GetDlgItem(GameoptWindow(), IDC_ACCEPT), NULL, FALSE);
			}
		}
	}

	SendDlgItemMessage(GameoptWindow(), IDC_USERS, OD_DISABLEPAINT, 0, 0);

	if (g_pChat != NULL) {
		Display_Users();
	} else {
		Net2DisplayUsers();
	}

	_last_unit_count = Session.Options.UnitCount;
	_last_tech_level = BuildLevel;
	_last_credits = Session.Options.Credits;
	_last_ai_players = Session.Options.AIPlayers;
	_last_ai_difficulty = Session.Options.AIDifficulty;
	_last_allies_allowed = Session.Options.AlliesAllowed;
	_last_bridge_destruction = Session.Options.BridgeDestruction;
	_last_fog_of_war = Session.Options.FogOfWar;
	_last_goodies = Session.Options.Goodies;
	_last_bases = Session.Options.Bases;
	_last_crap_engineers = Session.Options.CrapEngineers;
	_last_harvester_truce = Session.Options.HarvTruce;
	_last_mcv_redeploy = Session.Options.MCVRedeploy;
	_last_short_game = Session.Options.ShortGame;
	_last_game_speed = Session.Options.GameSpeed;
	_last_capture_the_flag = Session.Options.CTF;

	if (!same_scenario) {
		return(true);
	}
	return(false);
}


/// <summary>
/// Saves the current selection of a list box.
/// The multiplayer dialogs rebuild their list boxes from scratch whenever the game state
/// changes. Call this routine first so that whatever the player had highlighted can be put
/// back afterwards.
/// </summary>
/// <param name="listbox">The list box to record the selection of.</param>
/// <param name="lbdict">The dictionary to record the selected entries into.</param>
void LBSaveSelections(HWND listbox, Dictionary<Wstring,bool> & lbdict)
{
	int count = ListBox_GetCount(listbox);
	char buffer[128];
	Wstring key;

	if (count) {
		int style = GetWindowLong(listbox, GWL_STYLE);
		if (style & LBS_MULTIPLESEL) {
			for (int i = 0; i < count; i++) {
				if (ListBox_GetSel(listbox, i) != 0) {
					ListBox_GetText(listbox, i, buffer);
					key = buffer;
					bool value = true;
					lbdict.add(key, value);
				}
			}
		} else if (!(style & LBS_NOSEL)) {
			int index = ListBox_GetCurSel(listbox);
			if (index >= 0) {
				buffer[0] = '\0';
				ListBox_GetText(listbox, index, buffer);
				key = buffer;
				bool value = true;
				lbdict.add(key, value);
			}
		}
	}
}


/// <summary>
/// Restores a list box selection that was saved earlier.
/// This routine is the other half of LBSaveSelections. Call it once the list box has been
/// refilled to put the player's highlight back where it was.
/// </summary>
/// <param name="listbox">The list box to restore the selection within.</param>
/// <param name="lbdict">The dictionary the selection was saved into.</param>
void LBRestoreSelections(HWND listbox, Dictionary<Wstring,bool> & lbdict)
{
	int count = ListBox_GetCount(listbox);
	Wstring key;

	if (count) {
		int style = GetWindowLong(listbox, GWL_STYLE);
		if (style & LBS_MULTIPLESEL) {
			for (int i = 0; i < count; i++) {
				char buffer[128];
				ListBox_GetText(listbox, i, buffer);
				key = buffer;
				if (lbdict.contains(key)) {
					ListBox_SetSel(listbox, TRUE, i);
				}
			}
		} else if (!(style & LBS_NOSEL)) {
			bool value;
			if (lbdict.removeAny(key, value)) {
				char buffer[128];
				strcpy(buffer, key.get());
				int index = ListBox_FindStringExact(listbox, -1, buffer);
				if (index != LB_ERR) {
					ListBox_SetCurSel(listbox, index);
				}
			}
		}
	}
}


/// <summary>
/// Fetches the number of starting positions a scenario offers.
/// This routine is used to check that there is somewhere to put every player before a map is
/// allowed into a multiplayer game.
/// </summary>
/// <param name="index">Index into the multiplayer scenario list.</param>
/// <returns>Returns with the number of start positions on the map. Zero is returned if the
/// scenario could not be read.</returns>
int RandomMapWaypointCount(int index)
{
	char wp[32];
	char buf[132];
	if (index >= 0 && index < Session.Scenarios.Count()) {
		strcpy(buf, Session.Scenarios[index]->Get_Filename());
		CCFileClass file(buf);

		INIClass ini;
		if (!ini.Load(file)) {
			return(0);
		}

		int count = 0;

		for (int i = 0; i < MAX_PLAYERS; i++) {
			sprintf(wp, "%d", i);
			if (ini.Get_Int("Waypoints", wp, -1) != -1) {
				count++;
			}
		}

		if (count == 0) {
			count = ini.Get_Int("RandomMap", "NumPlayers", 0);
		}

		return(count);
	}
	return(0);
}


static int LastPreviewedScenario;
static int OriginalScenario;
static HWND ScenarioPick;


/// <summary>
/// Handles the idle processing while the map selection dialog is up.
/// This routine keeps the preview in step with whichever map is highlighted and pumps the
/// network layer the session is using, so that a game sitting in the lobby does not stall
/// while the host browses for a scenario.
/// </summary>
/// <returns>bool; Should the dialog be shut down?</returns>
bool Scenario_Select_Callback(void)
{
	int index = SendDlgItemMessage(ScenarioPick, IDC_AILEVEL_SLIDER, LB_GETCURSEL, 0, 0);
	if (index != LastPreviewedScenario && index != -1) {
		Set_Scenario_Info_From_Index(index);
		if (stricmp(Session.Scenarios[index]->Get_Filename(), "RandMap.Sed") == 0) {
			delete MultiplayerMapPreview;
			MultiplayerMapPreview = new MapPreviewClass;
			MultiplayerMapPreview->Read_PCX_Preview("RandMap.img");
			if (MultiplayerMapPreview->Get_Preview_Surface() == NULL) {
				Update_Network_Dialog_Preview(ScenarioPick);
			}
			InvalidateRect(ScenarioPick, NULL, FALSE);
		} else {
			Update_Network_Dialog_Preview(ScenarioPick);
		}
		LastPreviewedScenario = index;
		Session.Options.ScenarioIndex = OriginalScenario;
		Set_Scenario_Info_From_Index(OriginalScenario);
	}
	if (Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) {
		return(Net2Callback());
	}
	if (Session.Type == GAME_MODEM || Session.Type == GAME_NULL_MODEM) {
		return(ModemHostCallback());
	}
	Call_Back();
	return(false);
}

int CALLBACK Scenario_DlgProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);


/// <summary>
/// Brings up the multiplayer map selection dialog.
/// Use this routine to let the host choose the scenario for the game. The dialog does not
/// return until the player settles on a map or backs out.
/// </summary>
/// <param name="top">The window to parent and center the dialog against.</param>
/// <returns>Returns with the control ID that dismissed the dialog, either IDOK or
/// IDCANCEL.</returns>
int Scenario_Dialog(HWND top)
{
	Hide_Mouse();
	Draw_Menu_Background();
	Show_Mouse();
	ScenarioPick = WS_Create_Dialog(ProgramInstance, IDD_MPLAYER_SELECT_MAP, top, (DLGPROC)Scenario_DlgProc, FALSE);
	Center_Window_Within_Window(ScenarioPick);
	OwnerDraw::Subclass_Dialog(ScenarioPick, 0);
	ShowWindow(ScenarioPick, SW_NORMAL);
	return(WS_Wait_Dialog(ScenarioPick, Scenario_Select_Callback));
}


/// <summary>
/// Handles the messages for the multiplayer map selection dialog.
/// This routine fills the map list, paints the preview of the highlighted map, and services
/// the random map generator button.
/// </summary>
/// <returns>Returns with TRUE if the message was dealt with here, FALSE to leave it to the
/// dialog manager.</returns>
int CALLBACK Scenario_DlgProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message) {
		case WM_NCDESTROY:
			On_WM_NCDESTROY(window);
			break;

		case WM_PAINT:
			OwnerDraw::Draw_Dialog_Back(window);
			if (MultiplayerMapPreview) {
				MultiplayerMapPreview->Blit_Preview(window);
			}
			ValidateRect(window, NULL);
			break;

		case WM_ERASEBKGND:
			return(TRUE);

		case WM_DRAWITEM:
			OwnerDraw::Draw_Item((DRAWITEMSTRUCT *)lparam);
			return(TRUE);

		case WM_COMMAND:
			switch (LOWORD(wparam)) {
				case IDC_AILEVEL_SLIDER:
					return(FALSE);

				case IDOK: {
					int index = SendDlgItemMessage(window, IDC_AILEVEL_SLIDER, LB_GETCURSEL, 0, 0);
					Session.Options.ScenarioIndex = MAX(0, index);
					WS_Destroy_Dialog(window, IDOK);
					SendDlgItemMessage(GameoptWindow(), IDC_SCENARIONAME, WM_SETTEXT, 0, (LPARAM)Session.Scenarios[Session.Options.ScenarioIndex]);
					break;
				}

				case IDCANCEL:
					WS_Destroy_Dialog(window, IDCANCEL);
					break;

				case IDC_CREATE_RANDOM_MAP: {
					ShowWindow(window, SW_HIDE);
					int scenario = CreateRandomMap();
					if (scenario != -1) {
						SendDlgItemMessage(window, IDC_AILEVEL_SLIDER, LB_RESETCONTENT, 0, 0);
						for (int i = 0; i < Session.Scenarios.Count(); i++) {
							SendDlgItemMessage(window, IDC_AILEVEL_SLIDER, LB_INSERTSTRING, -1, (LPARAM)Session.Scenarios[i]);
						}
						SendDlgItemMessage(window, IDC_AILEVEL_SLIDER, LB_SETCURSEL, scenario, 0);
						Set_Scenario_Info_From_Index(scenario);
						if (!MultiplayerMapPreview->Get_Preview_Surface()) {
							Update_Network_Dialog_Preview(window);
						}
						Session.Options.ScenarioIndex = OriginalScenario;
						Set_Scenario_Info_From_Index(OriginalScenario);
					}
					ShowWindow(window, SW_SHOW);
					break;
				}
			}
			break;

		case OD_SUBCLASSED: {
			SendDlgItemMessage(window, IDC_AILEVEL_SLIDER, LB_RESETCONTENT, 0, 0);
			for (int i = 0; i < Session.Scenarios.Count(); i++) {
				SendDlgItemMessage(window, IDC_AILEVEL_SLIDER, LB_INSERTSTRING, -1, (LPARAM)Session.Scenarios[i]);
			}
			SendDlgItemMessage(window, IDC_AILEVEL_SLIDER, LB_SETCURSEL, Session.Options.ScenarioIndex, 0);
			OriginalScenario = Session.Options.ScenarioIndex;
			LastPreviewedScenario = -1;
			break;
		}
	}
	return(FALSE);
}


/// <summary>
/// Performs the last setup step before a multiplayer game begins.
/// This routine copies the agreed session options into the globals the game logic actually
/// reads, so that every machine starts the scenario with the same rules in force.
/// </summary>
void PregameSetup(void)
{
	Scen->Scenario = Session.Options.ScenarioIndex;
	Frame = 0;
	Session.NumPlayers = Session.Players.Count();
	DebugString("Pregame setup for %d players.\n", Session.NumPlayers);
	Options.GameSpeed = Session.Options.GameSpeed;
	Session.CommProtocol = DEFAULT_COMM_PROTOCOL;
	Special.IsHarvesterImmune = Session.Options.HarvTruce;
	Special.IsDestroyBridges = Session.Options.BridgeDestruction;
	Special.IsTGrowth = true;
	Special.IsTSpread = true;
	Special.Apply_To_Game();
}


/// <summary>
/// Updates the map preview shown in a network game dialog.
/// This routine is called whenever the selected scenario changes. A guest that does not have
/// the scenario locally asks the host for a preview instead of building one, so the picture
/// may not appear until that download arrives.
/// </summary>
/// <param name="win">The dialog window that displays the preview.</param>
void Update_Network_Dialog_Preview(HWND win)
{
	delete MultiplayerMapPreview;
	MultiplayerMapPreview = NULL;

	switch (Session.Type) {
		case GAME_IPX:
			if (WS_Top_Window_ID() == IDD_MPLAYER_GUEST && !Find_Local_Scenario(Session.ScenarioFileName, Session.ScenarioFileLength, Session.ScenarioDigest, Session.ScenarioIsOfficial, false)) {
				GlobalPacketType packet;
				memset(&packet, 0, sizeof(packet));
				packet.Command = NET_REQ_PREVIEW;
				while (true) {
					if (Ipx.Send_Global_Message(&packet, sizeof(packet), 1, &Session.HostAddress)) {
						break;
					}
					Call_Back();
				}
				while (Ipx.Global_Num_Send() != 0) {
					Call_Back();
				}
				return;
			}
			break;

		case GAME_INTERNET:
			if (!Is_Channel_Owner(NULL)) {
				if (WS_Top_Window_ID() == IDD_WOL_GUEST) {
					HWND handle = GetDlgItem(WS_Top_Window(), IDC_WOLGUEST_STAT1);
					if (handle) {
						SendMessage(handle, WM_SETTEXT, 0, (LPARAM)"");
						handle = GetDlgItem(WS_Top_Window(), IDC_WOLGUEST_STAT2);
						if (handle) {
							SendMessage(handle, WM_SETTEXT, 0, (LPARAM)"");
						}
						handle = GetDlgItem(WS_Top_Window(), IDC_WOLGUEST_STAT3);
						if (handle) {
							SendMessage(handle, WM_SETTEXT, 0, (LPARAM)"");
						}
						handle = GetDlgItem(WS_Top_Window(), IDC_WOLGUEST_STAT4);
						if (handle) {
							SendMessage(handle, WM_SETTEXT, 0, (LPARAM)"");
						}
						handle = GetDlgItem(WS_Top_Window(), IDC_WOLGUEST_STAT5);
						if (handle) {
							SendMessage(handle, WM_SETTEXT, 0, (LPARAM)"");
						}
					}
				}
				bool file_available = false;
				bool is_local = false;
				if (Find_Local_Scenario(Session.ScenarioFileName, Session.ScenarioFileLength, Session.ScenarioDigest, Session.ScenarioIsOfficial, false)) {
					is_local = true;
				} else {
					file_available = true;
				}

				if (is_local) {
					RawFileClass r(Session.ScenarioFileName);
					if (r.Is_Available()) {
						file_available = true;
					}
				}

				if (file_available) {
					if (stricmp(Session.ScenarioFileName, RANDOM_MAP_FILE_NAME) != 0 && MultiplayerMapPreview != NULL) {
						delete MultiplayerMapPreview;
						MultiplayerMapPreview = NULL;
					}
					InvalidateRect(win, NULL, FALSE);
					return;
				}
				break;
			}
			break;

		case GAME_MODEM:
		case GAME_NULL_MODEM:
			if (!Find_Local_Scenario(Session.ScenarioFileName, Session.ScenarioFileLength, Session.ScenarioDigest, Session.ScenarioIsOfficial, false)) {
				SerialPacketType packet;
				memset ((void*)&packet, 0, sizeof (packet));
				packet.Command = SERIAL_REQ_PREVIEW;

				CDTimerClass<SystemTimerClass> response_timer = PACKET_SENDING_TIMEOUT;
				while (response_timer) {
					int sent = NullModem.Send_Message(&packet, sizeof(packet), 1);
					while (NullModem.Num_Send() > 0 && response_timer) {
						Call_Back();
					}
					if (sent) break;
				}
				return;
			}
			break;
	}

	bool unavailable = CCFileClass(Session.ScenarioFileName).Is_Available() == false;
	if (unavailable) {
		if (MultiplayerMapPreview != NULL) {
			delete MultiplayerMapPreview;
			MultiplayerMapPreview = NULL;
		}
		return;
	}

	if (MultiplayerMapPreview != NULL) {
		delete MultiplayerMapPreview;
		MultiplayerMapPreview = NULL;
	}
	MultiplayerMapPreview = new MapPreviewClass;
	if (MultiplayerMapPreview != NULL) {
		MultiplayerMapPreview->Read_INI_Preview(Session.ScenarioFileName);
		InvalidateRect(win, NULL, FALSE);
	}
}


/// <summary>
/// Receives the random map preview image from the host.
/// This is the guest side of the preview handshake. The host is told that this machine is
/// ready, the compressed preview file is downloaded, and the decompressed image becomes the
/// preview shown in the multiplayer dialog.
/// </summary>
void Receive_Random_Map_Preview(void)
{
	Ipx.Set_Timing(50, -1, 5000);
	DebugString("Starting map preview download\n");

	GlobalPacketType packet;
	memset(&packet, 0, sizeof(packet));
	packet.Command = NET_PREVIEW_ACK;
	DebugString("Sending preview mode acks\n");
	while (true) {
		if (Ipx.Send_Global_Message(&packet, sizeof(packet), 1, &Session.HostAddress)) {
			break;
		}
		Call_Back();
	}
	while (Ipx.Global_Num_Send() != 0) {
		Call_Back();
	}
	DebugString("Preview mode acks sent\n");

	Session.GAddress = Session.HostAddress;
	DebugString("Calling Get_File_From_Host to receive the file download\n");
	char preview_name[256];
	bool got_file = Get_File_From_Host(preview_name, GAME_IPX, false);
	if (!got_file) {
		DebugString("got_file is false. Download failed\n");
		Ipx.Set_Timing(TIMER_SECOND / 2, -1, 10 * TIMER_SECOND);
		return;
	}

	DebugString("Loading the compressed preview image\n");
	RawFileClass file(preview_name);
	int size = file.Size();
	char * buffer = new char[size];
	file.Read(buffer, size);
	int preview_size = ((int *)buffer)[0];

	DebugString("Decompressing the preview image\n");

	BufferStraw bstraw(&((int *)buffer)[1], size);
	LZOStraw lzostraw(LZOStraw::DECOMPRESS);
	lzostraw.Get_From(&bstraw);
	char * preview = new char[2 * preview_size];
	lzostraw.Get(preview, preview_size);

	DebugString("Creating the new preview surface\n");
	if (MultiplayerMapPreview) {
		delete MultiplayerMapPreview;
	}
	MultiplayerMapPreview = new MapPreviewClass;
	MultiplayerMapPreview->Create_Preview_Surface(preview);
	InvalidateRect(WS_Top_Window(), NULL, FALSE);

	DebugString("Cleaning up the temporary decompression buffers\n");
	delete [] preview;
	delete [] buffer;

	Ipx.Set_Timing(TIMER_SECOND / 2, -1, 10 * TIMER_SECOND);
}


/// <summary>
/// Sets the session's scenario information from the scenario list.
/// This routine is used whenever the host picks a different map. If the scenario file is not
/// on the hard drive, the player is asked for the disk that holds it, and the selection
/// fails if that disk cannot be made available.
/// </summary>
/// <param name="index">Index into the multiplayer scenario list, or -1 to clear the
/// selection.</param>
/// <returns>bool; Was the scenario information set?</returns>
bool Set_Scenario_Info_From_Index(int index)
{
	if (index == -1) {
		Session.ScenarioFileName[0] = '\0';
		Session.ScenarioDigest[0] = '\0';
		Session.ScenarioFileLength = 0;
		Session.ScenarioIsOfficial = 0;
		Session.Options.ScenarioDescription[0] = '\0';
		return(false);
	}

	bool unavailable = CCFileClass(Session.Scenarios[index]->Get_Filename()).Is_Available() == false;

	if (unavailable) {
		if (CD::Get_Required_Disk() != DISK_LOCAL) {
			if (!Session.Scenarios[index]->Is_On_CD(CD::Get_Current_Disk(0)) && !CD().Force_Available(Session.Scenarios[index]->On_Which_CD())) { // RA2: the current disk is asked for with a timeout
				return(false);
			}
		} else {
			WWMessageBox().Process(TXT_UNABLE_READ_SCENARIO, TXT_OK);
			return(false);
		}
	}

	wcscpy(Session.Options.ScenarioDescription, Session.Scenarios[index]->Description()); // RA2: the scenario description is wide
	strcpy(Session.ScenarioDigest, (Session.Scenarios[index])->Get_Digest());
	strcpy(Session.ScenarioFileName, (Session.Scenarios[index])->Get_Filename());
	strcpy(Scen->ScenarioName, Session.ScenarioFileName);
	Session.ScenarioIsOfficial = Session.Scenarios[index]->Get_Official();
	Session.ScenarioFileLength = CCFileClass(Session.ScenarioFileName).Size();
	Session.ScenarioFileLength = CCFileClass(Session.ScenarioFileName).Size();
	return(true);
}


/// <summary>
/// Sends the random map preview image to the guests.
/// Only a generated map needs this -- an ordinary scenario's preview is built from the map
/// file the guest already holds. The guests are put into preview receive mode and, once they
/// have all answered, the compressed image is handed to the remote file sender.
/// </summary>
/// <remarks>Only the game host should call this routine.</remarks>
void Send_Preview_To_Guests(void)
{
	CDTimerClass<SystemTimerClass> response_timer;

	if (MultiplayerMapPreview != NULL && stricmp(Session.ScenarioFileName, RANDOM_MAP_FILE_NAME) == 0 && Session.Players.Count() > 1) {
		DebugString("Starting map preview upload\n");

		GlobalPacketType packet;
		memset(&packet, 0, sizeof(packet));
		packet.Command = NET_PREVIEW_MODE;
		strcpy(packet.Name, Session.Handle);

		Ipx.Set_Timing(50, -1, 5000);
		response_timer = TIMER_SECOND * 20;

		for (int i = 1; i < Session.Players.Count(); i++) {
			Ipx.Send_Global_Message(&packet, sizeof(packet), 1, &Session.Players[i]->Address);
			Call_Back();
		}

		while (Ipx.Global_Num_Send() != 0 && response_timer) {
			Call_Back();
		}

		if (response_timer) {
			IPXAddressClass sender_address;
			bool responses[MAX_PLAYERS];
			memset(responses, 0, sizeof(responses));
			int num_responses = 0;

			response_timer = TIMER_SECOND * 30;
			DebugString("Waiting for %d players to signal ready to receive\n", Session.Players.Count() - 1);

			while (num_responses < Session.Players.Count() - 1 && response_timer) {

				Call_Back();

				GlobalPacketType response;
				int length = 455;
				unsigned short product_id;

				if (Ipx.Get_Global_Message(&response, &length, &sender_address, &product_id)) {
					if (response.Command == NET_PREVIEW_ACK) {
						for (int j = 1; j < Session.Players.Count(); j++) {
							if (sender_address == Session.Players[j]->Address) {
								if (!responses[j]) {
									responses[j] = true;
									num_responses++;
								}
								break;
							}
						}
					}
				}
			}

			if (num_responses == Session.Players.Count() - 1) {
				if (MultiplayerMapPreview != NULL) {
					DebugString("Creating low-res preview image\n");

					int size = 0;
					unsigned * preview = MultiplayerMapPreview->Create_Paletted_Preview(64, size);
					DebugString("Preview size is %d bytes\n", size);

					unsigned * buffer = new unsigned[size];
					BufferPipe bpipe(buffer, size * sizeof(unsigned));

					LZOPipe lzopipe(LZOPipe::COMPRESS);
					lzopipe.Put_To(&bpipe);
					int comp_size = lzopipe.Put(preview, size);
					comp_size += lzopipe.End();

					DebugString("Compressed preview image is %d bytes\n", comp_size);

					RawFileClass file("Preview.bin");
					if (file.Is_Available()) {
						file.Delete();
					}

					file.Open(RawFileClass::WRITE);
					file.Write(&size, sizeof(size));
					file.Write(buffer, comp_size);
					file.Close();
					delete [] buffer;

					DebugString("Calling Send_Remote_File to send the preview\n");
					Send_Remote_File("Preview.bin", GAME_IPX, true, false);
					Ipx.Set_Timing(TIMER_SECOND / 2, -1, 10 * TIMER_SECOND);
				}
			}
		}
	}
}


/// <summary>
/// Fetches a digest string for the current random map seed.
/// A generated map has no file to checksum, so this routine builds the identity string that
/// stands in for one. It is what lets the multiplayer scenario checks tell one generated map
/// from another. The map description is deliberately left out -- renaming a map does not
/// make it a different map.
/// </summary>
/// <returns>Returns with the digest string for the map.</returns>
/// <remarks>The digest is allocated by this routine. The caller must delete it.</remarks>
char * CalcRandomMapDigest(void)
{
	unsigned char * data;
	char description[sizeof(RandomMapGen.SeedData.MapDescription)];
	char * digest;
	int size;
	unsigned char *bytes;
	unsigned int val;
	unsigned int hibit;
	unsigned int checksum = 0;

	memcpy(description, RandomMapGen.SeedData.MapDescription, sizeof(description));

	/// Hash all of SeedData except UseTransitions
	data = (unsigned char *)&RandomMapGen.SeedData.Biome;
	size = (sizeof(RandomMapGen.SeedData) - offsetof(MapSeedClass, Biome)); // RA2: the digest covers everything from Biome on
	memset(RandomMapGen.SeedData.MapDescription, '\0', sizeof(description));

	while (size > 0) {
		hibit = checksum >> 31;
		if (size >= 4) {
			val = (*(unsigned int *)data);
			checksum <<= 1;
			checksum += val;
			checksum += hibit;
			data += sizeof(unsigned int);
			size -= sizeof(unsigned int);
		} else {
			val = 0;
			bytes = data;
			while (size) {
				val <<= 8;
				val |= (*bytes++);
				size--;
			}
			checksum <<= 1;
			checksum += val;
			checksum += hibit;
		}
	}

	digest = new char[12];
	sprintf(digest, "%08X", checksum);
	memcpy(RandomMapGen.SeedData.MapDescription, description, sizeof(description));
	return(digest);
}


/// <summary>
/// Creates a new random map for a multiplayer game.
/// This routine prompts the host with the random map generator, saves the resulting seed out
/// under the random map file name, and makes sure the map has an entry in the scenario list
/// so that it can be picked like any other map.
/// </summary>
/// <returns>Returns with the scenario list index of the random map. Zero is returned if the
/// player backed out of the generator.</returns>
int CreateRandomMap(void)
{
	int result;
	if (Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) {
		result = Do_Random_Map_Dialog(Net2Callback);
	} else if (Session.Type == GAME_MODEM || Session.Type == GAME_NULL_MODEM) {
		result = Do_Random_Map_Dialog(ModemHostCallback);
	} else {
		result = Do_Random_Map_Dialog(MapGen_Call_Back);
	}

	if (result != IDOK) {
		return(0);
	}

	IsRandomMap = true;
	RandomMapGen.SeedData.Save(RANDOM_MAP_FILE_NAME);

	delete MultiplayerMapPreview;
	MultiplayerMapPreview = new MapPreviewClass;
	MultiplayerMapPreview->Read_PCX_Preview("RandMap.img");

	int index;
	bool found = false;
	for (index = 0; index < Session.Scenarios.Count(); index++) {
		if (stricmp(Session.Scenarios[index]->Get_Filename(), RANDOM_MAP_FILE_NAME) == 0) {
			found = true;
			break;
		}
	}

	if (!found) {
		char * digest = CalcRandomMapDigest();
		MultiMission * scenario = new MultiMission(RANDOM_MAP_FILE_NAME, RandomMapGen.SeedData.MapDescription, digest, true, NULL, 2, 4); // RA2: the constructor takes the official flag, game modes and the player range
		Session.Scenarios.Add(scenario);
		delete digest;
	} else {
		Session.Scenarios[index]->Set_Description(RandomMapGen.SeedData.MapDescription);
	}

	return(index);
}
