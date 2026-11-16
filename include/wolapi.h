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


#pragma once

// RA2: only the part of the WOL API that netshare.cpp uses, in a flat header; the rest of TibSun's wolapi.h is not included

#include <windows.h>
#include <ole2.h>


typedef 
enum Locale
    {	LOC_UNKNOWN	= 0,
	LOC_OTHER	= LOC_UNKNOWN + 1,
	LOC_USA	= LOC_OTHER + 1,
	LOC_CANADA	= LOC_USA + 1,
	LOC_UK	= LOC_CANADA + 1,
	LOC_GERMANY	= LOC_UK + 1,
	LOC_FRANCE	= LOC_GERMANY + 1,
	LOC_SPAIN	= LOC_FRANCE + 1,
	LOC_NETHERLANDS	= LOC_SPAIN + 1,
	LOC_BELGIUM	= LOC_NETHERLANDS + 1,
	LOC_AUSTRIA	= LOC_BELGIUM + 1,
	LOC_SWITZERLAND	= LOC_AUSTRIA + 1,
	LOC_ITALY	= LOC_SWITZERLAND + 1,
	LOC_DENMARK	= LOC_ITALY + 1,
	LOC_SWEDEN	= LOC_DENMARK + 1,
	LOC_NORWAY	= LOC_SWEDEN + 1,
	LOC_FINLAND	= LOC_NORWAY + 1,
	LOC_ISRAEL	= LOC_FINLAND + 1,
	LOC_SOUTH_AFRICA	= LOC_ISRAEL + 1,
	LOC_JAPAN	= LOC_SOUTH_AFRICA + 1,
	LOC_SOUTH_KOREA	= LOC_JAPAN + 1,
	LOC_CHINA	= LOC_SOUTH_KOREA + 1,
	LOC_SINGAPORE	= LOC_CHINA + 1,
	LOC_TAIWAN	= LOC_SINGAPORE + 1,
	LOC_MALAYSIA	= LOC_TAIWAN + 1,
	LOC_AUSTRALIA	= LOC_MALAYSIA + 1,
	LOC_NEW_ZEALAND	= LOC_AUSTRALIA + 1,
	LOC_BRAZIL	= LOC_NEW_ZEALAND + 1,
	LOC_THAILAND	= LOC_BRAZIL + 1,
	LOC_ARGENTINA	= LOC_THAILAND + 1,
	LOC_PHILIPPINES	= LOC_ARGENTINA + 1,
	LOC_GREECE	= LOC_PHILIPPINES + 1,
	LOC_IRELAND	= LOC_GREECE + 1,
	LOC_POLAND	= LOC_IRELAND + 1,
	LOC_PORTUGAL	= LOC_POLAND + 1,
	LOC_MEXICO	= LOC_PORTUGAL + 1,
	LOC_RUSSIA	= LOC_MEXICO + 1,
	LOC_TURKEY	= LOC_RUSSIA + 1
    }	Locale;

struct  Ladder
    {
    unsigned int sku;
    unsigned int team_no;
    unsigned int wins;
    unsigned int losses;
    unsigned int points;
    unsigned int kills;
    unsigned int rank;
    unsigned int rung;
    unsigned int disconnects;
    unsigned int team_rung;
    unsigned int provisional;
    unsigned int last_game_date;
    unsigned int win_streak;
    unsigned int reserved1;
    unsigned int reserved2;
    struct Ladder *next;
    unsigned char login_name[ 40 ];
    Locale locale;
    };
typedef int GroupID;

struct  Server
    {
    int gametype;
    int chattype;
    int timezone;
    float longitude;
    float lattitude;
    struct Server *next;
    unsigned char name[ 71 ];
    unsigned char connlabel[ 5 ];
    unsigned char conndata[ 128 ];
    unsigned char login[ 10 ];
    unsigned char password[ 10 ];
    };
struct  Channel
    {
    int type;
    unsigned int minUsers;
    unsigned int maxUsers;
    unsigned int currentUsers;
    unsigned int official;
    unsigned int tournament;
    unsigned int ingame;
    unsigned int flags;
    unsigned long reserved;
    unsigned long ipaddr;
    int latency;
    int hidden;
    struct Channel *next;
    unsigned char name[ 17 ];
    unsigned char topic[ 81 ];
    unsigned char location[ 65 ];
    unsigned char key[ 9 ];
    unsigned char exInfo[ 41 ];
    };
struct  User
    {
    unsigned int flags;
    GroupID group;
    unsigned long reserved;
    unsigned long reserved2;
    unsigned long reserved3;
    unsigned long squadID;
    unsigned long ipaddr;
    unsigned long squad_icon;
    struct User *next;
    unsigned char name[ 10 ];
    unsigned char squadname[ 41 ];
    unsigned char squadabbrev[ 10 ];
    Locale locale;
    int team;
    };
struct  Group
    {
    GroupID ident;
    int type;
    unsigned int members;
    struct Group *next;
    unsigned char name[ 65 ];
    };
struct  Squad
    {
    unsigned long id;
    int sku;
    int members;
    int color1;
    int color2;
    int color3;
    int icon1;
    int icon2;
    int icon3;
    struct Squad *next;
    int rank;
    int team;
    int status;
    unsigned char email[ 81 ];
    unsigned char icq[ 17 ];
    unsigned char motto[ 81 ];
    unsigned char url[ 129 ];
    unsigned char name[ 41 ];
    unsigned char abbreviation[ 41 ];
    };
struct  Update
    {
    unsigned long SKU;
    unsigned long version;
    int required;
    struct Update *next;
    unsigned char server[ 65 ];
    unsigned char patchpath[ 256 ];
    unsigned char patchfile[ 33 ];
    unsigned char login[ 33 ];
    unsigned char password[ 65 ];
    unsigned char localpath[ 256 ];
    };
typedef struct Server Server;

typedef struct Channel Channel;

typedef struct User User;

typedef struct Group Group;

typedef struct Update Update;

typedef struct Ladder Ladder;

typedef struct Squad Squad;

    struct __declspec(uuid("4DD3BAF4-7579-11D1-B1C6-006097176556")) IChat : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PumpMessages( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestServerList( 
            /* [in] */ unsigned long SKU,
            /* [in] */ unsigned long current_version,
            /* [in] */ LPCSTR loginname,
            /* [in] */ LPCSTR password,
            /* [in] */ int timeout) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestConnection( 
            /* [in] */ Server *server,
            /* [in] */ int timeout,
            int domangle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelList( 
            /* [in] */ int channelType,
            /* [in] */ int autoping) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelCreate( 
            /* [in] */ Channel *channel) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelJoin( 
            /* [in] */ Channel *channel) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelLeave( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserList( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPublicMessage( 
            /* [in] */ LPCSTR message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPrivateMessage( 
            /* [in] */ User *users,
            /* [in] */ LPCSTR message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestLogout( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPrivateGameOptions( 
            /* [in] */ User *users,
            /* [in] */ LPCSTR options) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPublicGameOptions( 
            /* [in] */ LPCSTR options) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPublicAction( 
            /* [in] */ LPCSTR action) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPrivateAction( 
            /* [in] */ User *users,
            /* [in] */ LPCSTR action) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestGameStart( 
            /* [in] */ User *users) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelTopic( 
            /* [in] */ LPCSTR topic) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVersion( 
            /* [in] */ unsigned long *version) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserKick( 
            /* [in] */ User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserIP( 
            /* [in] */ User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetGametypeInfo( 
            unsigned int gtype,
            int icon_size,
            unsigned char *__RPC_FAR *bitmap,
            int *bmp_bytes,
            LPCSTR *name,
            LPCSTR *URL) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestFind( 
            User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPage( 
            User *user,
            LPCSTR message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetFindPage( 
            int findOn,
            int pageOn) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSquelch( 
            User *user,
            int squelch) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSquelch( 
            User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetChannelFilter( 
            int channelType) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestGameEnd( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetLangFilter( 
            int onoff) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelBan( 
            LPCSTR name,
            int ban) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetGametypeList( 
            LPCSTR *list) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetHelpURL( 
            LPCSTR *url) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetProductSKU( 
            unsigned long SKU) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNick( 
            int num,
            LPCSTR *nick,
            LPCSTR *pass) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetNick( 
            int num,
            LPCSTR nick,
            LPCSTR pass,
            int domangle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetLobbyCount( 
            int *count) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestRawMessage( 
            LPCSTR ircmsg) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAttributeValue( 
            LPCSTR attrib,
            LPCSTR *value) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetAttributeValue( 
            LPCSTR attrib,
            LPCSTR value) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetChannelExInfo( 
            LPCSTR info) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE StopAutoping( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestSquadInfo( 
            unsigned long id) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestSetTeam( 
            int team) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestSetLocale( 
            Locale locale) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserLocale( 
            User *users) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserTeam( 
            User *users) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNickLocale( 
            int nicknum,
            Locale *locale) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetNickLocale( 
            int nicknum,
            Locale locale) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetLocaleString( 
            LPCSTR *loc_string,
            Locale locale) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetLocaleCount( 
            int *num) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetClientVersion( 
            unsigned long version) = 0;
        
    };
