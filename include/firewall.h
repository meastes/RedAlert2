/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

/*
**	RA2: the firewall detection settings (the global at 0xA71B18); only what session.cpp uses.
*/
class FirewallHelperClass
{
	public:
		void Read_Settings(void); // RA2: 0x005BA280, reads the detection result from RA2.INI

		int Detected; // RA2: +0x00
		int Settings; // RA2: +0x04
		int Delta; // RA2: +0x08
		int ReadDelta; // RA2: +0x0C
		int Port; // RA2: +0x10, the port pool start
};
