/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#define RA2_GEOMETRY_H // RA2: geometry.h follows the includes, as its constants take their initializers after the rectangle below (the thirteen initializers at 0x00593680-0x00593850)
#include "always.h" // RA2: as above

#include "missile.h" // RA2: as above

#include "_map.h" // RA2: as above
#include "cell.h" // RA2: as above
#include "coord.h" // RA2: as above
#include "face.h" // RA2: as above
#include "globals.h" // RA2: as above
#include "mouse.h" // RA2: as above
#include "velocity.h" // RA2: as above

const Rect MissileRect(0, 0, 0, 0); // RA2: 0x00A70BD8 (the first initializer, 0x00593680, clears it); a rectangle nothing in this module reads

#include "geometry.h" // RA2: this module holds its own copy of the geometry constants (initialized after MissileRect, from 0x005936A0)


/// <summary>
/// Handles one frame of guided missile flight.
/// This routine is used by the bullet logic to fly a homing projectile. It advances the
/// projectile along its current velocity, swings that velocity toward the target by no more
/// than the allowed turn rate, and flies the missile over the intervening terrain so that it
/// clears hills and bridges before it stoops onto its target.
/// </summary>
/// <param name="coord">The projectile position; advanced to its new position.</param>
/// <param name="velocity">The projectile velocity; steered toward the target.</param>
/// <param name="target_coord">The coordinate to home on, or COORD_NONE to merely level
/// out.</param>
/// <param name="rot">The greatest amount the projectile may turn during this frame.</param>
/// <param name="aircraft">Is the projectile chasing an aircraft?</param>
/// <param name="airburst">Should the projectile detonate in the air rather than stoop?</param>
/// <param name="veryhigh">Should the projectile cruise at extra altitude?</param>
/// <param name="level">Does the projectile fly level, never climbing or diving on its own?</param>
/// <returns>Returns with the distance still to be covered, with the vertical component
/// discounted. A projectile with nothing to home on returns its own speed.</returns>
int Projectile_Motion(Coord & coord, TVelocity3D<double> & velocity, Coord const & target_coord, DirType const & rot, bool aircraft, bool airburst, bool veryhigh, bool level) // RA2: the eighth argument is the projectile type's Level flag (BulletClass::AI pushes it from +0x26D)
{
	if (target_coord == COORD_NONE) { // RA2: as above
		/*
		 * Nothing to home on -- just level out.
		 */
		DirType pitch = velocity.Get_Pitch(); // RA2: as above
		pitch.Turn(DirType(DIR_NE), rot); // RA2: levels out towards 0x2000 (the executable stores 8192)
		velocity.Set_Pitch(pitch); // RA2: as above
		TVelocity3D<double> frame_velocity(velocity); // RA2: the executable still flies the frame before it returns
		coord = coord + Point3D(frame_velocity.X, frame_velocity.Y, frame_velocity.Z); // RA2: as above
		return(velocity.Speed()); // RA2: it returns the speed, not 1
	}

	TVelocity3D<double> frame_velocity(velocity); // RA2: as above
	coord = coord + Point3D(frame_velocity.X, frame_velocity.Y, frame_velocity.Z); // RA2: as above
	int target_distance = coord.Distance_To(target_coord); // RA2: as above
	int horizontal_distance = Point2D(coord).Distance_To(Point2D(target_coord)); // RA2: as above

	Coord delta = target_coord - coord; // RA2: as above

	/*
	 * Declared and then assigned, not copy-initialized: the temporary is built
	 * and block-copied into the local.
	 */
	TVelocity3D<double> target_direction; // RA2: as above
	target_direction = TVelocity3D<double>(delta.X, delta.Y, delta.Z); // RA2: as above

	DirType desired_yaw = velocity.Get_Yaw(); // RA2: as above
	desired_yaw.Turn(target_direction.Get_Yaw(), rot); // RA2: as above
	velocity.Set_Yaw(desired_yaw); // RA2: as above

	DirType desired_pitch = velocity.Get_Pitch(); // RA2: as above

	if (!aircraft && (airburst || horizontal_distance > ((veryhigh ? 6 : 3) * CELL_LEPTON)) && rot.As_Dir256() > 1) { // RA2: as above
		TVelocity3D<double> lookahead_velocity(velocity); // RA2: as above
		Coord lookahead_coord = coord + Point3D(lookahead_velocity.X, lookahead_velocity.Y, lookahead_velocity.Z) * 6; // RA2: as above

		int terrain_height = Map.Get_Height_GL(lookahead_coord); // RA2: as above
		if (Map[lookahead_coord].IsUnderBridge) { // RA2: as above
			terrain_height += BRIDGE_LEPTON_HEIGHT; // RA2: as above
		}

		int cell_distance = target_distance / CELL_LEPTON; // RA2: as above
		if (airburst || veryhigh) { // RA2: as above
			cell_distance = 10; // RA2: as above
		} else if (cell_distance >= 5) { // RA2: as above
			cell_distance = 5; // RA2: as above
		}

		if (!level) { // RA2: a level projectile skips the terrain following (the test at 0x00593EB2)
			int height_delta = coord.Z - LEVEL_LEPTON_H * cell_distance - terrain_height; // RA2: as above
			if (height_delta < -20) { // RA2: as above
				coord.Z += 18; // RA2: as above
			} else if (height_delta > 20) { // RA2: as above
				coord.Z -= 18; // RA2: as above
			}

			if (height_delta < -(LEVEL_LEPTON_H / 2)) { // RA2: as above
				desired_pitch.Turn(DirType(DIR_NE), rot / 2); // RA2: as above
			} else if (height_delta > LEVEL_LEPTON_H / 2) { // RA2: as above
				desired_pitch.Turn(DirType(Dir256(DIR_E + DIR_STEP_32)), rot / 2); // RA2: as above
			} else {
				DirType level_pitch(DIR_E); // RA2: as above
				DirType turn_rate = rot / 2; // RA2: as above
				desired_pitch.Turn(level_pitch, turn_rate); // RA2: as above
			}
		}
	} else if (!level) { // RA2: a level projectile does not pitch towards its target either (0x0059401C)
		desired_pitch.Turn(target_direction.Get_Pitch(), (rot + DirType(256)) / 2); // RA2: one expression; the executable adds a DirType of 256 to the turn rate and halves it, without a local
	}

	velocity.Set_Pitch(desired_pitch); // RA2: as above

	if (airburst) { // RA2: as above
		delta.Z = 0; // RA2: as above
	} else {
		delta.Z /= 4; // RA2: as above
	}

	return(delta.Length()); // RA2: as above
}
