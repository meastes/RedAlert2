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
**	RA2: the capture manager of a mind controlling unit (Yuri's mind controllers): it keeps the one unit under
**	control with its original house, takes a unit over, gives it back and draws the link between the two.
**	TibSun has no such file; the code is the executable's from 0x00467D10.
*/

#define RA2_GEOMETRY_H // RA2: this module has no copy of the geometry constants: the executable runs only the zero rectangle and the list before its first function at 0x00467DB0
#define INCLUDE_COM // RA2: the class identifier of GetClassID is declared with COM
#include "always.h" // RA2: the includes this file needs
#include "capmgr.h" // RA2: the includes this file needs
#include "crc.h" // RA2: the includes this file needs
#include "foot.h" // RA2: the includes this file needs
#include "house.h" // RA2: the includes this file needs
#include "swizzle.h" // RA2: the includes this file needs
#include "team.h" // RA2: the includes this file needs
#include "techno.h" // RA2: the includes this file needs
#include "temporal.h" // RA2: the includes this file needs
#include "techtype.h" // RA2: the includes this file needs
#include <new.h> // RA2: the includes this file needs


DynamicVectorClass<CaptureManagerClass *> CaptureManagerArray; // RA2: 0x008505A0; every capture manager in the game


/// <summary>
/// Makes the capture manager of a mind controlling unit: nothing is controlled yet, and the manager joins the list.
/// </summary>
CaptureManagerClass::CaptureManagerClass(TechnoClass * owner) // RA2: 0x00467DB0
{
	Unit = NULL; // RA2: the controlled unit and its original house start empty
	OriginalOwner = NULL; // RA2: as above
	Owner = owner; // RA2: the unit that controls
	CaptureManagerArray.Add(this); // RA2: the manager joins the list at 0x008505A0
}


/// <summary>
/// Makes a capture manager without an owner; it joins the list as well.
/// </summary>
CaptureManagerClass::CaptureManagerClass(void) // RA2: 0x00467E40
{
	Unit = NULL; // RA2: the controlled unit and its original house start empty
	OriginalOwner = NULL; // RA2: as above
	Owner = NULL; // RA2: no controlling unit yet
	CaptureManagerArray.Add(this); // RA2: the manager joins the list at 0x008505A0
}


/// <summary>
/// The constructor used when the manager is read from a save game; its fields come from the stream.
/// </summary>
CaptureManagerClass::CaptureManagerClass(NoInitClass const & x) : // RA2: 0x00467ED0
	AbstractClass(x) // RA2: as above
{
}


/// <summary>
/// Takes the manager out of the list.
/// </summary>
CaptureManagerClass::~CaptureManagerClass(void) // RA2: 0x00467F00
{
	CaptureManagerArray.Delete(this); // RA2: the manager leaves the list at 0x008505A0
}


/// <summary>
/// Whether the unit can be mind controlled: it must exist, belong to another house, not be immune to
/// psionics, not be controlled already and not be invulnerable.
/// </summary>
bool CaptureManagerClass::Can_Capture(TechnoClass * target) // RA2: 0x00467F80
{
	if (target == NULL) { // RA2: nothing to control
		return(false); // RA2: as above
	}
	if (target->Owner_HouseClass() == Owner->Owner_HouseClass()) { // RA2: not the controller's own house
		return(false); // RA2: as above
	}
	if (target->Techno_Type_Class()->ImmuneToPsionics) { // RA2: the type's immunity (+0xAFD)
		return(false); // RA2: as above
	}
	if (target->MindControlledBy != NULL) { // RA2: already controlled by another unit
		return(false); // RA2: as above
	}
	if (target->Unknown_Virtual_32C()) { // RA2: virtual slot 0x32C, true while the unit is iron curtained
		return(false); // RA2: as above
	}
	return(true); // RA2: as above
}


/// <summary>
/// Lets go of the unit under control, then takes the target over for the controller's house: it is taken out
/// of its team, loses its destination, target and archive target, is ordered to mission 5 and stops being
/// warped by a temporal weapon.
/// </summary>
bool CaptureManagerClass::Capture(TechnoClass * target) // RA2: 0x00467FF0
{
	if (Unit != NULL) { // RA2: only one unit is controlled; it is given back first
		Unit->MindControlledBy = NULL; // RA2: as above
		Unit->Captured(OriginalOwner, true); // RA2: back to its original house
		Unit = NULL; // RA2: as above
		OriginalOwner = NULL; // RA2: as above
	}
	bool capture = target != NULL && (target->AbstractFlags & 1) && Can_Capture(target); // RA2: the target must be a techno that can be controlled
	if (!capture) { // RA2: as above
		return(false); // RA2: as above
	}
	OriginalOwner = target->Owner_HouseClass(); // RA2: remembered for the way back
	if (!target->Captured(Owner->Owner_HouseClass(), true)) { // RA2: the controller's house takes it over
		OriginalOwner = NULL; // RA2: as above
		return(false); // RA2: as above
	}
	Unit = target; // RA2: as above
	target->MindControlledBy = Owner; // RA2: as above
	if (Unit != NULL && (Unit->AbstractFlags & 4) && ((FootClass *)Unit)->Team != NULL) { // RA2: a foot unit leaves its team
		((FootClass *)Unit)->Team->Remove((FootClass *)Unit, -1, false); // RA2: as above
	}
	Unit->Assign_Destination(NULL, true); // RA2: it forgets its orders
	Unit->Assign_Target(NULL); // RA2: as above
	Unit->Assign_Archive_Target(NULL); // RA2: as above
	Unit->QueueMission(MISSION_GUARD, true); // RA2: mission 5
	if (Unit->TemporalImUsing != NULL && Unit->TemporalImUsing->Target != NULL) { // RA2: a temporal weapon it fires lets go
		Unit->TemporalImUsing->LetGo(); // RA2: as above
	}
	return(true); // RA2: as above
}


/// <summary>
/// Forgets the unit under control when it is the one detached.
/// </summary>
bool CaptureManagerClass::Detach_Target(AbstractClass const * target) // RA2: 0x00468120
{
	if (Unit == target) { // RA2: the controlled unit is going away
		Unit = NULL; // RA2: as above
		OriginalOwner = NULL; // RA2: as above
	}
	return(true); // RA2: as above
}


/// <summary>
/// An empty function that accepts a detached object and returns true; nothing calls it.
/// </summary>
bool CaptureManagerClass::Detach_Unused(AbstractClass * target) // RA2: 0x00468140
{
	return(true); // RA2: as above
}


/// <summary>
/// Draws the link from the controller's firing point to a point 70 above the controlled unit, in the
/// controller's house color.
/// </summary>
void CaptureManagerClass::Draw_Link(void) const // RA2: 0x00468150
{
	if (Owner != NULL && Unit != NULL) { // RA2: only while a unit is controlled
		Coord to = Unit->Position; // RA2: the unit's coordinate
		to.Z += 70; // RA2: 70 above it
		Owner->Draw_Link_Line(Owner->Fire_Coord(0), to, Owner->House->RemapColorRGB); // RA2: in the house's remap color
	}
}


/// <summary>
/// Gives the unit under control back to its original house and forgets it.
/// </summary>
void CaptureManagerClass::FreeAll(void) // RA2: 0x004681E0
{
	if (Unit != NULL) { // RA2: as above
		Unit->MindControlledBy = NULL; // RA2: as above
		Unit->Captured(OriginalOwner, true); // RA2: back to its original house
		Unit = NULL; // RA2: as above
		OriginalOwner = NULL; // RA2: as above
	}
}


/// <summary>
/// Adds what makes the manager differ between machines to the checksum of the game state: the ids of the
/// controlled unit, its original house and the controller.
/// </summary>
void CaptureManagerClass::Compute_CRC(CRCEngine & crc) const // RA2: 0x00468220
{
	BASECLASS::Compute_CRC(crc); // RA2: as above
	if (Unit != NULL) { // RA2: as above
		crc(Unit->Fetch_ID()); // RA2: as above
	}
	if (OriginalOwner != NULL) { // RA2: as above
		crc(OriginalOwner->Fetch_ID()); // RA2: as above
	}
	if (Owner != NULL) { // RA2: as above
		crc(Owner->Fetch_ID()); // RA2: as above
	}
}


/// <summary>
/// Reads the manager back in from the save game stream; the three pointers are remapped by the swizzler afterwards.
/// </summary>
HRESULT STDMETHODCALLTYPE CaptureManagerClass::Load(IStream * stream) // RA2: 0x00468280
{
	HRESULT result = BASECLASS::Load(stream); // RA2: as above
	if (SUCCEEDED(result)) { // RA2: as above
		new (this) CaptureManagerClass(NoInitClass()); // RA2: as above
		Swizzle_Pointer(&Unit); // RA2: as above
		Swizzle_Pointer(&OriginalOwner); // RA2: as above
		Swizzle_Pointer(&Owner); // RA2: as above
	}
	return(result); // RA2: as above
}


/// <summary>
/// Writes the manager to the save game stream: only the base object, the pointers are written with it.
/// </summary>
HRESULT STDMETHODCALLTYPE CaptureManagerClass::Save(IStream * stream, BOOL cleardirty) // RA2: 0x00468300
{
	return(BASECLASS::Save(stream, cleardirty)); // RA2: as above
}


/// <summary>
/// Fetches the persistent class identifier for the capture manager.
/// </summary>
HRESULT STDMETHODCALLTYPE CaptureManagerClass::GetClassID(CLSID * retval) // RA2: 0x00468320
{
	if (retval == NULL) return(E_POINTER); // RA2: as above
	*retval = CLSID_CaptureManagerClass; // RA2: as above
	return(S_OK); // RA2: as above
}


/// <summary>
/// The size of the object.
/// </summary>
int CaptureManagerClass::Fetch_Object_Size(void) const // RA2: 0x00468360
{
	return(sizeof(*this)); // RA2: 0x30 bytes
}


/// <summary>
/// The kind of object: a capture manager.
/// </summary>
RTTIType CaptureManagerClass::Fetch_RTTI(void) const // RA2: 0x00468370
{
	return(RTTI_CAPTUREMANAGER); // RA2: kind 0x42
}
