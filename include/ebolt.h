// SPDX-License-Identifier: GPL-3.0-or-later
// RA2: not in TibSun. The electric bolt drawn by IsElectricBolt weapons; only the members and functions
// TechnoClass uses are declared (names from YRpp's EBolt.h).
#pragma once

#include "coord.h"

class UnitClass;
class TechnoClass;

class EBolt
{
	public:
		Coord Point1;
		Coord Point2;
		int Unknown_18;
		int Random;
		TechnoClass * Owner;
		int WeaponSlot;
		int Lifetime;
		bool AlternateColor; // RA2: +0x2C

		EBolt(void); // RA2: 0x4C1E10 in YR
		void Set_Owner(TechnoClass * owner, int which); // RA2: 0x4B2580
		void Fire(Coord source, Coord target, int zadjust); // RA2: 0x4B2430
};
