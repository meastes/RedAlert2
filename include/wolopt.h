// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// RA2: the options of Westwood Online kept in the registry, as the profile code reads them
#pragma once

class WOLOptionsClass
{
	public:
		WOLOptionsClass(void) : Options(0), IsValid(false) { Load(); } // RA2: 0x007395D0 clears both members and calls Load
		bool Load(void); // RA2: 0x004FF200, reads the options value from the registry
		bool Is_Bit1_Clear(void); // RA2: 0x004FF2C0, whether bit 1 of Options is clear (the profile code saves a persona's section only then)

		unsigned long Options; // RA2: +0x00
		bool IsValid; // RA2: +0x04, set when Load found the value
};
