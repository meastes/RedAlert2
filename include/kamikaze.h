/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

class AbstractClass;

/*
**	RA2 tracks the aircraft that fly into their target.
*/
class KamikazeClass
{
	public:
		void Remove(AbstractClass * target);
		HRESULT Save(IStream * stream); // RA2: 0x00531A40, Put_All saves the tracked kamikazes
		HRESULT Load(IStream * stream); // RA2: 0x00531AA0, Get_All loads the tracked kamikazes
		void Clear(void); // RA2: 0x005319E0, drops every tracked kamikaze
};

extern KamikazeClass Kamikazes;
