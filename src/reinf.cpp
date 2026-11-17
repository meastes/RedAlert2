/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/* $Header: /CounterStrike/REINF.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : REINF.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : May 24, 1994                                                 *
 *                                                                                             *
 *                  Last Update : July 26, 1996 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Create_Air_Reinforcement -- Creates air strike reinforcement                              *
 *   Create_Special_Reinforcement -- Ad hoc reinforcement handler.                             *
 *   Do_Reinforcements -- Create and place a reinforcement team.                               *
 *   _Consists_Only_Of_Infantry -- Determine if this group consists only of infantry.          *
 *   _Create_Group -- Create a group given team specification.                                 *
 *   _Pop_Group_Out_Of_Object -- Process popping the group out of the object.                  *
 *   _Who_Can_Pop_Out_Of -- Find a suitable host for these reinforcements.                     *
 *   _Need_To_Take -- Examines unit to determine if it should be confiscated.                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "reinf.h"

#include "_map.h"
#include "aircraft.h"
#include "airctype.h"
#include "building.h"
#include "cell.h"
#include "foot.h"
#include "globals.h"
#include "house.h"
#include "ilocos.h"
#include "incdec.h"
#include "inline.h"
#include "mouse.h"
#include "revent.h"
#include "script.h"
#include "taskforc.h"
#include "techno.h"
#include "unit.h"
#include "unittype.h"
#include "vox.h"
#include "iloco.h" // RA2
#include "ipiggy.h" // RA2
#include "scenario.h" // RA2
#include "rules.h" // RA2
#include "infatype.h" // RA2
static FootClass * _Create_Group(TeamTypeClass const * teamtype); // RA2: defined after its first caller
static bool _Pop_Group_Out_Of_Object(FootClass * group, TechnoClass * object); // RA2: defined after its first caller
void Record_Event_Cell(Cell const & cell); // RA2: 0x63C6C0, the ring of recent event cells (used by Do_Reinforcements before its definition)





/***********************************************************************************************
 * _Consists_Only_Of_Infantry -- Determine if this group consists only of infantry.            *
 *                                                                                             *
 *    Use this to determine if the specified group only contains infantry. Such a reinforcement*
 *    group is a candidate for popping out of a building or transport vehicle rather than      *
 *    driving/walking/sailing/flying onto the map under its own power.                         *
 *                                                                                             *
 * INPUT:   first -- Pointer to the first object in the group to examine.                      *
 *                                                                                             *
 * OUTPUT:  bool; Is the entire group composed of infantry type units?                         *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/25/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static bool _Consists_Only_Of_Infantry(FootClass const *first)
{
	while (first != NULL) {
		if (first->Fetch_RTTI() != RTTI_INFANTRY)
			return false;
		first = (FootClass const *)first->Next;
	}
	return true;
}


/***********************************************************************************************
 * _Who_Can_Pop_Out_Of -- Find a suitable host for these reinforcements.                       *
 *                                                                                             *
 *    This routine is used to scan nearby locations to determine if there is a suitable host   *
 *    for these reinforcements to "pop out of" (apologies to Aliens). Typical hosts include    *
 *    buildings and transport vehicles (of any kind).                                          *
 *                                                                                             *
 * INPUT:   origin   -- The cell that should be scanned from. Only this location and immediate *
 *                      adjacent locations will be scanned.                                    *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to a suitable host. If none could be found then NULL is     *
 *          returned.                                                                          *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/25/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static TechnoClass * _Who_Can_Pop_Out_Of(Cell origin)
{
	CellClass * cellptr = &Map[origin];
	TechnoClass * candidate = NULL;

	for (int f = FACING_NONE; f < FACING_COUNT; f++) {
		CellClass * ptr = cellptr;
		if (f != FACING_NONE) {
			ptr = &ptr->Adjacent_Cell(FacingType(f));
		}

		BuildingClass * building = ptr->Cell_Building();
		if (building && building->Strength > 0) {
			candidate = building;
		}

		UnitClass * unit = ptr->Cell_Unit();
		if (unit && unit->Strength && unit->Class->Max_Passengers() > 0) {
			return(unit);
		}
	}
	return(candidate);
}


/***********************************************************************************************
 * Do_Reinforcements -- Create and place a reinforcement team.                                 *
 *                                                                                             *
 *    This routine is called when a reinforcement team must be created and placed on the map.  *
 *    It will create all members of the team and place them at the location determined from    *
 *    the team composition. The reinforcement team should follow team orders until overridden  *
 *    by AI or player intervention.                                                            *
 *                                                                                             *
 * INPUT:   teamtype -- Pointer to the team type to create as a reinforcement.                 *
 *                                                                                             *
 * OUTPUT:  Was the reinforcement successfully created and placed?                             *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/08/1995 JLB : Created.                                                                 *
 *   05/18/1995 JLB : Returns success or failure condition.                                    *
 *   06/19/1995 JLB : Announces reinforcements.                                                *
 *   02/15/1996 JLB : Recognizes team reinforcement location.                                  *
 *=============================================================================================*/
/// <summary>
/// RA2: the drop-pod reinforcement: a PDPLANE paradrop plane (a loaner) is created at the house's edge (or the
/// team's transport origin), told to approach the target cell and carries the group as cargo. The executable has
/// no separate copy: it is inlined into Do_Reinforcements.
/// </summary>
static inline int _Place_Reinforcement(TechnoClass * plane, Cell const & dest) // RA2: not in TibSun, YR's helper
{
	ScenarioInit++; // RA2: as above
	Coord location(dest); // RA2: as above
	int placed = plane->Unlimbo(location, DIR_N); // RA2: as above
	ScenarioInit--; // RA2: as above
	return(placed); // RA2: as above
}


static bool _Create_Paradrop(TeamTypeClass const * teamtype, FootClass * object, Cell origin) // RA2: not in TibSun
{
	Cell dest; // RA2: as above
	TechnoTypeClass const * ptype = (TechnoTypeClass *)AircraftTypes[AircraftTypeClass::From_Name("PDPLANE")]; // RA2: as above
	ScenarioInit++; // RA2: as above
	AircraftClass * plane = (AircraftClass *)ptype->Create_One_Of(object->House); // RA2: as above
	ScenarioInit--; // RA2: as above
	if (plane) { // RA2: as above
		plane->IsALoaner = true; // RA2: as above
		if (teamtype->IsUseTransportOrigin) { // RA2: as above
			dest = teamtype->Get_Transport_Origin(); // RA2: as above
		} else { // RA2: as above
			int source = object->House->Control.Edge; // RA2: as above
			if (source < 0 || source > 3) source = SOURCE_NORTH; // RA2: a bad edge falls back to north
			dest = Map.Calculated_Cell((SourceType)source, CELL_NONE, CELL_NONE, SPEED_WINGED, true, MZONE_NORMAL); // RA2: as above
		} // RA2: as above
		plane->QueueMission(MISSION_PARADROP_APPROACH, false); // RA2: as above
		plane->Assign_Destination(NULL, true); // RA2: as above
		plane->Assign_Target(&Map[origin]); // RA2: as above
		int placed = _Place_Reinforcement(plane, dest); // RA2: as above
		if (placed) { // RA2: as above
			plane->Passenger = true; // RA2: as above
			plane->Cargo.Attach(object); // RA2: as above
			plane->Commence(); // RA2: as above
			return(true); // RA2: as above
		} else { // RA2: as above
			delete plane; // RA2: as above
		} // RA2: as above
	} // RA2: as above
	return(false); // RA2: as above
}


bool Do_Reinforcements(TeamTypeClass const * teamtype, WAYPOINT wp) // RA2: as above
{
	if (!teamtype || !teamtype->TaskForce->ClassCount) return(false); // RA2: no dropship case, no assert

	if (teamtype->Script == NULL || teamtype->Script->MissionCount == 0) { // RA2: the missing-script default is a GUARD mission (TMISSION 5), as in Do_Chrono_Reinforcements
		TeamMissionClass tmission(TMISSION_GUARD, 0); // RA2: as above
		if (teamtype->Script == NULL) { // RA2: as above
			((TeamTypeClass *)teamtype)->Script = new ScriptTypeClass(NULL); // RA2: as above
		} // RA2: as above
		ScriptTypeClass * script = teamtype->Script; // RA2: as above
		int count = script->MissionCount; // RA2: as above
		if (count < MAX_TEAM_MISSIONS) { // RA2: as above
			script->MissionList[count] = tmission; // RA2: as above
			script->MissionCount++; // RA2: as above
		} // RA2: as above
	} // RA2: as above

	bool drop_pod = false;
	if (teamtype->IsDroppod) { // RA2: no Has_Only_Infantry test
		drop_pod = true; // RA2: as above
	}

	FootClass * object = _Create_Group(teamtype);

	/// this dereferences the object before the NULL check below
	object->Team->IsDeleteTypeWhenDone = false; // RA2: never a deleted dropship team type

	/*
	**	Bail on this reinforcement if no reinforcements could be created.
	**	This is probably because the object maximum was reached.
	*/
	if (!object) {
		return(false);
	}

	bool atwaypoint = wp != -1; // RA2: kept in a local
	Cell origin; // RA2: as above
	if (atwaypoint) { // RA2: as above
		origin = Scen->Get_Waypoint_Cell(wp); // RA2: as above
	} else { // RA2: as above
		origin = teamtype->Get_Origin(); // RA2: as above
	} // RA2: as above

	/*
	**	Special case code to handle infantry types that run from a building. This presumes
	**	that infantry are never a transport (which is safe to do).
	*/
	if (!drop_pod && origin != CELL_NONE && _Consists_Only_Of_Infantry(object)) {

		/*
		**	Search for an object that these infantry can pop out of.
		*/
		TechnoClass * candidate = _Who_Can_Pop_Out_Of(origin);

		if (candidate != NULL) {
			return(_Pop_Group_Out_Of_Object(object, candidate));
		}
	}

	bool onmap = atwaypoint && Map.In_Local_Radar(origin, true); // RA2: only a waypoint arrival on the radar map counts as on the map

	bool ok; // RA2: as above
	if (!drop_pod) { // RA2: as above
		ok = Create_Reinforcement(teamtype, object, origin, onmap); // RA2: as above
	} else { // RA2: as above
		ok = _Create_Paradrop(teamtype, object, origin); // RA2: a drop pod team arrives by paradrop plane
	} // RA2: as above

	/*
	**	Announce when the reinforcements have arrived.
	*/
	if (ok && teamtype->House->Is_Ally(PlayerPtr)) { // RA2: no radar-event assignment and no VOX_REINFORCEMENTS here, RA2 records the event cell
		Record_Event_Cell(origin); // RA2: as above
	}

	return(true);
}



/***********************************************************************************************
 * _Create_Group -- Create a group given team specification.                                   *
 *                                                                                             *
 *    This routine will create all members of the group as specified by the team type.         *
 *                                                                                             *
 * INPUT:   teamtype -- Pointer to the team type that specifies what objects should be         *
 *                      created in this group.                                                 *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the first member of the group created.                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/25/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static FootClass * _Create_Group(TeamTypeClass const * teamtype)
{
	assert(teamtype != NULL);

	TeamClass * team = new TeamClass(teamtype, teamtype->House);
	if (team != NULL) {
		team->Force_Active();
	}

	bool hasunload = false;
	ScriptTypeClass * script = teamtype->Script;
	int count = script->MissionCount;
	for (int tm = 0; tm < count; tm++) {
		TeamMissionClass mission = script->MissionList[tm];
		if (mission.Mission == TMISSION_UNLOAD) {
			hasunload = true;
			break;
		}
	}

	bool has_air_transport = team->Has_Air_Transport();

	bool drop_pod = false;
	if (teamtype->IsDroppod) {
		if (teamtype->TaskForce->Has_Only_Infantry()) {
			drop_pod = true;
		}
	}

	/*
	**	Now that the official source for the reinforcement has been determined, the
	**	objects themselves must be created.
	*/
	FootClass * transport = NULL;
	FootClass * object = NULL;
	for (int index = 0; index < teamtype->TaskForce->ClassCount; index++) {
		TechnoTypeClass const * tclass = teamtype->TaskForce->Members[index].Class;

		for (int sub = 0; sub < teamtype->TaskForce->Members[index].Quantity; sub++) {
			ScenarioInit++;
			FootClass * temp = (FootClass *)tclass->Create_One_Of(teamtype->House);
			ScenarioInit--;

			if (temp != NULL) {

				if (tclass->Capacity > 0 && teamtype->IsFull) {
					temp->Storage.Increase_Amount(tclass->Capacity, 0);
				}

				switch (teamtype->VeteranLevel) {
					case 0:
						temp->Veterancy.Set_Dumbass(true);
						break;
					case 1:
						break;
					case 2:
						temp->Veterancy.Set_Veteran(true);
						break;
					case 3:
						temp->Veterancy.Set_Elite(true);
						break;
				}

				/*
				**	Add the member to the team.
				*/
				if (team != NULL) {
					ScenarioInit++;
					bool ok = team->Add(temp);
					ScenarioInit--;
					temp->IsInitiated = true;
				}

				bool transporter = false;
				if (tclass->Max_Passengers() > 0) {
					if (has_air_transport) {
						if (temp->RTTI == RTTI_AIRCRAFT) {
							transporter = true;
						} else {
							transporter = false;
						}
					} else {
						if (temp->RTTI == RTTI_UNIT) {
							transporter = true;
						} else {
							transporter = false;
						}
					}
				}

				/*
				**	Build the list of transporters and passengers.
				*/
				if (transporter) {
					/*
					**	Link to the list of transports.
					*/
					temp->Next = transport;
					transport = temp;
				} else {
					if (drop_pod) {
						temp->Link_DropPod();
					}

					/*
					**	Link to the list of normal objects.
					*/
					temp->Next = object;
					object = temp;
				}
			}
		}
	}

	/*
	**	If the group consists of transports and normal objects, then assign the normal
	**	objects to be passengers on the transport.
	*/
	if (transport != NULL && object != NULL) {
		transport->Cargo.Attach(object);

		/*
		**	HACK ALERT! If the this team has an unload mission, then flag the transport
		**	as a loaner so that it will exit from the map when the unload process is
		**	complete, but only if the transport is an aircraft type.
		*/
		if (hasunload && transport->RTTI == RTTI_AIRCRAFT) {
			transport->IsALoaner = true;
		}
	}

	/*
	**	For JUST transport helicopters, consider the loaner a gift if there are
	**	no passengers.
	*/
	//if (transport != NULL && object == NULL && transport->What_Am_I() == RTTI_AIRCRAFT/* && *((AircraftClass *)transport) == AIRCRAFT_TRANSPORT*/) {
	//	transport->IsALoaner = false;
	//}

	if (transport == NULL && object == NULL) {
		if (team != NULL) delete team;
		return(NULL);
	}

	/*
	**	If this group consists only of non-transport object, then just return with a pointer
	**	to the first member of the group.
	*/
	if (transport == NULL) {
		return(object);
	}

	return(transport);
}



/// <summary>
/// Can the whole reinforcement chain travel by tunnel?
/// This routine is used by the reinforcement handler to decide whether a group should
/// burrow into the map rather than arrive above ground. A single member that lacks the
/// tunnel locomotor disqualifies the entire chain.
/// </summary>
/// <param name="object">The first object of the reinforcement chain to examine.</param>
/// <returns>bool; Can every object in the chain burrow?</returns>
inline bool _Can_Burrow(FootClass * object)
{
	while (object != NULL) {
		TechnoTypeClass const * tclass = object->Techno_Type_Class();
		if (tclass->Locomotor != CLSID_TunnelLocomotion) {
			return(false);
		}
		object = (FootClass *)object->Next;
	}
	return(true);
}


/// <summary>
/// Brings a chain of reinforcement objects onto the map.
/// This routine is used by the reinforcement handlers to deliver a group, arriving either
/// by drop pod, by burrowing in underground, or by driving onto the map from the owning
/// house's edge. Any object that cannot be given a legal arrival cell is destroyed rather
/// than left in limbo.
/// </summary>
/// <param name="object">The first object of the reinforcement chain to deliver.</param>
/// <param name="xcell">The cell the reinforcement should arrive at or near.</param>
/// <param name="atwaypoint">Should the objects appear at the cell rather than travel in
/// from the map edge?</param>
/// <returns>bool; Was at least one object placed onto the map?</returns>
bool Create_Reinforcement(TeamTypeClass const * teamtype, FootClass * object, Cell const & xcell, bool atwaypoint)
{
	bool drop_pod = false;
	if (teamtype->IsDroppod) { // RA2: no Has_Only_Infantry test
		drop_pod = true; // RA2: as above
	}

	bool burrow = _Can_Burrow(object);

	FacingType eface = FACING_N;	// Facing to enter map.

	Cell cell = xcell;

	if (!drop_pod && !burrow && !atwaypoint) {

		/*
		**	The reinforcements must be delivered the old fashioned way -- by moving onto the
		**	map using their own power. First order of business is to determine where they
		**	should arrive from.
		*/
		int source = teamtype->House->Control.Edge; // RA2: an int; the cell is kept (no Calculated_Cell)
		if (source == -1) { // RA2: as above
			source = 0; // RA2: as above
		}

		eface = (FacingType)(source << 1);
	} else {
		cell = Map.Nearby_Location(cell, object->Techno_Type_Class()->Speed); // RA2: the virtual Techno_Type_Class
	}

	Cell newcell = cell;

	FootClass * o = (FootClass *)object->Next;
	object->Next = NULL;
	bool okvoice = false;
	if (newcell != CELL_NONE) {
		while (newcell != CELL_NONE && object != NULL) {
			DirType desiredfacing(eface);
			if (object->Fetch_RTTI() == RTTI_AIRCRAFT) { // RA2: the virtual Fetch_RTTI
				desiredfacing = DirType(DirType(eface).As_Int() - DirType(FACING_135).As_Int() - 1).Snap_To_8();
			}

			ScenarioInit++;

			/*
			**	Pick the location where the reinforcements appear and then place
			**	them there.
			*/
			bool placed = false;

			if (drop_pod) {
				object->Set_Coord(Coord(newcell)); // RA2: Set_Coord of the cell's coordinate, not PositionCoord = cell
				object->Assign_Destination(&Map[newcell], true); // RA2: as above
				object->Locomotion->Move_To(newcell.As_Coord());
				object->UpdateSight(false, false, false, NULL); // RA2: UpdateSight (+1048) instead of Look
				Coord coord = object->PositionCoord; // RA2: the position is copied and the area around it revealed
				Map.RevealArea3(coord, 0, object->LastSightRange + 3, 0); // RA2: as above
				placed = true;
			} else if (burrow) {
				Coord coord = Map[newcell].Cell_Coord() - Coord(0, 0, 400);
				placed = object->Unlimbo(coord, desiredfacing.As_Dir256());
				if (placed) {
					object->PositionCoord += Coord(0, 0, -CELL_LEPTON - object->Height);
					object->Assign_Destination(&Map[newcell], true); // RA2: as above
					object->Set_Speed(1);
					object->Locomotion->Move_To(newcell.As_Coord());
				}
			} else {
				placed = object->Unlimbo(Map[newcell].Cell_Coord(), desiredfacing.As_Dir256());
			}

			if (placed) {
				okvoice = true;

				/*
				**	If this object is part of a team, then the mission for this
				**	object will be guard. The team handler will assign the proper
				**	mission that it should follow.
				*/
				if (object->Fetch_RTTI() != RTTI_AIRCRAFT) { // RA2: the virtual Fetch_RTTI
					object->QueueMission(MISSION_GUARD, false); // RA2: the virtual QueueMission, not Assign_Mission
					object->Commence();
				}

				if (drop_pod) {

					/*
					**	Could not unlimbo at location specified so find an adjacent location that it can
					**	be unlimboed at. If this fails, then abort the whole placement process.
					*/
					FacingType adj;
					for (adj = FACING_N; adj < FACING_COUNT; adj++) {
						Cell trycell = newcell + AdjacentCell[(int)adj & 0x07]; // RA2: the inline table form, the cell first (a plain sum, no Cell copy)
						if (Map.In_Radar(trycell)) {
							newcell = trycell;
							break;
						}
					}
					if (adj >= FACING_COUNT) {
						newcell = CELL_NONE;
					}
				}

			} else {

				/*
				**	Could not unlimbo at location specified so find an adjacent location that it can
				**	be unlimboed at. If this fails, then abort the whole placement process.
				*/
				FacingType adj;
				for (adj = FACING_N; adj < FACING_COUNT; adj++) {
					Cell trycell = Cell(newcell + AdjacentCell[(int)adj & 0x07]); // RA2: the inline table form, the cell first
					if (!Map.In_Radar(trycell) && object->Can_Enter_Cell(&Map[trycell], adj) == MOVE_OK) {
						newcell = trycell;
						break;
					}
				}

				/*
				 * An adjacent location was found, so retry placing this same
				 * object there on the next pass through the loop.
				 */
				if (adj >= FACING_COUNT) {
					newcell = CELL_NONE;
				} else if (newcell != CELL_NONE) {
					ScenarioInit--;
					continue;
				}

				/*
				 * No usable location could be found, so abort placement of this
				 * object entirely.
				 */
				delete object;
			}

			ScenarioInit--;

			object = o;
			if (object != NULL) {
				o = (FootClass *)(ObjectClass *)object->Next;
				object->Next = NULL;
			}
		}

	/*
	 * This delete only fires when no valid arrival cell was ever found (the
	 * loop above never ran). When the loop runs and later aborts by setting
	 * newcell to CELL_NONE, the object it has advanced to is intentionally
	 * left for the trailing teardown -- do not hoist this into an
	 * unconditional post-loop delete.
	 */
	} else if (object != NULL) {
		delete object;
	}

	/*
	**	If there are still objects that could not be placed, then delete them.
	*/
	if (o != NULL) {
		while (o != NULL) {
			FootClass * old = o;
			o = (FootClass *)(ObjectClass *)o->Next;
			old->Next = 0;

			delete old;
		}
	}

	return(okvoice);
}















/***********************************************************************************************
 * _Pop_Group_Out_Of_Object -- Process popping the group out of the object.                    *
 *                                                                                             *
 *    This routine will cause the group to pop out of the object specified.                    *
 *                                                                                             *
 * INPUT:   group    -- Pointer to the first object in the group to be popped out.             *
 *                                                                                             *
 *          object   -- Pointer to the object that the group is to pop out of.                 *
 *                                                                                             *
 * OUTPUT:  bool; Was the group popped out of the specified object?                            *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/25/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
static bool _Pop_Group_Out_Of_Object(FootClass * group, TechnoClass * object)
{
	assert(group != NULL && object != NULL);
	int quantity = 0;

	/*
	**	Take every infantry member of this group and detach it from the group list
	**	and then make it pop out of the candidate source.
	*/
	while (group != NULL) {
		TechnoClass * todo = group;
		group = (FootClass *)(ObjectClass *)group->Next;
		todo->Next = NULL;

		switch ((RTTIType)object->RTTI) {

			/*
			**	The infantry just walks out of a building.
			*/
			case RTTI_BUILDING:
				if (object->Exit_Object(todo) != 2) {
					delete todo;
				} else {
					object->Transmit_Message(RADIO_OVER_OUT);
					++quantity;
				}
				break;

			/*
			**	Infantry get attached to transport vehicles and then unload.
			*/
			case RTTI_UNIT:
			case RTTI_AIRCRAFT:
				object->Cargo.Attach((FootClass *)todo);
				todo->Hidden();
				object->QueueMission(MISSION_UNLOAD, false); // RA2: the virtual QueueMission (+464), not Assign_Mission
				++quantity;
				break;

			default:
				delete todo;
				break;
		}
	}

	return(quantity != 0);
}

/// <summary>
/// RA2: refuses whatever it is asked. Nothing in RA2 calls it; its three arguments are known only from its stack
/// cleanup.
/// </summary>
bool __stdcall Reinforcement_Refused(int, int, int) // RA2: not in TibSun, at 0x63A680
{
	return(false);
}



/***********************************************************************************************
 * Create_Air_Reinforcement -- Creates air strike reinforcement                                *
 *                                                                                             *
 *    This routine is used to launch an airstrike. It will create the necessary aircraft and   *
 *    assign them to attack the target specified. This routine bypasses the normal             *
 *    reinforcement logic since it doesn't need the sophistication of unloading and following  *
 *    team mission lists.                                                                      *
 *                                                                                             *
 * INPUT:   house    -- The perpetrator of this air strike.                                    *
 *                                                                                             *
 *          air      -- The type of aircraft to make up this airstrike.                        *
 *                                                                                             *
 *          number   -- The number of aircraft in this airstrike.                              *
 *                                                                                             *
 *          mission  -- The mission to assign the aircraft.                                    *
 *                                                                                             *
 *          tarcom   -- The target to assign these aircraft.                                   *
 *                                                                                             *
 *          navcom   -- The navigation target to assign (if necessary).                        *
 *                                                                                             *
 * OUTPUT:  Returns the number of aircraft created for this airstrike.                         *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/04/1995 JLB : Commented.                                                               *
 *=============================================================================================*/
int Create_Air_Reinforcement(HouseClass * house, AircraftType air, int number, MissionType mission, AbstractClass * tarcom, AbstractClass * navcom, InfantryType passenger, int passengers) // RA2: the passenger count
{
	int sub;
	TechnoTypeClass const * type = (TechnoTypeClass *)AircraftTypes[air]; // RA2: Create_Air_Reinforcement rewritten (YR's Create_Paradrop_Planes form, Ghidra 0x63A690)

	for (sub = 0; sub < number; sub++) { // RA2: as above
		ScenarioInit++;
		TechnoClass * obj = (TechnoClass *)type->Create_One_Of(house);
		ScenarioInit--;
		if (!obj) return(sub);

		obj->IsALoaner = true;

		int edge = house->Control.Edge; // RA2: as above
		if (edge < 0 || edge > 3) edge = SOURCE_NORTH; // RA2: as above
		Cell newcell = Map.Calculated_Cell((SourceType)edge, CELL_NONE, CELL_NONE, SPEED_WINGED, true, MZONE_NORMAL); // RA2: as above

		obj->QueueMission(mission, false); // RA2: as above
		if (navcom != NULL) {
			obj->Assign_Destination(navcom, true); // RA2: as above
		}
		if (tarcom != NULL) {
			obj->Assign_Target(tarcom);
		}

		ScenarioInit++;
		int placed = obj->Unlimbo(Coord(newcell), DIR_N); // RA2: as above
		ScenarioInit--;
		if (!placed) { // RA2: as above
			if (obj) delete obj; // RA2: as above
			return(sub - 1); // RA2: as above
		}

		if (obj->RTTI == RTTI_AIRCRAFT && passenger != INFANTRY_NONE && passengers != 0) { // RA2: as above
			((AircraftClass *)obj)->Passenger = true; // RA2: as above
			InfantryTypeClass * itype = InfantryTypes[passenger]; // RA2: as above
			if (itype != NULL) { // RA2: as above
				int remaining = passengers; // RA2: as above
				do { // RA2: as above
					ObjectClass * inf = itype->Create_One_Of(house); // RA2: as above
					if (inf) { // RA2: as above
						inf->Limbo(); // RA2: as above
						obj->Cargo.Attach((FootClass *)inf); // RA2: as above
					} // RA2: as above
				} while (--remaining != 0); // RA2: as above
			} // RA2: as above
		}
		obj->Commence();
	}
	return(sub);
}



/// <summary>
/// RA2: brings a team in by chrono warp at its origin (or the waypoint): each member is placed next to the point
/// and warped there by a teleport locomotor riding on its own, and starts with the reinforcement chrono delay.
/// </summary>
bool Do_Chrono_Reinforcements(TeamTypeClass const * teamtype, WAYPOINT wp)
{
	if (!teamtype || !teamtype->TaskForce->ClassCount) return(false);

	ScriptTypeClass * script = teamtype->Script;
	if (!script || script->MissionCount == 0) {
		TeamMissionClass tmission(TMISSION_GUARD, 0);
		if (!script) {
			((TeamTypeClass *)teamtype)->Script = new ScriptTypeClass(NULL);
		}
		script = teamtype->Script;
		int count = script->MissionCount;
		if (count < MAX_TEAM_MISSIONS) {
			script->MissionList[count] = tmission;
			script->MissionCount++;
		}
	}

	FootClass * object = _Create_Group(teamtype);
	object->Team->IsDeleteTypeWhenDone = false;
	if (!object) {
		return(false);
	}

	Cell origin = teamtype->Get_Origin();
	if (wp != -1) {
		origin = Scen->Get_Waypoint_Cell(wp);
	}

	FootClass * next = (FootClass *)(ObjectClass *)object->Next;
	object->Next = NULL;
	FootClass * placed = NULL;

	while (object != NULL) {
		bool isunit = false;
		if (object != NULL && object->Fetch_RTTI() == RTTI_UNIT) {
			isunit = true;
		}

		Coord coord = Map[origin].Center_Coord();
		if (!isunit) {
			coord.X += ((object->Ammo & 3) - 2) << 8; // RA2: the x offset is scaled by 256, the y offset by 64 (shl 8 / shl 6)
			coord.Y += ((object->Ammo & 0xC) - 4) << 6;
		}
		object->Ammo--;

		Coord dest = COORD_NONE;
		CellClass * cellptr = &Map[coord];
		int tries = 0;
		while (dest == COORD_NONE && tries < 255) {
			Cell nearcell = Map.Nearby_Location(cellptr->Center_Coord().As_Cell(), object->Techno_Type_Class()->Speed);
			cellptr = &Map[nearcell];
			if (isunit) {
				dest = cellptr->Center_Coord();
			} else {
				dest = cellptr->Closest_Free_Spot(cellptr->Center_Coord());
			}
			tries++;
		}

		coord.X = dest.X;
		coord.Y = dest.Y;
		ScenarioInit++;
		int ok = object->Unlimbo(coord, DIR_N);
		ScenarioInit--;
		if (ok) {
			IPiggybackPtr piggy(object->Locomotion);
			ILocomotionPtr teleport(CLSID_TeleportLocomotion);
			teleport->Link_To_Object(object);
			piggy = IPiggybackPtr(teleport);
			if (piggy != NULL) {
				piggy->Begin_Piggyback(object->Locomotion);
				object->Locomotion = teleport;
			}
			object->ChronoDestCoords = coord;
			object->Locomotion->Move_To(coord);
			object->WarpingOut = true;
			object->ChronoLockRemaining = Rule->ChronoReinfDelay;
			object->Next = placed;
			placed = object;
			object = next;
			if (object != NULL) {
				next = (FootClass *)(ObjectClass *)object->Next;
				object->Next = NULL;
			}
		} else {
			delete object;
			object = next;
			if (object != NULL) {
				next = (FootClass *)(ObjectClass *)object->Next;
				object->Next = NULL;
			}
		}
	}

	object = placed;
	placed = (FootClass *)(ObjectClass *)object->Next;
	object->Next = NULL;
	while (object != NULL) {
		object->Mark(MARK_UP);
		object->TeleportState = 3;
		object->Commence();
		object = placed;
		if (object != NULL) {
			placed = (FootClass *)(ObjectClass *)object->Next;
			object->Next = NULL;
		}
	}

	if (teamtype->House->Is_Ally(PlayerPtr)) {
		Record_Event_Cell(origin);
	}
	return(true);
}



/***********************************************************************************************
 * Create_Special_Reinforcement -- Ad hoc reinforcement handler.                               *
 *                                                                                             *
 *    Use this routine to bring on a reinforcement that hasn't been anticipated by the trigger *
 *    system. An example of this would be replacement harvesters or airfield ordered units.    *
 *    The appropriate transport is created (if necessary) and a mission is assigned such that  *
 *    the object will legally bring itself onto the playing field.                             *
 *                                                                                             *
 * INPUT:   house    -- The owner of this reinforcement.                                       *
 *                                                                                             *
 *          type     -- The object to bring on.                                                *
 *                                                                                             *
 *          another  -- This is reserved for the transport class in those cases where the      *
 *                      transport MUST be forced to a specific type.                           *
 *                                                                                             *
 *          mission  -- The mission to assign this reinforcement team.                         *
 *                                                                                             *
 *          argument -- Optional team mission argument (usually a waypoint).                   *
 *                                                                                             *
 * OUTPUT:  Was the special reinforcement created without error?                               *
 *                                                                                             *
 * WARNINGS:   This routine will fail if a team type cannot be created.                        *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/04/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
bool Create_Special_Reinforcement(HouseClass * house, TechnoTypeClass const * type, TechnoTypeClass const * another, TeamMissionType mission, int argument)
{
	return false; // RA2: no IniName, IsReinforcable, IsTransient and 10 more (TibSun's use does not compile against RA2's declaration); not in RA2's reinf
}


/***********************************************************************************************
 * _Need_To_Take -- Examines unit to determine if it should be confiscated.                    *
 *                                                                                             *
 *    The unit is examined and if the owning house needs to confiscate it, then this routine   *
 *    will return TRUE. In other cases, the unit should be left to its own devices.            *
 *                                                                                             *
 * INPUT:   unit  -- Pointer to the object to examine.                                         *
 *                                                                                             *
 * OUTPUT:  bool; Should the object be confiscated by the player so that it becomes one of     *
 *                his normal game objects?                                                     *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/26/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool _Need_To_Take(AircraftClass const * air)
{
	return(false);
}
