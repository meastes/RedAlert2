/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/*
**	RA2: the kamikaze tracker: the missiles launched by spawn managers fly at a target; the tracker keeps
**	each one with the cell it is sent at and orders it there again every 30 frames. TibSun has no such file;
**	the code is the executable's from 0x00531360.
*/

#define INCLUDE_COM // RA2: as above
#include "geometry.h" // RA2: the geometry constants come before the rectangle of rect.h (the executable initializes them first)
#include "always.h" // RA2: the includes this file needs
#include "aircraft.h" // RA2: the includes this file needs
#include "airctype.h" // RA2: the includes this file needs
#include "cell.h" // RA2: the includes this file needs
#include "inline.h" // RA2: the includes this file needs
#include "globals.h" // RA2: the includes this file needs
#include "kamikaze.h" // RA2: the includes this file needs
#include "mission.h" // RA2: the includes this file needs
#include "swizzle.h" // RA2: the includes this file needs
#include "_map.h" // RA2: the includes this file needs
#include "techno.h" // RA2: the includes this file needs
#include <new.h> // RA2: the includes this file needs


KamikazeClass Kamikazes; // RA2: 0x00A6E428; the tracker of the missiles flying at their targets (initializer 0x00531550, destructor 0x005315C0)


/// <summary>
/// Makes an empty tracker whose timer runs 100 frames.
/// </summary>
KamikazeClass::KamikazeClass(void) // RA2: 0x00531650
{
	Timer = 100; // RA2: the first update comes after 100 frames
}


/// <summary>
/// Tracks a spawned missile flying at the target's cell, or at the cell ahead of it when it has no target;
/// an aircraft that is not a missile just crashes.
/// </summary>
void KamikazeClass::Add(AircraftClass * missile, AbstractClass * target) // RA2: 0x005316A0
{
	if (!missile->Class->MissileSpawn) { // RA2: only spawned missiles are tracked
		missile->Crash(0); // RA2: any other aircraft is crashed at once
		return; // RA2: as above
	}
	KamikazeControl * node = new KamikazeControl; // RA2: the record of the missile
	node->Unit = missile; // RA2: as above
	if (target == NULL) { // RA2: without a target the missile flies on straight ahead
		FacingType facing = missile->PrimaryFacing.Current().As_Dir8(); // RA2: the cell is the neighbour in the missile's facing
		CellClass * here = (CellClass *)missile->Get_Cell_Ptr(); // RA2: as above
		node->Cell = &here->Adjacent_Cell(facing); // RA2: as above
	} else {
		node->Cell = &Map[target->Center_Coord().As_Cell()]; // RA2: the cell under the target
	}
	missile->IsKamikaze = true; // RA2: the missile is gone after it fired
	missile->Ammo = 1; // RA2: as above
	Nodes.Add(node); // RA2: as above
}


/// <summary>
/// Every 30 frames each tracked missile is sent (again) at its cell, or at the cell ahead of it when it has none.
/// </summary>
void KamikazeClass::Update(void) // RA2: 0x005317C0
{
	if (Timer == 0) { // RA2: only when the timer has run out
		Timer = 30; // RA2: as above
		for (int index = 0; index < Nodes.Count(); index++) { // RA2: as above
			KamikazeControl * node = Nodes[index]; // RA2: as above
			CellClass * cell = node->Cell; // RA2: as above
			AircraftClass * missile = node->Unit; // RA2: as above
			missile->Ammo = 1; // RA2: the missile keeps its one shot
			if (cell == NULL) { // RA2: a record without a cell
				FacingType facing = missile->PrimaryFacing.Current().As_Dir8(); // RA2: flies at the neighbour in its facing
				CellClass * here = (CellClass *)missile->Get_Cell_Ptr(); // RA2: as above
				CellClass * adjacent = &here->Adjacent_Cell(facing); // RA2: as above
				missile->Assign_Target(adjacent); // RA2: as above
				missile->QueueMission(MISSION_ATTACK, false); // RA2: as above
			} else {
				missile->Assign_Target(cell); // RA2: the missile is sent at its cell
				missile->QueueMission(MISSION_ATTACK, false); // RA2: as above
			}
		}
	}
}


/// <summary>
/// A missile that is gone leaves the list; missiles flying at a cell that is detached are sent at the cell
/// under the target instead.
/// </summary>
void KamikazeClass::Remove(AbstractClass * target) // RA2: 0x00531880
{
	for (int index = Nodes.Count() - 1; index >= 0; index--) { // RA2: searched from the end
		KamikazeControl * node = Nodes[index]; // RA2: as above
		if (target == node->Unit) { // RA2: the missile itself is gone
			delete Nodes[index]; // RA2: as above
			Nodes.Delete_Index(index); // RA2: as above
			return; // RA2: as above
		}
		if (target == node->Cell) { // RA2: the cell it flies at is gone
			CellClass * cell = &Map[target->Center_Coord().As_Cell()]; // RA2: the cell under the target
			Nodes[index]->Unit->Assign_Target(cell); // RA2: as above
			Nodes[index]->Unit->QueueMission(MISSION_ATTACK, false); // RA2: as above
			Nodes[index]->Cell = cell; // RA2: as above
			return; // RA2: as above
		}
	}
}


/// <summary>
/// Deletes every record and gives the list's space back, as the list's own destructor would.
/// </summary>
KamikazeClass::~KamikazeClass(void) // RA2: 0x00531980
{
	for (int index = Nodes.Count() - 1; index >= 0; index--) { // RA2: as above
		delete Nodes[index]; // RA2: as above
	}
	DynamicVectorClass<KamikazeControl *> & nodes = Nodes; // RA2: the executable works on the list through a reference
	int length = nodes.Length(); // RA2: it clears the list and gives its space back at once
	nodes.Clear(); // RA2: as above
	nodes.Resize(length); // RA2: as above
}


/// <summary>
/// Drops every tracked missile and restarts the timer at 1.
/// </summary>
void KamikazeClass::Clear(void) // RA2: 0x005319E0
{
	for (int index = Nodes.Count() - 1; index >= 0; index--) { // RA2: as above
		delete Nodes[index]; // RA2: as above
	}
	DynamicVectorClass<KamikazeControl *> & nodes = Nodes; // RA2: as in the destructor
	int length = nodes.Length(); // RA2: as above
	nodes.Clear(); // RA2: as above
	nodes.Resize(length); // RA2: as above
	Timer = 1; // RA2: the first update comes at the next frame
}


/// <summary>
/// Saves the number of records, then each record as it is.
/// </summary>
HRESULT KamikazeClass::Save(IStream * stream) // RA2: 0x00531A40
{
	int count = Nodes.Count(); // RA2: the count comes first
	HRESULT result = stream->Write(&count, sizeof(count), 0); // RA2: as above
	if (SUCCEEDED(result)) { // RA2: as above
		for (int index = 0; index < count; index++) { // RA2: as above
			result = stream->Write(Nodes[index], sizeof(KamikazeControl), 0); // RA2: the record bytes, pointers unswizzled
			if (FAILED(result)) { // RA2: as above
				break; // RA2: as above
			}
		}
	}
	return result; // RA2: as above
}


/// <summary>
/// Loads the records written by Save and swizzles their pointers.
/// </summary>
HRESULT KamikazeClass::Load(IStream * stream) // RA2: 0x00531AA0
{
	int count; // RA2: the number of records
	HRESULT result = stream->Read(&count, sizeof(count), 0); // RA2: as above
	if (SUCCEEDED(result)) { // RA2: as above
		for (int index = 0; index < count; index++) { // RA2: as above
			KamikazeControl * node = new KamikazeControl; // RA2: as above
			result = stream->Read(node, sizeof(KamikazeControl), 0); // RA2: as above
			if (FAILED(result)) { // RA2: as above
				return result; // RA2: as above
			}
			Swizzler.Swizzle((void **)&node->Unit); // RA2: both pointers are saved as ids
			Swizzler.Swizzle((void **)&node->Cell); // RA2: as above
			Nodes.Add(node); // RA2: as above
		}
	}
	return result; // RA2: as above
}
