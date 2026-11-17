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
**	RA2: the radiation beam (the rad cannon and the chrono beams): a line of short segments drawn between a
**	source and a target for a number of frames, a pulse that runs along the line (type 1) or a swing that sweeps
**	across it (type 2). TibSun has no such file; the code is the executable's from 0x006351B0.
*/

#include "geometry.h" // RA2: the geometry constants come before the rectangle of rect.h (the executable initializes them first)
#include "always.h" // RA2: the includes this file needs
#include <math.h> // RA2: as above
#include "radbeam.h" // RA2: as above
#include "_rect.h" // RA2: as above
#include "_surface.h" // RA2: as above
#include "_tactica.h" // RA2: as above
#include "ccrand.h" // RA2: as above
#include "dsurface.h" // RA2: as above
#include "session.h" // RA2: as above
#include "tactical.h" // RA2: as above
#include "xsurface.h" // RA2: as above

namespace FPSCounter { extern unsigned CurrentFrameRate; } // RA2: 0x00A6EB1C, declared as RA2's code uses it

const double DEGREES_TO_RADIANS = M_PI / 180.0; // RA2: 0x007EE398; read from memory and loaded before the multiplication, so it is an object and not a literal
const double CIRCLE_DEGREES = 360.0; // RA2: a constant of this module (0x007A8898), the shortest span of a pulse
const double HALF_CIRCLE_DEGREES = 180.0; // RA2: a constant of this module (0x007A88A0), the span of a swing


DynamicVectorClass<RadBeam *> RadBeam::Array; // RA2: 0x00AB64E0; the live beams (its initializer is the one at 0x006353A0)

/// <summary>
/// Makes a beam, lists it and gives it the step length of its kind: 10 for the rad beam (type 1), 20 otherwise.
/// </summary>
RadBeam * __fastcall RadBeam::Allocate(int type) // RA2: 0x00635440
{
	RadBeam * beam = new RadBeam(type); // RA2: as above
	Array.Add(beam); // RA2: as above
	beam->Set_Step_Length(type == 1 ? 10 : 20); // RA2: as above
	return(beam); // RA2: as above
}

/// <summary>
/// Draws every listed beam once and deletes the ones that are done. The step length of the pulse beams follows the
/// frame rate, and when it falls below the desired rate part of the swing beams (type 2) stop drawing.
/// </summary>
void __fastcall RadBeam::Update_All(void) // RA2: 0x006354E0
{
	int total = Array.Count(); // RA2: as above
	int count = 0; // RA2: as above
	int active = 0; // RA2: as above
	for (int index = total - 1; index >= 0; index--) { // RA2: as above
		RadBeam * beam = Array[index]; // RA2: as above
		if (beam->Type == 2) { // RA2: as above
			count++; // RA2: as above
			if (beam->IsActive) { // RA2: as above
				active++; // RA2: as above
			} // RA2: as above
		} // RA2: as above
	} // RA2: as above
	double desired = Session.DesiredFrameRate; // RA2: as above
	double rate = FPSCounter::CurrentFrameRate / desired; // RA2: as above
	rate = rate * rate * rate; // RA2: as above
	double excess = (double)active / count - rate; // RA2: as above
	double cull = 0.0; // RA2: as above
	if (excess > 0.0) { // RA2: as above
		cull = count * excess; // RA2: as above
	} // RA2: as above
	cull /= active; // RA2: as above
	double stretch = 1.0 / rate; // RA2: as above

	for (index = total - 1; index >= 0; index--) { // RA2: as above
		RadBeam * beam = Array[index]; // RA2: as above
		bool done; // RA2: as above
		if (beam->Type == 1) { // RA2: as above
			int length = (int)(stretch * 10.0); // RA2: as above
			if (length < 10) { // RA2: as above
				length = 10; // RA2: as above
			} else if (length > 100) { // RA2: as above
				length = 100; // RA2: as above
			} // RA2: as above
			beam->Set_Step_Length(length); // RA2: as above
			done = beam->Draw_Pulse(); // RA2: as above
		} else if (beam->Type == 2) { // RA2: as above
			if (beam->IsActive) { // RA2: as above
				if (NonCriticalRandomNumber(0, 99) < (int)(cull * 100.0)) { // RA2: as above
					beam->Deactivate(); // RA2: as above
				} // RA2: as above
			} // RA2: as above
			done = beam->Draw_Swing(); // RA2: as above
		} else { // RA2: as above
			continue; // RA2: as above
		} // RA2: as above
		if (done) { // RA2: as above
			Array.Delete(beam); // RA2: as above
			delete beam; // RA2: as above
		} // RA2: as above
	} // RA2: as above
}

/// <summary>
/// Makes an active beam of the given type that has not been drawn yet.
/// </summary>
RadBeam::RadBeam(int type) : // RA2: 0x006356B0
	Prepared(false), // RA2: as above
	IsActive(true), // RA2: as above
	Depth(0), // RA2: as above
	Type(type), // RA2: as above
	StepLength(0), // RA2: as above
	Color(0, 0, 0), // RA2: as above
	Source(0, 0, 0), // RA2: as above
	Target(0, 0, 0), // RA2: as above
	Period(0), // RA2: as above
	Amplitude(0), // RA2: as above
	Phase(0), // RA2: as above
	StartDelay(0), // RA2: as above
	Time(0) // RA2: as above
{
}

/// <summary>
/// Sets the color the beam starts each pass with.
/// </summary>
void RadBeam::Set_Color(RGBClass const & color) // RA2: 0x00635710
{
	Color = color; // RA2: as above
}

/// <summary>
/// Sets the screen height difference of the ends, which shifts the depth of the drawn segments.
/// </summary>
void RadBeam::Set_Depth(int depth) // RA2: 0x00635730
{
	Depth = depth; // RA2: as above
}

/// <summary>
/// Sets the point the beam starts at.
/// </summary>
void RadBeam::Set_Source(Coord const & coord) // RA2: 0x00635740
{
	Source = coord; // RA2: as above
}

/// <summary>
/// Sets the point the beam ends at.
/// </summary>
void RadBeam::Set_Target(Coord const & coord) // RA2: 0x00635760
{
	Target = coord; // RA2: as above
}

/// <summary>
/// Sets the number of frames the beam lasts.
/// </summary>
void RadBeam::Set_Period(int period) // RA2: 0x00635780
{
	Period = period; // RA2: as above
}

/// <summary>
/// Sets how far the wave of the beam rises above the line.
/// </summary>
void RadBeam::Set_Amplitude(double amplitude) // RA2: 0x00635790
{
	Amplitude = amplitude; // RA2: as above
}

/// <summary>
/// Sets the phase of the wave.
/// </summary>
void RadBeam::Set_Phase(double phase) // RA2: 0x006357B0
{
	Phase = phase; // RA2: as above
}

/// <summary>
/// Sets the start delay, a factor of the swing beam's per frame offset.
/// </summary>
void RadBeam::Set_Start_Delay(int delay) // RA2: 0x006357D0
{
	StartDelay = delay; // RA2: as above
}

/// <summary>
/// Stops the beam drawing; it is deleted when its period is over.
/// </summary>
void RadBeam::Deactivate(void) // RA2: 0x006357E0
{
	IsActive = false; // RA2: as above
}

/// <summary>
/// Returns whether the beam still draws.
/// </summary>
bool RadBeam::Is_Active(void) const // RA2: 0x006357F0
{
	return(IsActive); // RA2: as above
}

/// <summary>
/// Sets the length of one segment of the beam.
/// </summary>
void RadBeam::Set_Step_Length(int length) // RA2: 0x00635800
{
	StepLength = length; // RA2: as above
}

/// <summary>
/// Draws one segment on the logical surface, clipped to the tactical view; the depth at each end follows the
/// beam's depth and the height of that end.
/// </summary>
void RadBeam::Draw_Line(Coord const & start, Coord const & end, short color) // RA2: 0x00635810
{
	Point2D first_pixel; // RA2: as above
	Point2D second_pixel; // RA2: as above
	TacticalMap->Coord_To_Pixel(start, first_pixel); // RA2: as above
	TacticalMap->Coord_To_Pixel(end, second_pixel); // RA2: as above
	Rect clip = TacticalRect; // RA2: as above
	if (Clip_Line_To_Rect(first_pixel, second_pixel, clip)) { // RA2: as above
		int first_z = Depth - Tactical::Z_Lepton_To_Pixel(start.Z) - 2; // RA2: as above
		int second_z = 0 - Tactical::Z_Lepton_To_Pixel(end.Z) - 2; // RA2: as above
		LogicalSurface->Draw_Depth_Shaded_Line(TacticalRect, first_pixel, second_pixel, color, first_z, second_z); // RA2: as above
	} // RA2: as above
}

/// <summary>
/// Draws one frame of a type 1 beam (the rad beam): on the first frame the line is measured, and while active every
/// step is drawn as a segment in its color. Returns true once the period is over. The code of Compute_Delta and of
/// Start_Pass is written out here, those two have no caller.
/// </summary>
bool RadBeam::Draw_Pulse(void) // RA2: 0x006358D0
{
	if (!Prepared) { // RA2: as above
		Prepared = true; // RA2: as above
		Delta = Target - Source; // RA2: the code of Compute_Delta is written out here
		Length = fastmath::sqrt(Delta.X * Delta.X + Delta.Y * Delta.Y + Delta.Z * Delta.Z); // RA2: as above
	} // RA2: as above
	if (IsActive) { // RA2: as above
		Steps = (int)(Length / StepLength); // RA2: the code of Start_Pass is written out here (the executable has no call to it)
		int x = Delta.X; // RA2: as above
		int y = Delta.Y; // RA2: as above
		int z = Delta.Z; // RA2: as above
		if (Steps != 0) { // RA2: as above
			x /= Steps; // RA2: as above
			y /= Steps; // RA2: as above
			z /= Steps; // RA2: as above
		} // RA2: as above
		StepDelta = Coord(x, y, z); // RA2: as above
		Current = Source; // RA2: as above
		double phase = Time; // RA2: as above
		phase /= Period; // RA2: as above
		Wave = phase * Amplitude; // RA2: as above
		CurrentColor = Color; // RA2: as above
		for (StepIndex = 0; StepIndex * StepLength < Length; StepIndex++) { // RA2: as above
			Compute_Pulse_Step(); // RA2: as above
			unsigned short green = CurrentColor.Get_Green(); // RA2: a 16 bit local for the green gun; the executable's loads and shifts only match with it
			Draw_Line(Current, Next, (short)DSurface::Build_Hicolor_Pixel(CurrentColor.Get_Red(), green, CurrentColor.Get_Blue())); // RA2: as above
		} // RA2: as above
	} // RA2: as above
	Time++; // RA2: as above
	return(Time >= Period); // RA2: as above
}

/// <summary>
/// Measures the line from the source to the target.
/// </summary>
void RadBeam::Compute_Delta(void) // RA2: 0x00635B30
{
	Delta = Target - Source; // RA2: as above
	Length = fastmath::sqrt(Delta.X * Delta.X + Delta.Y * Delta.Y + Delta.Z * Delta.Z); // RA2: as above
}

/// <summary>
/// Starts a pass: the number of steps, the step, the first step, the wave size and the color.
/// </summary>
void RadBeam::Start_Pass(void) // RA2: 0x00635BA0
{
	Steps = (int)(Length / StepLength); // RA2: as above
	int x = Delta.X; // RA2: as above
	int y = Delta.Y; // RA2: as above
	int z = Delta.Z; // RA2: as above
	if (Steps != 0) { // RA2: as above
		x /= Steps; // RA2: as above
		y /= Steps; // RA2: as above
		z /= Steps; // RA2: as above
	} // RA2: as above
	StepDelta = Coord(x, y, z); // RA2: as above
	Current = Source; // RA2: as above
	double phase = Time; // RA2: as above
	phase /= Period; // RA2: as above
	Wave = phase * Amplitude; // RA2: as above
	CurrentColor = Color; // RA2: as above
}

/// <summary>
/// Works out the ends of step StepIndex of a type 1 beam, the source moved along the line by whole steps and raised
/// by a sine over twice the beam's length (at least 360 degrees), and the step's color, the beam's color scaled by
/// half plus half the height of the wave against the amplitude.
/// </summary>
void RadBeam::Compute_Pulse_Step(void) // RA2: 0x00635C30; excepted: 464 bytes as in the executable, 22 bytes differ (the executable loads the three color bytes earlier, between the abs and the multiplication)
{
	double span = (Length < CIRCLE_DEGREES ? CIRCLE_DEGREES : Length) * 2.0; // RA2: as above
	double index = StepIndex; // RA2: as above
	double degrees = index / Steps * span; // RA2: as above
	double wave = fastmath::sin(DEGREES_TO_RADIANS * degrees) * Wave; // RA2: as above
	int step = (int)index; // RA2: as above
	Current = StepDelta * step + Source; // RA2: as above
	Current.Z = (int)(Current.Z + wave); // RA2: as above

	double index2 = StepIndex + 1; // RA2: as above
	double degrees2 = index2 / Steps * span; // RA2: as above
	double wave2 = fastmath::sin(DEGREES_TO_RADIANS * degrees2) * Wave; // RA2: as above
	int step2 = (int)index2; // RA2: as above
	Next = Source + StepDelta * step2; // RA2: as above
	Next.Z = (int)(Next.Z + wave2); // RA2: as above

	Scale = abs((int)wave2) * 0.5 / Amplitude + 0.5; // RA2: as above
	double red = Color.Get_Red() * Scale; // RA2: as above
	double green = Color.Get_Green() * Scale; // RA2: as above
	double blue = Color.Get_Blue() * Scale; // RA2: as above
	CurrentColor.Set((unsigned char)red, (unsigned char)green, (unsigned char)blue); // RA2: as above
}

/// <summary>
/// Draws one frame of a type 2 beam (the chrono beam): on the first frame the line is set up, then while active the
/// wave is updated and every step drawn as a segment in its color. Returns true once the period is over.
/// </summary>
bool RadBeam::Draw_Swing(void) // RA2: 0x00635E00
{
	if (!Prepared) { // RA2: as above
		Prepared = true; // RA2: as above
		Setup_Swing(); // RA2: as above
	} // RA2: as above
	if (IsActive) { // RA2: as above
		Update_Swing(); // RA2: as above
		for (StepIndex = 0; StepIndex < Steps; StepIndex++) { // RA2: as above
			Compute_Swing_Step(); // RA2: as above
			Draw_Line(Current, Next, (short)DSurface::Build_Hicolor_Pixel(CurrentColor.Get_Red(), CurrentColor.Get_Green(), CurrentColor.Get_Blue())); // RA2: as above
		} // RA2: as above
	} // RA2: as above
	Time++; // RA2: as above
	return(Time >= Period); // RA2: as above
}

/// <summary>
/// Measures the line of a type 2 beam and keeps its step in SwingDelta.
/// </summary>
void RadBeam::Setup_Swing(void) // RA2: 0x00635F80
{
	Delta = Target - Source; // RA2: as above
	Length = fastmath::sqrt(Delta.X * Delta.X + Delta.Y * Delta.Y + Delta.Z * Delta.Z); // RA2: as above
	Steps = (int)(Length / StepLength); // RA2: as above
	int x = Delta.X; // RA2: as above
	int y = Delta.Y; // RA2: as above
	int z = Delta.Z; // RA2: as above
	if (Steps != 0) { // RA2: as above
		x /= Steps; // RA2: as above
		y /= Steps; // RA2: as above
		z /= Steps; // RA2: as above
	} // RA2: as above
	SwingDelta = Coord(x, y, z); // RA2: as above
}

/// <summary>
/// Sets the wave of the frame (a triangle over the period, scaled by the amplitude), moves the line along SwingDelta
/// by StartDelay * Time * Phase steps, and measures it again.
/// </summary>
void RadBeam::Update_Swing(void) // RA2: 0x00636020
{
	int period = Period; // RA2: as above
	double phase; // RA2: as above
	if (Time < period / 2) { // RA2: as above
		phase = Time * 2; // RA2: as above
	} else { // RA2: as above
		phase = period - Time * 2 % period; // RA2: as above
	} // RA2: as above
	Wave = phase / period * Amplitude; // RA2: as above
	int offset = (int)(StartDelay * Time * Phase); // RA2: as above
	Delta = Delta + Coord(SwingDelta.X * offset, SwingDelta.Y * offset, SwingDelta.Z * offset); // RA2: as above
	Length = fastmath::sqrt(Delta.X * Delta.X + Delta.Y * Delta.Y + Delta.Z * Delta.Z); // RA2: as above
	Steps = (int)(Length / StepLength); // RA2: as above
	int x = Delta.X; // RA2: as above
	int y = Delta.Y; // RA2: as above
	int z = Delta.Z; // RA2: as above
	if (Steps != 0) { // RA2: as above
		x /= Steps; // RA2: as above
		y /= Steps; // RA2: as above
		z /= Steps; // RA2: as above
	} // RA2: as above
	StepDelta = Coord(x, y, z); // RA2: as above
}

/// <summary>
/// Works out the ends of step StepIndex of a type 2 beam, the source moved along the line by whole steps and raised
/// by a half sine over the beam, and the step's color.
/// </summary>
void RadBeam::Compute_Swing_Step(void) // RA2: 0x00636130; excepted: 432 bytes as in the executable, 21 bytes differ (the executable loads the three color bytes earlier, between the abs and the multiplication)
{
	double index = StepIndex; // RA2: as above
	double degrees = index / Steps * HALF_CIRCLE_DEGREES; // RA2: as above
	double wave = fastmath::sin(DEGREES_TO_RADIANS * degrees) * Wave; // RA2: as above
	int step = (int)index; // RA2: as above
	Current = StepDelta * step + Source; // RA2: as above
	Current.Z = (int)(Current.Z + wave); // RA2: as above

	double index2 = StepIndex + 1; // RA2: as above
	double degrees2 = index2 / Steps * HALF_CIRCLE_DEGREES; // RA2: as above
	double wave2 = fastmath::sin(DEGREES_TO_RADIANS * degrees2) * Wave; // RA2: as above
	int step2 = (int)index2; // RA2: as above
	Next = StepDelta * step2 + Source; // RA2: as above
	Next.Z = (int)(Next.Z + wave2); // RA2: as above

	Scale = abs((int)wave2) * 0.5 / Amplitude + 0.5; // RA2: as above
	double red = Color.Get_Red() * Scale; // RA2: as above
	double green = Color.Get_Green() * Scale; // RA2: as above
	double blue = Color.Get_Blue() * Scale; // RA2: as above
	CurrentColor.Set((unsigned char)red, (unsigned char)green, (unsigned char)blue); // RA2: as above
}
