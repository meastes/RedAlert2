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
**	RA2: the spawn manager of a carrier-like unit (aircraft carrier, Dreadnought, V3 and Dreadnought
**	missile launchers): it makes the spawned aircraft or missiles, launches them at the target, takes them
**	back and rebuilds the dead ones. TibSun has no such file; the code is the executable's from 0x006894A0.
*/

#define INCLUDE_COM
#define RA2_GEOMETRY_H // RA2: geometry.h follows the includes, as its constants take their initializers after those of the other headers
#include "always.h" // RA2: the includes this file needs
#include "spawnmgr.h" // RA2: the includes this file needs
#include "aircraft.h" // RA2: the includes this file needs
#include "airctype.h" // RA2: the includes this file needs
#include "cell.h" // RA2: the includes this file needs
#include "crc.h" // RA2: the includes this file needs
#include "foot.h" // RA2: the includes this file needs
#include "globals.h" // RA2: the includes this file needs
#include "house.h" // RA2: the includes this file needs
#include "kamikaze.h" // RA2: the includes this file needs
#include "mission.h" // RA2: the includes this file needs
#include "swizzle.h" // RA2: the includes this file needs
#include "_map.h" // RA2: the includes this file needs
#include "_rules.h" // RA2: the includes this file needs
#include "rules.h" // RA2: the includes this file needs
#include "techno.h" // RA2: the includes this file needs
#include "weapon.h" // RA2: the includes this file needs
#include <new.h> // RA2: the includes this file needs
#include "geometry.h" // RA2: this module holds its own copy of the geometry constants (initialized first, at 0x006894A0)

const Rect SpawnManagerRect(0, 0, 0, 0); // RA2: 0x00ABCFC0; a rectangle nothing in this module reads (its initializer is the one at 0x00689670, after those of the geometry constants)

DynamicVectorClass<SpawnManagerClass *> SpawnManagers; // RA2: 0x00ABD028; every spawn manager in the game


/// <summary>
/// Makes the spawn manager of the given owner: one aircraft of the spawn type per slot, kept limboed in the
/// owner until it is launched, and lists the manager.
/// </summary>
SpawnManagerClass::SpawnManagerClass(TechnoClass * owner, AircraftTypeClass * type, int number, int regen, int reload) // RA2: 0x00689710
{
	Owner = owner; // RA2: the constructor arguments are stored first
	SpawnType = type; // RA2: as above
	SpawnCount = number; // RA2: as above
	RegenRate = regen; // RA2: as above
	ReloadRate = reload; // RA2: as above
	for (int index = 0; index < SpawnCount; index++) { // RA2: as above
		SpawnControl * node = new SpawnControl; // RA2: one node per slot
		if (node == NULL) { // RA2: the allocation is tested
			break; // RA2: as above
		}
		node->Unit = (AircraftClass *)SpawnType->Create_One_Of(Owner->Owner_HouseClass()); // RA2: the aircraft is made for the owner's house (virtual slot 35 of the type)
		if (node->Unit != NULL) { // RA2: as above
			if (SpawnType == Rule->V3Rocket.Type || SpawnType == Rule->DMisl.Type) { // RA2: the two missile launchers' missiles are marked
				node->IsSpawnMissile = TRUE; // RA2: as above
			} else { // RA2: as above
				node->IsSpawnMissile = FALSE; // RA2: as above
			}
			node->Unit->Limbo(); // RA2: as above
			node->Unit->SpawnOwner = Owner; // RA2: as above
			node->Status = SPAWN_IDLE; // RA2: as above
			node->SpawnTimer = 0; // RA2: as above
			SpawnedNodes.Add(node); // RA2: as above
		}
	}
	UpdateTimer = 20; // RA2: the first update comes after 20 frames
	Target = NULL; // RA2: no target yet
	NewTarget = NULL; // RA2: as above
	Status = 0; // RA2: idle
	SpawnTimer = 0; // RA2: as above
	SpawnManagers.Add(this); // RA2: the manager joins the list at 0x00ABD028
}


/// <summary>
/// Makes an empty spawn manager, the one the class factory hands out, and lists it.
/// </summary>
SpawnManagerClass::SpawnManagerClass(void) // RA2: 0x00689910
{
	Owner = NULL; // RA2: as above
	SpawnType = NULL; // RA2: as above
	SpawnCount = 0; // RA2: as above
	UpdateTimer = 0; // RA2: as above
	Target = NULL; // RA2: as above
	NewTarget = NULL; // RA2: as above
	Status = 0; // RA2: as above
	SpawnTimer = 0; // RA2: as above
	SpawnManagers.Add(this); // RA2: as above
}


/// <summary>
/// The constructor used when the manager is read from a save game; its fields come from the stream.
/// </summary>
SpawnManagerClass::SpawnManagerClass(NoInitClass const & x) : // RA2: 0x00689A10
	AbstractClass(x), // RA2: as above
	UpdateTimer(0), // RA2: as above
	SpawnTimer(0) // RA2: as above
{
}


/// <summary>
/// Kills the spawned aircraft unless the game is ending, frees the nodes and takes the manager out of the list.
/// </summary>
SpawnManagerClass::~SpawnManagerClass(void) // RA2: 0x00689A80
{
	if (GameActive) { // RA2: as above
		KillNodes(); // RA2: as above
	}
	for (int index = SpawnedNodes.Count() - 1; index >= 0; index--) { // RA2: as above
		delete SpawnedNodes[index]; // RA2: as above
	}
	DynamicVectorClass<SpawnControl *> & nodes = SpawnedNodes; // RA2: the executable works on the node list through a reference
	int length = nodes.Length(); // RA2: it clears the node list and gives its space back at once, as the container's own destructor would
	nodes.Clear(); // RA2: as above
	nodes.Resize(length); // RA2: as above
	SpawnManagers.Delete(this); // RA2: as above
}


/// <summary>
/// Every live node loses its aircraft: a docked one is deleted, a launching missile leaves the kamikaze
/// tracker and is deleted, and a flying one is handed to the tracker to finish at the target. Each node then
/// waits for the regeneration time.
/// </summary>
void SpawnManagerClass::KillNodes(void) // RA2: 0x00689B70
{
	for (int index = SpawnedNodes.Count() - 1; index >= 0; index--) { // RA2: as above
		int status = SpawnedNodes[index]->Status; // RA2: as above
		if (status == SPAWN_DEAD) { // RA2: as above
			continue; // RA2: as above
		}
		if (status == SPAWN_IDLE || status == SPAWN_RELOADING) { // RA2: as above
			SpawnedNodes[index]->Status = SPAWN_DEAD; // RA2: as above
			SpawnedNodes[index]->Unit->Delete_Me(); // RA2: as above
			SpawnedNodes[index]->Unit = NULL; // RA2: as above
			SpawnedNodes[index]->SpawnTimer = RegenRate; // RA2: as above
		} else if (status == SPAWN_TAKEOFF) { // RA2: as above
			Kamikazes.Remove(SpawnedNodes[index]->Unit); // RA2: as above
			SpawnedNodes[index]->Status = SPAWN_DEAD; // RA2: as above
			SpawnedNodes[index]->Unit->Delete_Me(); // RA2: as above
			SpawnedNodes[index]->Unit = NULL; // RA2: as above
			SpawnedNodes[index]->SpawnTimer = RegenRate; // RA2: as above
		} else { // RA2: as above
			SpawnedNodes[index]->Status = SPAWN_DEAD; // RA2: as above
			Kamikazes.Add(SpawnedNodes[index]->Unit, Target); // RA2: as above
			SpawnedNodes[index]->Unit = NULL; // RA2: as above
			SpawnedNodes[index]->SpawnTimer = RegenRate; // RA2: as above
		}
	}
}


/// <summary>
/// The spawner's update, every ten frames: it walks the nodes (launching docked aircraft at the target,
/// bringing them home, reloading and rebuilding them) and then moves its own state along.
/// </summary>
void SpawnManagerClass::AI(void) // RA2: 0x00689C90
{
	if (UpdateTimer.Value()) { // RA2: the nodes are only looked at every ten frames
		return; // RA2: as above
	}
	UpdateTimer = 10; // RA2: as above

	for (int index = 0; index < SpawnedNodes.Count(); index++) { // RA2: as above
		bool burst = false; // RA2: as above
		AircraftClass * unit = SpawnedNodes[index]->Unit; // RA2: as above
		switch (SpawnedNodes[index]->Status) { // RA2: as above
			case SPAWN_IDLE: // RA2: as above
				if (Target == NULL || SpawnTimer.Value() || Status == 2 || Owner->House->Ion_Sensitives_Should_Be_Offline()) { // RA2: a docked aircraft waits for a target, the launch delay, the end of the attack and (never, in 1.006) an ion storm over the house
					break; // RA2: as above
				}
				if (SpawnedNodes[index]->IsSpawnMissile == TRUE) { // RA2: as above
					if (((FootClass *)Owner)->Locomotion->Is_Moving()) { // RA2: as above
						break; // RA2: as above
					}
					if (((FootClass *)Owner)->Locomotion->Is_Moving_Now()) { // RA2: as above
						break; // RA2: as above
					}
				}
				if (Owner->Techno_Type_Class()->MissileSpawn) { // RA2: the owner's type launches missiles: 9 frames between launches
					SpawnTimer = 9; // RA2: as above
				} else { // RA2: as above
					SpawnTimer = 20; // RA2: aircraft leave 20 frames apart
				}
				if (SpawnedNodes[index]->IsSpawnMissile == TRUE) { // RA2: a burst weapon alternates its burst index with the node's parity
					WeaponTypeClass * weapon = Owner->Get_Class_Weapon_Data(0)->Weapon; // RA2: as above
					if (weapon != NULL && weapon->Burst > 1) { // RA2: as above
						burst = true; // RA2: as above
						Owner->BurstIndex = index & 1; // RA2: as above
					}
				}
				SpawnedNodes[index]->Status = SPAWN_PREPARING; // RA2: as above
				{
					Coord coord = Owner->Fire_Coord(0); // RA2: the aircraft appear at the owner's fire coordinate, 10 leptons higher
					coord.Z += 10; // RA2: as above
					unit->Unlimbo(coord, Owner->PrimaryFacing.Current().As_Dir256()); // RA2: as above
				}
				if (burst) { // RA2: as above
					Owner->BurstIndex = 0; // RA2: as above
				}
				if (SpawnedNodes[index]->IsSpawnMissile == TRUE) { // RA2: as above
					if (NewTarget != NULL) { // RA2: as above
						Target = NewTarget; // RA2: as above
						NewTarget = NULL; // RA2: as above
					}
					unit->Assign_Destination(Target, true); // RA2: as above
				} else { // RA2: as above
					CellClass & cell = Map[Owner->Get_Cell()]; // RA2: other aircraft first fly to the cell north of the owner
					unit->Assign_Destination(&cell.Adjacent_Cell(FACING_N), true); // RA2: as above
				}
				unit->QueueMission(MISSION_MOVE, false); // RA2: as above
				break; // RA2: as above

			case SPAWN_TAKEOFF: // RA2: as above
				if (!SpawnedNodes[index]->SpawnTimer.Value()) { // RA2: as above
					UnlinkPointer(unit); // RA2: as above
				}
				break; // RA2: as above

			case SPAWN_PREPARING: // RA2: as above
				if (SpawnedNodes[index]->IsSpawnMissile == TRUE) { // RA2: as above
					break; // RA2: as above
				}
				if (NewTarget != NULL) { // RA2: as above
					Target = NewTarget; // RA2: as above
					NewTarget = NULL; // RA2: as above
				}
				if (Target == NULL) { // RA2: as above
					unit->Assign_Destination(Owner, true); // RA2: as above
					unit->Assign_Target(NULL); // RA2: as above
					unit->QueueMission(MISSION_MOVE, false); // RA2: as above
					SpawnedNodes[index]->Status = SPAWN_RETURNING; // RA2: as above
				} else { // RA2: as above
					CellClass & cell = Map[Owner->Get_Cell()]; // RA2: while the others gather they wait in the cell south of the owner
					unit->Assign_Destination(&cell.Adjacent_Cell(FACING_S), true); // RA2: as above
					unit->QueueMission(MISSION_MOVE, false); // RA2: as above
				}
				break; // RA2: as above

			case SPAWN_ATTACKING: // RA2: as above
				if (NewTarget != NULL) { // RA2: as above
					Target = NewTarget; // RA2: as above
					NewTarget = NULL; // RA2: as above
				}
				if (unit->Ammo == 0 || Target == NULL) { // RA2: as above
					unit->Assign_Destination(Owner, true); // RA2: as above
					unit->Assign_Target(NULL); // RA2: as above
					unit->QueueMission(MISSION_MOVE, false); // RA2: as above
					SpawnedNodes[index]->Status = SPAWN_RETURNING; // RA2: as above
				} else { // RA2: as above
					SpawnedNodes[index]->Unit->Assign_Target(Target); // RA2: as above
					SpawnedNodes[index]->Unit->QueueMission(MISSION_ATTACK, false); // RA2: as above
				}
				break; // RA2: as above

			case SPAWN_RETURNING: // RA2: as above
				if (NewTarget != NULL) { // RA2: as above
					Target = NewTarget; // RA2: as above
					NewTarget = NULL; // RA2: as above
				}
				if (unit->Ammo > 0 && Target != NULL) { // RA2: as above
					SpawnedNodes[index]->Status = SPAWN_ATTACKING; // RA2: as above
					SpawnedNodes[index]->Unit->Assign_Target(Target); // RA2: as above
					SpawnedNodes[index]->Unit->QueueMission(MISSION_ATTACK, false); // RA2: as above
				} else if (unit->Get_Cell() == Owner->Get_Cell() && Coord(unit->Position).Z - Coord(Owner->Position).Z < 20) { // RA2: back over the owner's cell and less than 20 leptons above it
					unit->Limbo(); // RA2: as above
					SpawnedNodes[index]->Status = SPAWN_RELOADING; // RA2: as above
					SpawnedNodes[index]->SpawnTimer = ReloadRate; // RA2: as above
				} else { // RA2: as above
					unit->Assign_Destination(Owner, true); // RA2: as above
					unit->Assign_Target(NULL); // RA2: as above
					unit->QueueMission(MISSION_MOVE, false); // RA2: as above
				}
				break; // RA2: as above

			case SPAWN_RELOADING: // RA2: as above
				if (!SpawnedNodes[index]->SpawnTimer.Value()) { // RA2: as above
					SpawnedNodes[index]->Status = SPAWN_IDLE; // RA2: as above
					unit->Ammo = unit->Class->MaxAmmo; // RA2: as above
					unit->EstimatedHealth = unit->Strength = unit->Class->MaxStrength; // RA2: as above
				}
				break; // RA2: as above

			case SPAWN_DEAD: // RA2: as above
				if (!SpawnedNodes[index]->SpawnTimer.Value()) { // RA2: as above
					SpawnedNodes[index]->Unit = (AircraftClass *)SpawnType->Create_One_Of(Owner->Owner_HouseClass()); // RA2: as above
					if (SpawnType == Rule->V3Rocket.Type || SpawnType == Rule->DMisl.Type) { // RA2: as above
						SpawnedNodes[index]->IsSpawnMissile = TRUE; // RA2: as above
					} else { // RA2: as above
						SpawnedNodes[index]->IsSpawnMissile = FALSE; // RA2: as above
					}
					SpawnedNodes[index]->Unit->Limbo(); // RA2: as above
					SpawnedNodes[index]->Unit->SpawnOwner = Owner; // RA2: as above
					SpawnedNodes[index]->Status = SPAWN_IDLE; // RA2: as above
				}
				break; // RA2: as above
		}
	}

	bool flag = false; // RA2: set when a missile was handed to the kamikaze tracker
	switch (Status) { // RA2: the manager's own state: 0 idle, 1 launching, 2 attacking
		case 0: // RA2: as above
			if (NewTarget != NULL) { // RA2: as above
				Target = NewTarget; // RA2: as above
				NewTarget = NULL; // RA2: as above
			}
			if (Target != NULL) { // RA2: as above
				if (!Owner->IsCloseEnoughToAttack(Target)) { // RA2: as above
					ResetTarget(); // RA2: as above
					return; // RA2: as above
				}
				Status = 1; // RA2: as above
			}
			break; // RA2: as above

		case 1:
		{
			if (Target == NULL) { // RA2: as above
				ResetTarget(); // RA2: as above
				return; // RA2: as above
			}
			int index; // RA2: as above
			for (index = 0; index < SpawnedNodes.Count(); index++) { // RA2: as above
				int status = SpawnedNodes[index]->Status; // RA2: as above
				if (status != SPAWN_PREPARING && status != SPAWN_DEAD) { // RA2: as above
					return; // RA2: as above
				}
			}
			for (index = 0; index < SpawnedNodes.Count(); index++) { // RA2: as above
				if (SpawnedNodes[index]->Status == SPAWN_PREPARING) { // RA2: as above
					if (SpawnedNodes[index]->Unit->Class->MissileSpawn) { // RA2: as above
						flag = true; // RA2: as above
						Kamikazes.Add(SpawnedNodes[index]->Unit, Target); // RA2: as above
						Kamikazes.Timer = 2; // RA2: as above
						if (SpawnedNodes[index]->IsSpawnMissile == TRUE) { // RA2: as above
							SpawnedNodes[index]->Status = SPAWN_TAKEOFF; // RA2: as above
							if (SpawnType == Rule->V3Rocket.Type) { // RA2: as above
								SpawnedNodes[index]->SpawnTimer = Rule->V3Rocket.PauseFrames + Rule->V3Rocket.TiltFrames; // RA2: as above
							} else { // RA2: as above
								SpawnedNodes[index]->SpawnTimer = Rule->DMisl.PauseFrames + Rule->DMisl.TiltFrames; // RA2: as above
							}
						} else { // RA2: as above
							UnlinkPointer(SpawnedNodes[index]->Unit); // RA2: as above
						}
					} else { // RA2: as above
						SpawnedNodes[index]->Status = SPAWN_ATTACKING; // RA2: as above
						SpawnedNodes[index]->Unit->Assign_Target(Target); // RA2: as above
						SpawnedNodes[index]->Unit->QueueMission(MISSION_ATTACK, false); // RA2: as above
					}
				}
			}
			if (flag) { // RA2: as above
				ResetTarget(); // RA2: as above
			}
			Status = 2; // RA2: as above
			break; // RA2: as above
		}

		case 2:
		{
			flag = true; // RA2: as above
			for (int index = 0; index < SpawnedNodes.Count(); index++) { // RA2: as above
				int status = SpawnedNodes[index]->Status; // RA2: as above
				if (status == SPAWN_ATTACKING || status == SPAWN_RETURNING) { // RA2: as above
					flag = false; // RA2: as above
				}
			}
			if (flag) { // RA2: as above
				Status = 0; // RA2: as above
			}
			break; // RA2: as above
		}
	}
}


/// <summary>
/// Queues a new target; the target already being attacked is not queued again.
/// </summary>
void SpawnManagerClass::SetTarget(AbstractClass * target) // RA2: 0x0068A490
{
	if (target != Target) { // RA2: the target being attacked is not queued again
		NewTarget = target; // RA2: as above
	}
}


/// <summary>
/// Flying missiles go to the kamikaze tracker to finish at the target and leave their nodes; then the manager
/// forgets both of its targets.
/// </summary>
void SpawnManagerClass::ResetTarget(void) // RA2: 0x0068A4B0
{
	for (int index = 0; index < SpawnedNodes.Count(); index++) { // RA2: as above
		if (SpawnedNodes[index]->Status == SPAWN_PREPARING && SpawnedNodes[index]->Unit->Class->MissileSpawn) { // RA2: as above
			Kamikazes.Add(SpawnedNodes[index]->Unit, Target); // RA2: as above
			Kamikazes.Timer = 2; // RA2: as above
			UnlinkPointer(SpawnedNodes[index]->Unit); // RA2: as above
		}
	}
	Status = 0; // RA2: as above
	NewTarget = NULL; // RA2: as above
	Target = NULL; // RA2: as above
}


/// <summary>
/// Makes the queued target the target now.
/// </summary>
/// <returns>Returns true when there was a queued target.</returns>
bool SpawnManagerClass::UpdateTarget(void) // RA2: 0x0068A540
{
	if (NewTarget != NULL) { // RA2: as above
		Target = NewTarget; // RA2: as above
		NewTarget = NULL; // RA2: as above
		return(true); // RA2: as above
	}
	return(false); // RA2: as above
}


/// <summary>
/// Forgets an object that is going away: the target (the queued target takes over, or the manager resets),
/// the queued target, one of the spawned aircraft (its node then waits for the regeneration time, unless it is
/// only being let go while healthy) or the owner (every spawned aircraft dies).
/// </summary>
void SpawnManagerClass::UnlinkPointer(AbstractClass * ptr) // RA2: 0x0068A560
{
	if (ptr == Target) { // RA2: as above
		Target = NULL; // RA2: as above
		if (NewTarget == NULL) { // RA2: as above
			ResetTarget(); // RA2: as above
		}
		return; // RA2: as above
	}
	if (ptr == NewTarget) { // RA2: as above
		NewTarget = NULL; // RA2: as above
		return; // RA2: as above
	}
	for (int index = SpawnedNodes.Count() - 1; index >= 0; index--) { // RA2: as above
		if (ptr == SpawnedNodes[index]->Unit) { // RA2: as above
			AircraftClass * unit = SpawnedNodes[index]->Unit; // RA2: as above
			if (unit->Strength > 0 && !unit->IsKamikaze && SpawnedNodes[index]->IsSpawnMissile != TRUE) { // RA2: as above
				return; // RA2: as above
			}
			SpawnedNodes[index]->Unit = NULL; // RA2: as above
			SpawnedNodes[index]->Status = SPAWN_DEAD; // RA2: as above
			SpawnedNodes[index]->SpawnTimer = RegenRate; // RA2: as above
			return; // RA2: as above
		}
	}
	if (ptr == Owner) { // RA2: as above
		KillNodes(); // RA2: as above
		ResetTarget(); // RA2: as above
	}
}


/// <summary>
/// Counts the nodes that have not died.
/// </summary>
int SpawnManagerClass::CountAliveSpawns(void) // RA2: 0x0068A630
{
	int count = 0; // RA2: as above
	for (int index = 0; index < SpawnedNodes.Count(); index++) { // RA2: as above
		if (SpawnedNodes[index]->Status != SPAWN_DEAD) { // RA2: as above
			count++; // RA2: as above
		}
	}
	return(count); // RA2: as above
}


/// <summary>
/// Counts the nodes whose aircraft sit docked (idle or reloading).
/// </summary>
int SpawnManagerClass::CountDockedSpawns(void) // RA2: 0x0068A650
{
	int count = 0; // RA2: as above
	for (int index = 0; index < SpawnedNodes.Count(); index++) { // RA2: as above
		int status = SpawnedNodes[index]->Status; // RA2: as above
		if (status == SPAWN_RELOADING || status == SPAWN_IDLE) { // RA2: as above
			count++; // RA2: as above
		}
	}
	return(count); // RA2: as above
}


/// <summary>
/// Counts the nodes whose missile is in its launch.
/// </summary>
int SpawnManagerClass::CountLaunchingSpawns(void) // RA2: 0x0068A680
{
	int count = 0; // RA2: as above
	for (int index = 0; index < SpawnedNodes.Count(); index++) { // RA2: as above
		if (SpawnedNodes[index]->Status == SPAWN_TAKEOFF) { // RA2: as above
			count++; // RA2: as above
		}
	}
	return(count); // RA2: as above
}


/// <summary>
/// Adds what makes the manager differ between machines to the checksum of the game state.
/// </summary>
void SpawnManagerClass::Compute_CRC(CRCEngine & crc) const // RA2: 0x0068A6A0
{
	BASECLASS::Compute_CRC(crc); // RA2: as above
	crc(Status); // RA2: state, the ids of the queued target and the target, both timers, the node and spawn counts, then the ids of the spawn type and owner that exist
	if (NewTarget != NULL) { // RA2: as above
		crc(NewTarget->Fetch_ID()); // RA2: as above
	}
	if (Target != NULL) { // RA2: as above
		crc(Target->Fetch_ID()); // RA2: as above
	}
	crc(SpawnTimer.Value()); // RA2: as above
	crc(UpdateTimer.Value()); // RA2: as above
	crc(SpawnedNodes.Count()); // RA2: as above
	crc(SpawnCount); // RA2: as above
	if (SpawnType != NULL) { // RA2: as above
		crc(SpawnType->Fetch_ID()); // RA2: as above
	}
	if (Owner != NULL) { // RA2: as above
		crc(Owner->Fetch_ID()); // RA2: as above
	}
}


/// <summary>
/// Fetches the persistent class identifier for the spawn manager.
/// </summary>
HRESULT STDMETHODCALLTYPE SpawnManagerClass::GetClassID(CLSID * retval) // RA2: 0x0068A790
{
	if (retval == NULL) return(E_POINTER); // RA2: as above
	*retval = CLSID_SpawnManagerClass; // RA2: as above
	return(S_OK); // RA2: as above
}


/// <summary>
/// Reads the manager back in from the save game stream: the node count, then each node whole. The pointers
/// are remapped by the swizzler afterwards.
/// </summary>
HRESULT STDMETHODCALLTYPE SpawnManagerClass::Load(IStream * stream) // RA2: 0x0068A7D0
{
	HRESULT result = BASECLASS::Load(stream); // RA2: as above
	if (SUCCEEDED(result)) { // RA2: as above
		new (this) SpawnManagerClass(NoInitClass()); // RA2: as above
		int count; // RA2: as above
		result = stream->Read(&count, sizeof(count), NULL); // RA2: as above
		if (SUCCEEDED(result)) { // RA2: as above
			new (&SpawnedNodes) DynamicVectorClass<SpawnControl *>; // RA2: as above
			for (int index = 0; index < count; index++) { // RA2: as above
				SpawnControl * node = new SpawnControl; // RA2: as above
				result = stream->Read(node, sizeof(SpawnControl), NULL); // RA2: as above
				if (FAILED(result)) { // RA2: as above
					return(result); // RA2: as above
				}
				Swizzle_Pointer(&node->Unit); // RA2: as above
				SpawnedNodes.Add(node); // RA2: as above
			}
			Swizzle_Pointer(&Owner); // RA2: as above
			Swizzle_Pointer(&SpawnType); // RA2: as above
			Swizzle_Pointer(&Target); // RA2: as above
			Swizzle_Pointer(&NewTarget); // RA2: as above
		}
	}
	return(result); // RA2: as above
}


/// <summary>
/// Writes the manager to the save game stream: the base object, the node count, then each node whole.
/// </summary>
HRESULT STDMETHODCALLTYPE SpawnManagerClass::Save(IStream * stream, BOOL cleardirty) // RA2: 0x0068A970
{
	HRESULT result = BASECLASS::Save(stream, cleardirty); // RA2: as above
	if (FAILED(result)) { // RA2: as above
		return(result); // RA2: as above
	}
	int count = SpawnedNodes.Count(); // RA2: as above
	result = stream->Write(&count, sizeof(count), NULL); // RA2: as above
	if (FAILED(result)) { // RA2: as above
		return(result); // RA2: as above
	}
	for (int index = 0; index < count; index++) { // RA2: as above
		result = stream->Write(SpawnedNodes[index], sizeof(SpawnControl), NULL); // RA2: as above
		if (FAILED(result)) { // RA2: as above
			return(result); // RA2: as above
		}
	}
	return(result); // RA2: as above
}


/// <summary>
/// The size of the object.
/// </summary>
int SpawnManagerClass::Fetch_Object_Size(void) const // RA2: 0x0068A9E0
{
	return(sizeof(*this)); // RA2: 0x74 bytes
}


/// <summary>
/// The kind of object: a spawn manager.
/// </summary>
RTTIType SpawnManagerClass::Fetch_RTTI(void) const // RA2: 0x0068A9F0
{
	return(RTTI_SPAWNMANAGER); // RA2: kind 0x41
}
