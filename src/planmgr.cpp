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
**	RA2: the planning mode (waypoint mode): the planned orders of selected objects are kept
**	as nodes along a path per object, drawn on the tactical view and sent together when
**	planning mode ends. TibSun has no such file; the code is the executable's from 0x006101B0.
*/

#define RA2_GEOMETRY_H // RA2: geometry.h follows the includes, as its constants take their initializers after those of the other headers
#include "always.h" // RA2: the includes this file needs
#include <string.h> // RA2: the includes this file needs
#include "convert.h" // RA2: the includes this file needs
#include "draw.h" // RA2: the includes this file needs
#include "dsurface.h" // RA2: the includes this file needs
#include "rect.h" // RA2: the includes this file needs
#include "shapeset.h" // RA2: the includes this file needs
#include "vector.h" // RA2: the includes this file needs
#include "_tactica.h" // RA2: the includes this file needs
#include "globals.h" // RA2: the includes this file needs
#include "house.h" // RA2: the includes this file needs
#include "tactical.h" // RA2: the includes this file needs
#include "techno.h" // RA2: the includes this file needs
#include "foot.h" // RA2: the includes this file needs
#include "event.h" // RA2: the includes this file needs
#include "target.h" // RA2: the includes this file needs
#include "planning.h" // RA2: the includes this file needs
#include <stddef.h> // RA2: the includes this file needs
#undef LoadString // RA2: as above
#include "stringtable.h" // RA2: the includes this file needs
#include "voc.h" // RA2: the includes this file needs
#include "_rules.h" // RA2: the includes this file needs
#include "rules.h" // RA2: the includes this file needs
#include "queue.h" // RA2: the includes this file needs
#include "_surface.h" // RA2: the includes this file needs
#include "dbgprint.h" // RA2: the includes this file needs
#include "planmgr.h" // RA2: the includes this file needs
#include "geometry.h" // RA2: this module holds its own copy of the geometry constants (initialized first, at 0x006101B0)
#include "mission.h" // RA2: the includes this file needs

extern Rect __fastcall Get_Title_Screen_Area(void); // RA2: 0x006F1F30
extern void __fastcall Enqueue_Event(EventClass * event); // RA2: 0x0062E6E0 (queue.cpp); puts the event on the list to run
extern bool __fastcall Add_Outgoing_Event(EventClass event); // RA2: 0x0062E670 (queue.cpp); sends the event to the other players, false when the list is full

Rect PlanningRect(0, 0, 0, 0); // RA2: 0x00A76670; a rectangle nothing in this module reads (its initializer is the one at 0x00610380)

DynamicVectorClass<EventClass *> PlanningTokenClass::Events; // RA2: 0x00A76648; the planned events held until the end of planning mode
DynamicVectorClass<PlanningNodeClass *> PlanningNodeClass::QueuedNodes; // RA2: 0x00A76758; the nodes queued to be rebuilt
DynamicVectorClass<TechnoClass *> PlanningTokenClass::Technos; // RA2: 0x00A76700; the objects with a planning token
DynamicVectorClass<PlanningNodeClass *> PlanningNodeClass::Unknown1; // RA2: 0x00A76628
DynamicVectorClass<PlanningNodeClass *> PlanningNodeClass::DrawnNodes; // RA2: 0x00A766D8; the nodes drawn on the tactical view
DynamicVectorClass<PlanningTokenClass *> PlanningTokenClass::Array; // RA2: 0x00A76738; every planning token
INoticeSink * PlanningSink; // RA2: 0x00A76680
int PlanningUnitCounts[8]; // RA2: 0x00A76684
PlanningNodeClass * PlanningSelectedNode; // RA2: 0x00A766B0
int PlanningHelpCounts[5]; // RA2: 0x00A766B4
bool PlanningRefusalShown; // RA2: 0x00A766C8
PlanningNodeClass * PlanningPickedNode; // RA2: 0x00A766F8
int PlanningBlockedNode; // RA2: 0x00A766FC
PlanningPoint PlanningMouseDown; // RA2: 0x00A76728
int PlanningPendingRefusal; // RA2: 0x00A76750
bool PlanningDragging; // RA2: 0x00A76770
bool PlanningLoopClosed; // RA2: 0x00A76788
PlanningNodeClass * PlanningHoverNode; // RA2: 0x00A7678C
bool PlanningCommandFlag; // RA2: 0x00A76790
bool PlanningTearDown; // RA2: 0x00A767A5
bool PlanningBusy; // RA2: 0x00A767A6
bool PlanningNodeClass::PlanningModeActive; // RA2: 0x00A767A4

void __fastcall Planning_Empty(void) // RA2: 0x00610760
{
}

int __fastcall Planning_House(TechnoClass * techno) // RA2: 0x00610770; the house a techno plans for: its own, the player's when its house is human controlled
{
	HouseClass * house = techno->House; // RA2: as above
	int index = house->HeapID; // RA2: as above
	if (house->Is_Player_Control() && PlayerPtr) { // RA2: as above
		index = PlayerPtr->HeapID; // RA2: as above
	}
	return index; // RA2: as above
}

Point2D __fastcall Planning_Coord_Pixel(Coord const & coord) // RA2: 0x006107A0; a coordinate as a pixel of the tactical view moved to the screen
{
	Point2D pixel; // RA2: as above
	TacticalMap->Coord_To_Pixel(coord, pixel); // RA2: as above
	pixel.X += TacticalRect.X; // RA2: as above
	pixel.Y += TacticalRect.Y; // RA2: as above
	return pixel; // RA2: as above
}

PlanningMemberClass::PlanningMemberClass(void) : Owner(0), Packet(0), Branch(-1), Active(false) // RA2: 0x006107E0
{
}

void PlanningMemberClass::Set_Owner(TechnoClass * owner) // RA2: 0x00610800
{
	Owner = owner; // RA2: as above
}

void PlanningMemberClass::Set_Packet(EventClass * packet) // RA2: 0x00610810
{
	Packet = packet; // RA2: as above
}

void PlanningMemberClass::Set_Branch(int branch) // RA2: 0x00610820
{
	Branch = branch; // RA2: as above
}

void PlanningMemberClass::Set_Active(bool active) // RA2: 0x00610830
{
	Active = active; // RA2: as above
}

TechnoClass * PlanningMemberClass::Get_Owner(void) // RA2: 0x00610840
{
	return Owner; // RA2: as above
}

EventClass * PlanningMemberClass::Get_Packet(void) // RA2: 0x00610850
{
	return Packet; // RA2: as above
}

int PlanningMemberClass::Get_Branch(void) // RA2: 0x00610860
{
	return Branch; // RA2: as above
}

bool PlanningMemberClass::Get_Active(void) // RA2: 0x00610870
{
	return Active; // RA2: as above
}

PlanningBranchClass::PlanningBranchClass(void) : Count(0), Color(-1) // RA2: 0x00610880
{
}

void PlanningBranchClass::Set_Event(EventClass const & event) // RA2: 0x006108A0
{
	Event = event; // RA2: as above
}

void PlanningBranchClass::Set_Count(int count) // RA2: 0x006108C0
{
	Count = count; // RA2: as above
}

void PlanningBranchClass::Set_Color(int color) // RA2: 0x006108D0
{
	Color = color; // RA2: as above
}

void PlanningBranchClass::Clear_Count(void) // RA2: 0x006108E0
{
	Count = 0; // RA2: as above
}

int PlanningBranchClass::Inc_Count(void) // RA2: 0x006108F0
{
	return ++Count; // RA2: as above
}

int PlanningBranchClass::Dec_Count(void) // RA2: 0x00610900
{
	return --Count; // RA2: as above
}

EventClass PlanningBranchClass::Get_Event(void) // RA2: 0x00610910
{
	return Event; // RA2: as above
}

int PlanningBranchClass::Get_Count(void) // RA2: 0x00610930
{
	return Count; // RA2: as above
}

int PlanningBranchClass::Get_Color(void) // RA2: 0x00610940
{
	return Color; // RA2: as above
}

Coord PlanningBranchClass::Get_Coord(void) // RA2: 0x00610950
{
	return Coord(Planning_Event_Coord(Event)); // RA2: as above
}

Coord __fastcall Planning_Event_Coord(EventClass const & event) // RA2: 0x00610980; where the order of a mission event points (target, destination or follow), (-1, -1, -1) for none
{
	if (event.Type == EventClass::MEGAMISSION) { // RA2: as above
		AbstractClass * object; // RA2: as above
		if (event.Data.MegaMission.Target.Is_Valid()) { // RA2: as above
			object = event.Data.MegaMission.Target.As_Abstract(); // RA2: as above
			if (object) { // RA2: as above
				return object->As_Coord(); // RA2: as above
			}
		} else if (event.Data.MegaMission.Follow.Is_Valid()) { // RA2: as above
			object = event.Data.MegaMission.Follow.As_Abstract(); // RA2: as above
			if (object) { // RA2: as above
				return object->As_Coord(); // RA2: as above
			}
		} else if (event.Data.MegaMission.Destination.Is_Valid()) { // RA2: as above
			object = event.Data.MegaMission.Destination.As_Abstract(); // RA2: as above
			if (object) { // RA2: as above
				return object->As_Coord(); // RA2: as above
			}
		}
	}
	return Coord(-1, -1, -1); // RA2: as above
}

Point2D PlanningBranchClass::Get_Pixel(void) // RA2: 0x00610A20
{
	Coord coord = Planning_Event_Coord(Event); // RA2: as above
	return Planning_Coord_Pixel(coord); // RA2: as above
}

PlanningNodeClass::PlanningNodeClass(int id) : Id(id), Queued(false), LoopBranchIndex(-1), Color(-1) // RA2: 0x00610A80
{
}

PlanningNodeClass::~PlanningNodeClass() // RA2: 0x00610AF0
{
	Clear(); // RA2: as above
	Planning_Unlist_Node(this); // RA2: as above
}

int PlanningNodeClass::Get_Id(void) // RA2: 0x00610C10
{
	return Id; // RA2: as above
}

int PlanningNodeClass::Get_Count(void) // RA2: 0x00610C20
{
	return PlanningMembers.Count(); // RA2: as above
}

int PlanningNodeClass::Index_In(TechnoClass * techno) // RA2: 0x00610C30
{
	PlanningTokenClass * token = techno->Get_Planning_Token(); // RA2: as above
	return token->PlanningNodes.ID(this); // RA2: as above
}

PlanningNodeClass * PlanningNodeClass::Prev_Node(TechnoClass * techno) // RA2: 0x00610C60
{
	PlanningTokenClass * token = techno->Get_Planning_Token(); // RA2: as above
	int index = token->PlanningNodes.ID(this); // RA2: as above
	if (index == -1) return 0; // RA2: as above
	int count = token->PlanningNodes.Count(); // RA2: as above
	if (count == 1) return this; // RA2: as above
	index--; // RA2: as above
	if (index < 0) index = count - 1; // RA2: as above
	return token->Node_At(index); // RA2: as above
}

PlanningNodeClass * PlanningNodeClass::Next_Node(TechnoClass * techno) // RA2: 0x00610CE0
{
	PlanningTokenClass * token = techno->Get_Planning_Token(); // RA2: as above
	int index = token->PlanningNodes.ID(this); // RA2: as above
	if (index == -1) return 0; // RA2: as above
	int count = token->PlanningNodes.Count(); // RA2: as above
	if (count == 1) return this; // RA2: as above
	index++; // RA2: as above
	if (index >= count) index = 0; // RA2: as above
	return token->Node_At(index); // RA2: as above
}

void PlanningNodeClass::Add_Member(TechnoClass * techno, EventClass const * event) // RA2: 0x00610D60
{
	PlanningMemberClass * member = new PlanningMemberClass; // RA2: as above
	member->Set_Owner(techno); // RA2: as above
	EventClass * packet = new EventClass; // RA2: as above
	*packet = *event;
	member->Set_Packet(packet); // RA2: as above
	PlanningMembers.Add(member); // RA2: as above
	techno->Get_Planning_Token()->Add_Node(this); // RA2: as above
	Assign_Branch(member); // RA2: as above
}

void PlanningNodeClass::Remove_Member(TechnoClass * techno) // RA2: 0x00610E70
{
	PlanningMemberClass * member = Get_Member(techno); // RA2: as above
	if (member) { // RA2: as above
		Detach(member); // RA2: as above
		delete member->Get_Packet(); // RA2: as above
		PlanningMembers.Delete(member); // RA2: as above
		delete member; // RA2: as above
		if (Get_Count() == 0) { // RA2: as above
			delete this; // RA2: as above
		}
	}
}

int PlanningNodeClass::Find_Member(TechnoClass * techno) // RA2: 0x00611050
{
	int result = -1; // RA2: as above
	int count = PlanningMembers.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		if (PlanningMembers[i]->Get_Owner() == techno) { // RA2: as above
			result = i; // RA2: as above
			break; // RA2: as above
		}
	}
	return result; // RA2: as above
}

TechnoClass * PlanningNodeClass::Member_Owner(int index) // RA2: 0x00611090
{
	return PlanningMembers[index]->Owner; // RA2: as above
}

Rect __fastcall Planning_Centered_Rect(Point2D center, int size); // RA2: as above

EventClass PlanningNodeClass::Get_Event(int index) // RA2: 0x006110A0
{
	PlanningMemberClass * member = PlanningMembers[index]; // RA2: as above
	return *member->Get_Packet(); // RA2: as above
}

EventClass PlanningNodeClass::Get_Event_For(TechnoClass * techno) // RA2: 0x006110D0
{
	return *Get_Member(techno)->Packet; // RA2: as above
}

bool PlanningNodeClass::Has_Member(TechnoClass * techno) // RA2: 0x00611120
{
	return Find_Member(techno) != -1; // RA2: as above
}

HouseClass * PlanningNodeClass::Lead_House(void) // RA2: 0x00611170
{
	return PlanningMembers[0]->Owner->House; // RA2: as above
}

Coord PlanningNodeClass::Lead_Coord(void) // RA2: 0x00611180
{
	Coord coord = Planning_Event_Coord(*PlanningMembers[0]->Packet); // RA2: as above
	return coord; // RA2: as above
}

Point2D PlanningNodeClass::Lead_Pixel(void) // RA2: 0x006111C0
{
	Coord coord = Lead_Coord(); // RA2: as above
	Point2D pixel = Planning_Coord_Pixel(coord); // RA2: as above
	return pixel; // RA2: as above
}

bool PlanningNodeClass::Hit_Test(int x, int y) // RA2: 0x00611230
{
	return Lead_Box_Rect().Is_Point_Within(Point2D(x, y)); // RA2: as above
}

Rect __fastcall Planning_Centered_Rect(Point2D center, int size) // RA2: 0x006112D0; a size by size rectangle centered on a pixel
{
	int offset = -(size / 2); // RA2: as above
	Point2D corner = center; // RA2: as above
	corner.X += offset; // RA2: as above
	corner.Y += offset; // RA2: as above
	Rect rect; // RA2: as above
	rect = Rect(corner, size, size); // RA2: as above
	return rect; // RA2: as above
}

bool PlanningNodeClass::Lead_At(Coord coord) // RA2: 0x00611310
{
	Coord c = Planning_Event_Coord(*PlanningMembers[0]->Packet); // RA2: as above
	return c.X == coord.X && c.Y == coord.Y && c.Z == coord.Z; // RA2: as above
}

Rect PlanningNodeClass::Lead_Rect(void) // RA2: 0x00611360
{
	Point2D pixel; // RA2: as above
	pixel = Lead_Pixel(); // RA2: as above
	Rect rect = Planning_Centered_Rect(pixel, 8); // RA2: as above
	return rect; // RA2: as above
}

bool PlanningNodeClass::Is_Past_Start(void) // RA2: 0x006113F0
{
	if (PlanningMembers.Count() == 0) return false; // RA2: as above
	PlanningTokenClass * token = PlanningMembers[0]->Owner->Get_Planning_Token(); // RA2: as above
	int index = token->PlanningNodes.ID(this); // RA2: as above
	int start = token->StepsToClosedLoop; // RA2: as above
	return start != -1 && index >= start; // RA2: as above
}

void PlanningNodeClass::Promote_Events(void) // RA2: 0x00611440
{
	int count = PlanningMembers.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		EventClass * packet = PlanningMembers[i]->Packet; // RA2: as above
		if (packet->Data.MegaMission.Mission == 2) packet->Data.MegaMission.Mission = 0x1d; // RA2: as above
	}
}

PlanningMemberClass * PlanningNodeClass::Get_Member(TechnoClass * techno) // RA2: 0x00611470
{
	int index = Find_Member(techno); // RA2: as above
	if (index == -1) { // RA2: as above
		return NULL; // RA2: as above
	}
	return PlanningMembers[index]; // RA2: as above
}

int PlanningNodeClass::Branch_Count(void) // RA2: 0x006114C0
{
	return PlanningBranches.Count(); // RA2: as above
}

int PlanningNodeClass::Has_Loop(void) // RA2: 0x006114D0
{
	return LoopBranch.Count > 0; // RA2: as above
}

int PlanningNodeClass::Is_Loop_Alone(void) // RA2: 0x006114E0
{
	if (LoopBranch.Count > 0 && LoopBranchIndex == -1) return 1; // RA2: as above
	return 0; // RA2: as above
}

TechnoClass * PlanningNodeClass::Owner_At_Branch(int branch) // RA2: 0x00611500
{
	int count = PlanningMembers.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		PlanningMemberClass * m = PlanningMembers[i]; // RA2: as above
		if (m->Branch == branch) return m->Owner; // RA2: as above
	}
	return 0; // RA2: as above
}

TechnoClass * PlanningNodeClass::Active_Owner(void) // RA2: 0x00611530
{
	int count = PlanningMembers.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		PlanningMemberClass * m = PlanningMembers[i]; // RA2: as above
		if (m->Active) return m->Owner; // RA2: as above
	}
	return 0; // RA2: as above
}

PlanningBranchClass * PlanningNodeClass::Get_Branch(int index) // RA2: 0x00611560
{
	return PlanningBranches[index]; // RA2: as above
}

PlanningBranchClass * PlanningNodeClass::Branch_Of(TechnoClass * techno) // RA2: 0x00611570
{
	PlanningMemberClass * member = Get_Member(techno); // RA2: as above
	int branch = member->Branch; // RA2: as above
	PlanningBranchClass * result = 0; // RA2: as above
	if (branch != -1) result = PlanningBranches[branch]; // RA2: as above
	return result; // RA2: as above
}

EventClass * PlanningNodeClass::Loop_Event(void) // RA2: 0x006115C0
{
	return &LoopBranch.Event; // RA2: as above
}

Coord PlanningNodeClass::Branch_Coord(int index) // RA2: 0x006115D0
{
	Coord coord = Planning_Event_Coord(PlanningBranches[index]->Event); // RA2: as above
	return coord; // RA2: as above
}

Coord PlanningNodeClass::Loop_Coord(void) // RA2: 0x00611610
{
	Coord coord = Planning_Event_Coord(LoopBranch.Event); // RA2: as above
	return coord; // RA2: as above
}

Point2D PlanningNodeClass::Branch_Pixel(int index) // RA2: 0x00611640
{
	return PlanningBranches[index]->Get_Pixel(); // RA2: as above
}

Point2D PlanningNodeClass::Loop_Pixel(void) // RA2: 0x006116B0
{
	return LoopBranch.Get_Pixel(); // RA2: as above
}

void PlanningNodeClass::Set_Queued(void) // RA2: 0x00611710
{
	Queued = true; // RA2: as above
}

bool PlanningNodeClass::Get_Queued(void) // RA2: 0x00611720
{
	return Queued; // RA2: as above
}

void PlanningNodeClass::Rebuild(void) // RA2: 0x00611730
{
	Clear(); // RA2: as above
	int count = PlanningMembers.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		Assign_Branch(PlanningMembers[i]); // RA2: as above
	}
	Update_Active(); // RA2: as above
	Queued = false; // RA2: as above
}

void PlanningNodeClass::Update_Active(void) // RA2: 0x00611770
{
	int count = PlanningMembers.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		PlanningMemberClass * member = PlanningMembers[i]; // RA2: as above
		TechnoClass * owner = member->Owner; // RA2: as above
		if (Is_Loop_Start(owner)) { // RA2: as above
			member->Active = true; // RA2: as above
			if (LoopBranch.Inc_Count() == 1) { // RA2: as above
				LoopBranch.Set_Event(owner->Get_Planning_Token()->Last_Node()->Get_Event_For(owner)); // RA2: as above
			}
		}
	}
	LoopBranchIndex = -1; // RA2: as above
	if (LoopBranch.Get_Count() != 0) { // RA2: as above
		EventClass self = LoopBranch.Get_Event(); // RA2: as above
		int branches = PlanningBranches.Count(); // RA2: as above
		int b = 0; // RA2: as above
		if (branches > 0) { // RA2: as above
			do { // RA2: as above
				EventClass other = PlanningBranches[b]->Get_Event(); // RA2: as above
				Coord a = Planning_Event_Coord(self); // RA2: as above
				Coord c = Planning_Event_Coord(other); // RA2: as above
				if (a == c) { // RA2: as above
					LoopBranchIndex = b; // RA2: as above
					return; // RA2: as above
				}
				b++; // RA2: as above
			} while (b < branches); // RA2: as above
		}
	}
}

void PlanningNodeClass::Dump(void) // RA2: 0x006118E0
{
	int branch_count = PlanningBranches.Count(); // RA2: as above
	for (int branch_index = 0; branch_index < branch_count; ++branch_index) { // RA2: as above
		PlanningBranchClass * branch = PlanningBranches[branch_index]; // RA2: as above
		int count = branch->Get_Count(); // RA2: as above
		EventClass event = branch->Get_Event(); // RA2: as above
		Coord origin = Planning_Event_Coord(event); // RA2: as above
		DebugString("Branch [%d];  Member count[%d];  Origin event location[%d, %d, %d]\n", // RA2: as above
			branch_index, count, origin.X, origin.Y, origin.Z); // RA2: as above
		int member_count = PlanningMembers.Count(); // RA2: as above
		for (int member_index = 0; member_index < member_count; ++member_index) { // RA2: as above
			PlanningMemberClass * member = PlanningMembers[member_index]; // RA2: as above
			if (member->Get_Branch() == branch_index) { // RA2: as above
				TechnoClass * owner = member->Get_Owner(); // RA2: as above
				char const * name = owner->Class_Of()->IniName; // RA2: as above
				Cell cell = owner->Get_Cell(); // RA2: as above
				DebugString("  Member [%s];  Cell location[%d, %d]\n", name, cell.X, cell.Y); // RA2: as above
			}
		}
	}
	int count = LoopBranch.Get_Count(); // RA2: as above
	if (count > 0) { // RA2: as above
		EventClass event = LoopBranch.Get_Event(); // RA2: as above
		Coord origin = Planning_Event_Coord(event); // RA2: as above
		DebugString("Loop Branch [%d];  Member count[%d];  Origin event location[%d, %d, %d]\n", // RA2: as above
			LoopBranchIndex, count, origin.X, origin.Y, origin.Z); // RA2: as above
		int member_count = PlanningMembers.Count(); // RA2: as above
		for (int member_index = 0; member_index < member_count; ++member_index) { // RA2: as above
			PlanningMemberClass * member = PlanningMembers[member_index]; // RA2: as above
			if (member->Get_Active()) { // RA2: as above
				TechnoClass * owner = member->Get_Owner(); // RA2: as above
				char const * name = owner->Class_Of()->IniName; // RA2: as above
				Cell cell = owner->Get_Cell(); // RA2: as above
				DebugString("  Member [%s];  Cell location[%d, %d]\n", name, cell.X, cell.Y); // RA2: as above
			}
		}
	}
}

int PlanningNodeClass::Find_Branch(TechnoClass * techno) // RA2: 0x00611A80
{
	PlanningNodeClass * prev = Prev_Node(techno); // RA2: as above
	if (prev == techno->Get_Planning_Token()->Last_Node()) return -1; // RA2: as above
	EventClass event = prev->Get_Event_For(techno); // RA2: as above
	int count = PlanningBranches.Count(); // RA2: as above
	if (count == 0) return -1; // RA2: as above
	Coord origin = Planning_Event_Coord(event); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		PlanningBranchClass * branch = PlanningBranches[i]; // RA2: as above
		if (branch->Get_Count()) { // RA2: as above
			EventClass other = branch->Get_Event(); // RA2: as above
			Coord c = Planning_Event_Coord(other); // RA2: as above
			if (origin == c) return i; // RA2: as above
		}
	}
	return -1; // RA2: as above
}

static inline void Find_Loop_Branch(PlanningNodeClass * node) // RA2: no function of its own; expanded in Assign_Branch and Locate_Loop_Branch
{
	node->LoopBranchIndex = -1; // RA2: as above
	if (node->LoopBranch.Get_Count()) { // RA2: as above
		EventClass self = node->LoopBranch.Get_Event(); // RA2: as above
		int branches = node->PlanningBranches.Count(); // RA2: as above
		for (int b = 0; b < branches; b++) { // RA2: as above
			EventClass other = node->PlanningBranches[b]->Get_Event(); // RA2: as above
			Coord a = Planning_Event_Coord(self); // RA2: as above
			Coord c = Planning_Event_Coord(other); // RA2: as above
			if (a == c) { // RA2: as above
				node->LoopBranchIndex = b; // RA2: as above
				return; // RA2: as above
			}
		}
	}
}

int PlanningNodeClass::Assign_Branch(PlanningMemberClass * member) // RA2: 0x00611BD0
{
	TechnoClass * owner = member->Get_Owner(); // RA2: as above
	int index = Find_Branch(owner); // RA2: as above
	if (index == -1) { // RA2: as above
		PlanningNodeClass * previous = Prev_Node(owner); // RA2: as above
		PlanningNodeClass * last = owner->Get_Planning_Token()->Last_Node(); // RA2: as above
		if (previous == last) { // RA2: as above
			member->Set_Branch(-1); // RA2: as above
			return -1; // RA2: as above
		}
		EventClass event = previous->Get_Event_For(owner); // RA2: as above
		PlanningBranchClass * branch = new PlanningBranchClass; // RA2: as above
		branch->Set_Event(event); // RA2: as above
		branch->Inc_Count(); // RA2: as above
		PlanningBranches.Add(branch); // RA2: as above
		int count = PlanningBranches.Count() - 1; // RA2: as above
		if (Is_Loop_Alone()) { // RA2: as above
			Find_Loop_Branch(this); // RA2: as above
		}
		Reset_Colors(false); // RA2: as above
		member->Set_Branch(count); // RA2: as above
		return count; // RA2: as above
	} else { // RA2: as above
		PlanningBranchClass * branch = PlanningBranches[index]; // RA2: as above
		branch->Set_Color(-1); // RA2: as above
		Reset_Colors(false); // RA2: as above
		branch->Inc_Count(); // RA2: as above
		member->Set_Branch(index); // RA2: as above
		return index; // RA2: as above
	}
}

inline void PlanningNodeClass::Detach(PlanningMemberClass * member) // RA2: 0x00611E20; expanded in Remove_Member and kept out of line too
{
	if (member->Get_Active()) { // RA2: as above
		Deactivate(member); // RA2: as above
	}
	if (member->Get_Branch() != -1) { // RA2: as above
		PlanningBranchClass * branch = PlanningBranches[member->Get_Branch()]; // RA2: as above
		branch->Set_Color(-1); // RA2: as above
		Reset_Colors(false); // RA2: as above
		if (branch->Dec_Count() == 0) { // RA2: as above
			Planning_Queue_Node(this); // RA2: as above
		}
	}
	member->Set_Branch(-1); // RA2: as above
}

void PlanningNodeClass::Clear(void) // RA2: 0x00611EE0
{
	int branches = PlanningBranches.Count(); // RA2: as above
	for (int i = 0; i < branches; i++) { // RA2: as above
		delete PlanningBranches[i]; // RA2: as above
	}
	PlanningBranches.Clear(); // RA2: as above
	int members = PlanningMembers.Count(); // RA2: as above
	for (int j = 0; j < members; j++) { // RA2: as above
		PlanningMemberClass * member = PlanningMembers[j]; // RA2: as above
		member->Set_Branch(-1); // RA2: as above
		member->Set_Active(false); // RA2: as above
	}
	LoopBranch.Clear_Count(); // RA2: as above
	LoopBranchIndex = -1; // RA2: as above
}

bool PlanningNodeClass::Is_Loop_Start(TechnoClass * techno) // RA2: 0x00611F60
{
	PlanningTokenClass * token = techno->Get_Planning_Token(); // RA2: as above
	return token->PlanningNodes.ID(this) == token->StepsToClosedLoop; // RA2: as above
}

void PlanningNodeClass::Deactivate(PlanningMemberClass * member) // RA2: 0x00611FA0
{
	member->Set_Active(false); // RA2: as above
	if (LoopBranch.Dec_Count() == 0) { // RA2: as above
		LoopBranchIndex = -1; // RA2: as above
	}
}

void PlanningNodeClass::Locate_Loop_Branch(void) // RA2: 0x00611FD0
{
	Find_Loop_Branch(this); // RA2: as above
}

void PlanningNodeClass::Select_All(void) // RA2: 0x00612090
{
	int count = PlanningMembers.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		TechnoClass * owner = PlanningMembers[i]->Owner; // RA2: as above
		if (!owner->IsSelected) { // RA2: as above
			owner->Select(); // RA2: as above
		}
	}
}

void PlanningNodeClass::Unselect_All(void) // RA2: 0x006120C0
{
	int count = PlanningMembers.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		TechnoClass * owner = PlanningMembers[i]->Owner; // RA2: as above
		if (owner->IsSelected) { // RA2: as above
			owner->Unselect(); // RA2: as above
		}
	}
}


void PlanningNodeClass::Draw_Lead_Box(int color) // RA2: 0x006120F0; the marker is listed among the drawn nodes when it is on the view; one instruction differs from the executable (the sum of the marker's X and Width is added with its operands the other way round)
{
	Rect clip = Get_Title_Screen_Area(); // RA2: as above
	Rect rect = Lead_Rect(); // RA2: as above
	CompositeSurface->Draw_Rect(clip, rect, color); // RA2: as above
	if (clip.Is_Overlapping(rect)) { // RA2: as above
		DrawnNodes.Add(this); // RA2: as above
	}
}

void PlanningNodeClass::Fill_Lead_Box(int color) // RA2: 0x00612230
{
	Rect rect; // RA2: as above
	Rect clip = Get_Title_Screen_Area(); // RA2: as above
	rect = Lead_Rect(); // RA2: as above
	Planning_Inflate(rect, -1); // RA2: as above
	CompositeSurface->Fill_Rect(clip, rect, color); // RA2: as above
}

void PlanningNodeClass::Frame_Lead_Box(int color) // RA2: 0x00612300
{
	Rect clip = Get_Title_Screen_Area(); // RA2: as above
	Rect rect = Lead_Box_Rect(); // RA2: as above
	for (int i = 0; i < 3; i++) { // RA2: as above
		CompositeSurface->Draw_Rect(clip, rect, color); // RA2: as above
		Planning_Inflate(rect, -1); // RA2: as above
	}
}

void PlanningNodeClass::Frame_Lead_Box_Outer(int color) // RA2: 0x00612420
{
	Rect clip = Get_Title_Screen_Area(); // RA2: as above
	Rect rect = Lead_Box_Rect2(); // RA2: as above
	for (int i = 0; i < 3; i++) { // RA2: as above
		CompositeSurface->Draw_Rect(clip, rect, color); // RA2: as above
		Planning_Inflate(rect, -1); // RA2: as above
	}
}

void __fastcall Planning_Inflate(Rect & rect, int amount) // RA2: 0x00612570; grows a rectangle by amount on every side; one that would vanish becomes empty
{
	int twice = amount * 2; // RA2: as above
	if (twice < 0 && (twice >= rect.Width || twice >= rect.Height)) { // RA2: as above
		rect.X = 0; // RA2: as above
		rect.Y = 0; // RA2: as above
		rect.Width = 0; // RA2: as above
		rect.Height = 0; // RA2: as above
		return; // RA2: as above
	}
	rect.X -= amount; // RA2: as above
	rect.Y -= amount; // RA2: as above
	rect.Width += twice; // RA2: as above
	rect.Height += twice; // RA2: as above
}

static inline RGBClass Planning_House_Color(TechnoClass * techno) // RA2: no function of its own; the remap color of the house a techno plans for
{
	return Houses[Planning_House(techno)]->RemapColorRGB; // RA2: as above
}

int PlanningNodeClass::Node_Color(void) // RA2: 0x006125D0
{
	if (Color != -1) { // RA2: as above
		return Color; // RA2: as above
	}
	TechnoClass * owner = PlanningMembers[0]->Owner; // RA2: as above
	Color = Planning_Adjusted_Pixel(Planning_House_Color(owner), BlackColor, 0x50); // RA2: as above
	int count = PlanningBranches.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		if (PlanningBranches[i]->Get_Color() == Color) { // RA2: as above
			return Color; // RA2: as above
		}
	}
	if (count == 0) { // RA2: as above
		PlanningTokenClass * token = owner->Get_Planning_Token(); // RA2: as above
		if (token->Reached(token->Node_Index(this))) { // RA2: as above
			Color = Planning_Bright_Color(owner); // RA2: as above
		}
	} else { // RA2: as above
		Color = Planning_Bright_Color(owner); // RA2: as above
	}
	return Color; // RA2: as above
}

int PlanningNodeClass::Branch_Color(int index) // RA2: 0x006127E0
{
	PlanningBranchClass * branch = PlanningBranches[index]; // RA2: as above
	if (branch->Color != -1) return branch->Color; // RA2: as above
	TechnoClass * last_owner = NULL; // RA2: as above
	int count = PlanningMembers.Count(); // RA2: as above
	for (int i = 0; i < count; ++i) { // RA2: as above
		PlanningMemberClass * member = PlanningMembers[i]; // RA2: as above
		if (member->Branch == index) { // RA2: as above
			TechnoClass * owner = member->Owner; // RA2: as above
			PlanningTokenClass * token = owner->Get_Planning_Token(); // RA2: as above
			if (!token->Reached(token->Node_Index(this))) { // RA2: as above
				RGBClass color = Houses[Planning_House(owner)]->RemapColorRGB; // RA2: as above
				color.Adjust(0x50, BlackColor); // RA2: as above
				return branch->Color = DSurface::Build_Hicolor_Pixel(color.Get_Red(), color.Get_Green(), color.Get_Blue()); // RA2: as above
			}
			last_owner = owner; // RA2: as above
		}
	}
	if (last_owner == NULL) { // RA2: as above
		return -1; // RA2: as above
	} else { // RA2: as above
		RGBClass color = Houses[Planning_House(last_owner)]->RemapColorRGB; // RA2: as above
		color.Adjust(0x64, WhiteColor); // RA2: as above
		return branch->Color = DSurface::Build_Hicolor_Pixel(color.Get_Red(), color.Get_Green(), color.Get_Blue()); // RA2: as above
	}
}

int PlanningNodeClass::Loop_Color(void) // RA2: 0x006129B0
{
	if (LoopBranch.Color != -1) return LoopBranch.Color; // RA2: as above
	TechnoClass * last_owner = NULL; // RA2: as above
	int count = PlanningMembers.Count(); // RA2: as above
	for (int i = 0; i < count; ++i) { // RA2: as above
		PlanningMemberClass * member = PlanningMembers[i]; // RA2: as above
		if (member->Active) { // RA2: as above
			TechnoClass * owner = member->Owner; // RA2: as above
			PlanningTokenClass * token = owner->Get_Planning_Token(); // RA2: as above
			if (!token->Get_F98()) { // RA2: as above
				RGBClass color = Houses[Planning_House(owner)]->RemapColorRGB; // RA2: as above
				color.Adjust(0x50, BlackColor); // RA2: as above
				return LoopBranch.Color = DSurface::Build_Hicolor_Pixel(color.Get_Red(), color.Get_Green(), color.Get_Blue()); // RA2: as above
			}
			last_owner = owner; // RA2: as above
		}
	}
	if (last_owner == NULL) { // RA2: as above
		return -1; // RA2: as above
	} else { // RA2: as above
		RGBClass color = Houses[Planning_House(last_owner)]->RemapColorRGB; // RA2: as above
		color.Adjust(0x64, WhiteColor); // RA2: as above
		return LoopBranch.Color = DSurface::Build_Hicolor_Pixel(color.Get_Red(), color.Get_Green(), color.Get_Blue()); // RA2: as above
	}
}

void PlanningNodeClass::Reset_Colors(bool all) // RA2: 0x00612B50
{
	Color = -1; // RA2: as above
	if (all) { // RA2: as above
		int count = PlanningBranches.Count(); // RA2: as above
		int i = 0; // RA2: as above
		if (count > 0) do { // RA2: as above
			PlanningBranches[i]->Set_Color(-1); // RA2: as above
			i++; // RA2: as above
		} while (i < count); // RA2: as above
		LoopBranch.Set_Color(-1); // RA2: as above
	}
}

Rect PlanningNodeClass::Lead_Box_Rect(void) // RA2: 0x00612B90
{
	Rect rect = Lead_Rect(); // RA2: as above
	Planning_Inflate(rect, 4); // RA2: as above
	return rect; // RA2: as above
}

Rect PlanningNodeClass::Lead_Box_Rect2(void) // RA2: 0x00612C30
{
	Rect rect = Lead_Box_Rect(); // RA2: as above
	Planning_Inflate(rect, 4); // RA2: as above
	return rect; // RA2: as above
}

PlanningTokenClass::PlanningTokenClass(TechnoClass * owner) : F1C(false), F8C(-1), ClosedLoopNodeCount(-1), StepsToClosedLoop(-1), F98(false), F99(false) // RA2: 0x00612CC0
{
	OwnerUnit = owner; // RA2: as above
}

PlanningTokenClass::~PlanningTokenClass() // RA2: 0x00612D20
{
	if (OwnerUnit) { // RA2: as above
		OwnerUnit->Set_Planning_Token(0); // RA2: as above
	}
	Planning_Unlist_Token(this); // RA2: as above
	Cancel_Owner(); // RA2: as above
}

TechnoClass * PlanningTokenClass::Get_Owner(void) // RA2: 0x00610840
{
	return OwnerUnit; // RA2: as above
}

inline void PlanningTokenClass::Cancel_Owner(void) // RA2: 0x00612EC0; expanded in the destructor and kept out of line too
{
	int count = PlanningNodes.Count(); // RA2: as above
	bool none = (count == 0); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		PlanningNodeClass * node = PlanningNodes[i]; // RA2: as above
		node->Remove_Member(Get_Owner()); // RA2: as above
	}
	if (!none) { // RA2: as above
		Planning_Unlist_Techno(OwnerUnit); // RA2: as above
	}
	PlanningNodes.Clear(); // RA2: as above
	F8C = -1; // RA2: as above
	ClosedLoopNodeCount = -1; // RA2: as above
	StepsToClosedLoop = -1; // RA2: as above
	F98 = false; // RA2: as above
	F99 = false; // RA2: as above
}

void PlanningTokenClass::Destroy(void) // RA2: 0x006130B0
{
	Cancel_Owner(); // RA2: as above
	OwnerUnit = 0; // RA2: as above
	delete this; // RA2: as above
}

void PlanningTokenClass::Set_F1C(void) // RA2: 0x00611710
{
	F1C = true; // RA2: as above
}

void PlanningTokenClass::Clear_F1C(void) // RA2: 0x006132D0
{
	F1C = false; // RA2: as above
}

bool PlanningTokenClass::Get_F1C(void) // RA2: 0x00611720
{
	return F1C; // RA2: as above
}

bool PlanningTokenClass::Owner_Is_Mission(void) // RA2: 0x006132F0
{
	return OwnerUnit->CurrentMission == 0x1c || OwnerUnit->MissionQueue == 0x1c; // RA2: as above
}

bool PlanningTokenClass::Has_Started(void) // RA2: 0x00613310
{
	return PlanningNodes.Count() > 0 && (F8C == -1 || F8C > 0); // RA2: as above
}

static inline void Finish_Consumed_Step(PlanningTokenClass * token) // RA2: no function of its own; the end of Pop_Front
{
	token->F8C = max(-1, token->F8C - 1); // RA2: as above
	if (token->Node_Count() == 0) { // RA2: as above
		Planning_Unlist_Techno(token->Get_Owner()); // RA2: as above
	}
}

EventClass PlanningTokenClass::Pop_Front(void) // RA2: 0x00613330
{
	PlanningNodeClass * node = PlanningNodes[0]; // RA2: as above
	EventClass packet = Event_At(0); // RA2: as above
	Popped = packet; // RA2: as above
	PlanningNodes.Delete_Index(0); // RA2: as above
	if (Loop_Start_Zero()) { // RA2: as above
		F99 = true; // RA2: as above
		PlanningNodes.Add(node); // RA2: as above
		Planning_Queue_Node(node); // RA2: as above
	} else { // RA2: as above
		node->Remove_Member(OwnerUnit); // RA2: as above
		if (Loop_Start_Positive()) { // RA2: as above
			Dec_Loop_Start(); // RA2: as above
		}
	}
	Finish_Consumed_Step(this); // RA2: as above
	return packet; // RA2: as above
}

void PlanningTokenClass::Share_Range(TechnoClass * techno, int start, int count) // RA2: 0x00613550
{
	if (techno->Get_Planning_Token()) { // RA2: as above
		for (int i = start; i < count + start; i++) { // RA2: as above
			PlanningNodeClass * node = PlanningNodes[i]; // RA2: as above
			EventClass event = node->Get_Event_For(OwnerUnit); // RA2: as above
			event.Data.Target.Whom = TargetClass(techno); // RA2: as above
			node->Add_Member(techno, &event); // RA2: as above
		}
	}
}

void PlanningTokenClass::Share_From(TechnoClass * techno, int start) // RA2: 0x00613720
{
	int count = PlanningNodes.Count() - start; // RA2: as above
	Share_Range(techno, start, count); // RA2: as above
}

void PlanningTokenClass::Remove_Node_At(int index) // RA2: 0x006138F0
{
	if (PlanningNodes.Count() == 1) { // RA2: as above
		F8C = -1; // RA2: as above
	} else if (F8C > 0 && index < F8C) { // RA2: as above
		F8C--; // RA2: as above
	}
	if (StepsToClosedLoop >= 0) { // RA2: as above
		if (index < StepsToClosedLoop) { // RA2: as above
			StepsToClosedLoop--; // RA2: as above
		} else { // RA2: as above
			ClosedLoopNodeCount--; // RA2: as above
			if (ClosedLoopNodeCount < 3) { // RA2: as above
				StepsToClosedLoop = -1; // RA2: as above
				ClosedLoopNodeCount = 0; // RA2: as above
			}
		}
	}
	PlanningNodeClass * node = PlanningNodes[index]; // RA2: as above
	node->Remove_Member(OwnerUnit); // RA2: as above
	PlanningNodes.Delete_Index(index); // RA2: as above
}

void PlanningTokenClass::Reset_Node_Colors(void) // RA2: 0x00613A80
{
	if (F98) { // RA2: as above
		PlanningNodeClass * node = Loop_Node(); // RA2: as above
		if (node != NULL) node->Reset_Colors(true); // RA2: as above
		F98 = false; // RA2: as above
	}
	int start = F8C == -1 ? 0 : F8C; // RA2: as above
	int count = PlanningNodes.Count(); // RA2: as above
	for (int index = start; index < count; index++) { // RA2: as above
		PlanningNodeClass * node = PlanningNodes[index]; // RA2: as above
		if (node != NULL) node->Reset_Colors(true); // RA2: as above
	}
	F8C = -1; // RA2: as above
}

int PlanningTokenClass::Get_F8C(void) // RA2: 0x00613B30
{
	return F8C; // RA2: as above
}

bool PlanningTokenClass::Reached(int index) // RA2: 0x00613B40
{
	if (F8C == -1) return false; // RA2: as above
	return index >= F8C; // RA2: as above
}

int PlanningTokenClass::Node_Count(void) // RA2: 0x00613B60
{
	return PlanningNodes.Count(); // RA2: as above
}

int PlanningTokenClass::Current_Or_Count(void) // RA2: 0x00613B70
{
	if (F8C == -1) return PlanningNodes.Count(); // RA2: as above
	return F8C; // RA2: as above
}

int PlanningTokenClass::Remaining(void) // RA2: 0x00613B80
{
	if (F8C == -1) return 0; // RA2: as above
	return PlanningNodes.Count() - F8C; // RA2: as above
}

void PlanningTokenClass::Add_Node(PlanningNodeClass * node) // RA2: 0x00613BA0
{
	PlanningNodes.Add(node); // RA2: as above
	if (F8C == -1) { // RA2: as above
		F8C = PlanningNodes.Count() - 1; // RA2: as above
	}
}

PlanningNodeClass * PlanningTokenClass::Node_At(int index) // RA2: 0x00613C00
{
	int count = PlanningNodes.Count(); // RA2: as above
	if (count == 0) return 0; // RA2: as above
	if (index >= count) { // RA2: as above
		int start = StepsToClosedLoop; // RA2: as above
		if (!Loop_Start_Valid()) return 0; // RA2: as above
		index = (index - start) % ClosedLoopNodeCount + start; // RA2: as above
	}
	if (index < 0 || index >= count) return 0; // RA2: as above
	return PlanningNodes[index]; // RA2: as above
}

PlanningNodeClass * PlanningTokenClass::Last_Node(void) // RA2: 0x00613C50
{
	int index = PlanningNodes.Count() - 1; // RA2: as above
	if (index < 0) return 0; // RA2: as above
	return PlanningNodes[index]; // RA2: as above
}

bool PlanningTokenClass::Has_Node(PlanningNodeClass * node) // RA2: 0x00613C60
{
	return PlanningNodes.ID(node) != -1; // RA2: as above
}

int PlanningTokenClass::Node_Index(PlanningNodeClass * node) // RA2: 0x00613C80
{
	return PlanningNodes.ID(node); // RA2: as above
}

EventClass PlanningTokenClass::Event_At(int index) // RA2: 0x00613CA0
{
	PlanningNodeClass * node = PlanningNodes[index]; // RA2: as above
	TechnoClass * owner = OwnerUnit; // RA2: as above
	return *node->Get_Member(owner)->Packet; // RA2: as above
}

EventClass PlanningTokenClass::Last_Popped(void) // RA2: 0x00613D00
{
	return Popped; // RA2: as above
}

PlanningNodeClass * PlanningTokenClass::Loop_Node(void) // RA2: 0x00613D20
{
	if (StepsToClosedLoop == -1) return 0; // RA2: as above
	return PlanningNodes[StepsToClosedLoop]; // RA2: as above
}

void PlanningTokenClass::Set_Loop(int count, int start) // RA2: 0x00613D40
{
	ClosedLoopNodeCount = count; // RA2: as above
	StepsToClosedLoop = start; // RA2: as above
	F98 = true; // RA2: as above
}

int PlanningTokenClass::Get_Loop_Count(void) // RA2: 0x00613D60
{
	return ClosedLoopNodeCount; // RA2: as above
}

int PlanningTokenClass::Get_Loop_Start(void) // RA2: 0x00613D70
{
	return StepsToClosedLoop; // RA2: as above
}

bool PlanningTokenClass::Loop_Start_Valid(void) // RA2: 0x00613D80
{
	return StepsToClosedLoop > 0 || StepsToClosedLoop == 0; // RA2: as above
}

bool PlanningTokenClass::Loop_Start_Positive(void) // RA2: 0x00613DA0
{
	return StepsToClosedLoop > 0; // RA2: as above
}

bool PlanningTokenClass::Loop_Start_Zero(void) // RA2: 0x00613DB0
{
	return StepsToClosedLoop == 0; // RA2: as above
}

bool PlanningTokenClass::Get_F98(void) // RA2: 0x00613DC0
{
	return F98; // RA2: as above
}

bool PlanningTokenClass::Get_F99(void) // RA2: 0x00613DD0
{
	return F99; // RA2: as above
}

void PlanningTokenClass::Dec_Loop_Start(void) // RA2: 0x00613DE0
{
	StepsToClosedLoop--; // RA2: as above
	StepsToClosedLoop = (StepsToClosedLoop <= 0) ? 0 : StepsToClosedLoop; // RA2: as above
}

void Planning_Set_Sink(INoticeSink * sink) // RA2: 0x00613E00
{
	PlanningSink = sink; // RA2: as above
}

void __cdecl Planning_Reset(void) // RA2: 0x00613E10
{
	PlanningNodeClass::PlanningModeActive = false; // RA2: as above
	PlanningTearDown = false; // RA2: as above
	PlanningHoverNode = NULL; // RA2: as above
	PlanningPickedNode = NULL; // RA2: as above
	PlanningSelectedNode = NULL; // RA2: as above
	PlanningMouseDown = Point2D(0, 0); // RA2: as above
	PlanningDragging = false; // RA2: as above
}

void __cdecl Planning_Reset_Wrapper(void) // RA2: 0x00613E40
{
	Planning_Reset_State(); // RA2: as above
}

void __cdecl Planning_Reset_State(void) // RA2: 0x00613E50; deletes every token and clears every list and flag
{
	int count = PlanningTokenClass::Array.Count(); // RA2: as above
	PlanningTearDown = true; // RA2: as above
	PlanningNodeClass::PlanningModeActive = false; // RA2: as above
	PlanningBusy = false; // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		PlanningTokenClass * token = PlanningTokenClass::Array[i]; // RA2: as above
		delete token; // RA2: as above
	}
	memset(PlanningUnitCounts, 0, sizeof(PlanningUnitCounts)); // RA2: as above
	PlanningTokenClass::Technos.Clear(); // RA2: as above
	PlanningTokenClass::Array.Clear(); // RA2: as above
	PlanningNodeClass::Unknown1.Clear(); // RA2: as above
	PlanningNodeClass::DrawnNodes.Clear(); // RA2: as above
	PlanningHoverNode = NULL; // RA2: as above
	PlanningCommandFlag = false; // RA2: as above
	PlanningLoopClosed = false; // RA2: as above
	PlanningBlockedNode = 0; // RA2: as above
	PlanningRefusalShown = false; // RA2: as above
	PlanningTearDown = false; // RA2: as above
	PlanningPendingRefusal = -1; // RA2: as above
	memset(PlanningHelpCounts, 0, sizeof(PlanningHelpCounts)); // RA2: as above
}

void __fastcall Planning_Update(void) // RA2: 0x00614010; the per frame update: drops the steps whose target is gone and shows a pending refusal; not matched: the executable is 734 bytes, this is 688 (the loop over the tokens has another register assignment and more inline expansion)
{
	Planning_Hover(); // RA2: as above
	if (PlanningSelectedNode && CurrentObject.Count() > 0) { // RA2: as above
		PlanningSelectedNode = NULL; // RA2: as above
	}
	int count = PlanningTokenClass::Array.Count(); // RA2: as above
	for (int index = 0; index < count; ++index) { // RA2: as above
		PlanningTokenClass * token = PlanningTokenClass::Array[index]; // RA2: as above
		TechnoClass * owner = token->OwnerUnit; // RA2: as above
		for (int path_index = token->PlanningNodes.Count() - 1; path_index >= 0; --path_index) { // RA2: as above
			PlanningNodeClass * event_node = token->PlanningNodes[path_index]; // RA2: as above
			EventClass event = event_node->Get_Event_For(token->OwnerUnit); // RA2: as above
			bool remove = true; // RA2: as above
			if (event.Type == EventClass::MEGAMISSION) { // RA2: as above
				if (event.Data.MegaMission.Target.Is_Valid()) { // RA2: as above
					remove = event.Data.MegaMission.Target.As_Abstract() == 0; // RA2: as above
				} else if (event.Data.MegaMission.Destination.Is_Valid()) { // RA2: as above
					remove = event.Data.MegaMission.Destination.As_Abstract() == 0; // RA2: as above
				}
			}
			if (!remove) { // RA2: as above
				continue; // RA2: as above
			}
			PlanningNodeClass * node = token->Node_At(path_index); // RA2: as above
			PlanningNodeClass * next_node = node->Next_Node(owner); // RA2: as above
			Planning_Queue_Node(next_node); // RA2: as above
			token->Remove_Node_At(path_index); // RA2: as above
		}
	}
	Planning_Process_Queue(); // RA2: as above
	int refusal = PlanningPendingRefusal; // RA2: as above
	if (refusal != -1) { // RA2: as above
		Planning_Refuse_Command(refusal); // RA2: as above
		PlanningPendingRefusal = -1; // RA2: as above
	}
	PlanningCommandFlag = false; // RA2: as above
	PlanningLoopClosed = false; // RA2: as above
	PlanningRefusalShown = false; // RA2: as above
}

void __fastcall Planning_Free_Nodes(void) // RA2: 0x006142F0; runs the planned events still held and frees them
{
	for (int i = PlanningTokenClass::Events.Count() - 1; i >= 0; i--) { // RA2: as above
		EventClass * event = PlanningTokenClass::Events[i]; // RA2: as above
		Enqueue_Event(event); // RA2: as above
		PlanningTokenClass::Events.Delete_Index(i); // RA2: as above
		delete event; // RA2: as above
	}
}

void __fastcall Planning_Store_Event(EventClass const & source) // RA2: 0x00614350
{
	EventClass * event = new EventClass; // RA2: as above
	*event = source;
	if (event->Type == EventClass::MEGAMISSION) { // RA2: as above
		event->Data.MegaMission.IsPlanningEvent = 2; // RA2: as above
	}
	PlanningTokenClass::Events.Add(event); // RA2: as above
}

void __fastcall Planning_Unlist_Node(PlanningNodeClass * node) // RA2: 0x006143E0
{
	PlanningNodeClass::QueuedNodes.Delete(node); // RA2: as above
	PlanningNodeClass::Unknown1.Delete(node); // RA2: as above
	PlanningNodeClass::DrawnNodes.Delete(node); // RA2: as above
	if (PlanningHoverNode == node) { // RA2: as above
		PlanningHoverNode = NULL; // RA2: as above
	}
	if (PlanningPickedNode == node) { // RA2: as above
		PlanningPickedNode = NULL; // RA2: as above
	}
	if (PlanningSelectedNode == node) { // RA2: as above
		PlanningSelectedNode = NULL; // RA2: as above
	}
}

void __fastcall Planning_Unlist_Token(PlanningTokenClass * token) // RA2: 0x006144F0
{
	if (!PlanningTearDown) { // RA2: as above
		PlanningTokenClass::Array.Delete(token); // RA2: as above
	}
}

void __fastcall Planning_Unlist_Techno(TechnoClass * techno) // RA2: 0x00614550
{
	int house = Planning_House(techno); // RA2: as above
	Planning_Uncount(house); // RA2: as above
	PlanningTokenClass::Technos.Delete(techno); // RA2: as above
}

void TechnoClass::Select_Planning_Members(void) // RA2: 0x006145E0
{
	if (!PlanningBusy) { // RA2: as above
		PlanningBusy = true; // RA2: as above
		if (PlanningNodeClass::PlanningModeActive) { // RA2: as above
			PlanningTokenClass * token = Get_Planning_Token(); // RA2: as above
			if (token) { // RA2: as above
				PlanningNodeClass * node = token->Last_Node(); // RA2: as above
				if (node) { // RA2: as above
					node->Select_All(); // RA2: as above
				}
			}
		}
		PlanningBusy = false; // RA2: as above
	}
}

void __fastcall Planning_Queue_Node(PlanningNodeClass * node) // RA2: 0x00614650
{
	if (node && !node->Get_Queued()) { // RA2: as above
		node->Set_Queued(); // RA2: as above
		PlanningNodeClass::QueuedNodes.Add(node); // RA2: as above
	}
}

void __cdecl Planning_Process_Queue(void) // RA2: 0x006146C0
{
	int count = PlanningNodeClass::QueuedNodes.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		PlanningNodeClass * node = PlanningNodeClass::QueuedNodes[i]; // RA2: as above
		node->Rebuild(); // RA2: as above
	}
	PlanningNodeClass::QueuedNodes.Clear(); // RA2: as above
}

void __cdecl Planning_Begin(void) // RA2: 0x00614760; starts planning mode: sound and a notice to the sink
{
	if (!PlanningNodeClass::PlanningModeActive) { // RA2: as above
		PlanningNodeClass::PlanningModeActive = true; // RA2: as above
		VocClass::PlayGlobal(Rule->StartPlanningModeSound, 1.0f, 0x2000); // RA2: as above
		if (PlanningSink) { // RA2: as above
			PlanningSink->Unknown_Virtual_0(0x11C7, 0); // RA2: as above
		}
	}
}

void __cdecl Planning_End(void) // RA2: 0x006147B0; ends planning mode: sends the commit event, sound and a notice to the sink
{
	if (PlanningNodeClass::PlanningModeActive) { // RA2: as above
		EventClass event(PlayerPtr->HeapID, EventClass::PLAN_COMMIT); // RA2: as above
		Add_Outgoing_Event(event); // RA2: as above
		PlanningHoverNode = NULL; // RA2: as above
		PlanningPickedNode = NULL; // RA2: as above
		PlanningSelectedNode = NULL; // RA2: as above
		PlanningNodeClass::PlanningModeActive = false; // RA2: as above
		VocClass::PlayGlobal(Rule->EndPlanningModeSound, 1.0f, 0x2000); // RA2: as above
		if (PlanningSink) { // RA2: as above
			PlanningSink->Unknown_Virtual_0(0x11C8, 0); // RA2: as above
		}
	}
}

bool Planning_Is_Active(void) // RA2: 0x00614840
{
	return PlanningNodeClass::PlanningModeActive; // RA2: as above
}

void __fastcall Commit_Plans(int house) // RA2: 0x00614850; for every object of a house: starts the path again when it is not under way and its mission allows it; the expansion of TechnoClass::TryNextPlanningTokenNode (0x00615360) and of Planning_Mission_Allows stand in it
{
	int count = Technos.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		TechnoClass * techno = Technos[i]; // RA2: as above
		if (house == Planning_House(techno)) { // RA2: as above
			PlanningTokenClass * token = techno->Get_Planning_Token(); // RA2: as above
			if (token != NULL) { // RA2: as above
				bool started = token->Has_Started(); // RA2: as above
				token->Reset_Node_Colors(); // RA2: as above
				bool permitted = Planning_Mission_Allows(techno); // RA2: as above
				if (!started && permitted) { // RA2: as above
					techno->TryNextPlanningTokenNode(); // RA2: as above
				}
			}
		}
	}
}

void PlanningNodeClass::Send_Delete_Event(void) // RA2: 0x00614AA0
{
	EventClass event = *PlanningMembers[0]->Packet; // RA2: as above
	xTargetClass * target; // RA2: as above
	if (event.Data.MegaMission.Target.Is_Valid()) { // RA2: as above
		target = &event.Data.MegaMission.Target; // RA2: as above
	} else if (event.Data.MegaMission.Follow.Is_Valid()) { // RA2: as above
		target = &event.Data.MegaMission.Follow; // RA2: as above
	} else if (event.Data.MegaMission.Destination.Is_Valid()) { // RA2: as above
		target = &event.Data.MegaMission.Destination; // RA2: as above
	} else { // RA2: as above
		return; // RA2: as above
	}
	AbstractClass * object = target->As_Abstract(); // RA2: as above
	if (object) { // RA2: as above
		EventClass remove(PlayerPtr->HeapID, EventClass::PLAN_NODE_DELETE, TargetClass(object)); // RA2: as above
		Add_Outgoing_Event(remove); // RA2: as above
	}
}

bool __fastcall Planning_Allows_Event_Type(int type) // RA2: 0x00614B50; whether an event type may be sent now: no mission events while planning mode is on
{
	if (PlanningNodeClass::PlanningModeActive && type == 4) { // RA2: as above
		return false; // RA2: as above
	}
	return true; // RA2: as above
}

void __stdcall Planning_Send_Event(EventClass event) // RA2: 0x00614B70; sends a mission event marked as planned
{
	if (event.Type == EventClass::MEGAMISSION) { // RA2: as above
		event.Data.MegaMission.IsPlanningEvent = true; // RA2: as above
		Add_Outgoing_Event(event); // RA2: as above
	}
}

extern void __fastcall Show_Player_Message(wchar_t const * text, int delay); // RA2: 0x006F58E0 (uicmnds); puts a text on the message list in the player's color, -1 for the default delay

inline void __fastcall Planning_Message(char const * label) // RA2: 0x00616FB0 (expanded in Planning_Refuse); shows a text of the string table
{
	wchar_t const * text = StringTable::LoadString(label, NULL, "D:\\Ra2\\PlanMgr.cpp", 3231); // RA2: as above
	Show_Player_Message(text, 0x1E0); // RA2: as above
}

inline void __fastcall Planning_Refuse(char const * label) // RA2: 0x00616FD0; shows a refusal once per frame and scolds
{
	if (!PlanningRefusalShown) { // RA2: as above
		PlanningRefusalShown = true; // RA2: as above
		Planning_Message(label); // RA2: as above
		VocClass::PlayGlobal(Rule->ScoldSound, 1.0f, 0x2000); // RA2: as above
	}
}

inline bool __fastcall Planning_Target_Fits(int mission, TargetClass target) // RA2: no function of its own; whether an attack or an area guard order aims at a cell or (for the area guard) at nothing
{
	if (mission == 1 || mission == 11) { // RA2: as above
		do { // RA2: as above
			if (RTTIType(target) == RTTI_CELL) { // RA2: as above
				break; // RA2: as above
			}
			if (mission != 11 || target.Is_Valid()) { // RA2: as above
				return false; // RA2: as above
			}
		} while (false); // RA2: as above
		return true; // RA2: as above
	}
	return false; // RA2: as above
}

inline bool __fastcall Planning_Mission_Allows(TechnoClass * techno) // RA2: 0x00615A10; whether the current mission of a techno lets planning mode take over: guarding, or attacking or guarding an area with a cell or (for the area) nothing as the target
{
	if (techno) { // RA2: as above
		int mission = techno->Get_Mission(); // RA2: as above
		if (mission == 5) { // RA2: as above
			return true; // RA2: as above
		}
		return Planning_Target_Fits(mission, TargetClass(techno->TarCom)); // RA2: as above
	}
	return false; // RA2: as above
}

inline void __fastcall Planning_Refuse_Order(EventClass const * event) // RA2: 0x00617020; says why planning mode refuses an order of this type
{
	if (event->Type == 9) { // RA2: as above
		Planning_Refuse("MSG:PlanningModeNoDeploy"); // RA2: as above
	} else if (event->Type == 6) { // RA2: as above
		Planning_Refuse("MSG:PlanningModeNoStop"); // RA2: as above
	} else if (event->Type == 7) { // RA2: as above
		Planning_Refuse("MSG:PlanningModeNoScatter"); // RA2: as above
	}
}

inline bool __fastcall Planning_Terminal_Mission(int mission) // RA2: 0x006159C0 (no function of its own); the missions that end a path
{
	return mission == 7 || mission == 8 || mission == 9; // RA2: as above
}

inline bool __fastcall Planning_Cell_Mission(int mission, TargetClass target) // RA2: 0x006159E0; an attack or an area guard aimed at a cell, or for the area guard at nothing
{
	if (mission == 1 || mission == 11) { // RA2: as above
		if (RTTIType(target) == RTTI_CELL) { // RA2: as above
			return true; // RA2: as above
		}
		if (mission == 11 && !target.Is_Valid()) { // RA2: as above
			return true; // RA2: as above
		}
	}
	return false; // RA2: as above
}

inline int __fastcall Planning_Event_Result(PlanningTokenClass * token, PlanningEventTargets const * event) // RA2: 0x006171E0; what planning mode shows for an order: 0 for one that ends the path, 2 at the last free node, -1 for none
{
	int mission = event->CurrentMission(); // RA2: as above
	TargetClass const & target = event->Target(); // RA2: as above
	if (Planning_Terminal_Mission(mission)) { // RA2: as above
		return 0; // RA2: as above
	}
	if (Planning_Cell_Mission(mission, target)) { // RA2: as above
		return 0; // RA2: as above
	}
	if (token->Node_Count() == 15) { // RA2: as above
		return 2; // RA2: as above
	}
	return -1; // RA2: as above
}

inline bool Planning_Valid_House(int house) // RA2: no function of its own; whether a house number indexes the counters
{
	return house >= 0 && house < 8; // RA2: as above
}

inline void __fastcall Planning_Recount(int house) // RA2: 0x00616D00; raises the count of a house
{
	if (Planning_Valid_House(house)) { // RA2: as above
		PlanningUnitCounts[house]++; // RA2: as above
	}
}

inline void __fastcall Planning_Set_Pending_Refusal(int value) // RA2: 0x00617240; sets the refusal to show at the end of the frame
{
	PlanningPendingRefusal = value; // RA2: as above
}

inline bool Planning_Token_Looped(PlanningTokenClass * token) // RA2: no function of its own; the path of the token has closed a loop
{
	return token->Loop_Start_Positive() || token->Loop_Start_Zero(); // RA2: as above
}

inline void __fastcall Planning_Attach_Order(PlanningNodeClass * const & node, PlanningTokenClass * token, TechnoClass * techno, EventClass * event) // RA2: no function of its own; puts the techno on the node: with the members of the last node of its path, or alone
{
	PlanningNodeClass * last = token->Last_Node(); // RA2: as above
	if (last) { // RA2: as above
		Planning_Copy_Members(node, last, *event); // RA2: as above
	} else { // RA2: as above
		node->Add_Member(techno, event); // RA2: as above
	}
}

PlanningTokenClass * __fastcall Planning_New_Token(TechnoClass * techno) // RA2: 0x00615820; makes the planning token of a techno and lists it
{
	PlanningTokenClass * token = new PlanningTokenClass(techno); // RA2: as above
	PlanningTokenClass::Array.Add(token); // RA2: as above
	techno->Set_Planning_Token(token); // RA2: as above
	return token; // RA2: as above
}

inline int Planning_Token_Index(PlanningTokenClass * token, PlanningNodeClass * node) // RA2: no function of its own; the index of a node in the path of a token
{
	return token->Node_Index(node); // RA2: as above
}

inline void __fastcall Planning_Close_Loop(TechnoClass * techno, PlanningNodeClass * node) // RA2: 0x006164E0; makes the path of a techno loop back to a node
{
	int start = Planning_Token_Index(techno->Get_Planning_Token(), node); // RA2: as above
	PlanningTokenClass * token = techno->Get_Planning_Token(); // RA2: as above
	token->ClosedLoopNodeCount = token->PlanningNodes.Count() - start; // RA2: as above
	token->StepsToClosedLoop = start; // RA2: as above
	token->F98 = true; // RA2: as above
	if (!PlanningLoopClosed) { // RA2: as above
		if (techno->House->Is_Player_Control()) { // RA2: as above
			PlanningLoopClosed = true; // RA2: as above
			PlanningNodeClass * last = token->Last_Node(); // RA2: as above
			int count = last->PlanningMembers.Count(); // RA2: as above
			for (int i = 0; i < count; i++) { // RA2: as above
				TechnoClass * member = last->PlanningMembers[i]->Owner; // RA2: as above
				if (member->IsSelected) { // RA2: as above
					member->Unselect(); // RA2: as above
				}
			}
		}
		Planning_Convert_Loop_Moves(techno); // RA2: as above
	}
	node->Update_Active(); // RA2: as above
}

inline void __fastcall Planning_Path_Add(TechnoClass * techno, PlanningNodeClass * node) // RA2: 0x00616350; puts a node on the path of a techno, closing the path into a loop when it holds the node already
{
	PlanningTokenClass * token = techno->Get_Planning_Token(); // RA2: as above
	if (!token) { // RA2: as above
		token = Planning_New_Token(techno); // RA2: as above
	}
	if (token->Node_Index(node) != -1) { // RA2: as above
		Planning_Close_Loop(techno, node); // RA2: as above
	} else { // RA2: as above
		Planning_Path_Append(techno, node); // RA2: as above
	}
}

inline PlanningNodeClass * __fastcall Planning_Find_Node(int house, int id, Coord coord) // RA2: 0x00615FB0; the planned node at a coordinate (of an id and a house unless -1)
{
	int count = PlanningNodeClass::Unknown1.Count(); // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		PlanningNodeClass * node = PlanningNodeClass::Unknown1[i]; // RA2: as above
		if (node->Lead_At(coord) && (id == -1 || id == node->Id)) { // RA2: as above
			if (house == -1 || house == Planning_House(node->Member_Owner(0))) { // RA2: as above
				return node; // RA2: as above
			}
		}
	}
	return NULL; // RA2: as above
}

void __fastcall Planning_Execute(EventClass * event) // RA2: 0x00614BA0; carries out a planning event: plan commit restarts the paths of the event's house, plan connect adds the node at the second target's coordinate to the first target's path, plan node delete takes the node at the target's coordinate out of every path (remembering the next node for the player); any other event is an order for its techno
{
	if (event->Type == EventClass::PLAN_COMMIT) { // RA2: as above
		Commit_Plans(event->ID); // RA2: as above
	} else if (event->Type == EventClass::PLAN_CONNECT) { // RA2: as above
		TechnoClass * techno = event->Data.NavCom.Whom.As_Techno(); // RA2: as above
		if (techno) { // RA2: as above
			AbstractClass * object = event->Data.NavCom.Where.As_Abstract(); // RA2: as above
			if (object) { // RA2: as above
				Coord coord = object->As_Coord(); // RA2: as above
				PlanningNodeClass * node = Planning_Find_Node(event->ID, -1, coord); // RA2: as above
				if (node) { // RA2: as above
					Planning_Path_Add(techno, node); // RA2: as above
				}
			}
		}
	} else if (event->Type == EventClass::PLAN_NODE_DELETE) { // RA2: as above
		AbstractClass * object = event->Data.Target.Whom.As_Abstract(); // RA2: as above
		if (object) { // RA2: as above
			Coord coord = object->As_Coord(); // RA2: as above
			int house = event->ID; // RA2: as above
			PlanningNodeClass * node = Planning_Find_Node(house, -1, coord); // RA2: as above
			if (node) { // RA2: as above
				TechnoClass * owner = node->PlanningMembers[0]->Owner; // RA2: as above
				int index = owner->Get_Planning_Token()->Node_Index(node); // RA2: as above
				PlanningNodeClass * next = owner->Get_Planning_Token()->Node_At(index + 1); // RA2: as above
				Planning_Remove_Node(node); // RA2: as above
				if (next && house == PlayerPtr->HeapID) { // RA2: as above
					PlanningSelectedNode = next; // RA2: as above
				}
			}
		}
	} else { // RA2: as above
		TechnoClass * techno = event->Data.Target.Whom.As_Techno(); // RA2: as above
		Planning_Add_Order(techno, event); // RA2: as above
	}
}

bool __fastcall Planning_Add_Order(TechnoClass * techno, EventClass * event) // RA2: 0x00614EC0; plans an order for a techno: adds it to the node at the order's coordinate, or to a new node, unless the order or the techno's path refuses it
{
	if (!techno) { // RA2: as above
		return false; // RA2: as above
	}
	bool player = event->ID == PlayerPtr->HeapID; // RA2: as above
	Coord const & coord = Planning_Event_Coord(*event); // RA2: as above
	if (coord == Coord(-1, -1, -1)) { // RA2: as above
		return false; // RA2: as above
	}
	int id = event->Frame; // RA2: as above
	PlanningNodeClass * node = Planning_Find_Node(event->ID, id, coord); // RA2: as above
	if (node && node->Has_Member(techno)) { // RA2: as above
		return true; // RA2: as above
	}
	int kind = Planning_Order_Kind(techno, (PlanningEventTargets const *)event); // RA2: as above
	if (kind != -1) { // RA2: as above
		if (!player) { // RA2: as above
			return false; // RA2: as above
		}
		if (kind == 0) { // RA2: as above
			Planning_Refuse_Order(event); // RA2: as above
		} else if (kind == 1) { // RA2: as above
			Planning_Refuse("MSG:PostTerminatingCommand"); // RA2: as above
		} else if (kind == 2) { // RA2: as above
			Planning_Refuse("MSG:PostContinualCommand"); // RA2: as above
		}
		return false; // RA2: as above
	}
	PlanningTokenClass * token = techno->Get_Planning_Token(); // RA2: as above
	if (!token) { // RA2: as above
		token = Planning_New_Token(techno); // RA2: as above
	}
	if (player) { // RA2: as above
		int result = Planning_Event_Result(token, (PlanningEventTargets const *)event); // RA2: as above
		if (result != -1) { // RA2: as above
			Planning_Set_Pending_Refusal(result); // RA2: as above
		}
	}
	if (Planning_Token_Looped(token)) { // RA2: as above
		if (player) { // RA2: as above
			Planning_Refuse("MSG:PostLoop"); // RA2: as above
		}
		return false; // RA2: as above
	}
	if (token->Node_Count() == 0) { // RA2: as above
		PlanningTokenClass::Technos.Add(techno); // RA2: as above
		Planning_Recount(event->ID); // RA2: as above
	} else if (token->Node_Count() == 16) { // RA2: as above
		if (player) { // RA2: as above
			Planning_Refuse("MSG:NodeMaximum"); // RA2: as above
		}
		return false; // RA2: as above
	}
	if (!node) { // RA2: as above
		node = new PlanningNodeClass(event->Frame); // RA2: as above
		PlanningNodeClass::Unknown1.Add(node); // RA2: as above
	}
	Planning_Attach_Order(node, token, techno, event); // RA2: as above
	if (player) { // RA2: as above
		VocClass::PlayGlobal(Rule->AddPlanningModeCommandSound, 1.0f, 0x2000); // RA2: as above
	}
	return true; // RA2: as above
}

inline bool TechnoClass::TryNextPlanningTokenNode(void) // RA2: 0x00615360; starts carrying out the techno's planned path once: queues the planning mission, holds the token's first event and marks the token started
{
	PlanningTokenClass * token = Get_Planning_Token(); // RA2: as above
	if (token != NULL && token->Node_Count() > 0 && (token->Get_F8C() == -1 || token->Get_F8C() > 0) && !token->Get_F1C()) { // RA2: as above
		QueueMission(MISSION_WAIT, true); // RA2: as above
		EventClass event = token->Pop_Front(); // RA2: as above
		Planning_Store_Event(event); // RA2: as above
		token->Set_F1C(); // RA2: as above
		return true; // RA2: as above
	}
	return false; // RA2: as above
}

inline void __fastcall Planning_Uncount(int house) // RA2: 0x00616D20 (outside this part); lowers the count of a house, never below 0
{
	if (house >= 0 && house < 8) { // RA2: as above
		int value = PlanningUnitCounts[house] - 1; // RA2: as above
		PlanningUnitCounts[house] = (value < 0) ? 0 : value; // RA2: as above
	}
}

inline int __fastcall Planning_Bright_Color(TechnoClass * techno) // RA2: 0x00619D50 (outside this part); the pixel of the house color brought 100/256 toward white
{
	RGBClass color = Houses[Planning_House(techno)]->RemapColorRGB; // RA2: as above
	color.Adjust(100, WhiteColor); // RA2: as above
	return ((color.Get_Green() >> DSurface::GreenLeft) << DSurface::GreenRight) // RA2: as above
		| ((color.Get_Red() >> DSurface::RedLeft) << DSurface::RedRight) // RA2: as above
		| ((color.Get_Blue() >> DSurface::BlueLeft) << DSurface::BlueRight); // RA2: as above
}
