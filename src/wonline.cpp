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

// RA2: the first part of wonline.cpp (0x0073C040-0x0073FEE0): the lobby helpers, the chat text senders, the channel list requests, the idle callback, the reset of the globals, the lobby, game and user lists
#include "always.h"

#include "_keyboar.h"
#include "_map.h"
#include "_rules.h"
#include "arraylist.h"
#include "dict.h" // RA2: the lobby dictionaries
#include "conquer.h"
#include "data.h"
#include "dbgprint.h"
#include "globals.h"
#include "goptions.h"
#include "ini.h" // RA2: the includes this file needs
#include "rcptr.h" // RA2: the profile pointer
#include "rules.h" // RA2: the CRCs of the rules
#include "init.h" // RA2: Title_Screen_Restore
#include "msgloop.h" // RA2: Peek_Windows_Message, MessageLoopCount
#include "ipxmgr.h"
#include "language\language.h"
#include "netshare.h"
#include "ownrdraw.h"
#include "scenario.h"
#include "srfcache.h" // RA2: the images of the game list
#include "session.h"
#include "theme.h"
#include "widewstring.h" // RA2: the lobby text is wide
#include "windlg.h"
#include "winstub.h"
#include "wolapi.h" // RA2: the includes this file needs
#include "wonline.h"
#include "wstring.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <shellapi.h>
#include <windows.h>
#undef LoadString // RA2: windows.h maps it to LoadStringA
#include "stringtable.h" // RA2: the texts are loaded from the string table by label

// RA2: the per user lobby record. It is 0x94 bytes (as TibSun's), the player's accepted state is at +0x20 (0x0073D150 stores it)
struct ChannelUserInfo
{
	char name[20];
	int color;
	int house;
	int Unknown_1C; // RA2: as above
	int accepted;
	char pad[0x70];
};

// RA2: globals and functions of the rest of wonline.cpp and of other files, as the executable's callers use them
extern ChannelUserInfo g_UserInfo[]; // RA2: 0x00B29CF0, one record per user of the channel
extern int g_RuleCRC; // RA2: the three CRCs 0x0073CFD0 xors (0x00B2AC18 to 0x00B2AC20)
extern int g_ArtCRC; // RA2: as above
extern int g_AICRC; // RA2: as above
extern ArrayList<unsigned int> g_ChanListQueue; // RA2: 0x00B29390, the channel list requests that wait for an answer
extern HANDLE g_WaitEventHandles[18]; // RA2: 0x00B264F8, EV_COUNT events
extern unsigned int g_ActiveWaitFlags; // RA2: 0x00B2ABE0, the events the wait dialog (0xC4) waits for
extern wchar_t g_LastErrorMessage[]; // RA2: 0x00B2A8EC, wide
extern int g_IsChannelCreator; // RA2: 0x00B2AC58, 1 for the host of the channel
extern bool g_GameStartRequested; // RA2: 0x00B2AC04
extern int g_MatchState; // RA2: 0x00B26898, set with the next three by Set_Match_State
extern int g_MatchTimeout; // RA2: 0x00B2AB88
extern int g_MatchResult; // RA2: 0x00B2AC2C
extern long g_MatchStartTime; // RA2: 0x00B2A8A4
extern ArrayList<User> g_UserList; // RA2: 0x00B2A728
extern Channel g_CurrentChannel; // RA2: 0x00B299C0
extern int g_AllowPage; // RA2: the lobby settings, 0x00B290A0
extern int g_AllowFind; // RA2: 0x00B2A818
extern int g_LangFilter; // RA2: 0x00B2A7E4
extern int g_LobbyMusic; // RA2: 0x00B297C8
extern int g_ShowAllChannels; // RA2: 0x00B2A634
extern int g_DisplayAsian; // RA2: 0x00B2A804, the two settings TibSun does not have
extern int g_DisplayLatin; // RA2: 0x00B2A7E8
extern int g_LanguageCode; // RA2: 0x00B2AC5C
extern int Online_Session_State; // RA2: as in ownrdraw.cpp
extern ArrayList<Wstring> g_WDTTeamLookupQueue; // RA2: 0x00B2A648, the names whose team is asked for (0x0073E100 sends them in batches)
extern ArrayList<Wstring> g_UserLocaleLookupQueue; // RA2: 0x00B29468, the names whose locale is asked for
extern unsigned g_LastWDTUserTeamRequestTime; // RA2: 0x00B28FF8
extern unsigned g_LastChannelLocaleRequestTime; // RA2: 0x00B28FF4
extern int g_NextWDTStatePollTime; // RA2: 0x00B292DC
extern int WolOptionsLocked; // RA2: 0x00B2AC40, as in netshare.cpp (2 once the host started the game)
extern bool g_StartGameReceived; // RA2: 0x00B2AC45, the start of the game was announced, Start_Game_Now runs it
extern bool g_StartGameRunning; // RA2: 0x00B2AC44, Start_Game_Now is running (it sets it first and clears it at its end)
void Start_Game_Now(void); // RA2: 0x0074F300, the executable's own debug text names it so
class PlayerProfile : public rc_object // RA2: the profile of the player, only its loader is used here
{
	public:
		static rc_ptr<PlayerProfile> Load_Default(void); // RA2: 0x00738670
};
extern rc_ptr<PlayerProfile> g_PlayerProfile; // RA2: 0x00B28CC0, set by Reset_WOL_Globals
extern ArrayList<Channel> g_UserChannels; // RA2: 0x00B26C88
extern ArrayList<Channel> g_GameChannelList; // RA2: 0x00B29990
extern Dictionary<Wstring, int> g_WDTTeams; // RA2: 0x00B294B8
extern Dictionary<Wstring, int> g_UserLocales; // RA2: 0x00B293A0
extern Dictionary<Wstring, Ladder> g_BattleClansLadderDict; // RA2: 0x00B296F8
extern Dictionary<Wstring, Ladder> g_WDTLadderDict; // RA2: 0x00B26970
extern Dictionary<Wstring, Ladder> g_GeneralLadderDict; // RA2: 0x00B2AB98
extern Dictionary<Wstring, Channel> g_BuddyChannels; // RA2: 0x00B29478, the channel a buddy is in, by name
extern ArrayList<Wstring> g_BuddyList; // RA2: 0x00B26920, the names of the buddies
extern ArrayList<Wstring> g_BuddyRequests; // RA2: 0x00B29838, the buddies whose status waits to be asked for
extern Dictionary<Wstring, unsigned int> g_BuddyStatus; // RA2: 0x00B29848, the status of a buddy by name
extern Dictionary<Wstring, Ladder> * g_ActiveLadder; // RA2: 0x00B2AC24
extern int g_InstalledLadderSKU; // RA2: 0x00B2A7F0
extern int g_SelectedLadderSKU; // RA2: 0x00B26564
extern int g_GameSKU; // RA2: 0x007FD370, 0x2100
extern Channel g_PendingChannel; // RA2: 0x00B26A60, cleared with the current channel
extern Server g_GameServer; // RA2: 0x00B28CD0
extern Server g_WDTServer; // RA2: 0x00B29880
extern int g_CurrentServerIndex; // RA2: 0x00B292D8
extern int g_TargetServerIndex; // RA2: 0x00B2A7F4
extern bool g_IgnoreNetStatus; // RA2: 0x00B2ABD8
extern bool g_ShowMOTD; // RA2: 0x007FD2FC, true at first
extern Wstring g_Motd; // RA2: 0x00B26954, the message of the day
extern bool g_PlayingNetGame; // RA2: 0x00B2ABD9, as windlg.cpp
extern bool g_ConnectionLost; // RA2: 0x00B2ABDA
extern int g_ServerType; // RA2: 0x007FD300, -1 at first
extern int g_LastChatListTime; // RA2: 0x00B2ABD0
extern int g_LastGameListTime; // RA2: 0x00B2ABD4
extern unsigned short const * g_Lobbies[8]; // RA2: 0x00B26C94, the eight lobby names, wide
extern int WDT_TERRITORY_LOBBY_INDEXES[8]; // RA2: 0x00B26540, eight indexes
extern int g_dwChatAdvise; // RA2: 0x00B2AC60
extern int g_dwNetUtilAdvise; // RA2: 0x00B2AC64
extern int g_dwDownloadAdvise; // RA2: 0x00B2AC68
extern int g_SquadInfoRequests; // RA2: 0x00B2ABF4
extern int g_RequestingOwnSquadInfo; // RA2: 0x00B2ABF8
extern int g_OwnSquadID; // RA2: 0x00B2ABFC
extern char g_OwnSquadName[41]; // RA2: 0x00B2A8B0
extern int g_ChannelCount; // RA2: 0x00B2ABE8
extern int g_LadderPos; // RA2: 0x00B29CE8
extern int g_JoinLobbyNow; // RA2: 0x00B2ABF0
extern int g_UserAge; // RA2: 0x00B2AC6C
extern int g_UserConsent; // RA2: 0x00B2AC70
extern int CurrentLevel; // RA2: 0x00B2AC38, the view of the lobby (0 is the server choice)
extern int g_Col_ChanName; // RA2: 0x00B29CCC, the column of the channel names in the lobby lists
extern int g_Col_GameName; // RA2: 0x00B2A8A8, the columns of the game list (TibSun's g_Col_GameName, g_Col_GameIcon, g_Col_GamePrivate, g_Col_GamePingTime and the observer column RA2 adds)
extern int g_Col_GameIcon; // RA2: 0x00B2A6F8
extern int g_Col_GamePrivate; // RA2: 0x00B2A6F4
extern int g_Col_GameObserver; // RA2: 0x00B299A0
extern int g_Col_GamePingTime; // RA2: 0x00B292A8
extern bool g_GameListRefreshing; // RA2: 0x00B2AC80, set while Refresh_Game_List fills the list
extern int ObserverMode; // RA2: 0x00A72C68, as in init.cpp
void __fastcall Clear_Listbox(HWND list); // RA2: 0x00600DB0, removes every row of an owner drawn list box
void __fastcall Get_Window_Item_Text(HWND list, int row, Wstring & text); // RA2: 0x00600EB0, the text of a row
#define OD_INSERTSTRING (WM_USER + 199) // RA2: 0x4C7, inserts the string at the row of wParam and returns the new row (name guessed from its use)
#define OD_FINDSTRING (WM_USER + 197) // RA2: 0x4C5, the row whose text is lParam, -1 if none (name guessed from its use)
WideWstring WideWstring_FromInt(int value); // RA2: 0x007710C0, the number as wide text ("%d")
WideWstring operator+(wchar_t const * first, WideWstring const & second); // RA2: 0x00771380

int Count_AI_Slots(void); // RA2: 0x005C9400, the slots holding an easy, medium or hard computer player
bool Is_Observer_Slot_Occupied(void); // RA2: 0x005C94B0, the observer slot holds a seated entry whose team is -1
bool Is_Observer_Slot_Full(void); // RA2: 0x005C9490, defined in netdlg2.cpp
bool Is_Real_Player_Entry(NodeNameType * entry); // RA2: 0x005C9320, the entry is a player (not open, closed, waiting or a computer player)
Wstring Lobby_Entry_Name(NodeNameType * entry); // RA2: 0x005CD0E0, the lobby's name for an open, closed, waiting or computer player entry
void Load_Observer_Mode(void); // RA2: 0x005CD380, reads MultiPlayer ObserverMode from RA2.INI (the counterpart of Save_Observer_Mode)
void Save_Observer_Mode(void); // RA2: 0x005CD3A0, defined in netdlg2.cpp
void Update_Observer_Buttons(HWND dialog); // RA2: 0x005CD540, defined in netdlg2.cpp
void Enable_Player_Combos(HWND dialog, bool value); // RA2: 0x005CD030, defined in netdlg2.cpp
int Player_Side_Combo_ID(int player); // RA2: 0x004D1350, defined in netdlg2.cpp (as the next eight)
int Player_Color_Combo_ID(int player); // RA2: 0x004D1CB0
int Side_Combo_Data(HWND dialog, int id, int index); // RA2: 0x004D1C70
void Fill_Observer_Combo(HWND dialog, int id); // RA2: 0x004D1640
void Fill_Random_Color_Combo(HWND dialog, int id); // RA2: 0x004D2250
void Select_Color_Combo(HWND dialog, int id, int color); // RA2: 0x004D2480
void Fill_Side_Combo(HWND dialog, int id); // RA2: 0x004D1560
void Fill_Color_Combo(HWND dialog, int id); // RA2: 0x004D2080
int Get_Slot_Control_Id(int slot); // RA2: 0x005C97A0
char * Wide_To_Narrow(wchar_t const * text); // RA2: 0x006FA120, static buffer
Wstring Wstring_FromWide(WideWstring const & text); // RA2: 0x007715D0, keeps the 7 bit characters
Wstring Wstring_Narrow(WideWstring const & text); // RA2: 0x00771280, narrows each character of a wide string (the counterpart of Wstring_ToWide)
void Draw_Player_List(int keep_selection, bool force); // RA2: 0x0073FEE0, one more argument than TibSun's
void Show_Wait_Window(unsigned int event, WideWstring const & text, bool block); // RA2: 0x0074D4D0, the wait dialog with a wide text
void Reset_Wait_Events(void); // RA2: 0x00752930, resets every wait event
BOOL CALLBACK WOL_Find_Page_Dialog_Proc(HWND win, UINT uMsg, WPARAM wParam, LPARAM lParam); // RA2: 0x007485D0
bool Wait_Callback_Thunk(void); // RA2: 0x0073C120, defined below
bool Is_Channel_Flag_8(Channel const * channel); // RA2: 0x0073C3C0, defined below

void Spaces_To_Underscores(char * text) // RA2: 0x0073C040, turns every space of the text into an underscore, nothing calls it
{
	for (char * space = strchr(text, ' '); space != NULL; space = strchr(space, ' ')) { // RA2: as above
		*space = '_';
	}
}

bool Is_User_Message_Shown(User const * user) // RA2: 0x0073C070, a message from this user shows when the user is an operator, a lobby dialog is open or Latin text is allowed
{
	if (!user) return(false); // RA2: as above
	if (user->flags & 1) return(true); // RA2: as above
	if (WS_Find_Dialog(0xC2)) return(true); // RA2: as above
	if (WS_Find_Dialog(0xC9)) return(true); // RA2: as above
	return g_DisplayLatin != 0; // RA2: as above
}

bool Is_Text_Shown(unsigned char const * flags, WideWstring const & text) // RA2: 0x0073C0B0, as Is_User_Message_Shown but with the first byte of a user record, and the Asian setting for text above 0xFF
{
	if (flags == NULL) { // RA2: as above
		return(false);
	}
	if (*flags & 1) { // RA2: as above
		return(true);
	}
	if (WS_Find_Dialog(0xC2) != NULL) { // RA2: as above
		return(true);
	}
	if (WS_Find_Dialog(0xC9) != NULL) { // RA2: as above
		return(true);
	}
	if (text.Is_Narrow()) { // RA2: as above
		return(g_DisplayLatin != 0); // RA2: as above
	}
	return(g_DisplayAsian != 0); // RA2: as above
}

bool Wait_Callback_Thunk(void) // RA2: 0x0073C120, a jump to WOL_Wait_Callback (the message box callback)
{
	return(WOL_Wait_Callback()); // RA2: as above
}

//	SKU, reported to WOLAPI for the purpose of finding patches.
IPXAddressClass User_To_IPX_Address(User const * user) // RA2: 0x0073C130, the address of a user from its IP: net 0, the IP in the first four node bytes
{
	NetNumType net; // RA2: as above
	NetNodeType node; // RA2: as above
	memset(net, 0, sizeof(net)); // RA2: as above
	memset(node, 0, sizeof(node)); // RA2: as above
	unsigned long ip = ntohl(user->ipaddr); // RA2: as above
	node[3] = (unsigned char)ip; // RA2: as above
	node[2] = (unsigned char)(ip >> 8); // RA2: as above
	node[1] = (unsigned char)(ip >> 16); // RA2: as above
	node[0] = (unsigned char)(ip >> 24); // RA2: as above
	IPXAddressClass address(net, node, 0); // RA2: as above
	return(address); // RA2: as above
}

void Set_Lobby_Music(bool online) // RA2: 0x0073C1B0, restarts the lobby score (theme -2) or queues theme -3
{
	if (online) { // RA2: as above
		if (!Theme.Still_Playing()) { // RA2: as above
			Theme.Stop();
			Theme.Queue_Song((ThemeType)-2); // RA2: as above
			return;
		}
		if (Theme.What_Is_Playing() == Theme.From_Name("INTRO")) { // RA2: as above
			Theme.Stop();
			Theme.Queue_Song((ThemeType)-2); // RA2: as above
		}
	} else {
		Theme.Queue_Song((ThemeType)-3); // RA2: as above
	}
}

// RA2: the four values of the match state are set together (0x0073D150 and the quick match start 0x00759E20 inline this)
inline void Set_Match_State(int state, int timeout, int result) // RA2: as above
{
	g_MatchResult = result; // RA2: as above
	g_MatchState = state; // RA2: as above
	g_MatchTimeout = timeout; // RA2: as above
	g_MatchStartTime = time(0); // RA2: as above
}

/// <summary>
/// Opens the Find Page dialog, waits for it to close, then restores the input-ignore flag
/// and forces the tactical map to fully redraw.
/// </summary>
void DoFindPage(void) // RA2: 0x0073C230, dialog 0xCF is opened by OwnerDraw::Begin_Dialog and the lobby is redrawn after it
{
	HWND dlg = OwnerDraw::Begin_Dialog(0xCF, (DLGPROC)WOL_Find_Page_Dialog_Proc, 0); // RA2: as above
	IgnoreInput = true;
	Keyboard->Clear();
	ShowWindow(dlg, SW_NORMAL);
	WS_Wait_Dialog(dlg, OwnerDraw::Dialog_Message_Handler, false, true, true); // RA2: as above
	Keyboard->Clear();
	IgnoreInput = Scen->IsInputLocked;
	Map.Flag_To_Redraw(GS_REDRAW_ALL);
}

/// <summary>
/// Searches the global user list for an entry whose name matches the given name
/// case-insensitively.
/// </summary>
/// <param name="name">Name to search for.</param>
/// <returns>Index of the matching user in g_UserList, or -1 if no user was found.</returns>
void Open_Tournament_Page(void) // RA2: 0x0073C2A0, a jump to ViewHTML with the tournament page
{
	ViewHTML("http://www.westwood.com/westwoodonline/tournaments/redalert2/", 0); // RA2: as above
}

void Open_Registration_Page(void) // RA2: 0x0073C2B0, the registration page for the language code
{
	char url[256];
	sprintf(url, "http://games2.westwood.com/cgi-bin/cgiclient?register&request=reg_menu&LANGCODE=%d", g_LanguageCode); // RA2: as above
	ViewHTML(url, 0);
}

int Player_Name_To_Index(Wstring const & name) // RA2: 0x0073C2E0, the name is a Wstring
{
	int i;
	User * user = 0;
	for (i = 0; i < g_UserList.length(); i++) {
		g_UserList.getPointer(&user, i);
		if (stricmp(name.text(), (char *)user->name) == 0) { // RA2: as above
			return(i);
		}
	}
	return(-1);
}

bool Is_Channel_Tournament(Channel const * channel) // RA2: 0x0073C340, only in an Internet game: the tournament flag or channel flag 8
{
	if (Session.Type != GAME_INTERNET) return(false); // RA2: as above
	return(channel->tournament || Is_Channel_Flag_8(channel)); // RA2: as above
}

bool Is_Current_Channel_Tournament(void) // RA2: 0x0073C370, the same test of the current channel
{
	return(Is_Channel_Tournament(&g_CurrentChannel)); // RA2: as above
}

bool Is_Current_Channel_Flag_8(void) // RA2: 0x0073C3A0, channel flag 8 of the current channel
{
	return(Is_Channel_Flag_8(&g_CurrentChannel)); // RA2: as above
}

bool Is_Channel_Flag_8(Channel const * channel) // RA2: 0x0073C3C0, only in an Internet game: bit 8 of the channel's reserved word
{
	if (Session.Type != GAME_INTERNET) return(false); // RA2: as above
	return((channel->reserved >> 8) & 1); // RA2: as above
}

bool Is_Channel_Flag_11(Channel const * channel) // RA2: 0x0073C3E0, as above for bit 11
{
	if (Session.Type != GAME_INTERNET) return(false); // RA2: as above
	return((channel->reserved >> 11) & 1); // RA2: as above
}

bool Is_Channel_Flag_12(Channel const * channel) // RA2: 0x0073C400, as above for bit 12
{
	if (Session.Type != GAME_INTERNET) return(false); // RA2: as above
	return((channel->reserved >> 12) & 1); // RA2: as above
}

Wstring g_NickName; // RA2: a Wstring, the persona of the login (0x0073C420 constructs it)
Wstring g_LoginPassword; // RA2: a Wstring, the password of the login (0x0073C450 constructs it)

void Send_Player_List(void) // RA2: 0x0073C480, sends the names of the eight slots (the lobby entry names for open and AI slots) as game option L
{
	char text[256];
	strcpy(text, "L"); // RA2: as above
	for (int i = 0; i < 8; ++i) { // RA2: as above
		NodeNameType * entry = Session.Slots[i]; // RA2: as above
		Wstring name;
		if (Is_Real_Player_Entry(entry)) { // RA2: as above
			name = Wstring_Narrow(entry->Name); // RA2: as above
		} else {
			name = Lobby_Entry_Name(entry); // RA2: as above
		}
		strncat(text, name.get(), 256); // RA2: as above
		strncat(text, ",", 256); // RA2: as above
	}
	g_pChat->RequestPublicGameOptions(text); // RA2: as above
}

/// <summary>
/// Displays a web page in the player's own browser.
/// This routine hands the page to whatever program the system has registered for HTML and
/// then waits on it, pumping the wait callback throughout so the Westwood Online connection
/// does not lapse while the player reads. The game window is brought back to the front once
/// the browser is finished with. If no browser is installed the player is told so.
/// </summary>
/// <param name="name">URL or page name passed to the browser as the "[open]" argument.</param>
/// <param name="no_ask">Should the browser be launched without asking the player first?</param>
void ViewHTML(const char * name, int no_ask) // RA2: 0x0073C580, the launch and the missing browser texts come from the string table
{
	/*
	 * Create a throwaway HTML file so FindExecutable can tell us which browser handles it.
	 */
	char filename[160];
	tmpnam(filename);
	strcat(filename, ".html");

	FILE * file = fopen(filename, "w");
	fprintf(file, "<title>Hi there.</title>");
	fclose(file);

	char exeName[MAX_PATH + 1];
	int result = (int)FindExecutable(filename, NULL, exeName);

	_unlink(filename);

	/// Ask the user for confirmation (unless no_ask forces an immediate launch), then spawn the browser.
	if (result > 32) { // RA2: as above
		if (!no_ask && ODMessageBox(StringTable::LoadString("TXT_LAUNCHBROWSER", NULL, "D:\\Ra2\\wonline.cpp", 1172), MB_YESNO, Wait_Callback_Thunk, false) != IDYES) { // RA2: as above
			return;
		}

		char commandLine[MAX_PATH + 10];
		sprintf(commandLine, "[open] %s", name);
		
		STARTUPINFO startupInfo;
		memset(&startupInfo, 0, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);

		PROCESS_INFORMATION processInfo;

		BOOL createSuccess = CreateProcess(exeName, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo); // RA2: as above

		if (createSuccess) {
			HANDLE process = processInfo.hProcess;
			if (process) {
				int done_waiting = FALSE;

				WOL_Wait_Callback();

				WaitForInputIdle(process, 5000);

				/// Wait for the browser to exit, or for the game window to regain focus.
				do {
					WOL_Wait_Callback();
					Sleep(500);

					DWORD exitCode;
					GetExitCodeProcess(process, &exitCode);
					if (exitCode != STILL_ACTIVE) {
						done_waiting = TRUE;
					}

					if (GameInFocus || GetTopWindow(NULL) == MainWindow || GetForegroundWindow() == MainWindow) {
						done_waiting = TRUE;
					}
					WOL_Wait_Callback();
				} while (!done_waiting); // RA2: as above

				/*
				 * Bring the game window back to the foreground now that the browser is done.
				 */
				WOL_Wait_Callback();
	
				if (GetTopWindow(NULL) != MainWindow) {
					WOL_Wait_Callback();
					SetForegroundWindow(MainWindow);
					ShowWindow(MainWindow, SW_RESTORE);
					WOL_Wait_Callback();
				}

				int waits = 10;
				while (waits) {
					Sleep(100);
					WOL_Wait_Callback();
					waits--;
				}
			}
		}
	} else {
		/*
		 * No handler found for this file type -- tell the user there's no browser installed.
		 */
		ODMessageBox(StringTable::LoadString("TXT_NOBROWSER", NULL, "D:\\Ra2\\wonline.cpp", 1240), MB_OK, Wait_Callback_Thunk, false); // RA2: as above
	}
}

/// <summary>
/// Determines the sort order of two channel users.
/// Channel owners come first, then users with voice, then everyone else; equals are listed
/// alphabetically. This is the order the user list box is kept in.
/// </summary>
/// <returns>Returns with a negative value if the u1 user sorts first, a positive value if
/// the u2 one does, and zero if the two cannot be told apart.</returns>
void Read_WOL_Settings(void) // RA2: 0x0073C7C0, two more settings than TibSun (DisplayAsian, DisplayLatin) and the observer mode
{
	g_AllowPage = ConfigINI.Get_Int("WOnline", "AllowPage", TRUE);
	g_AllowFind = ConfigINI.Get_Int("WOnline", "AllowFind", TRUE);
	g_LangFilter = ConfigINI.Get_Int("WOnline", "LangFilter", TRUE);
	g_LobbyMusic = ConfigINI.Get_Int("WOnline", "LobMusic", TRUE);
	g_ShowAllChannels = ConfigINI.Get_Int("WOnline", "ShowAll", TRUE);
	g_DisplayAsian = ConfigINI.Get_Int("WOnline", "DisplayAsian", TRUE); // RA2: as above
	g_DisplayLatin = ConfigINI.Get_Int("WOnline", "DisplayLatin", TRUE); // RA2: as above
	Load_Observer_Mode(); // RA2: as above
}

/// <summary>
/// Writes the current WOL lobby settings (paging, finding, language filter, lobby music,
/// show-all) to the config INI and saves it to disk.
/// </summary>
void Write_WOL_Settings(void) // RA2: 0x0073C890, the file is RA2.INI, as above
{
	RawFileClass file("RA2.INI"); // RA2: as above
	ConfigINI.Put_Int("WOnline", "AllowPage", g_AllowPage);
	ConfigINI.Put_Int("WOnline", "AllowFind", g_AllowFind);
	ConfigINI.Put_Int("WOnline", "LangFilter", g_LangFilter);
	ConfigINI.Put_Int("WOnline", "LobMusic", g_LobbyMusic);
	ConfigINI.Put_Int("WOnline", "ShowAll", g_ShowAllChannels);
	ConfigINI.Put_Int("WOnline", "DisplayAsian", g_DisplayAsian); // RA2: as above
	ConfigINI.Put_Int("WOnline", "DisplayLatin", g_DisplayLatin); // RA2: as above
	ConfigINI.Save(file, false);
	Save_Observer_Mode(); // RA2: as above
}

/// <summary>
/// Applies the current WOL settings to the active chat session: pushes the language filter
/// and find/page flags to the chat client, and starts, continues, or fades out the lobby
/// music depending on the LobMusic setting.
/// </summary>
void Apply_WOL_Settings(void) // RA2: 0x0073C990, the observer buttons are updated and the lobby music is queued as in Set_Lobby_Music
{
	if (g_pChat) {
		g_pChat->SetLangFilter(g_LangFilter);
		g_pChat->SetFindPage(g_AllowFind, g_AllowPage);
	}
	Update_Observer_Buttons(NULL); // RA2: as above

		/// Manage the lobby music according to the LobMusic setting.
	bool music = g_LobbyMusic != 0; // RA2: as above
	if (music && !Theme.Still_Playing()) { // RA2: as above
		Theme.Stop();
		Theme.Queue_Song((ThemeType)-2); // RA2: as above
	} else if (music) { // RA2: as above
		if (Theme.What_Is_Playing() == Theme.From_Name("INTRO")) { // RA2: as above
			Theme.Stop();
			Theme.Queue_Song((ThemeType)-2); // RA2: as above
		}
	} else if (!music) { // RA2: as above
		Theme.Queue_Song((ThemeType)-3); // RA2: as above
	}
}

/// <summary>
/// Sends a chat message on the player's behalf.
/// The message goes privately to whoever is selected in the user list box, or out to the
/// whole channel when nothing is selected.
/// </summary>
/// <param name="msg">Text of the message to send.</param>
/// <returns>Returns with 0 if the message went to the whole channel, or 1 if it went
/// privately to the selected users.</returns>
int Send_Chat_Message(WideWstring const & msg) // RA2: 0x0073CA50, the text is wide and goes through the wide IChat calls
{
	static int sel_user_indexes[128];
	HWND users = GetDlgItem(WS_Top_Window(), IDC_USERS);
	int count = ListBox_GetSelItems(users, (WPARAM)32, (LPARAM)sel_user_indexes);

	if (count == 0) {
		/*
		 * No users selected -- broadcast to the whole channel.
		 */
		g_pChat->RequestPublicMessageWide((unsigned short const *)msg.text()); // RA2: as above
		return(0);
	} else {
		/*
		 * Build a linked list of the selected users and send them a private message.
		 */
		User * last = NULL;
		User * head = NULL;
		User * item = NULL;
		User source;
		memset(&source, 0, sizeof(source));
		for (int i = 0; i < count; i++) {
			g_UserList.get(source, sel_user_indexes[i]);
			item = new User;
			*item = source;
			item->next = NULL;
			if (i == 0) head = item;
			if (last) last->next = item;
			last = item;
		}

		if (head) g_pChat->RequestPrivateMessageWide(head, (unsigned short const *)msg.text()); // RA2: as above

		/// Free the temporary user list.
		while (head) {
			item = head;
			head = head->next;
			delete (item);
		}
		return(1);
	}
}

/// <summary>
/// Sends a chat action on the player's behalf.
/// The action goes privately to whoever is selected in the user list box, or out to the
/// whole channel when nothing is selected.
/// </summary>
/// <param name="msg">Text of the action to send.</param>
/// <returns>Returns with 0 if the action went to the whole channel, or 1 if it went
/// privately to the selected users.</returns>
int Send_Chat_Action(WideWstring const & msg) // RA2: 0x0073CB80, as above
{
	static int sel_user_indexes[128];
	HWND users = GetDlgItem(WS_Top_Window(), IDC_USERS);
	int count = ListBox_GetSelItems(users, (WPARAM)32, (LPARAM)sel_user_indexes);

	if (count == 0) {
		/*
		 * No users selected -- broadcast to the whole channel.
		 */
		g_pChat->RequestPublicActionWide((unsigned short const *)msg.text()); // RA2: as above
		return(0);
	} else {
		/*
		 * Build a linked list of the selected users and send them a private action.
		 */
		User * last = NULL;
		User * head = NULL;
		User * item = NULL;
		User source;
		memset(&source, 0, sizeof(source));
		for (int i = 0; i < count; i++) {
			g_UserList.get(source, sel_user_indexes[i]);
			item = new User;
			*item = source;
			item->next = NULL;
			if (i == 0) head = item;
			if (last) last->next = item;
			last = item;
		}

		if (head) g_pChat->RequestPrivateActionWide(head, (unsigned short const *)msg.text()); // RA2: as above

		/// Free the temporary user list.
		while (head) {
			item = head;
			head = head->next;
			delete (item);
		}
		return(1);
	}
}

static inline bool Same_Player_Name(wchar_t const * first, wchar_t const * second) // RA2: the inline compare of two lobby names (case insensitive, as narrow text)
{
	char * name = Wide_To_Narrow(first); // RA2: as above
	return(stricmp(name, Wide_To_Narrow(second)) == 0); // RA2: as above
}

/// <summary>
/// Encodes the current game options and scenario info, followed by each user's name,
/// house, and color, into a single comma-separated string for transmission over WOL.
/// </summary>
/// <param name="out">Buffer that receives the encoded options string.</param>
void Encode_Game_Options(char * out) // RA2: 0x0073CCB0, RA2's option text: seed, speed, credits, units, switches, mode, map name, scenario, then the players and the computer players
{
	static char useroptions[512];

	Wstring name = Session.Options.ScenarioDescription ? Wstring_FromWide(Session.Options.ScenarioDescription) : ""; // RA2: only the 7 bit characters of the map name are sent

	/// Encode the game options and scenario info.
	sprintf(out, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%d,%d,%d,%s,%s:", // RA2: as above
			Seed, Options.GameSpeed, Session.Options.GameSpeed, Session.Options.Credits, Session.Options.UnitCount, // RA2: as above
			Session.Options.ShortGame, Session.Options.SWAllowed, Session.Options.BuildOffAlly, Session.Options.MCVRedeploy, // RA2: as above
			Session.Options.Goodies, Session.Options.MPModeIndex, name.get(), Session.unknown_8D4, Session.ScenarioIsOfficial, // RA2: as above
			Session.ScenarioFileLength, Session.ScenarioFileName, Session.ScenarioDigest); // RA2: as above

	/*
	 * Append each user's name, house, and color.
	 * Append each user's name, country, color, address, port and ping.
	 */
	memset(useroptions, 0, sizeof(useroptions));
	for (int i = 0, count = Session.Players.Count(); i < count; i++) { // RA2: as above
		NodeNameType * entry = Session.Players[i]; // RA2: as above
		int country = entry->Get_Country(); // RA2: as above
		int color = entry->Get_Color(); // RA2: as above
		int address = 0; // RA2: as above
		int port = 0; // RA2: as above
		int ping = 0; // RA2: as above
		for (int j = 0; j < 8; j++) { // RA2: as above
			if (Same_Player_Name(Session.Players[i]->Name, Session.Slots[j]->Name)) { // RA2: as above
				NetNumType net; // RA2: as above
				NetNodeType node; // RA2: as above
				unsigned short socket; // RA2: as above
				Session.unknown_2994[j].Get_Address(net, node, &socket); // RA2: as above
				address = *(int *)node; // RA2: as above
				port = Session.WolPlayerState[j]; // RA2: as above
				ping = Session.PlayerPortBase[j]; // RA2: as above
				break;
			}
		}
		Wstring text(Wstring_Narrow(Session.Players[i]->Name)); // RA2: as above
		sprintf(useroptions + strlen(useroptions), "%s,%d,%d,%x,%x,%x,", text.get(), country, color, address, port, ping); // RA2: as above
	}
	useroptions[strlen(useroptions) - 1] = ':'; // RA2: as above
	strcat(useroptions, "@:"); // RA2: as above

	/// Append the computer players.
	for (int k = 0; k < 8; k++) { // RA2: as above
		sprintf(useroptions + strlen(useroptions), "%d,%d,%d,", Session.Options.AISlots.Difficulties[k], Session.Options.AISlots.Countries[k], Session.Options.AISlots.Colors[k]); // RA2: as above
	}

	strcat(out, useroptions);
}

/// <summary>
/// Combines the rules, AI, and art CRCs into a single hash used to verify that all players
/// are running matching game data.
/// </summary>
/// <returns>Combined CRC hash.</returns>
int GetINIHash(void) // RA2: 0x0073CFD0
{
	return(g_RuleCRC ^ g_AICRC ^ g_ArtCRC);
}

/// <summary>
/// Encodes extended game/session info -- build number, data hash, options, and a flag
/// describing which house(s) still have room -- into a comma-separated string for
/// transmission over WOL.
/// </summary>
/// <param name="out">Buffer that receives the encoded info string.</param>
void Encode_Channel_ExInfo(char * out) // RA2: 0x0073CFF0, RA2's info text: build, hash, computer players, observer slot state, scenario file
{
	int i = 0;
	int house1_count = 0;
	int house0_count = 0;

	/// Count how many users are currently on each house.
	for (int j = 0; j < (int)g_CurrentChannel.currentUsers; j++) {
		int house = g_UserInfo[j].house;
		if (house == 0) {
			house0_count++;
		} else if (house == 1) { // RA2: as above
			house1_count++;
		}
	}

	/// Flag which house(s) still have free slots.
	if (house1_count < (int)g_CurrentChannel.maxUsers / 2) {
		i = 2;
	}

	if (house0_count < (int)g_CurrentChannel.maxUsers / 2) {
		i++;
	}

	sprintf(out, "%d,%d,%d,%d,%d,%s", Build_Number(), GetINIHash(), Count_AI_Slots(), Is_Observer_Slot_Occupied(), !Is_Observer_Slot_Full(), Session.ScenarioFileName); // RA2: as above
}

/// <summary>
/// Records whether the given user (or the local player if who is NULL) has accepted an
/// invitation/request, then refreshes the user list display.
/// </summary>
/// <param name="who">Name of the user to update, or NULL for the local player.</param>
void Parse_Channel_ExInfo(char * text, int * build, int * hash, int * ai_count, int * observer, int * not_full, Wstring * scenario) // RA2: 0x0073D070, the counterpart of Encode_Channel_ExInfo
{
	char * token = strtok(text, ","); // RA2: as above
	*build = token != NULL ? atol(token) : 0;
	token = strtok(NULL, ",");
	*hash = token != NULL ? atol(token) : 0;
	token = strtok(NULL, ",");
	*ai_count = token != NULL ? atol(token) : 0;
	token = strtok(NULL, ",");
	*observer = token != NULL ? atol(token) : 0;
	token = strtok(NULL, ",");
	*not_full = token != NULL ? atol(token) : 0;
	*scenario = strtok(NULL, ",");
}



/// <summary>
/// Records whether the given user (or the local player if who is empty) has accepted the game
/// options, then refreshes the user list display. When the host is waiting for the only other
/// player of a two player game and that player has accepted, the game is started.
/// </summary>
/// <param name="who">Name of the user to update, empty for the local player.</param>
/// <param name="status">New accepted status to store.</param>
void SetPlayerAccepted(Wstring const & who, int status) // RA2: 0x0073D150, the name is a Wstring and the host starts a two player game when the guest accepts
{
	Wstring const nick(who.length() > 0 ? who : g_NickName); // RA2: as above
	if (nick == g_NickName && status == 0) { // RA2: as above
		HWND dialog = WS_Find_Dialog(201); // RA2: as above
		Enable_Player_Combos(dialog, true); // RA2: as above
	}

	User * user = 0;
	for (int index = 0; index < g_UserList.length(); ++index) { // RA2: as above
		g_UserList.getPointer(&user, index); // RA2: as above
		if (stricmp(nick.text(), (char *)user->name) == 0) { // RA2: as above
			if (index != -1) {
				g_UserInfo[index].accepted = status; // RA2: as above
				Draw_Player_List(0, false); // RA2: as above
			}
			break;
		}
	}

	bool local = g_IsChannelCreator == 1; // RA2: as above
	Wstring const lower = nick.toLower(); // RA2: as above
	int player_index; // RA2: as above
	for (player_index = 0; player_index < Session.Players.Count(); ++player_index) { // RA2: as above
		Wstring const player = Wstring_Narrow(Session.Players[player_index]->Name); // RA2: as above
		if (player.toLower() == lower) break; // RA2: as above
	}
	if (player_index >= Session.Players.Count()) return; // RA2: as above
	Session.Players[player_index]->Slot.Unknown77 = status; // RA2: as above

	if (local && (Online_Session_State == 2 || Online_Session_State == 1) && !g_GameStartRequested && player_index == 1 && g_UserList.length() == 2) { // RA2: as above
		User * node; // RA2: as above
		User * head = 0; // RA2: as above
		User copy; // RA2: as above
		memset(&copy, 0, sizeof(copy)); // RA2: as above
		for (int user_index = 0; user_index < g_UserList.length(); ++user_index) { // RA2: as above
			g_UserList.get(copy, user_index); // RA2: as above
			node = new User; // RA2: as above
			*node = copy;
			node->next = head; // RA2: as above
			head = node; // RA2: as above
		}
		Set_Match_State(9, 90, 0); // RA2: as above
		g_pChat->RequestGameStart(head);
		g_GameStartRequested = true;
		node = head; // RA2: as above
		while (node) { // RA2: as above
			head = node->next; // RA2: as above
			delete node; // RA2: as above
			node = head; // RA2: as above
		}
	}
	Draw_Player_List(0, false); // RA2: as above
}

/// <summary>
/// Looks up whether the given user (or the local player if who is NULL) has accepted an
/// invitation/request.
/// </summary>
/// <param name="who">Name of the user to query, or NULL for the local player.</param>
/// <returns>The user's accepted status, or -1 if the user could not be found.</returns>
int GetPlayerAccepted(Wstring const & who) // RA2: 0x0073D3F0, the name is a Wstring and the answer is the player slot's status
{
	Wstring const nick(who.length() > 0 ? who : g_NickName); // RA2: as above
	Wstring const lower = nick.toLower(); // RA2: as above
	if (Wstring_ToWide(lower) == static_cast<WideWstring const &>(WideWstring(Session.GameName)).toLower()) { // RA2: as above
		return(1);
	}
	int found = -1; // RA2: as above
	for (int index = 0; index < Session.Players.Count(); index++) { // RA2: as above
		Wstring const player = Wstring_Narrow(Session.Players[index]->Name); // RA2: as above
		if (player.toLower() == lower) found = index; // RA2: as above
	}
	if (found != -1) { // RA2: as above
		return(Session.Players[found]->Slot.Unknown77); // RA2: as above
	}
	return(-1);
}

/// <summary>
/// Leaves the current channel (waiting for the leave to complete) and joins the lobby channel
/// selected by Pick_Lobby.
/// </summary>
/// <returns>The index of the joined lobby channel, or -1 if the leave timed out, the join
/// failed, or no lobby was available.</returns>
/// <param name="who">Name of the player whose country/color changed.</param>
/// <param name="house">New country selection.</param>
/// <param name="color">New color selection.</param>
/// <returns>1 if the player's country actually changed, 0 otherwise.</returns>
int Assign_House_And_Color(Wstring const & who, int house, int color) // RA2: 0x0073D550, works on the lobby slots and fills the side and color boxes for the observer
{
	int found = -1; // RA2: as above
	Wstring const lower = who.toLower(); // RA2: as above
	for (int index = 0; index < 8; ++index) { // RA2: as above
		Wstring const player_name = Wstring_Narrow(Session.Slots[index]->Name); // RA2: as above
		if (lower == player_name.toLower()) { // RA2: as above
			found = index; // RA2: as above
			break;
		}
	}
	int changed = 0; // RA2: as above
	if (found == -1) return(changed); // RA2: as above

	HWND window = WS_Find_Dialog(0xC2); // RA2: as above
	if (!window) window = WS_Find_Dialog(0xC9); // RA2: as above
	NodeNameType * player = Session.Slots[found]; // RA2: as above
	if (player->Get_Country() != house) { // RA2: as above
		changed = 1; // RA2: as above
		player->Set_Country(house); // RA2: as above
		int side = Player_Side_Combo_ID(found); // RA2: as above
		int previous = Side_Combo_Data(window, side, -1); // RA2: as above
		if (house == -3) { // RA2: as above
			if (previous != -3) { // RA2: as above
				Fill_Observer_Combo(window, side); // RA2: as above
				int observer_control = Get_Slot_Control_Id(found); // RA2: as above
				SendMessage(GetDlgItem(window, observer_control), 0x4F1, 0, 1); // RA2: as above
				int color_control = Player_Color_Combo_ID(found); // RA2: as above
				Fill_Random_Color_Combo(window, color_control); // RA2: as above
				Select_Color_Combo(window, color_control, 8); // RA2: as above
			}
		} else if (previous == -3) { // RA2: as above
			Fill_Side_Combo(window, side); // RA2: as above
			int observer_control = Get_Slot_Control_Id(found); // RA2: as above
			SendMessage(GetDlgItem(window, observer_control), 0x4F1, 0, 0); // RA2: as above
			int color_control = Player_Color_Combo_ID(found); // RA2: as above
			Fill_Color_Combo(window, color_control); // RA2: as above
			Select_Color_Combo(window, color_control, -2); // RA2: as above
		}
		if (player->Get_Country() == -3) player->Slot.SpectatorFlag = -1; // RA2: as above
	}
	if (Session.Slots[found]->Get_Color() != color) player->Set_Color(color); // RA2: as above
	if (Session.Slots[found] == Session.Players[0]) Session.PrefColor = color; // RA2: as above
	Draw_Player_List(0, false); // RA2: as above
	return(changed); // RA2: as above
}

/// <summary>
/// Fills a login dialog in from a remembered login.
/// This routine is used to spare the player from retyping credentials. If the nickname is
/// one Westwood Online has kept, its password and locale are recalled into the dialog's
/// controls along with it; if it is not, the dialog is left as it was.
/// </summary>
/// <param name="name">Nickname to search for among the stored logins, or NULL to skip
/// the lookup.</param>
/// Most names are exposed by debug prints in RA2, rest guessed
enum {
	EV_ABORT,
	EV_GOT_SERVERS,
	EV_CONNECTED,
	EV_CHAN_LIST,
	EV_CHAN_JOINED,
	EV_ERROR,
	EV_EXIT,
	EV_STARTGAME,
	EV_QUITGAME,
	EV_NETIGNORE,
	EV_CHAN_JOIN_FAIL,
	EV_11, // continue waiting?
	EV_12,
	EV_13,
	EV_LOCALE,
	EV_SKIRMISH,
	EV_16, // RA2: as above
	EV_17, // RA2: as above
	EV_COUNT,
};

/// <summary>
/// Saves a login's nickname, password, and locale into one of the 32 stored login slots.
/// Reuses whichever slot already stores this nickname if one exists, otherwise the first
/// empty slot, otherwise cycles to the slot after the session's last-used nickname slot.
/// </summary>
/// <param name="loginName">Nickname to store.</param>
/// <param name="password">Password to store alongside the nickname.</param>
/// <param name="passwordPlaintext">Whether the password is stored/sent as plaintext.</param>
/// <param name="locale">Locale value to associate with this login.</param>
/// <returns>The result of setting the nick's locale (from SetNickLocale), or the computed
/// slot number if no slot ended up being written to.</returns>
/// <param name="channel_type">0 for the chat channels, 0x21 for the game channels.</param>
/// <param name="autoping">Should the server ping the channels?</param>
/// <returns>True if the request was sent.</returns>
bool Request_Channel_List(int channel_type, unsigned char autoping) // RA2: 0x0073D760, not in TibSun: the request and its queue entry in one place
{
	DebugString("Requesting channel list type = %d, ping = %d\n", channel_type, autoping); // RA2: as above
	if (g_pChat->RequestChannelList(channel_type, autoping) != S_OK) { // RA2: as above
		return(false);
	}
	/// Not found -- use the first empty slot instead.
	unsigned int type = channel_type; // RA2: as above
	g_ChanListQueue.addTail(type);
	return(true);
}

/// <summary>
/// Dialog procedure for the "New Game" creation dialog. Handles owner-draw
/// painting, sets up the player-count controls (a slider, or WDT 1on1/2on2
/// radio buttons for World Domination Tour games), enforces tournament and
/// battle-clan slider interlocks, and stores the result on OK.
/// </summary>
/// <param name="win">Dialog window handle.</param>
/// <param name="uMsg">Window message identifier.</param>
/// <param name="wParam">Message-specific first parameter.</param>
/// <param name="lParam">Message-specific second parameter.</param>
/// <returns>TRUE if the message was handled, FALSE otherwise.</returns>
/// <returns>True if the channel list arrived.</returns>
bool Wait_Channel_List(void) // RA2: 0x0073D8F0, not in TibSun: waits for the answer, 30 seconds at most
{
	{
		WideWstring title(StringTable::LoadString("TXT_REQ_CHANLIST", NULL, "D:\\Ra2\\wonline.cpp", 2042)); // RA2: as above
		Show_Wait_Window(1, title, false); // RA2: as above
	}
	int elapsed = 0; // RA2: as above
	for (;;) { // RA2: as above
		DWORD event = WaitForMultipleObjects(18, g_WaitEventHandles, FALSE, 100); // RA2: as above
		while (event == WAIT_TIMEOUT) { // RA2: as above
			elapsed += 100; // RA2: as above
			if (elapsed > 30000) { // RA2: as above
				swprintf(g_LastErrorMessage, StringTable::LoadString("TXT_CANT_CONNECT", NULL, "D:\\Ra2\\wonline.cpp", 2051)); // RA2: as above
				SetEvent(g_WaitEventHandles[EV_ABORT]);
				return(false);
			}
			WOL_Wait_Callback();
			event = WaitForMultipleObjects(18, g_WaitEventHandles, FALSE, 100); // RA2: as above
		}
		if (event == WAIT_FAILED) { // RA2: as above
			Close_Wait_Window(1); // RA2: as above
			DebugString("WaitForMultipleObjects() failed in Get_Channel_List\n"); // RA2: as above
			return(false);
		}
		ResetEvent(g_WaitEventHandles[event]); // RA2: as above
		if (event == EV_ERROR) { // RA2: as above
			Close_Wait_Window(1); // RA2: as above
			return(false);
		}
		if (event == EV_ABORT) { // RA2: as above
			Close_Wait_Window(1); // RA2: as above
			break;
		}
		if (event == EV_CHAN_LIST) { // RA2: as above
			Close_Wait_Window(1); // RA2: as above
			return(true);
		}
	}
	return(false);
}

/// <summary>
/// Nudges the host with a NET_PING packet while waiting in a live game channel as a guest.
/// Sets up the session's player/host address from the channel's first user, then sends a
/// single ping packet to the host address.
/// </summary>
bool Queue_Channel_List_Request(int channel_type, unsigned char autoping, bool wait) // RA2: 0x0073DB20, not in TibSun
{
	if (g_ChanListQueue.length()) { // RA2: as above
		bool result = Wait_Channel_List(); // RA2: as above
		if (!result) return(result); // RA2: as above
	}
	Reset_Wait_Events(); // RA2: as above
	bool requested = Request_Channel_List(channel_type, autoping); // RA2: as above
	if (!requested) { // RA2: as above
		SetEvent(g_WaitEventHandles[EV_ABORT]);
		return(false);
	}
	if (wait) { // RA2: as above
		bool result = Wait_Channel_List(); // RA2: as above
		return(result);
	}
	return(true);
}

/// <summary>
/// Shows the World Domination Tour "available servers" dialog and waits for it to close.
/// </summary>
/// <returns>Result code returned by the dialog (see WS_Wait_Dialog/WS_Destroy_Dialog).</returns>
bool Request_Chat_Channel_List(bool wait) // RA2: 0x0073DD20, not in TibSun
{
	if (g_ChanListQueue.length()) { // RA2: as above
		bool result = Wait_Channel_List(); // RA2: as above
		if (!result) return(result); // RA2: as above
	}
	for (int i = 0; i < 18; ++i) ResetEvent(g_WaitEventHandles[i]); // RA2: as above
	if (!Request_Channel_List(0, 0)) { // RA2: as above
		SetEvent(g_WaitEventHandles[EV_ABORT]);
		return(false);
	}
	if (wait) return(Wait_Channel_List()); // RA2: as above
	return(true);
}

/// <summary>
/// Dialog procedure for the World Domination Tour "available servers" list. Populates the
/// list box on OD_SUBCLASSED (stripping any "region:" prefix from each server name and
/// pre-selecting the player's preferred server), and on OK stores the chosen server as the
/// new preferred server and saves the multiplayer settings.
/// </summary>
/// <param name="win">Handle of the dialog window.</param>
/// <param name="uMsg">Window message being processed.</param>
/// <param name="wParam">Message-specific parameter.</param>
/// <param name="lParam">Message-specific parameter.</param>
/// <returns>TRUE if the message was handled, FALSE otherwise.</returns>
bool Request_Game_Channel_List(unsigned char autoping, bool wait) // RA2: 0x0073DF10, not in TibSun
{
	if (g_ChanListQueue.length()) { // RA2: as above
		bool result = Wait_Channel_List(); // RA2: as above
		if (!result) return(result); // RA2: as above
	}
	for (int i = 0; i < 18; ++i) ResetEvent(g_WaitEventHandles[i]); // RA2: as above
	if (!Request_Channel_List(0x21, autoping)) { // RA2: as above
		SetEvent(g_WaitEventHandles[EV_ABORT]);
		return(false);
	}
	if (wait) return(Wait_Channel_List()); // RA2: as above
	return(true);
}

inline bool Is_Tournament_Session_Inline(void) // RA2: 0x0073BDB0 is this test, inlined here
{
	return(Online_Session_State == 2 || Online_Session_State == 1);
}

bool WOL_Wait_Callback(void) // RA2: 0x0073E100, no channel list requests (TibSun's are gone), the match timeout and the start of the game are RA2's
{
	Title_Screen_Restore(false); // RA2: as above

	static int prev_exinfo = 0;
	static int prev_ping = 0;

	MSG msg;
	int counter = 0;
	MessageLoopCount++; // RA2: as above
	while (Peek_Windows_Message(&msg, NULL, 0, 0, 1)) { // RA2: as above
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		counter++;
		if (counter == 100) break;
	}

	// RA2: a tournament match that waited longer than its timeout is cancelled (states 2 to 5) or given 30 seconds more
	if (Is_Tournament_Session_Inline() && g_MatchTimeout != -1 && g_MatchState != 1 && g_MatchState != 0 && g_MatchState != 11 && (unsigned)(time(NULL) - g_MatchStartTime) > (unsigned)g_MatchTimeout) {
		if (g_MatchState >= 2 && g_MatchState <= 5) {
			Set_Match_State(0, -1, 2);
		} else {
			Lobby_Chat_Printf(0x20BFFF, StringTable::LoadString("WOL:MatchTimeout", NULL, "D:\\Ra2\\wonline.cpp", 2155));
			Set_Match_State(2, 30, 0);
		}
		SetEvent(g_WaitEventHandles[EV_ERROR]);
	}

	WS_Top_Window_ID(); // RA2: the result is not used

	if (g_WDTTeamLookupQueue.length() > 200) {
		g_WDTTeamLookupQueue.clear();
	}

	if (g_LastWDTUserTeamRequestTime < (unsigned)time(0) - 2 && g_WDTTeamLookupQueue.length() > 65 ||
		g_LastWDTUserTeamRequestTime < (unsigned)time(0) - 3 && g_WDTTeamLookupQueue.length() > 10 ||
		g_LastWDTUserTeamRequestTime < (unsigned)time(0) - 15 && g_WDTTeamLookupQueue.length() > 0) {

		User * list = NULL;
		User * last_user = NULL;
		for (int i = 0; i < 10; i++) {
			if (g_WDTTeamLookupQueue.length() > 0) {
				User * new_user = new User;
				memset(new_user, 0, sizeof(User));
				Wstring name;
				g_WDTTeamLookupQueue.removeTail(name);
				char * rawName = name.get();
				if (strlen(rawName) > 9) {
					rawName[9] = '\0';
				}
				strcpy((char *)new_user->name, rawName);
				if (last_user == NULL) {
					list = new_user;
				} else {
					last_user->next = new_user;
				}
				last_user = new_user;
			} else {
				break;
			}
		}

		if (g_pChat != NULL) {
			g_pChat->RequestUserTeam(list);
		}

		User * d = list;
		while (d != NULL) {
			User * next = d->next;
			delete d;
			d = next;
		}

		g_LastWDTUserTeamRequestTime = time(0);
	}

	if (g_UserLocaleLookupQueue.length() > 200) {
		g_UserLocaleLookupQueue.clear();
	}

	if (g_LastChannelLocaleRequestTime < (unsigned)time(0) - 2 && g_UserLocaleLookupQueue.length() > 65 ||
		g_LastChannelLocaleRequestTime < (unsigned)time(0) - 3 && g_UserLocaleLookupQueue.length() > 10 ||
		g_LastChannelLocaleRequestTime < (unsigned)time(0) - 15 && g_UserLocaleLookupQueue.length() > 0) {

		User * list = NULL;
		User * last_user = NULL;
		for (int i = 0; i < 10; i++) {
			if (g_UserLocaleLookupQueue.length() > 0) {
				User * new_user = new User;
				memset(new_user, 0, sizeof(User));
				Wstring name;
				g_UserLocaleLookupQueue.removeTail(name);
				char * rawName = name.get();
				if (strlen(rawName) > 9) {
					rawName[9] = '\0';
				}
				strcpy((char *)new_user->name, rawName);
				if (last_user == NULL) {
					list = new_user;
				} else {
					last_user->next = new_user;
				}
				last_user = new_user;
			} else {
				break;
			}
		}

		if (g_pChat != NULL) {
			g_pChat->RequestUserLocale(list);
		}

		User * d = list;
		while (d != NULL) {
			User * next = d->next;
			delete d;
			d = next;
		}

		g_LastChannelLocaleRequestTime = time(0);
	}

	if (Session.Type == GAME_INTERNET && Online_Session_State == 2 && time(0) > g_NextWDTStatePollTime) { // RA2: an Internet tournament session, not TibSun's Session.IsWDT
		g_NextWDTStatePollTime = time(0) + 30;
		Get_WDT_State_Silent();
	}

	if ((g_pChat) && (g_SuspendChatPump == 0)) g_pChat->PumpMessages();
	if (g_pNetUtil) g_pNetUtil->PumpMessages();
	if (g_pDownload) g_pDownload->PumpMessages();

	Call_Back();

	if ((g_CurrentChannel.name[0]) && (g_CurrentChannel.type == 0x21) && (g_IsChannelCreator)) Send_Lobby_Options(false, false); // RA2: for PumpGameopts(false)

	if ((g_pChat != NULL) && (g_CurrentChannel.type != 0) && (g_IsChannelCreator)) {
		if ((time(NULL) - prev_exinfo) > 10) {
			char exinfo_out[80];
			memset(exinfo_out, 0, 80);
			Encode_Channel_ExInfo(exinfo_out);
			exinfo_out[40] = 0;
			g_pChat->SetChannelExInfo(exinfo_out);
			prev_exinfo = time(NULL);
		}
	}

	if (WolOptionsLocked != 2) { // RA2: no pings once the game is locked in
		if ((g_pChat != NULL) && (g_CurrentChannel.type != 0) && ((time(NULL) - prev_ping) > 10)) {
			User user;
			Wstring nick = ((Wstring const &)g_NickName).toLower(); // RA2: the names are compared as Wstrings, not with stricmp; the const member returns the lower case copy
			int handle;
			int i;
			in_addr addr;

			if (!Is_Tournament_Session_Inline()) { // RA2: as above
				for (i = 0; i < g_UserList.length(); i++) {
					g_UserList.get(user, i);
					Wstring const user_name((char *)user.name); // RA2: as above
					if (nick == user_name.toLower()) continue; // RA2: as above
					addr.s_addr = user.ipaddr;
					if (user.ipaddr != 0) g_pNetUtil->RequestPing(inet_ntoa(addr), 1000, &handle);
				}
			}
			prev_ping = time(NULL);
		}
	}

	if (WolOptionsLocked == 2) { // RA2: the host announced the start, the game is started from here
		if (g_StartGameReceived && !g_StartGameRunning) {
			Start_Game_Now();
		}
	}

	Handle_Preview_Download();
	return(false);
}

void Reset_WOL_Globals(void) // RA2: 0x0073E8D0, loads the default profile, clears the buddy lists and sets the SKU of the tournament ladder
{
	g_PlayerProfile = PlayerProfile::Load_Default(); // RA2: as above

	g_UserChannels.clear();
	g_GameChannelList.clear();
	g_UserList.clear();

	g_WDTTeamLookupQueue.clear();
	g_WDTTeams.clear();

	g_UserLocaleLookupQueue.clear();
	g_UserLocales.clear();

	g_NextWDTStatePollTime = time(0) + 86400;
	g_LastChannelLocaleRequestTime = 0;
	g_LastWDTUserTeamRequestTime = 0;

	g_BattleClansLadderDict.clear();
	g_WDTLadderDict.clear();
	g_BuddyChannels.clear(); // RA2: as above
	g_BuddyList.clear(); // RA2: as above
	g_BuddyRequests.clear(); // RA2: as above
	g_BuddyStatus.clear(); // RA2: as above

	if (Session.Type == GAME_INTERNET && Online_Session_State == 2) { // RA2: an Internet tournament session, not TibSun's Addon_Enabled(ADDON_FIRESTORM) && Session.IsWDT
		int sku = 0x2300; // RA2: the SKU of the tournament ladder
		g_InstalledLadderSKU = sku;
		g_SelectedLadderSKU = sku;
		g_ActiveLadder = &g_WDTLadderDict;
	} else {
		int sku = g_GameSKU;
		g_InstalledLadderSKU = sku;
		g_SelectedLadderSKU = sku;
		g_ActiveLadder = &g_GeneralLadderDict;
	}

	memset(&g_CurrentChannel, 0, sizeof(g_CurrentChannel));
	memset(&g_PendingChannel, 0, sizeof(g_PendingChannel)); // RA2: as above
	memset(&g_GameServer, 0, sizeof(g_GameServer));
	memset(&g_WDTServer, 0, sizeof(g_WDTServer));

	g_LastErrorMessage[0] = 0;

	g_MatchResult = 0;
	g_MatchState = 1;
	g_MatchTimeout = -1;
	g_MatchStartTime = time(0);

	g_CurrentServerIndex = 0;
	g_TargetServerIndex = 0;

	g_IsChannelCreator = 0;
	g_ActiveWaitFlags = 0;

	g_GameStartRequested = false;

	g_IgnoreNetStatus = false;
	g_ShowMOTD = true;
	g_Motd.clear(); // RA2: as above

	memset(&g_WaitEventHandles, 0, sizeof(g_WaitEventHandles));

	g_PlayingNetGame = false;
	g_ConnectionLost = false;

	g_ServerType = -1;

	g_SuspendChatPump = 0;

	g_LastChatListTime = 0xFFFFFFFF;
	g_LastGameListTime = 0xFFFFFFFF;

	g_Lobbies[0] = StringTable::LoadString("TXT_LOB_1", NULL, "D:\\Ra2\\wonline.cpp", 2485); // RA2: as above
	g_Lobbies[1] = StringTable::LoadString("TXT_LOB_2", NULL, "D:\\Ra2\\wonline.cpp", 2486); // RA2: as above
	g_Lobbies[2] = StringTable::LoadString("TXT_LOB_3", NULL, "D:\\Ra2\\wonline.cpp", 2487); // RA2: as above
	g_Lobbies[3] = StringTable::LoadString("TXT_LOB_4", NULL, "D:\\Ra2\\wonline.cpp", 2488); // RA2: as above
	g_Lobbies[4] = StringTable::LoadString("TXT_LOB_5", NULL, "D:\\Ra2\\wonline.cpp", 2489); // RA2: as above
	g_Lobbies[5] = StringTable::LoadString("TXT_LOB_6", NULL, "D:\\Ra2\\wonline.cpp", 2490); // RA2: as above
	g_Lobbies[6] = StringTable::LoadString("TXT_LOB_7", NULL, "D:\\Ra2\\wonline.cpp", 2491); // RA2: as above
	g_Lobbies[7] = StringTable::LoadString("TXT_LOB_8", NULL, "D:\\Ra2\\wonline.cpp", 2492); // RA2: as above

	memset(WDT_TERRITORY_LOBBY_INDEXES, 0xFFFFFFFF, sizeof(WDT_TERRITORY_LOBBY_INDEXES));

	g_pChat = NULL;
	g_pNetUtil = NULL;
	g_pDownload = NULL;

	g_dwChatAdvise = 0;
	g_dwNetUtilAdvise = 0;
	g_dwDownloadAdvise = 0;

	g_SquadInfoRequests = 0;
	g_RequestingOwnSquadInfo = 0;
	g_OwnSquadID = 0;
	memset(g_OwnSquadName, 0, 41); // RA2: as above

	g_ChannelCount = 0;

	g_LadderPos = 1;

	g_JoinLobbyNow = 0;

	g_ChanListQueue.setEmpty();

	WolOptionsLocked = 0; // RA2: TibSun's g_GameStarted

	g_UserAge = 0;
	g_UserConsent = 0;

	g_RuleCRC = RulesClass::Get_Rule_Unique_ID();
	RulesClass::Load_Art_INI();
	g_ArtCRC = RulesClass::Get_Art_Unique_ID();
	g_AICRC = RulesClass::Get_AI_Unique_ID();

	CurrentLevel = 0; // RA2: as above
}

void Refresh_Lobby_List(void) // RA2: 0x0073EE80, fills the lobby list of dialog 0x113 with the "Lob_33_" channels in number order, the current one selected
{
	HWND dialog = WS_Find_Dialog(0x113);
	if (!dialog) return; // RA2: as above
	HWND list = GetDlgItem(dialog, 0x53D);
	if (!list) return; // RA2: as above

	Wstring selected; // RA2: the text of the first visible row
	int top_row = SendMessageA(list, LB_GETTOPINDEX, 0, 0); // RA2: the first visible row is kept, not the selected one
	if (top_row != -1) Get_Window_Item_Text(list, top_row, selected);
	SendMessageA(list, OD_DISABLEPAINT, 0, 1);
	Clear_Listbox(list);

	int count = g_UserChannels.length();
	for (int i = 0; i < count; ++i) {
		Channel channel;
		g_UserChannels.get(channel, i);
		if (strncmp((char *)channel.name, "Lob_33_", strlen("Lob_33_")) != 0) continue;
		int number = atol((char *)channel.name + strlen("Lob_33_"));
		bool current = strcmp((char *)channel.name, (char *)g_CurrentChannel.name) == 0;
		wchar_t title[80];
		int page = number / 8;
		int name_index = number % 8;
		if (page == 0) {
			swprintf(title, L"%s  (%d)", (wchar_t const *)g_Lobbies[name_index], channel.currentUsers);
		} else {
			swprintf(title, L"%s_%d  (%d)", (wchar_t const *)g_Lobbies[name_index], page + 1, channel.currentUsers);
		}
		int rows = SendMessageA(list, LB_GETCOUNT, 0, 0);
		int at = 0;
		Channel other;
		for (; at < rows; ++at) {
			int index = SendMessageA(list, LB_GETITEMDATA, at, 0);
			g_UserChannels.get(other, index);
			if (atol((char *)other.name + strlen("Lob_33_")) > number) break;
		}
		int row = SendMessageA(list, OD_INSERTSTRING, at, (LPARAM)channel.name);
		SendMessageA(list, LB_SETITEMDATA, row, i);
		OwnerDraw::CellData cell;
		cell.type = OwnerDraw::CellData::TEXT;
		cell.string.set((unsigned short *)title);
		cell.hint.set((unsigned short *)L"");
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_ChanName, row), (LPARAM)&cell);
		if (current) SendMessageA(list, LB_SETCURSEL, row, 0);
	}

	int restored = SendMessageA(list, OD_FINDSTRING, 0, (LPARAM)selected.text());
	if (restored != -1) SendMessageA(list, LB_SETTOPINDEX, restored, 0);
	SendMessageA(list, OD_DISABLEPAINT, 0, 0);
	InvalidateRect(list, NULL, false);
}

WideWstring Get_Locale_Name(int locale) // RA2: 0x0073F1C0, the localized name of a locale number, from the string "WOL:Locale" and the number in two digits
{
	Wstring label = "WOL:Locale" + Wstring_FromInt(locale, 2); // RA2: as above
	WideWstring text(StringTable::LoadString(label.get(), NULL, "D:\\Ra2\\wonline.cpp", 2644)); // RA2: as above
	if (locale < 37) { // RA2: both ways return the same text, the test is kept
		return(text);
	} else {
		return(text);
	}
}

WideWstring User_Tooltip(User const & user) // RA2: 0x0073F250, the text of the tooltip of a lobby user: name, rank in the active ladder, squad and locale
{
	Wstring name((char *)user.name); // RA2: as above
	WideWstring text = Wstring_ToWide(name) + L" : "; // RA2: as above
	name.toLower();
	Ladder ladder;
	memset(&ladder, 0, sizeof(ladder));
	if (g_ActiveLadder && g_ActiveLadder->getValue(name, ladder)) { // RA2: as above
		text += WideWstring(StringTable::LoadString("TXT_RANK", NULL, "D:\\Ra2\\wonline.cpp", 2672)) + WideWstring(L" ") + WideWstring_FromInt(ladder.rung); // RA2: as above
	} else {
		text += StringTable::LoadString("TXT_UNRANKED", NULL, "D:\\Ra2\\wonline.cpp", 2674); // RA2: as above
	}
	Wstring squad((char *)user.squadname); // RA2: as above
	if (squad.length() == 0) squad = (char *)user.squadabbrev; // RA2: the abbreviation stands for a squad without a name
	if (squad.length() > 0) {
		text += L": " + Wstring_ToWide(squad); // RA2: as above
	}
	if (g_UserLocales.contains(name)) { // RA2: as above
		int * locale = 0;
		if (g_pChat && g_UserLocales.getPointer(name, &locale) && locale && *locale) { // RA2: as above
			text += L" : " + Get_Locale_Name(*locale); // RA2: as above
		}
	}
	return(text);
}

WideWstring Get_Mission_Description(Wstring const & filename) // RA2: 0x0073F550, the description of the scenario of a file name, a fixed text for the random map and none for an unknown file
{
	if (stricmp(filename.text(), "RandMap.Sed") == 0) { // RA2: as above
		return(WideWstring(StringTable::LoadString("TXT_RANDOM_MAP_DESCRIPTION", NULL, "D:\\Ra2\\wonline.cpp", 2705))); // RA2: as above
	}
	for (int index = 0; index < Session.Scenarios.Count(); index++) { // RA2: as above
		MultiMission * mission = Session.Scenarios[index]; // RA2: as above
		if (stricmp(filename.text(), mission->Get_Filename()) == 0) { // RA2: as above
			return(WideWstring(mission->Description())); // RA2: as above
		}
	}
	return(WideWstring(L"")); // RA2: as above
}

WideWstring Game_List_Tooltip(HWND list, int row) // RA2: 0x0073F5F0, the tooltip of a row of the game list: the host as User_Tooltip makes it, the ping and the description of the scenario
{
	if (row == -1) { // RA2: as above
		return(WideWstring(StringTable::LoadString("STT:LobbyListGames", NULL, "D:\\Ra2\\wonline.cpp", 2722))); // RA2: as above
	}
	Channel channel;
	memset(&channel, 0, sizeof(channel));
	int index = SendMessageA(list, LB_GETITEMDATA, row, 0);
	if (index >= 0 && index < g_GameChannelList.length()) { // RA2: as above
		g_GameChannelList.get(channel, index);
		Wstring host((char *)channel.name);
		host.truncate('\''); // RA2: the host name ends before the first quote
		User user;
		memset(&user, 0, sizeof(user));
		strncpy((char *)user.name, host.get(), 10);
		WideWstring tooltip = User_Tooltip(user);
		tooltip += WideWstring(L", ");
		wchar_t ping_text[64];
		if (channel.latency != -1) {
			swprintf(ping_text, StringTable::LoadString("MSG:PingInfo", NULL, "D:\\Ra2\\wonline.cpp", 2756), channel.latency);
			tooltip += ping_text;
		} else {
			tooltip += StringTable::LoadString("TXT_UNKNOWN_PING", NULL, "D:\\Ra2\\wonline.cpp", 2759);
		}
		char options[132];
		strncpy(options, (char *)channel.exInfo, 128);
		int build = 0;
		int hash = 0;
		int ai_count = 0;
		int observer = 0;
		int not_full = 0;
		Wstring scenario;
		Parse_Channel_ExInfo(options, &build, &hash, &ai_count, &observer, &not_full, &scenario);
		if (scenario.length() > 0) {
			WideWstring description = Get_Mission_Description(scenario);
			if (description.length() > 0) {
				tooltip += L", ";
				tooltip += description;
			}
		}
		return(tooltip);
	}
	return(WideWstring(StringTable::LoadString("STT:LobbyListGames", NULL, "D:\\Ra2\\wonline.cpp", 2729))); // RA2: as above
}

WideWstring User_List_Tooltip(HWND list, int row) // RA2: 0x0073F850, the tooltip of a row of the user list: User_Tooltip of the user
{
	if (row == -1) { // RA2: as above
		return(WideWstring(StringTable::LoadString("STT:LobbyListUsers", NULL, "D:\\Ra2\\wonline.cpp", 2785))); // RA2: as above
	}
	int index = SendMessageA(list, LB_GETITEMDATA, row, 0);
	if (index >= 0 && index < g_UserList.length()) { // RA2: as above
		User user;
		memset(&user, 0, sizeof(user));
		g_UserList.get(user, index);
		return(User_Tooltip(user));
	}
	return(WideWstring(StringTable::LoadString("STT:LobbyListUsers", NULL, "D:\\Ra2\\wonline.cpp", 2790))); // RA2: as above
}

void Refresh_Game_List(void) // RA2: 0x0073F8F0, fills the game list of dialog 0x113 from the game channels: name with the players, kind, password, observer and ping cells, the selection and the first visible row kept
{
	HWND dialog = WS_Find_Dialog(0x113);
	if (!dialog) return; // RA2: as above
	HWND list = GetDlgItem(dialog, 0x6EE);
	if (!list) return; // RA2: as above

	Wstring selected;
	int selected_row = SendMessageA(list, LB_GETCURSEL, 0, 0);
	if (selected_row != -1) Get_Window_Item_Text(list, selected_row, selected);
	Wstring top;
	int top_row = SendMessageA(list, LB_GETTOPINDEX, 0, 0);
	if (top_row != -1) Get_Window_Item_Text(list, top_row, top);
	SendMessageA(list, OD_DISABLEPAINT, 0, 1);
	g_GameListRefreshing = true; // RA2: as above
	SendMessageA(list, LB_RESETCONTENT, 0, 0);

	int count = g_GameChannelList.length();
	for (int i = 0; i < count; ++i) {
		Channel channel;
		memset(&channel, 0, sizeof(channel));
		g_GameChannelList.get(channel, i);
		if (Is_Channel_Flag_11(&channel) || Is_Channel_Flag_12(&channel)) continue; // RA2: as above
		char options[132];
		strncpy(options, (char *)channel.exInfo, 128);
		int build = 0;
		int hash = 0;
		int ai_count = 0;
		int observer = 0;
		int not_full = 0;
		Wstring scenario;
		Parse_Channel_ExInfo(options, &build, &hash, &ai_count, &observer, &not_full, &scenario);
		int participants = channel.currentUsers - observer + ai_count;
		int capacity = channel.maxUsers - ((observer || not_full) ? 1 : 0);
		wchar_t title[80];
		swprintf(title, L"%hs %d/%d", channel.name, participants, capacity); // RA2: as above
		int row = SendMessageA(list, OD_INSERTSTRING, -1, (LPARAM)channel.name);
		SendMessageA(list, LB_SETITEMDATA, row, i);

		OwnerDraw::CellData cell;
		cell.type = OwnerDraw::CellData::TEXT;
		cell.string.set((unsigned short *)title);
		cell.hint.set((unsigned short *)L"");
		cell.color = ColorSystem;
		if (ObserverMode) { // RA2: a game that takes no observer, or has one, cannot be joined as an observer
			if (!not_full || observer) cell.color = ColorNoJoin;
		} else if (participants == capacity) {
			cell.color = ColorNoJoin;
		}
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_GameName, row), (LPARAM)&cell);

		cell.type = OwnerDraw::CellData::INVALID;
		cell.hint.set((unsigned short *)L"");
		cell.type = OwnerDraw::CellData::SURFACE;
		cell.hint.set((unsigned short *)L"");
		if (Is_Channel_Flag_8(&channel)) { // RA2: a battle clan game
			cell.surf = SurfaceCache.GetSurface("wolclan.pcx", false);
			cell.hint.set(StringTable::LoadString("TXT_BATTLECLAN_GAME", NULL, "D:\\Ra2\\wonline.cpp", 2903));
		} else if (Is_Channel_Tournament(&channel)) {
			cell.surf = SurfaceCache.GetSurface("woltrny.pcx", false);
			cell.hint.set(StringTable::LoadString("TXT_TOURNAMENT_GAME", NULL, "D:\\Ra2\\wonline.cpp", 2908));
		} else {
			cell.surf = SurfaceCache.GetSurface("gt18.pcx", false);
			cell.hint.set((unsigned short *)L"");
		}
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_GameIcon, row), (LPARAM)&cell);

		cell.type = OwnerDraw::CellData::SURFACE;
		cell.hint.set((unsigned short *)L"");
		if (channel.flags & 0x100) {
			cell.surf = SurfaceCache.GetSurface("wolpriv.pcx", false);
			cell.hint.set(StringTable::LoadString("TXT_GAME_PASSWORD", NULL, "D:\\Ra2\\wonline.cpp", 2922));
		} else {
			cell.type = OwnerDraw::CellData::INVALID;
			cell.hint.set((unsigned short *)L"");
		}
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_GamePrivate, row), (LPARAM)&cell);

		cell.type = OwnerDraw::CellData::INVALID;
		cell.string.set((unsigned short *)L"");
		cell.hint.set((unsigned short *)L"");
		if (channel.reserved & 0x600) {
			if (!observer && not_full) {
				cell.type = OwnerDraw::CellData::SURFACE;
				cell.surf = SurfaceCache.GetSurface("wolrslkob.pcx", false);
			} else {
				cell.type = OwnerDraw::CellData::SURFACE;
				cell.surf = SurfaceCache.GetSurface("wolreslk.pcx", false);
			}
		} else if (!observer && not_full) {
			cell.type = OwnerDraw::CellData::SURFACE;
			cell.surf = SurfaceCache.GetSurface("wolob.pcx", false);
		}
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_GameObserver, row), (LPARAM)&cell);

		if (channel.latency == -1) {
			cell.type = OwnerDraw::CellData::TEXT;
			cell.string.set((unsigned short *)L"??");
		} else {
			cell.pingtime = channel.latency;
			cell.type = OwnerDraw::CellData::PING;
		}
		swprintf(title, StringTable::LoadString("WOL:GamePing", NULL, "D:\\Ra2\\wonline.cpp", 2962), channel.latency);
		cell.hint.set((unsigned short *)title);
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_GamePingTime, row), (LPARAM)&cell);
	}

	int restored = SendMessageA(list, OD_FINDSTRING, 0, (LPARAM)selected.text());
	SendMessageA(list, LB_SETCURSEL, restored, 0);
	int restored_top = SendMessageA(list, OD_FINDSTRING, 0, (LPARAM)top.text());
	if (restored_top != -1) SendMessageA(list, LB_SETTOPINDEX, restored_top, 0);
	SendMessageA(list, OD_DISABLEPAINT, 0, 0);
	InvalidateRect(list, NULL, false);
	if (dialog == WS_Top_Window()) SetFocus(GetDlgItem(dialog, 0x540));
}

/// <summary>
/// Shows the World Domination Tour "select country/locale" dialog and waits for it to close,
/// then restores the given window.
/// </summary>
/// <param name="window">Window to restore (show normally) once the dialog closes.</param>
/// <returns>Result code returned by the dialog (see WS_Wait_Dialog/WS_Destroy_Dialog).</returns>
/// <param name="event">Event bit(s) to clear from the pending wait mask.</param>
void Close_Wait_Window(unsigned int event) // RA2: 0x0074D780, the focus goes back to the top dialog
{
	HWND win = WS_Find_Dialog(196); // RA2: as above
	if (win) {
		g_ActiveWaitFlags &= ~event;
		if (g_ActiveWaitFlags == 0) {
			WS_Destroy_Dialog(win, 0);
		}
	}
	HWND top = WS_Top_Window(); // RA2: as above
	HWND control = GetDlgItem(top, 1344); // RA2: as above
	if (control) { // RA2: as above
		SetFocus(control); // RA2: as above
	}
}

/// <summary>
/// Clears all World Domination Tour sound category lists and marks them as uninitialized.
/// </summary>
void Reset_Wait_Events(void) // RA2: 0x00752930, not in TibSun
{
	for (int i = 0; i < EV_COUNT; i++) {
		ResetEvent(g_WaitEventHandles[i]);
	}
}
