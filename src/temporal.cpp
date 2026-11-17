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
**	RA2: the temporal weapon link of the chrono legionnaire: while its owner fires, the
**	target is held (and later erased) by the warp; several owners at one target form a
**	chain of links that add their warp rates. TibSun has no such file; the code is the
**	executable's from 0x006E1E70.
*/

#define INCLUDE_COM // RA2: the includes this file needs
#include "always.h" // RA2: as above
#include "temporal.h" // RA2: as above
#include "crc.h" // RA2: as above
#include "globals.h" // RA2: as above
#include "noinit.h" // RA2: as above
#include "swizzle.h" // RA2: as above
#include "techno.h" // RA2: as above
#include "_rules.h" // RA2: as above
#include "anim.h" // RA2: as above
#include "foot.h" // RA2: as above
#include "building.h" // RA2: as above
#include "house.h" // RA2: as above
#include "rules.h" // RA2: as above
#include "super.h" // RA2: as above
#include "techtype.h" // RA2: as above
#include "weapon.h" // RA2: as above
#include "cell.h" // RA2: as above
#include "unit.h" // RA2: as above
#include "unittype.h" // RA2: as above
#include "spawnmgr.h" // RA2: as above
#include "capmgr.h" // RA2: as above
#include "revent.h" // RA2: as above
#include "vox.h" // RA2: as above

DynamicVectorClass<TemporalClass *> Temporals; // RA2: 0x00AC02E0; every temporal link; its initializer and destructor come first in the module (0x006E1E70, 0x006E1EB0)


/// <summary>
/// Creates a temporal link with no owner.
/// </summary>
TemporalClass::TemporalClass(void) : // RA2: 0x006E1EF0
	BASECLASS(), // RA2: as above
	LifeTimer() // RA2: the timer starts at the current frame
{
	Temporals.Add(this); // RA2: the vector add is expanded in place (the capacity check and the store)
}


/// <summary>
/// Creates the temporal link of a techno.
/// The link holds no target yet and is entered in the list of all links.
/// </summary>
TemporalClass::TemporalClass(TechnoClass * owner) : // RA2: 0x006E1F80
	BASECLASS(), // RA2: the base constructor is called first
	LifeTimer() // RA2: as above
{
	Owner = owner; // RA2: the owner argument is stored before the target is cleared
	Target = NULL; // RA2: as above
	LifeTimer = CDTimerClass<FrameTimerClass>(); // RA2: the executable starts the timer a second time through a temporary
	NextTemporal = NULL; // RA2: as above
	PrevTemporal = NULL; // RA2: as above
	Temporals.Add(this); // RA2: the link is entered in the list after the chain pointers and before the warp values are cleared
	WarpRemaining = 0; // RA2: as above
	WarpPerStep = 0; // RA2: as above
	Unused_38 = NULL; // RA2: as above
	SourceSW = NULL; // RA2: as above
}


/// <summary>
/// Creates a temporal link that is about to be loaded.
/// </summary>
TemporalClass::TemporalClass(NoInitClass const & x) : // RA2: 0x006E2040
	BASECLASS(x), // RA2: as above
	LifeTimer() // RA2: the timer is started again, not left alone, although the object is loaded
{
}


/// <summary>
/// Removes the temporal link from the list of all links.
/// </summary>
TemporalClass::~TemporalClass(void) // RA2: 0x006E2080
{
	Temporals.Delete(this); // RA2: the vector delete is expanded in place: it finds the link, shortens the list and moves the later entries down
}


/// <summary>
/// Adds the link to the checksum of the game state (it adds nothing of its own).
/// </summary>
void TemporalClass::Compute_CRC(CRCEngine & crc) const // RA2: 0x006E2100
{
	BASECLASS::Compute_CRC(crc); // RA2: the executable only calls the base class
}


/// <summary>
/// Loads the link and has its pointers swizzled once all objects exist.
/// </summary>
HRESULT STDMETHODCALLTYPE TemporalClass::Load(IStream * stream) // RA2: 0x006E2110
{
	HRESULT result = BASECLASS::Load(stream); // RA2: the base class loads the whole object
	if (SUCCEEDED(result)) { // RA2: as above
		new (this) TemporalClass(NoInitClass()); // RA2: the constructor is expanded in place (base class, timer, virtual tables) and the list of links is left alone
		Swizzle_Pointer(&Owner); // RA2: six pointers are swizzled, in the order Owner, Target, NextTemporal, PrevTemporal, Unused_38, SourceSW
		Swizzle_Pointer(&Target); // RA2: as above
		Swizzle_Pointer(&NextTemporal); // RA2: as above
		Swizzle_Pointer(&PrevTemporal); // RA2: as above
		Swizzle_Pointer(&Unused_38); // RA2: as above
		Swizzle_Pointer(&SourceSW); // RA2: as above
	}
	return(result); // RA2: as above
}


/// <summary>
/// Saves the link (the base class writes all of it).
/// </summary>
HRESULT STDMETHODCALLTYPE TemporalClass::Save(IStream * stream, BOOL cleardirty) // RA2: 0x006E21D0
{
	return(BASECLASS::Save(stream, cleardirty)); // RA2: only the base class is saved
}


/// <summary>
/// Returns the class identifier of the link.
/// </summary>
HRESULT STDMETHODCALLTYPE TemporalClass::GetClassID(CLSID * retval) // RA2: 0x006E21F0
{
	if (retval == NULL) { // RA2: a NULL pointer returns E_POINTER (0x80004003)
		return(E_POINTER); // RA2: as above
	}
	*retval = CLSID_TemporalClass; // RA2: copies the 16 bytes at 0x007A1E20, the class identifier
	return(S_OK); // RA2: as above
}


/// <summary>
/// Advances the warp of the held target by one frame.
/// A link that is not first in the chain only lets go. For the first link the warp
/// rate of the whole chain is taken off the time remaining, and when it runs out the
/// target is erased: the owner is credited with the kill, a building loses its
/// occupants and cargo, and the unit or building is deleted.
/// </summary>
void TemporalClass::AI(void) // RA2: 0x006E2230
{
	if (Target != NULL && Target->TemporalTargetingMe == this && NextTemporal != NULL) { // RA2: as above
		Target->TemporalTargetingMe = NULL; // RA2: as above
		Target->BeingWarpedOut = false; // RA2: as above
		Detach(); // RA2: call 0x006E2710, which lets go of the whole chain
		return; // RA2: as above
	}

	WarpRemaining -= GetWarpPerStep(0); // RA2: expanded in place for the first link; only the walk to the previous link is a call (0x006E2470)
	if (WarpRemaining > 0) { // RA2: as above
		return; // RA2: as above
	}

	if (Target == NULL) { // RA2: as above
		Target = NULL; // RA2: as above
		PrevTemporal = NULL; // RA2: as above
		NextTemporal = NULL; // RA2: as above
		SourceSW = NULL; // RA2: as above
		Unused_38 = NULL; // RA2: as above
	} else {
		new AnimClass(Rule->WarpAway, Target->Get_Coord(), 0, 1, ShapeFlags_Type(0x600), 0); // RA2: the warp-away animation (Rules WarpAway) at the target's coordinate, flags 0x600
	}

	if (Owner->Techno_Type_Class()->IsTrainable) { // RA2: a trainable owner (+0xA9A of its type) is credited with the kill, valued by the costs of both sides
		TechnoTypeClass const * target_type = Target->Techno_Type_Class(); // RA2: as above
		TechnoTypeClass const * owner_type = Owner->Techno_Type_Class(); // RA2: as above
		Owner->Veterancy.Made_A_Kill(owner_type->Cost_Of(Owner->House), target_type->Cost_Of(Target->House)); // RA2: as above
	}

	if (Target != NULL) { // RA2: as above
		if (Target->Fetch_RTTI() == RTTI_BUILDING) { // RA2: as above
			BuildingClass * building = (BuildingClass *)Target; // RA2: as above
			if (building != NULL) { // RA2: as above
				if (building->GetOccupantCount() > 0) { // RA2: as above
					building->KillOccupants(NULL); // RA2: as above
				}
				if (!ScenarioInit) { // RA2: as above
					while (building->Cargo.Is_Something_Attached()) { // RA2: as above
						FootClass * passenger = building->Cargo.Detach_Object(); // RA2: as above
						delete passenger; // RA2: as above
					}
				}
			}
			if (SourceSW != NULL) { // RA2: the superweapon that fired the warp (the Chronosphere) is released from holding
				SourceSW->Suspend(false); // RA2: as above
				SourceSW = NULL; // RA2: as above
			}
			Target->Death_Announcement(Owner); // RA2: as above
			Target->Record_The_Kill(Owner); // RA2: as above
			Target->Delete_Me(); // RA2: as above
			building->House->IsRecalcNeeded = true; // RA2: as above
		} else {
			Target->Death_Announcement(Owner); // RA2: as above
			Target->Record_The_Kill(Owner); // RA2: as above
			Target->Delete_Me(); // RA2: as above
		}
	}

	Target = NULL; // RA2: as above
	PrevTemporal = NULL; // RA2: as above
	NextTemporal = NULL; // RA2: as above
	SourceSW = NULL; // RA2: as above
	Unused_38 = NULL; // RA2: as above
}


/// <summary>
/// The warp this link and the links behind it take off the target per frame.
/// Each link in the chain adds the attack value of its owner's weapon; the walk
/// down the chain stops after 50 links.
/// </summary>
int TemporalClass::GetWarpPerStep(int helpercount) // RA2: 0x006E2470
{
	int helpers = 0; // RA2: the warp of the links behind this one
	if (PrevTemporal != NULL && helpercount <= 50) { // RA2: as above
		helpers = PrevTemporal->GetWarpPerStep(helpercount + 1); // RA2: the chain walk is cut off after 50 links
	}
	WarpPerStep = Owner->Get_Class_Weapon_Data(Owner->What_Weapon_Should_I_Use(NULL))->Weapon->Attack; // RA2: as above
	return(WarpPerStep + helpers); // RA2: as above
}


/// <summary>
/// An object is being removed from the game.
/// When it is the owner the link lets go of its target; when it is the target the
/// link forgets it.
/// </summary>
void TemporalClass::Detach_Techno(AbstractClass const * target) // RA2: 0x006E24C0
{
	if (target == Owner) { // RA2: the owner is the one removed
		if (Target != NULL) { // RA2: as above
			LetGo(); // RA2: call 0x006E2500
			Target = NULL; // RA2: as above
		}
	} else if (target == Target) { // RA2: the held target is the one removed
		Target = NULL; // RA2: as above
		PrevTemporal = NULL; // RA2: as above
		NextTemporal = NULL; // RA2: as above
		SourceSW = NULL; // RA2: as above
		Unused_38 = NULL; // RA2: as above
	}
}


/// <summary>
/// Lets the target go, taking the link out of the chain.
/// The last link of a chain hands the target to the one before it; the only link
/// frees the target (a building is powered again) and has it redrawn.
/// </summary>
void TemporalClass::LetGo(void) // RA2: 0x006E2500
{
	if (NextTemporal == NULL) { // RA2: as above
		if (PrevTemporal != NULL) { // RA2: the last link of a chain: the previous link takes over the target and the remaining time
			if (Target != NULL) { // RA2: as above
				Target->TemporalTargetingMe = PrevTemporal; // RA2: as above
			}
			PrevTemporal->NextTemporal = NULL; // RA2: as above
			PrevTemporal->WarpRemaining = WarpRemaining; // RA2: as above
		} else if (Target != NULL) { // RA2: the only link: the target is freed
			Target->TemporalTargetingMe = NULL; // RA2: as above
			Target->BeingWarpedOut = false; // RA2: as above
			if (Target->Fetch_RTTI() == RTTI_BUILDING) { // RA2: as above
				BuildingClass * building = (BuildingClass *)Target; // RA2: as above
				building->House->RecalcPower = true; // RA2: as above
				building->EnableTemporal(); // RA2: call 0x0044D380, the powered animations run again
				building->House->IsRecalcNeeded = true; // RA2: as above
			}
			Target->Mark(MARK_CHANGE); // RA2: virtual +0x11C, with the argument 2
		}
	} else {
		if (PrevTemporal != NULL) { // RA2: a link in the middle (or at the front) of a chain is cut out of it
			PrevTemporal->NextTemporal = NextTemporal; // RA2: as above
			if (NextTemporal != NULL) { // RA2: as above
				NextTemporal->PrevTemporal = PrevTemporal; // RA2: as above
			}
		} else {
			NextTemporal->PrevTemporal = NULL; // RA2: as above
		}
	}
	Target = NULL; // RA2: as above
	PrevTemporal = NULL; // RA2: as above
	NextTemporal = NULL; // RA2: as above
	SourceSW = NULL; // RA2: as above
	Unused_38 = NULL; // RA2: as above
}


/// <summary>
/// Drops the target without touching the chain (nothing in the executable calls it).
/// </summary>
void TemporalClass::Release_Target(void) // RA2: 0x006E25F0
{
	if (Target != NULL) { // RA2: as above
		Target->TemporalTargetingMe = NULL; // RA2: the target is told first (+0x20C), then its warp flag (+0x204)
		Target->BeingWarpedOut = false; // RA2: as above
		Target = NULL; // RA2: as above
	}
}


/// <summary>
/// Frees the target and has it redrawn, without touching the chain (nothing in the executable calls it).
/// </summary>
void TemporalClass::Restore_Target(void) // RA2: 0x006E2610
{
	if (Target != NULL) { // RA2: as above
		Target->TemporalTargetingMe = NULL; // RA2: as above
		Target->BeingWarpedOut = false; // RA2: as above
		if (Target->Fetch_RTTI() == RTTI_BUILDING) { // RA2: as above
			BuildingClass * building = (BuildingClass *)Target; // RA2: as above
			building->House->RecalcPower = true; // RA2: as above
			building->EnableTemporal(); // RA2: as above
			building->House->IsRecalcNeeded = true; // RA2: as above
		}
		Target->Mark(MARK_CHANGE); // RA2: as above
	}
}


/// <summary>
/// Does nothing (it has no callers).
/// </summary>
void TemporalClass::Empty(void) // RA2: 0x006E2670
{
}


/// <summary>
/// Lets go of the target and of both neighbours in the chain.
/// The neighbours are detached in turn, so the whole chain is released.
/// </summary>
void TemporalClass::JustLetGo(void) // RA2: 0x006E2680
{
	if (Target != NULL) { // RA2: here the warp flag is cleared before the target's link (the other way round than in LetGo)
		Target->BeingWarpedOut = false; // RA2: as above
		Target->TemporalTargetingMe = NULL; // RA2: as above
		Target = NULL; // RA2: as above
	}
	if (PrevTemporal != NULL) { // RA2: as above
		if (PrevTemporal->NextTemporal == this) { // RA2: as above
			PrevTemporal->NextTemporal = NULL; // RA2: as above
		}
		PrevTemporal->Detach(); // RA2: call 0x006E2710
		PrevTemporal = NULL; // RA2: as above
	}
	if (NextTemporal != NULL) { // RA2: as above
		if (NextTemporal->PrevTemporal == this) { // RA2: as above
			NextTemporal->PrevTemporal = NULL; // RA2: as above
		}
		NextTemporal->Detach(); // RA2: as above
		NextTemporal = NULL; // RA2: as above
	}
	PrevTemporal = NULL; // RA2: as above
	NextTemporal = NULL; // RA2: as above
	SourceSW = NULL; // RA2: as above
	Unused_38 = NULL; // RA2: as above
}


/// <summary>
/// Forgets the target and both neighbours (nothing in the executable calls it).
/// </summary>
void TemporalClass::Clear_Links(void) // RA2: 0x006E26F0
{
	Target = NULL; // RA2: as above
	PrevTemporal = NULL; // RA2: as above
	NextTemporal = NULL; // RA2: as above
	SourceSW = NULL; // RA2: as above
	Unused_38 = NULL; // RA2: as above
}


/// <summary>
/// Releases the target and the whole chain around this link.
/// Unlike JustLetGo it has a target for certain and keeps the neighbour pointers
/// until the end.
/// </summary>
void TemporalClass::Detach(void) // RA2: 0x006E2710
{
	Target->BeingWarpedOut = false; // RA2: no test for a target: the executable dereferences it at once
	Target->TemporalTargetingMe = NULL; // RA2: as above
	Target = NULL; // RA2: as above
	if (PrevTemporal != NULL) { // RA2: the previous link is detached recursively, its pointer to this link first cleared
		if (PrevTemporal->NextTemporal == this) { // RA2: as above
			PrevTemporal->NextTemporal = NULL; // RA2: as above
		}
		PrevTemporal->Detach(); // RA2: recursion into 0x006E2710
	}
	if (NextTemporal != NULL) { // RA2: as above
		if (NextTemporal->PrevTemporal == this) { // RA2: as above
			NextTemporal->PrevTemporal = NULL; // RA2: as above
		}
		NextTemporal->Detach(); // RA2: as above
	}
	PrevTemporal = NULL; // RA2: as above
	NextTemporal = NULL; // RA2: as above
	SourceSW = NULL; // RA2: as above
	Unused_38 = NULL; // RA2: as above
}


/// <summary>
/// Can the weapon hold this object?
/// It must be a warpable type and must not be under the Iron Curtain.
/// </summary>
bool TemporalClass::CanWarpTarget(TechnoClass * target) // RA2: 0x006E2770
{
	if (target == NULL) { // RA2: as above
		return(false); // RA2: as above
	}
	if (!target->Techno_Type_Class()->Warpable) { // RA2: the Warpable flag of the type (+0xB01)
		return(false); // RA2: as above
	}
	if (target->Unknown_Virtual_32C()) { // RA2: virtual +0x32C: the Iron Curtain timer of the target is still running
		return(false); // RA2: as above
	}
	return(true); // RA2: as above
}


/// <summary>
/// The owner fires the weapon at an object.
/// Whatever the owner held before is let go and the target's spawns and captures
/// are released. A target that can be held is linked to this weapon (behind the
/// link already holding it, if any), its remaining warp time is ten times its
/// strength, a harvester or building under attack is reported, and a building is
/// switched off.
/// </summary>
void TemporalClass::Fire(TechnoClass * target) // RA2: 0x006E27B0
{
	if (target != NULL) { // RA2: as above
		if (target->SpawnManager != NULL) { // RA2: spawned aircraft of the target are sent away first (+0x238)
			target->SpawnManager->KillNodes(); // RA2: as above
		}
		if (target->CaptureManager != NULL) { // RA2: and so are the units it mind controls (+0x230)
			target->CaptureManager->FreeAll(); // RA2: as above
		}
	}

	if (Owner->TemporalImUsing != NULL && Owner->TemporalImUsing->Target != NULL) { // RA2: the owner's own link lets go of what it held before
		Owner->TemporalImUsing->LetGo(); // RA2: call 0x006E2500
	}

	if (CanWarpTarget(target) && Owner->TemporalTargetingMe == NULL) { // RA2: an owner that is itself held cannot fire; CanWarpTarget is expanded in place
		Target = target; // RA2: as above
		if (Target->TemporalTargetingMe == NULL) { // RA2: as above
			Target->TemporalTargetingMe = this; // RA2: as above
			WarpRemaining = Target->Techno_Type_Class()->MaxStrength * 10; // RA2: ten frames per point of the target's strength
			if (Target != NULL && Target->Fetch_RTTI() == RTTI_UNIT) { // RA2: as above
				TechnoClass * object = Target; // RA2: the cast to a unit tests the object for NULL first
				UnitClass * unit; // RA2: as above
				if (object == NULL) { // RA2: as above
					unit = NULL; // RA2: as above
				} else {
					unit = object->Fetch_RTTI() == RTTI_UNIT ? (UnitClass *)object : NULL; // RA2: as above
				}
				if (unit->Class->IsToHarvest && Target->House == PlayerPtr) { // RA2: as above
					if (Submit_Radar_Event(RADAREVENT_HARVESTER_ATTACKED, Cell(Target->Destination_Coord(NULL)))) { // RA2: a harvester of the player is reported on the radar (event 4) and announced
						Queue_Speech("EVA_OreMinerUnderAttack", -1, -1); // RA2: as above
					}
				}
			}
			if (Target->Fetch_RTTI() == RTTI_BUILDING && !Target->Techno_Type_Class()->IsInsignificant && !Target->Considered_Vehicle()) { // RA2: as above
				TechnoClass * object = Target; // RA2: the cast to a building tests the object for NULL first
				BuildingClass * building; // RA2: as above
				if (object == NULL) { // RA2: as above
					building = NULL; // RA2: as above
				} else {
					building = object->Fetch_RTTI() == RTTI_BUILDING ? (BuildingClass *)object : NULL; // RA2: as above
				}
				Target->House->Attacked(building); // RA2: as above
			}
		} else {
			NextTemporal = Target->TemporalTargetingMe; // RA2: the target is already held: this link goes in front of the holding link
			PrevTemporal = NextTemporal->PrevTemporal; // RA2: as above
			NextTemporal->PrevTemporal = this; // RA2: as above
			if (PrevTemporal != NULL) { // RA2: as above
				PrevTemporal->NextTemporal = this; // RA2: as above
			}
		}

		Target->BeingWarpedOut = true; // RA2: the target is marked as being warped out (+0x204)
		if (Target->Fetch_RTTI() == RTTI_BUILDING) { // RA2: as above
			BuildingClass * building = (BuildingClass *)Target; // RA2: as above
			building->House->RecalcPower = true; // RA2: as above
			building->DisableTemporal(); // RA2: call 0x0044D320, the powered animations are paused
			building->House->IsRecalcNeeded = true; // RA2: as above
		}
		Target->Mark(MARK_CHANGE); // RA2: as above
		if (Target->TemporalImUsing != NULL && Target->TemporalImUsing->Target != NULL) { // RA2: as above
			Target->TemporalImUsing->LetGo(); // RA2: as above
		}
	}
}


/// <summary>
/// Returns the size of a temporal link.
/// </summary>
int TemporalClass::Fetch_Object_Size(void) const // RA2: 0x006E2A00
{
	return(sizeof(*this)); // RA2: 0x50
}


/// <summary>
/// Returns the type of the object: a temporal link.
/// </summary>
RTTIType TemporalClass::Fetch_RTTI(void) const // RA2: 0x006E2A10
{
	return(RTTI_TEMPORAL); // RA2: 0x46
}
