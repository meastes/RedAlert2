/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

#include "coord.h"
#include "rect.h"

class BeaconClass;
class Surface;

/*
**	RA2: the beacons that players place on the map in a multiplayer game.
*/
class BeaconManagerClass
{
	public:
		BeaconClass * FindBeaconAt(Coord coord);
		bool CanPlaceBeacon(int owner); // RA2: 0x0042E700; the house may place another beacon (YR: CanPlaceBeacon)
		BeaconClass * FindSelected(int owner); // RA2: 0x0042E940; the house's beacon that is being edited, if any
		void EditBeaconMessage(wchar_t const * text, int a, int b, bool done); // RA2: 0x0042EC20; text being typed for the beacon, or none to stop editing
		void Draw_On_Radar(Surface * surface, Rect bounds); // RA2: 0x0042EED0; marks the beacons of allies on the radar
		void Remove_Beacon(int x, int y); // RA2: 0x0042E990; removes a beacon; Command_Delete_Plan_Or_Beacon calls it with -1 and -1 when no planned node is selected
		bool Is_Flashing(void); // RA2: 0x0042EFD0; a visible beacon of an ally is in its lit phase
};

extern BeaconManagerClass BeaconManager;
