/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

/*
**	RA2: talks to the packet router that relays the packets of an internet game.
*/
class OwnerTalkClass
{
	public:
		unsigned char Mask_From_ID(int id);
		bool Send_Player_List(void); // RA2: 0x0063F130, sends the connection list to the packet router; false if it fails
		void Build_Connection_List(void); // RA2: 0x0063F2A0, fills the connection list from the session players
		bool Start_Router(unsigned char * node); // RA2: 0x0063F510, starts the packet router process (the caller passes NULL)
		void Send_Shutdown(void); // RA2: 0x0063FC90; tells the packet router to shut down (Disconnect_Gracefully)
		bool Wait_For_Router_Shutdown(unsigned long timeout, bool force); // RA2: 0x0063FD70

		char Unknown_00[0x1C]; // RA2: +0x00 to +0x1B, not reconstructed
		void * RouterProcess; // RA2: +0x1C, the packet router's process handle; non-NULL while a router runs (0x00525DC0)
};

extern OwnerTalkClass OwnerTalk;
