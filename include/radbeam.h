// SPDX-License-Identifier: GPL-3.0-or-later
// RA2: not in TibSun. The rad / chrono beam; only the functions TechnoClass uses are declared (names from
// YRpp's RadBeam.h).
#pragma once

#include "coord.h"
#include "rgb.h"

class RadBeam
{
	public:
		static RadBeam * __fastcall Allocate(int type); // RA2: 0x635440; 1 the rad beam (10 frames), else 20
		void Set_Color(RGBClass const & color); // RA2: 0x635710
		void Set_Depth(int depth); // RA2: 0x635730
		void Set_Source(Coord const & coord); // RA2: 0x635740
		void Set_Target(Coord const & coord); // RA2: 0x635760
		void Set_Period(int period); // RA2: 0x635780
		void Set_Amplitude(double amplitude); // RA2: 0x635790
		void Set_Phase(double phase); // RA2: 0x6357B0; +0x40, 0.0 for the eruption
		void Set_Start_Delay(int delay); // RA2: 0x6357D0; +0x48, a random 0-20 for the eruption
};
