/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

#include "ftimer.h" // RA2: the tracker holds a frame timer
#include "timer.h" // RA2: as above
#include "vector.h" // RA2: the tracker holds a list of its records

class AbstractClass;
class AircraftClass;
class CellClass;

/*
**	RA2: a tracked missile and the cell it flies at (8 bytes, written to the saved game as they are).
*/
class KamikazeControl
{
	public:
		AircraftClass * Unit; // RA2: +0x00; the missile
		CellClass * Cell; // RA2: +0x04; the cell it is sent at
};

/*
**	RA2 tracks the aircraft that fly into their target.
*/
class KamikazeClass
{
	public:
		CDTimerClass<FrameTimerClass> Timer; // RA2: +0x00; the tracker's update timer (set to 2 when a missile is handed over)

		DynamicVectorClass<KamikazeControl *> Nodes; // RA2: +0x0C; the tracked missiles

		KamikazeClass(void); // RA2: 0x00531650, the timer runs 100 frames
		~KamikazeClass(void); // RA2: 0x00531980, deletes the records
		void Add(AircraftClass * missile, AbstractClass * target); // RA2: 0x005316A0, tracks a missile flying at the target
		void Update(void); // RA2: 0x005317C0, every 30 frames sends each missile at its cell again
		void Remove(AbstractClass * target); // RA2: 0x00531880
		HRESULT Save(IStream * stream); // RA2: 0x00531A40, Put_All saves the tracked kamikazes
		HRESULT Load(IStream * stream); // RA2: 0x00531AA0, Get_All loads the tracked kamikazes
		void Clear(void); // RA2: 0x005319E0, drops every tracked kamikaze
};

extern KamikazeClass Kamikazes;
