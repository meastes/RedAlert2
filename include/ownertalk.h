/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

#include "vector.h"
#include "ipxaddr.h"

/*
**	RA2: talks to the packet router that relays the packets of an internet game.
*/
class OwnerTalkClass
{
	public:
		void Reset(void); // RA2: 0x0063F070, clears the connection state before a network session starts (Start_Network_Session)
		unsigned char Mask_From_ID(int id);
		char * Name_From_Index(int index); // RA2: 0x0063FB40, the name of the connection at an index (YR's name)
		IPXAddressClass * Address_From_Name(char const * name); // RA2: 0x0063FA00, the address of the connection of that name (YR's name)
		unsigned short Port_From_Name(char const * name); // RA2: 0x0063FAF0, its port (YR's name)
		bool Send_Player_List(void); // RA2: 0x0063F130, sends the connection list to the packet router; false if it fails
		void Build_Connection_List(void); // RA2: 0x0063F2A0, fills the connection list from the session players
		bool Start_Router(unsigned char * node); // RA2: 0x0063F510, starts the packet router process (the caller passes NULL)
		void Send_Shutdown(void); // RA2: 0x0063FC90; tells the packet router to shut down (Disconnect_Gracefully)
		bool Wait_For_Router_Shutdown(unsigned long timeout, bool force); // RA2: 0x0063FD70

		DynamicVectorClass<void *> Connections; // RA2: +0x00, the connections the router relays (0x18 bytes)
		bool Unknown_18; // RA2: +0x18
		char Unknown_19[3]; // RA2: +0x19 to +0x1B, padding
		void * RouterProcess; // RA2: +0x1C, the packet router's process handle; non-NULL while a router runs (0x00525DC0)
};

extern OwnerTalkClass OwnerTalk;
