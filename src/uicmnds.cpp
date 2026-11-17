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
**	RA2: the commands the keyboard and the interface give the player: deploy, stop, guard, the
**	control groups, attack move, selection by type, state or rank. TibSun has no such file; the
**	code is the executable's from 0x006F57E0.
*/

#include "always.h" // RA2: the includes this file needs
#include <string.h> // RA2: as above
#include "vector.h" // RA2: as above
#include "_tactica.h" // RA2: as above
#include "globals.h" // RA2: as above
#include "house.h" // RA2: as above
#include "tactical.h" // RA2: as above
#include "techno.h" // RA2: as above
#include "foot.h" // RA2: as above
#include "infantry.h" // RA2: as above
#include "building.h" // RA2: as above
#include "event.h" // RA2: as above
#include "target.h" // RA2: as above
#include "msglist.h" // RA2: as above
#include "session.h" // RA2: as above
#include "scenario.h" // RA2: as above
#include "voc.h" // RA2: as above
#include "_rules.h" // RA2: as above
#include "rules.h" // RA2: as above
#include "queue.h" // RA2: as above
#include "_map.h" // RA2: as above
#include "display.h" // RA2: as above
#include "planmgr.h" // RA2: as above

#include "_keyboar.h" // RA2: as above
#include "goptions.h" // RA2: as above
#include "mouse.h" // RA2: as above
#include "planning.h" // RA2: as above
#include "stringtable.h" // RA2: as above
#include "beacon.h" // RA2: as above
#include <wchar.h> // RA2: as above
#include <stdio.h> // RA2: as above
#include "tab.h" // RA2: the declarations of the commands this file defines

typedef DynamicVectorClass<TechnoClass *> TechnoList; // RA2: as above
typedef bool (__fastcall * TechnoTest)(TechnoClass * techno, int argument); // RA2: a test the selection routines apply to an object
typedef void (__fastcall * TechnoAction)(TechnoClass * techno, int argument); // RA2: an action the selection routines apply to an object

TechnoList * TypeSelectList; // RA2: 0x00AC1344; the objects a selection by type or state works on

extern int MuteSWLaunches; // RA2: 0x00A3D548, nonzero while the launch announcements are muted
extern PlanningNodeClass * Planning_Selected_Node(void); // RA2: 0x00617A00 (planmgr); the planned node the player has selected, if any
extern void __fastcall Planning_Show_Help(bool sound, bool enable); // RA2: 0x00616EC0 (planmgr); shows the help text of planning mode (when it is turned on or off), the number of times per kind it allows
extern void __fastcall Planning_Refuse_Area_Guard(void); // RA2: 0x00616E70 (planmgr); refuses the guard command while planning mode is on

/// <summary>
/// Frees the list of objects a type, state or rank selection works on.
/// </summary>
void Free_Type_Select_List(void) // RA2: 0x006F57E0
{
	if (TypeSelectList != NULL) { // RA2: as above
		delete TypeSelectList; // RA2: the list is deleted through its virtual destructor (the call at 0x006F57EE passes 1) and the pointer cleared
		TypeSelectList = NULL; // RA2: as above
	}
}

/// <summary>
/// Whether an object belongs to the player's side, which depends on the game type.
/// </summary>
static inline bool Is_Player_Side(TechnoClass * techno) // RA2: no function of its own
{
	if (Session.Type == GAME_NORMAL) { // RA2: as above
		return techno->House->IsPlayerControl; // RA2: a normal game reads the house flag at +0x135 directly, any other game type calls HouseClass::Is_Player_Control (0x004F52B0)
	}
	return techno->House->Is_Player_Control(); // RA2: as above
}

/// <summary>
/// Whether every object of the player's control group is selected.
/// </summary>
bool __fastcall Group_Is_All_Selected(int group) // RA2: 0x006F5800
{
	int count = Technos.Count(); // RA2: the executable walks the whole object list of the game (Technos), skipping empty slots and objects in limbo
	for (int index = 0; index < count; index++) { // RA2: as above
		TechnoClass * techno = Technos[index]; // RA2: as above
		if (techno != NULL && !techno->IsInLimbo && Is_Player_Side(techno) && !techno->IsSelected && techno->Group == group - 1) { // RA2: as above
			return false; // RA2: as above
		}
	}
	return true; // RA2: as above
}

/// <summary>
/// Counts the player's objects in a control group.
/// </summary>
int __fastcall Group_Count(int group) // RA2: 0x006F5870
{
	int members = 0; // RA2: counts the player's objects whose Group (the number minus 1) matches
	int count = Technos.Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		TechnoClass * techno = Technos[index]; // RA2: as above
		if (techno != NULL && !techno->IsInLimbo && Is_Player_Side(techno) && techno->Group == group - 1) { // RA2: as above
			members++; // RA2: as above
		}
	}
	return members; // RA2: as above
}

/// <summary>
/// Puts a text on the player's message list in the player's color.
/// </summary>
void __fastcall Show_Player_Message(wchar_t const * text, int delay) // RA2: 0x006F58E0
{
	if (delay == -1) { // RA2: as above
		delay = 240; // RA2: -1 stands for the default time, 240 frames
	}
	int color = 3; // RA2: as above
	if (PlayerPtr != NULL) { // RA2: as above
		color = PlayerPtr->Scheme; // RA2: as above
	}
	Session.Messages.Add_Message(NULL, 0, text, color, TextPrintType(0x4046), delay, true); // RA2: as above
}

/// <summary>
/// The style the player's messages are printed in.
/// </summary>
int Message_Style(void) // RA2: 0x006F5920; no caller in the executable, Show_Player_Message has the value inline
{
	return 0x4046; // RA2: as above
}

/// <summary>
/// How many frames the player's messages stay by default.
/// </summary>
int Message_Delay(void) // RA2: 0x006F5930; no caller in the executable, Show_Player_Message has the value inline
{
	return 240; // RA2: as above
}

/// <summary>
/// Deploys the selected objects that can deploy, or has an occupied building let its occupants out.
/// </summary>
void __cdecl Command_Deploy(void) // RA2: 0x006F5940
{
	bool deployed = false; // RA2: the first selected object is a building with occupants: it lets them out (the deploy event) and nothing else happens
	if (CurrentObject.Count() <= 0) { // RA2: as above
		return; // RA2: as above
	}
	if (CurrentObject[0] != NULL) { // RA2: as above
		BuildingClass * building = (BuildingClass *)CurrentObject[0]; // RA2: as above
		if (building != NULL && building->Fetch_RTTI() == RTTI_BUILDING && building->GetOccupantCount() > 0 && building->House->Is_Player_Control()) { // RA2: as above
			ObjectClass * object = CurrentObject[0]; // RA2: as above
			if (object != NULL && (object->AbstractFlags & 1) && ((TechnoClass *)object)->ClickedEvent(EventClass::DEPLOY)) { // RA2: as above
				VocClass::PlayGlobal(Rule->DeploySound, 1.0f, 0x2000); // RA2: as above
			}
			return; // RA2: as above
		}
	}
	int count = CurrentObject.Count(); // RA2: as above
	bool undeployed = false; // RA2: as above
	int index = 0; // RA2: as above
	bool deploying = false; // RA2: as above
	for (; index < count; index++) { // RA2: as above
		ObjectClass * object = CurrentObject[index]; // RA2: as above
		if (object != NULL && object->Fetch_RTTI() == RTTI_INFANTRY && ((InfantryClass *)object)->Is_Deployer()) { // RA2: as above
			if (((InfantryClass *)object)->Is_Deploying()) { // RA2: as above
				deploying = true; // RA2: as above
			} else {
				undeployed = true; // RA2: as above
			}
			if (deploying && undeployed) { // RA2: as above
				break; // RA2: as above
			}
		}
	}
	for (index = 0; index < count; index++) { // RA2: as above
		ObjectClass * object = CurrentObject[index]; // RA2: as above
		if (object != NULL && (object->Can_Player_Move() || object->Can_Player_Fire()) && (object->AbstractFlags & 1) && ((TechnoClass *)object)->Can_Attack_Now() && ((TechnoClass *)object)->Can_Deploy_Now()) { // RA2: as above
			if (object->Fetch_RTTI() == RTTI_INFANTRY && ((InfantryClass *)object)->Is_Deployer() && deploying && undeployed && ((InfantryClass *)object)->Is_Deploying()) { // RA2: as above
				continue; // RA2: as above
			}
			if (((TechnoClass *)object)->ClickedEvent(EventClass::DEPLOY)) { // RA2: as above
				deployed = true; // RA2: as above
			}
		}
	}
	if (deployed) { // RA2: as above
		VocClass::PlayGlobal(Rule->DeploySound, 1.0f, 0x2000); // RA2: as above
	}
}

/// <summary>
/// Orders the selected objects to guard the area they stand in.
/// </summary>
void __fastcall Command_Guard(void) // RA2: 0x006F5B10
{
	if (Planning_Is_Active()) { // RA2: as above
		Planning_Refuse_Area_Guard(); // RA2: planning mode refuses the order and says so (0x00616E70)
		return; // RA2: as above
	}
	if (CurrentObject.Count() > 0) { // RA2: as above
		bool agreed = true; // RA2: as above
		int count = CurrentObject.Count(); // RA2: as above
		for (int index = 0; index < count; index++) { // RA2: as above
			ObjectClass * object = CurrentObject[index]; // RA2: as above
			if (object != NULL && (object->AbstractFlags & 1) && object->Can_Player_Move() && object->Can_Player_Fire()) { // RA2: as above
				agreed = ((TechnoClass *)object)->Player_Assign_Mission(MISSION_GUARD_AREA, (AbstractClass *)object->Get_Target_Cell_Ptr(), NULL, NULL); // RA2: as above
				if (!agreed) { // RA2: as above
					return; // RA2: as above
				}
			}
		}
		if (agreed) { // RA2: as above
			VocClass::PlayGlobal(Rule->GuardSound, 1.0f, 0x2000); // RA2: as above
		}
	}
}

/// <summary>
/// Orders the selected objects to stop.
/// </summary>
void __cdecl Command_Stop(void) // RA2: 0x006F5BC0
{
	if (CurrentObject.Count() > 0) { // RA2: as above
		bool agreed = true; // RA2: every selected techno that can move or fire gets the idle event; the stop sound plays when all agreed
		int count = CurrentObject.Count(); // RA2: as above
		for (int index = 0; index < count; index++) { // RA2: as above
			ObjectClass * object = CurrentObject[index]; // RA2: as above
			if (object != NULL && (object->AbstractFlags & 1) && (object->Can_Player_Move() || object->Can_Player_Fire())) { // RA2: as above
				agreed = ((TechnoClass *)object)->ClickedEvent(EventClass::IDLE); // RA2: as above
				if (!agreed) { // RA2: as above
					break; // RA2: as above
				}
			}
		}
		if (agreed) { // RA2: as above
			VocClass::PlayGlobal(Rule->StopSound, 1.0f, 0x2000); // RA2: as above
		}
	}
}

int CheerFrame; // RA2: 0x00AC134C; the frame the last cheer was queued, plus the cheer rate

/// <summary>
/// Queues a cheer event once the cheer rate allows another one.
/// </summary>
void __cdecl Command_Cheer(void) // RA2: 0x006F5C50
{
	if (Frame >= CheerFrame && MuteSWLaunches == 0) { // RA2: as above
		OutList.Add(EventClass(PlayerPtr->HeapID, EventClass::ALL_CHEER)); // RA2: a cheer event goes out at most once per MaximumCheerRate frames, never while launch announcements are muted
		CheerFrame = Rule->MaximumCheerRate + Frame; // RA2: as above
	}
}

/// <summary>
/// Orders the selected objects that can move to scatter.
/// </summary>
void __cdecl Command_Scatter(void) // RA2: 0x006F5D10
{
	if (CurrentObject.Count() > 0) { // RA2: as above
		bool agreed = true; // RA2: every selected techno that can move gets the scatter event; the scatter sound plays when all agreed
		int count = CurrentObject.Count(); // RA2: as above
		for (int index = 0; index < count; index++) { // RA2: as above
			ObjectClass * object = CurrentObject[index]; // RA2: as above
			if (object != NULL && (object->AbstractFlags & 1) && object->Can_Player_Move()) { // RA2: as above
				agreed = ((TechnoClass *)object)->ClickedEvent(EventClass::SCATTER); // RA2: as above
				if (!agreed) { // RA2: as above
					break; // RA2: as above
				}
			}
		}
		if (agreed) { // RA2: as above
			VocClass::PlayGlobal(Rule->ScatterSound, 1.0f, 0x2000); // RA2: as above
		}
	}
}

/// <summary>
/// Makes the selected objects the control group, taking them out of any other group first.
/// </summary>
void __fastcall Group_Assign(int group) // RA2: 0x006F5D90
{
	int count = Technos.Count(); // RA2: the group number is stored minus 1; objects already in the group leave it first, the selected ones enter it
	for (int index = 0; index < count; index++) { // RA2: as above
		TechnoClass * techno = Technos[index]; // RA2: as above
		if (techno != NULL && !techno->IsInLimbo && Is_Player_Side(techno)) { // RA2: as above
			if (techno->Group == group - 1) { // RA2: as above
				techno->Group = -1; // RA2: as above
			}
			if (techno->IsSelected) { // RA2: as above
				techno->Group = group - 1; // RA2: as above
			}
		}
	}
}

/// <summary>
/// Takes every object of the player out of a control group.
/// </summary>
void __fastcall Group_Clear(int group) // RA2: 0x006F5E10
{
	int count = Technos.Count(); // RA2: the group number is stored minus 1 in Group; -1 means no group
	for (int index = 0; index < count; index++) { // RA2: as above
		TechnoClass * techno = Technos[index]; // RA2: as above
		if (techno != NULL && !techno->IsInLimbo && Is_Player_Side(techno) && techno->Group == group - 1) { // RA2: as above
			techno->Group = -1; // RA2: as above
		}
	}
}


/// <summary>
/// Whether the selection is exactly the control group: every member selected and nothing else.
/// </summary>
bool __fastcall Group_Is_Selection(int group) // RA2: 0x006F5E80
{
	for (int index = Technos.Count() - 1; index >= 0; index--) { // RA2: as above
		TechnoClass * techno = Technos[index]; // RA2: walks the object list from the end; an object of the group that is not selected, or a selected one outside it, ends it
		if (techno != NULL && !techno->IsInLimbo && Is_Player_Side(techno)) { // RA2: as above
			if (techno->Group == group - 1 && !techno->IsSelected) { // RA2: as above
				return false; // RA2: as above
			}
			if (techno->Group != group - 1 && techno->IsSelected) { // RA2: as above
				return false; // RA2: as above
			}
		}
	}
	return true; // RA2: as above
}

int GroupSelectFrame = -100; // RA2: 0x007F8BCC; the frame the last control group was selected
int GroupSelectGroup = -1; // RA2: 0x007F8BD0; the control group that was selected last
int RadarJumpFrame = -100; // RA2: 0x007F8BD4; the frame of the last jump to a radar event

/// <summary>
/// Leaves the modes that wait for a click on the map (power, waypoint, repair, sell and beacon).
/// </summary>
static inline void Cancel_Click_Modes(void) // RA2: no function of its own
{
	Map.Power_Mode_Control(0); // RA2: as above
	Map.Waypoint_Mode_Control(0, false); // RA2: as above
	Map.Repair_Mode_Control(0); // RA2: as above
	Map.Sell_Mode_Control(0); // RA2: as above
	Map.Place_Beacon_Mode_Control(0); // RA2: as above
}

/// <summary>
/// Selects the control group; a second press within 25 frames centers the view on it.
/// </summary>
void __fastcall Group_Select(int group) // RA2: 0x006F5F00
{
	Cancel_Click_Modes(); // RA2: a second press of the same group within 25 frames, while the selection is exactly that group, centers the view instead
	if (Frame - GroupSelectFrame < 25 && group == GroupSelectGroup && Group_Is_Selection(group)) { // RA2: as above
		Group_Reselect(group); // RA2: as above
		return; // RA2: as above
	}
	GroupSelectFrame = Frame; // RA2: as above
	GroupSelectGroup = group; // RA2: as above
	TacticalMap->Unselect_All(); // RA2: as above
	AllowVoice = true; // RA2: as above
	for (int index = 0; index < Technos.Count(); index++) { // RA2: as above
		TechnoClass * techno = Technos[index]; // RA2: as above
		if (techno != NULL && !techno->IsInLimbo && techno->Group == group - 1 && Is_Player_Side(techno) && !techno->IsSelected) { // RA2: as above
			techno->Select(); // RA2: as above
			techno->RadarFlashTimer = Rule->RadarCombatFlashTime; // RA2: as above
			AllowVoice = false; // RA2: as above
		}
	}
	AllowVoice = true; // RA2: as above
	TechnoClass::Reset_Action_Line_Timer(); // RA2: as above
}

/// <summary>
/// Selects the control group on top of the selection and centers the view on it.
/// </summary>
void __fastcall Group_Reselect(int group) // RA2: 0x006F60D0
{
	Cancel_Click_Modes(); // RA2: the first selected object keeps the selection only when it is a foot object of this group
	if (CurrentObject.Count() != 0) { // RA2: as above
		ObjectClass * object = CurrentObject[0]; // RA2: as above
		if (!object->Is_Foot() || ((FootClass *)CurrentObject[0])->Group != group - 1) { // RA2: as above
			TacticalMap->Unselect_All(); // RA2: as above
		}
	}
	for (int index = 0; index < Technos.Count(); index++) { // RA2: as above
		TechnoClass * techno = Technos[index]; // RA2: as above
		if (techno != NULL && !techno->IsInLimbo && techno->Group == group - 1 && Is_Player_Side(techno) && !techno->IsSelected) { // RA2: as above
			techno->Select(); // RA2: as above
			AllowVoice = false; // RA2: as above
		}
	}
	Map.Center_Map(); // RA2: as above
	Map.Flag_To_Redraw((GScreenRedrawFlags)1); // RA2: as above
	AllowVoice = true; // RA2: as above
}

/// <summary>
/// Adds the control group to the selection.
/// </summary>
void __fastcall Group_Select_Members(int group) // RA2: 0x006F61F0
{
	Cancel_Click_Modes(); // RA2: the same selection loop as Group_Select, without unselecting first and without the radar flash
	AllowVoice = true; // RA2: as above
	for (int index = 0; index < Technos.Count(); index++) { // RA2: as above
		TechnoClass * techno = Technos[index]; // RA2: as above
		if (techno != NULL && !techno->IsInLimbo && techno->Group == group - 1 && Is_Player_Side(techno) && !techno->IsSelected) { // RA2: as above
			techno->Select(); // RA2: as above
			AllowVoice = false; // RA2: as above
		}
	}
	AllowVoice = true; // RA2: as above
}

/// <summary>
/// Sends an alliance event for a house in a multiplayer game that allows changing alliances.
/// </summary>
void __fastcall Send_Ally_Event(unsigned long house) // RA2: 0x006F62D0
{
	if (Session.Type != GAME_NORMAL && !Scen->Special.IsAllianceFixed && Rule->AlliesAllowed) { // RA2: as above
		OutList.Add(EventClass(PlayerPtr->HeapID, EventClass::ALLY, house)); // RA2: only in a multiplayer game whose scenario does not fix the alliances and where the rules allow them
	}
}

/// <summary>
/// Asks for an alliance with the owner of the selected object when that owner is another human player.
/// </summary>
void __cdecl Command_Ally(void) // RA2: 0x006F6380
{
	if (CurrentObject.Count() != 0 && !PlayerPtr->IsDefeated) { // RA2: as above
		if (CurrentObject[0]->Owner_HouseClass() != PlayerPtr && CurrentObject[0]->Owner_HouseClass()->IsHuman) { // RA2: as above
			unsigned long owner = CurrentObject[0]->Owner(); // RA2: a defeated player cannot ask; the owner must be another human player (IsHuman at +0x134)
			if (Session.Type != GAME_NORMAL && !Scen->Special.IsAllianceFixed && Rule->AlliesAllowed) { // RA2: as above
				OutList.Add(EventClass(PlayerPtr->HeapID, EventClass::ALLY, owner)); // RA2: as above
			}
		}
	}
}


/// <summary>
/// Deletes the planned node the player has selected, or else the player's beacon.
/// </summary>
void __cdecl Command_Delete_Plan_Or_Beacon(void) // RA2: 0x006F6490
{
	PlanningNodeClass * node = Planning_Selected_Node(); // RA2: 0x00617A00 returns the planned node the player picked (PlanningSelectedNode)
	if (node != NULL) { // RA2: as above
		node->Send_Delete_Event(); // RA2: as above
	} else {
		BeaconManager.Remove_Beacon(-1, -1); // RA2: as above
	}
}

/// <summary>
/// Starts placing a beacon.
/// </summary>
void __cdecl Command_Beacon(void) // RA2: 0x006F64B0
{
	if (MuteSWLaunches == 0) { // RA2: as above
		Map.Place_Beacon_Mode_Control(-1); // RA2: -1 asks for the beacon placement mode (DisplayClass::Place_Beacon_Mode_Control, 0x0049CAC0)
	}
}

/// <summary>
/// Turns planning mode on.
/// </summary>
void __fastcall Command_Planning_Mode_On(bool sound) // RA2: 0x006F64D0
{
	if (MuteSWLaunches == 0) { // RA2: as above
		Planning_Begin(); // RA2: 0x00614760 starts planning mode; 0x00616EC0 shows the help for it
		Planning_Show_Help(sound, true); // RA2: as above
	}
}

/// <summary>
/// Turns planning mode off.
/// </summary>
void __fastcall Command_Planning_Mode_Off(bool sound) // RA2: 0x006F64F0
{
	Planning_End(); // RA2: 0x006147B0 ends planning mode; the help is shown only when the launch announcements are not muted
	if (MuteSWLaunches == 0) { // RA2: as above
		Planning_Show_Help(sound, false); // RA2: as above
	}
}

/// <summary>
/// Whether planning mode is on.
/// </summary>
bool __fastcall Is_Planning_Mode_On(void) // RA2: 0x006F6510
{
	return Planning_Is_Active(); // RA2: a plain jump to Planning_Is_Active (0x00614840)
}

/// <summary>
/// Whether every selected object can attack while moving.
/// </summary>
bool __fastcall Selection_Can_Attack_Move(void) // RA2: 0x006F6520
{
	int count = CurrentObject.Count(); // RA2: vtable slot 275 (offset 1100) of the object, CanAttackOnTheMove
	for (int index = 0; index < count; index++) { // RA2: as above
		ObjectClass * object = CurrentObject[index]; // RA2: as above
		if (object != NULL && object->Is_Techno() && !((TechnoClass *)object)->CanAttackOnTheMove()) { // RA2: as above
			return false; // RA2: as above
		}
	}
	return true; // RA2: as above
}

bool AttackMoveMode; // RA2: 0x00AC1330; the next order the player gives is an attack move

/// <summary>
/// Makes the next order an attack move, unless a selected object cannot attack while moving.
/// </summary>
void __cdecl Command_Attack_Move(void) // RA2: 0x006F6570
{
	if (CurrentObject.Count() == 0) { // RA2: as above
		Show_Player_Message(StringTable::LoadString("MSG:NothingSelected", NULL, "D:\\Ra2\\UICmnds.cpp", 745), -1); // RA2: nothing selected: the message MSG:NothingSelected (UICmnds.cpp line 745) and the mode still turns on
	} else {
		int count = CurrentObject.Count(); // RA2: as above
		for (int index = 0; index < count; index++) { // RA2: as above
			ObjectClass * object = CurrentObject[index]; // RA2: as above
			if (object != NULL && object->Is_Techno() && !((TechnoClass *)object)->CanAttackOnTheMove()) { // RA2: as above
				Show_Player_Message(StringTable::LoadString("MSG:AttackMoveUnsupported", NULL, "D:\\Ra2\\UICmnds.cpp", 751), -1); // RA2: as above
				return; // RA2: as above
			}
		}
	}
	AttackMoveMode = true; // RA2: as above
}

/// <summary>
/// Ends the attack move mode.
/// </summary>
void __cdecl Cancel_Attack_Move(void) // RA2: 0x006F6660
{
	AttackMoveMode = false; // RA2: the same store as Clear_Attack_Move_Flag, at its own address with no caller in the executable
}

/// <summary>
/// Whether an order is given as a forced attack move: the mode is on, or both the force-fire and force-move keys are held and every selected object can attack while moving.
/// </summary>
bool __fastcall Can_Force_Order(void) // RA2: 0x006F6670
{
	if (AttackMoveMode) { // RA2: as above
		return true; // RA2: the mode is on, or the force-attack and the select keys are both held (Options at +0x94, +0x98 and +0x9C, +0xA0)
	}
	bool fire = Keyboard->Down(Options.KeyForceAttack1) || Keyboard->Down(Options.KeyForceAttack2); // RA2: as above
	bool select = Keyboard->Down(Options.KeySelect1) || Keyboard->Down(Options.KeySelect2); // RA2: as above
	if (!fire || !select) { // RA2: as above
		return false; // RA2: as above
	}
	int count = CurrentObject.Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		ObjectClass * object = CurrentObject[index]; // RA2: as above
		if (object != NULL && object->Is_Techno() && !((TechnoClass *)object)->CanAttackOnTheMove()) { // RA2: as above
			return false; // RA2: as above
		}
	}
	return true; // RA2: as above
}

/// <summary>
/// A cursor number, 71, that the display class sets at 0x0049B924.
/// </summary>
MouseType Cursor_Type_71(void) // RA2: 0x006F6730
{
	return MouseType(71); // RA2: the executable returns the constant 71, a cursor number (the display class passes it on at 0x0049B924)
}

/// <summary>
/// The area guard cursor.
/// </summary>
MouseType Cursor_Area_Guard(void) // RA2: 0x006F6740
{
	return MouseType(22); // RA2: the executable returns the constant 22 (the area guard cursor)
}

/// <summary>
/// The first frame of cursor 77.
/// </summary>
int Cursor_Start_Frame_77(void) // RA2: 0x006F6750
{
	return Map.Get_Mouse_Start_Frame(MouseType(77), false); // RA2: MouseClass::Get_Mouse_Start_Frame (0x0059F170) for cursor 77, the small variant off
}

/// <summary>
/// The first frame of cursor 76.
/// </summary>
int Cursor_Start_Frame_76(void) // RA2: 0x006F6760
{
	return Map.Get_Mouse_Start_Frame(MouseType(76), false); // RA2: the same for cursor 76
}

/// <summary>
/// Ends the attack move mode.
/// </summary>
void __cdecl Clear_Attack_Move_Flag(void) // RA2: 0x006F6770
{
	AttackMoveMode = false; // RA2: called from the display class at 0x0049BDD7, 0x0049BDF5 and 0x0049C3DB
}

int TypeSelectMode; // RA2: 0x00AC132C; what the last selection by type, state or rank selected by, 0 for none

/// <summary>
/// Sets how the next selection by type works and ends the attack move mode.
/// </summary>
void __fastcall Set_Type_Select_Mode(unsigned mode) // RA2: 0x006F6780
{
	TypeSelectMode = mode; // RA2: stores the mode (0xAC132C) and clears the attack move flag (0xAC1330)
	AttackMoveMode = false; // RA2: as above
}

bool SelectAcrossMap; // RA2: 0x00AC133C; a selection by type or state reaches over the whole map, not only the screen

/// <summary>
/// Puts the message that says what the last selection by type or state covered in the buffer.
/// </summary>
void __fastcall Get_Selection_Scope_Text(wchar_t * buffer) // RA2: 0x006F6790
{
	if (CurrentObject.Count() == 0) { // RA2: as above
		wcscpy(buffer, StringTable::LoadString("MSG:NothingSelected", NULL, "D:\\Ra2\\UICmnds.cpp", 898)); // RA2: MSG:NothingSelected, MSG:SelAcrossMap or MSG:SelAcrossScreen by UICmnds.cpp lines 898, 900 and 902
		return; // RA2: as above
	}
	if (SelectAcrossMap) { // RA2: as above
		wcscpy(buffer, StringTable::LoadString("MSG:SelAcrossMap", NULL, "D:\\Ra2\\UICmnds.cpp", 900)); // RA2: as above
		return; // RA2: as above
	}
	wcscpy(buffer, StringTable::LoadString("MSG:SelAcrossScreen", NULL, "D:\\Ra2\\UICmnds.cpp", 902)); // RA2: as above
}

/// <summary>
/// Puts the message that says how many objects of a kind are selected and what they are worth in the buffer.
/// </summary>
void __fastcall Get_Selection_Info_Text(wchar_t * buffer, wchar_t const * name) // RA2: 0x006F6810
{
	if (TypeSelectList->Count() == 0 || name == NULL) { // RA2: as above
		wcscpy(buffer, StringTable::LoadString("MSG:NavEmpty", NULL, "D:\\Ra2\\UICmnds.cpp", 911)); // RA2: an empty list or no name: MSG:NavEmpty (line 911); the name is upper-cased for the messages
		return; // RA2: as above
	}
	wchar_t upper[100]; // RA2: as above
	wcscpy(upper, name); // RA2: as above
	_wcsupr(upper); // RA2: as above
	int count = CurrentObject.Count(); // RA2: as above
	if (count == 0) { // RA2: as above
		swprintf(buffer, StringTable::LoadString("MSG:NoUnitsSel", NULL, "D:\\Ra2\\UICmnds.cpp", 921), upper); // RA2: as above
		return; // RA2: as above
	}
	int total = 0; // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		ObjectClass * object = CurrentObject[index]; // RA2: as above
		if (object != NULL && object->Is_Techno()) { // RA2: as above
			TechnoClass * techno = (TechnoClass *)object; // RA2: as above
			HouseClass * owner = techno->House; // RA2: as above
			total += techno->Class_Of()->Cost_Of(owner); // RA2: as above
		}
	}
	swprintf(buffer, StringTable::LoadString("MSG:UnitsWorth", NULL, "D:\\Ra2\\UICmnds.cpp", 941), count, upper, total); // RA2: as above
}

/// <summary>
/// Collects every object of the game that passes the test.
/// </summary>
TechnoList * __fastcall Collect_Technos(TechnoTest test, int argument) // RA2: 0x006F6930
{
	TechnoList * list = new TechnoList(0, NULL); // RA2: the same walk as the group functions: the test decides, nothing is skipped here
	int count = Technos.Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		TechnoClass * techno = Technos[index]; // RA2: as above
		if (test(techno, argument)) { // RA2: as above
			list->Add(techno); // RA2: as above
		}
	}
	return list; // RA2: as above
}

/// <summary>
/// Collects every object on the screen that passes the test.
/// </summary>
TechnoList * __fastcall Collect_Visible_Technos(TechnoTest test, int argument) // RA2: 0x006F69F0
{
	TechnoList * list = new TechnoList(0, NULL); // RA2: walks the objects of the last render pass (Tactical 0x006AC7B0 and 0x006AC7C0)
	int count = TacticalMap->Selectable_Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		ObjectClass * object = TacticalMap->Selectable_Object(index); // RA2: the cast to TechnoClass is taken inside the if, so the executable keeps no register for it (matters for Select_Type, which expands this function)
		if (object->Is_Techno()) { // RA2: as above
			TechnoClass * techno = (TechnoClass *)object; // RA2: as above
			if (test(techno, argument)) { // RA2: as above
				list->Add(techno); // RA2: as above
			}
		}
	}
	return list; // RA2: as above
}

/// <summary>
/// Collects the selected objects that are technos.
/// </summary>
TechnoList * Collect_Selected_Technos(void) // RA2: 0x006F6AD0
{
	TechnoList * list = new TechnoList(0, NULL); // RA2: the selected objects that are technos, in selection order
	int count = CurrentObject.Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		ObjectClass * object = CurrentObject[index]; // RA2: as above
		if (object != NULL && object->Is_Techno()) { // RA2: as above
			list->Add((TechnoClass *)object); // RA2: as above
		}
	}
	return list; // RA2: as above
}

/// <summary>
/// Applies the action to every object of the list.
/// </summary>
void __fastcall For_Each_Techno(TechnoList * list, TechnoAction action, int argument) // RA2: 0x006F6B90
{
	int count = list->Count(); // RA2: calls the action for each object of the list in order
	for (int index = 0; index < count; index++) { // RA2: as above
		action((*list)[index], argument); // RA2: as above
	}
}

/// <summary>
/// Applies the action to every object of the list that passes the test.
/// </summary>
void __fastcall For_Each_Techno_If(TechnoList * list, TechnoTest test, TechnoAction action, int test_argument, int argument) // RA2: 0x006F6BC0
{
	int count = list->Count(); // RA2: the test runs first, the action only for objects that pass it
	for (int index = 0; index < count; index++) { // RA2: as above
		TechnoClass * techno = (*list)[index]; // RA2: as above
		if (test(techno, test_argument)) { // RA2: as above
			action(techno, argument); // RA2: as above
		}
	}
}

/// <summary>
/// Whether every object of the list passes the test.
/// </summary>
bool __fastcall All_Technos_Pass(TechnoList * list, TechnoTest test, int argument) // RA2: 0x006F6C10
{
	int count = list->Count(); // RA2: stops at the first object that fails the test
	for (int index = 0; index < count; index++) { // RA2: as above
		if (!test((*list)[index], argument)) { // RA2: as above
			return false; // RA2: as above
		}
	}
	return true; // RA2: as above
}

/// <summary>
/// Whether any object of the list passes the test.
/// </summary>
bool __fastcall Any_Techno_Passes(TechnoList * list, TechnoTest test, int argument) // RA2: 0x006F6C50
{
	int count = list->Count(); // RA2: stops at the first object that passes the test
	for (int index = 0; index < count; index++) { // RA2: as above
		if (test((*list)[index], argument)) { // RA2: as above
			return true; // RA2: as above
		}
	}
	return false; // RA2: as above
}

/// <summary>
/// Finds a name in a list of names; -1 when it is not there.
/// </summary>
int __fastcall Find_Name_Index(DynamicVectorClass<char const *> const & names, char const * name) // RA2: 0x006F6C90
{
	for (int index = 0; index < names.Count(); index++) { // RA2: as above
		if (strcmp(name, names[index]) == 0) { // RA2: as above
			return index; // RA2: a plain strcmp search; -1 when the name is not in the list
		}
	}
	return -1; // RA2: as above
}

int __fastcall Is_Owned_Active(TechnoClass * techno, int argument); // RA2: 0x006F6FE0; defined below

static inline TechnoList * Collect_Owned_Visible(void) // RA2: no function of its own; the objects on the screen that are active and the player's
{
	TechnoList * list = new TechnoList(0, NULL); // RA2: as above
	int count = TacticalMap->Selectable_Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		ObjectClass * object = TacticalMap->Selectable_Object(index); // RA2: as above
		if (object->Is_Techno()) { // RA2: as above
			TechnoClass * techno = (TechnoClass *)object; // RA2: as above
			if (((TechnoTest)Is_Owned_Active)(techno, 0)) { // RA2: as above
				list->Add(techno); // RA2: as above
			}
		}
	}
	return list; // RA2: as above
}

static inline bool Has_Unselected_Combatants(TechnoList * list) // RA2: no function of its own
{
	int count = list->Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		TechnoClass * techno = (*list)[index]; // RA2: as above
		if (techno->Fetch_RTTI() != RTTI_BUILDING && techno->Techno_Type_Class()->IsSelectableCombatant && !techno->IsSelected) { // RA2: as above
			return true; // RA2: as above
		}
	}
	return false; // RA2: as above
}

/// <summary>
/// Selects the player's combatants on the screen, or all over the map when none on the screen is left to select.
/// </summary>
void __fastcall Select_All_Combatants(bool clear) // RA2: 0x006F6D00
{
	if (MuteSWLaunches != 0) { // RA2: as above
		return; // RA2: ignored while launch announcements are muted
	}
	TacticalMap->Unselect_Foreign_Object(); // RA2: as above
	if (TypeSelectMode != 1) { // RA2: as above
		SelectAcrossMap = false; // RA2: as above
	}
	TechnoTest test = (TechnoTest)Is_Owned_Active; // RA2: as above
	TechnoList * list; // RA2: as above
	if (SelectAcrossMap) { // RA2: as above
		list = Collect_Technos(test, 0); // RA2: as above
	} else {
		list = Collect_Owned_Visible(); // RA2: as above
		if (!Has_Unselected_Combatants(list)) { // RA2: as above
			SelectAcrossMap = true; // RA2: as above
		}
		if (SelectAcrossMap) { // RA2: as above
			delete list; // RA2: as above
			list = Collect_Technos(test, 0); // RA2: as above
		}
	}
	if (clear) { // RA2: as above
		TacticalMap->Unselect_All(); // RA2: as above
	}
	int count = list->Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		TechnoClass * techno = (*list)[index]; // RA2: as above
		if (techno->Fetch_RTTI() != RTTI_BUILDING && techno->Techno_Type_Class()->IsSelectableCombatant) { // RA2: as above
			techno->Select(); // RA2: as above
		}
	}
	wchar_t buffer[256]; // RA2: as above
	Get_Selection_Scope_Text(buffer); // RA2: as above
	Show_Player_Message(buffer, -1); // RA2: as above
	delete list; // RA2: as above
	TypeSelectMode = 1; // RA2: as above
	AttackMoveMode = false; // RA2: as above
}

/// <summary>
/// Whether the object is active and belongs to the player's side.
/// </summary>
int __fastcall Is_Owned_Active(TechnoClass * techno, int argument) // RA2: 0x006F6FE0
{
	if (techno != NULL && techno->IsActive && Is_Player_Side(techno)) { // RA2: as above
		return 1; // RA2: the executable returns 1 or 0 as an int, not as a bool
	}
	return 0; // RA2: as above
}

int __fastcall Is_Player_Alive_Unit_Of(TechnoClass * techno, char const * name); // RA2: 0x006F7190; defined below

/// <summary>
/// Takes every object of a kind out of the selection: those on the screen, or on the whole map after a selection by type reached over it.
/// </summary>
void __fastcall Unselect_Type(char const * name) // RA2: 0x006F7020
{
	TechnoTest test = (TechnoTest)Is_Player_Alive_Unit_Of; // RA2: the test pointer Is_Player_Alive_Unit_Of gets the name as its second argument
	TechnoList * list; // RA2: as above
	if (SelectAcrossMap) { // RA2: as above
		list = Collect_Technos(test, (int)name); // RA2: as above
	} else {
		list = Collect_Visible_Technos(test, (int)name); // RA2: as above
	}
	int count = list->Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		(*list)[index]->Unselect(); // RA2: as above
	}
	delete list; // RA2: as above
}

/// <summary>
/// Whether the object is active, the player's, and of the kind of the name.
/// </summary>
int __fastcall Is_Player_Alive_Unit_Of(TechnoClass * techno, char const * name) // RA2: 0x006F7190
{
	if (techno != NULL && techno->IsActive && Is_Player_Side(techno) && techno->Is_Type_Named(name)) { // RA2: as above
		return 1; // RA2: returns an int, 1 or 0, like Is_Owned_Active
	}
	return 0; // RA2: as above
}

/// <summary>
/// Selects every object of a kind: those on the screen, or on the whole map after a selection by type reached over it.
/// </summary>
void __fastcall Select_Type(char const * name) // RA2: 0x006F71E0
{
	bool voice = AllowVoice; // RA2: the voice flag is saved, switched off while the objects are selected and put back at the end
	AllowVoice = false; // RA2: as above
	TechnoTest test = (TechnoTest)Is_Player_Alive_Unit_Of; // RA2: as above
	TechnoList * list = SelectAcrossMap ? Collect_Technos(test, (int)name) : Collect_Visible_Technos(test, (int)name); // RA2: as above
	int count = list->Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		(*list)[index]->Select(); // RA2: as above
	}
	delete list; // RA2: as above
	AllowVoice = voice; // RA2: as above
}

/// <summary>
/// Whether the object is of any of the kinds in the list.
/// </summary>
bool __fastcall Matches_Any_Name(TechnoClass * techno, DynamicVectorClass<char const *> const & names) // RA2: 0x006F7610
{
	int count = names.Count(); // RA2: a loop over the names; the name goes through a local before Is_Type_Named, which the executable needs
	for (int index = 0; index < count; index++) { // RA2: as above
		char const * name = names[index]; // RA2: the name goes through a local first
		if (techno->Is_Type_Named(name)) { // RA2: as above
			return true; // RA2: as above
		}
	}
	return false; // RA2: as above
}

bool TypeSelectKeyHeld; // RA2: 0x00AC133D; the type select key is down (set by Type_Select_Key_Down)
unsigned long TypeSelectKeyTime; // RA2: 0x00AC1340; timeGetTime when the type select key went down

/// <summary>
/// The type select key went down: remembers the moment.
/// </summary>
void Type_Select_Key_Down(void) // RA2: 0x006F7650
{
	if (!TypeSelectKeyHeld) { // RA2: the key press is noted once: the flag at 0x00AC133D and timeGetTime in 0x00AC1340
		TypeSelectKeyHeld = true; // RA2: as above
		TypeSelectKeyTime = timeGetTime(); // RA2: as above
	}
}

/// <summary>
/// Whether the type select key is down.
/// </summary>
bool Is_Type_Select_Key_Held(void) // RA2: 0x006F76B0
{
	return TypeSelectKeyHeld; // RA2: returns the flag at 0x00AC133D
}

/// <summary>
/// Collects the type names of the selected objects, each once.
/// </summary>
DynamicVectorClass<char const *> * Collect_Selected_Type_Names(void) // RA2: 0x006F76C0
{
	DynamicVectorClass<char const *> * names = new DynamicVectorClass<char const *>(0, NULL); // RA2: the kind names of the selected objects, each once; Find_Name_Index and Add are expanded here
	int count = CurrentObject.Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		char const * name = CurrentObject[index]->Class_Of()->Name(); // RA2: as above
		if (Find_Name_Index(*names, name) == -1) { // RA2: as above
			names->Add(name); // RA2: as above
		}
	}
	return names; // RA2: as above
}

/// <summary>
/// Applies the action to every object of a kind: those on the screen, or on the whole map after a selection by type reached over it.
/// </summary>
void __fastcall For_Each_Of_Type(char const * name, TechnoAction action, int argument) // RA2: 0x006F7800
{
	TechnoTest test = (TechnoTest)Is_Player_Alive_Unit_Of; // RA2: the same two walks as Unselect_Type, then the action for each object (a call through the pointer, the argument on the stack)
	TechnoList * list; // RA2: as above
	if (SelectAcrossMap) { // RA2: as above
		list = Collect_Technos(test, (int)name); // RA2: as above
	} else {
		list = Collect_Visible_Technos(test, (int)name); // RA2: as above
	}
	int count = list->Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		action((*list)[index], argument); // RA2: as above
	}
	delete list; // RA2: as above
}

/// <summary>
/// Takes an object that is going away out of the list a type, state or rank selection works on.
/// </summary>
void __fastcall Detach_From_Command_List(AbstractClass * object) // RA2: 0x006F7970
{
	if (TypeSelectList != NULL) { // RA2: the list is searched from its end and the first hit is removed with Delete_Index
		for (int index = TypeSelectList->Count() - 1; index >= 0; index--) { // RA2: the executable searches from the end
			if ((*TypeSelectList)[index] == object) { // RA2: as above
				TypeSelectList->Delete_Index(index); // RA2: as above
				return; // RA2: as above
			}
		}
	}
}

int __fastcall Is_Selectable_Player_Techno(TechnoClass * techno, int argument); // RA2: 0x006F7DF0; defined below

/// <summary>
/// Makes the list a state or rank selection works on: the selected objects, or when none is a techno, the player's objects on the screen that are not buildings.
/// </summary>
void __cdecl Refresh_Type_Select_List(void) // RA2: 0x006F79C0
{
	if (TypeSelectList != NULL) { // RA2: the old list is deleted, the selected technos are collected and, when there are none, the player technos on the screen that are not buildings
		delete TypeSelectList; // RA2: as above
		TypeSelectList = NULL; // RA2: as above
	}
	TypeSelectList = Collect_Selected_Technos(); // RA2: as above
	if (TypeSelectList->Count() == 0) { // RA2: as above
		delete TypeSelectList; // RA2: as above
		TypeSelectList = NULL; // RA2: as above
		TypeSelectList = Collect_Visible_Technos((TechnoTest)Is_Selectable_Player_Techno, 0); // RA2: as above
	}
}



/// <summary>
/// Whether the object is active, the player's, and not a building.
/// </summary>
int __fastcall Is_Selectable_Player_Techno(TechnoClass * techno, int argument) // RA2: 0x006F7DF0
{
	if (techno != NULL && techno->IsActive && Is_Player_Side(techno) && techno->Fetch_RTTI() != RTTI_BUILDING) { // RA2: the test the function above passes to Collect_Visible_Technos; RA2 expands it there
		return 1; // RA2: returns an int, 1 or 0, like Is_Owned_Active
	}
	return 0; // RA2: as above
}

/// <summary>
/// The message that says which health state a selection by health picked.
/// </summary>
wchar_t const * __fastcall Health_Message(int kind) // RA2: 0x006F7E40
{
	if (kind == 0) { // RA2: MSG:Critical, MSG:HeavilyDamaged, MSG:Healthy by UICmnds.cpp lines 1454, 1456, 1458; an empty string otherwise
		return StringTable::LoadString("MSG:Critical", NULL, "D:\\Ra2\\UICmnds.cpp", 1454); // RA2: UICmnds.cpp lines 1454, 1456 and 1458
	}
	if (kind == 1) { // RA2: as above
		return StringTable::LoadString("MSG:HeavilyDamaged", NULL, "D:\\Ra2\\UICmnds.cpp", 1456); // RA2: as above
	}
	if (kind == 2) { // RA2: as above
		return StringTable::LoadString("MSG:Healthy", NULL, "D:\\Ra2\\UICmnds.cpp", 1458); // RA2: as above
	}
	return L""; // RA2: as above
}

/// <summary>
/// The health state the next selection by health picks: the one after the last, -1 stands for none yet.
/// </summary>
int __fastcall Next_Health_Kind(int last) // RA2: 0x006F7EA0
{
	if (last == -1) { // RA2: -1 gives 0, 0 gives 1, 1 gives 2, anything else 0
		return 0; // RA2: as above
	}
	if (last == 0) { // RA2: as above
		return 1; // RA2: as above
	}
	return last == 1 ? 2 : 0; // RA2: as above
}


/// <summary>
/// The message that says which rank a selection by rank picked.
/// </summary>
wchar_t const * __fastcall Rank_Message(int kind) // RA2: 0x006F8130
{
	if (kind == 0) { // RA2: MSG:Elite, MSG:Veteran, MSG:LittleExperience by UICmnds.cpp lines 1528, 1530, 1532; an empty string otherwise
		return StringTable::LoadString("MSG:Elite", NULL, "D:\\Ra2\\UICmnds.cpp", 1528); // RA2: UICmnds.cpp lines 1528, 1530 and 1532
	}
	if (kind == 1) { // RA2: as above
		return StringTable::LoadString("MSG:Veteran", NULL, "D:\\Ra2\\UICmnds.cpp", 1530); // RA2: as above
	}
	if (kind == 2) { // RA2: as above
		return StringTable::LoadString("MSG:LittleExperience", NULL, "D:\\Ra2\\UICmnds.cpp", 1532); // RA2: as above
	}
	return L""; // RA2: as above
}

/// <summary>
/// The rank the next selection by rank picks: the one after the last, -1 stands for none yet.
/// </summary>
int __fastcall Next_Rank_Kind(int last) // RA2: 0x006F8190
{
	if (last == -1) { // RA2: the same steps as Next_Health_Kind, at its own address
		return 0; // RA2: as above
	}
	if (last == 0) { // RA2: as above
		return 1; // RA2: as above
	}
	return last == 1 ? 2 : 0; // RA2: as above
}


/// <summary>
/// Collects the type names of a list of objects, each once.
/// </summary>
DynamicVectorClass<char const *> * __fastcall Collect_Type_Names(TechnoList const & objects) // RA2: 0x006F84A0
{
	DynamicVectorClass<char const *> * names = new DynamicVectorClass<char const *>(0, NULL); // RA2: the same walk as Collect_Selected_Type_Names over a list the caller gives
	int count = objects.Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		char const * name = objects[index]->Class_Of()->Name(); // RA2: as above
		if (Find_Name_Index(*names, name) == -1) { // RA2: as above
			names->Add(name); // RA2: as above
		}
	}
	return names; // RA2: as above
}

/// <summary>
/// Picks the kind of the list's objects that is worth the most but still less than the limit, or the one worth the most; returns its name in a new buffer.
/// </summary>
char * __fastcall Pick_Type_By_Cost(TechnoList const & objects, char const * name, int limit, int * best) // RA2: 0x006F85E0
{
	DynamicVectorClass<char const *> * names = Collect_Type_Names(objects); // RA2: the kinds of the list with what the objects of each cost (Cost_Of of the type for the house of the object, vtable offset 132)
	DynamicVectorClass<int> * costs = new DynamicVectorClass<int>(0, NULL); // RA2: the cost of the objects of each kind
	int count = names->Count(); // RA2: as above
	for (int index = 0; index < count; index++) { // RA2: as above
		costs->Add(0); // RA2: as above
	}
	int total = objects.Count(); // RA2: as above
	for (int item = 0; item < total; item++) { // RA2: as above
		TechnoClass * object = objects[item]; // RA2: as above
		char const * id = object->Class_Of()->Name(); // RA2: as above
		int found = Find_Name_Index(*names, id); // RA2: as above
		HouseClass * house = object->House; // RA2: as above
		int sum = (*costs)[found]; // RA2: as above
		int cost = ((TechnoTypeClass const *)object->Class_Of())->Cost_Of(house); // RA2: as above
		(*costs)[found] = sum + cost; // RA2: as above
	}
	*best = 0; // RA2: as above
	int pick = -1; // RA2: as above
	if (name != NULL) { // RA2: as above
		for (int entry = 0; entry < count; entry++) { // RA2: as above
			if (strcmp(name, (*names)[entry]) != 0) { // RA2: as above
				int cost = (*costs)[entry]; // RA2: as above
				if (cost < limit && cost > *best) { // RA2: as above
					*best = cost; // RA2: as above
					pick = entry; // RA2: as above
				}
			}
		}
	}
	if (name == NULL || *best == 0) { // RA2: as above
		int most = 0; // RA2: as above
		int which = -1; // RA2: as above
		for (int slot = 0; slot < count; slot++) { // RA2: as above
			if ((*costs)[slot] > most) { // RA2: as above
				most = (*costs)[slot]; // RA2: as above
				which = slot; // RA2: as above
			}
		}
		*best = most; // RA2: as above
		pick = which; // RA2: as above
	}
	char const * selected = name; // RA2: as above
	if (pick != -1) { // RA2: as above
		selected = (*names)[pick]; // RA2: as above
	}
	char * result = new char[100]; // RA2: as above
	if (selected != NULL) { // RA2: as above
		strcpy(result, selected); // RA2: as above
	}
	delete names; // RA2: as above
	delete costs; // RA2: as above
	return result; // RA2: as above
}

/// <summary>
/// Forgets the control group and the radar event the last selection came from.
/// </summary>
void Reset_Group_Select(void) // RA2: 0x006F89E0
{
	GroupSelectGroup = -1; // RA2: forgets the control group and the frame (-1 and -100)
	GroupSelectFrame = -100; // RA2: as above
	RadarJumpFrame = -100; // RA2: the frame of the last jump to a radar event, 0x007F8BD4, goes back too
}

TechnoClass * SidebarStructureTabObject; // RA2: 0x00AC1334; the building the structure tab shows
TechnoClass * SidebarDefenseTabObject; // RA2: 0x00AC1338; the building the defense tab shows

/// <summary>
/// Makes a building the one the structure tab or the defense tab shows, by the building type's build category.
/// </summary>
void __fastcall Set_Tab_Building(BuildingClass * building) // RA2: 0x006F8A00
{
	if (building->Class->BuildCat == 5) { // RA2: build category 5 (BuildCat at +0xBA0 of the type) goes to the defense tab slot, any other to the structure tab slot
		SidebarDefenseTabObject = building; // RA2: as above
	} else {
		SidebarStructureTabObject = building; // RA2: as above
	}
}

/// <summary>
/// Forgets an object in the two sidebar tab slots; all of them for NULL.
/// </summary>
void __fastcall Clear_Sidebar_Tab_Object(TechnoClass * techno) // RA2: 0x006F8A20
{
	if (techno == NULL) { // RA2: NULL clears both slots, an object only the slot that holds it
		SidebarStructureTabObject = NULL; // RA2: as above
		SidebarDefenseTabObject = NULL; // RA2: as above
		return; // RA2: as above
	}
	if (techno == SidebarStructureTabObject) { // RA2: as above
		SidebarStructureTabObject = NULL; // RA2: as above
		return; // RA2: as above
	}
	if (techno == SidebarDefenseTabObject) { // RA2: as above
		SidebarDefenseTabObject = NULL; // RA2: as above
	}
}

/// <summary>
/// The building the structure tab shows.
/// </summary>
TechnoClass * Get_Structure_Tab_Object(void) // RA2: 0x006F8A50
{
	return SidebarStructureTabObject; // RA2: returns the slot at 0x00AC1334
}

/// <summary>
/// The building the defense tab shows.
/// </summary>
TechnoClass * Get_Defense_Tab_Object(void) // RA2: 0x006F8A60
{
	return SidebarDefenseTabObject; // RA2: returns the slot at 0x00AC1338
}

/// <summary>
/// Whether this is a TechnoClass object, by its abstract flags.
/// </summary>
bool AbstractClass::Is_Techno(void) const // RA2: 0x006F8A70
{
	if (this == NULL) { // RA2: as above
		return false; // RA2: bit 0 of the abstract flags marks a techno; a null object is none
	}
	bool flag = AbstractFlags & 1; // RA2: as above
	return flag; // RA2: as above
}
