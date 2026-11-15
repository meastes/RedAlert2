/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

class CommandClass
{
	public:
		virtual ~CommandClass(void) {}

		virtual char const * Get_Unique_Name(void) const = 0;
		virtual char const * Get_Display_Name(void) const { return(Get_Unique_Name()); }
		virtual char const * Get_Category(void) const = 0; // RA2: returns wide text (0x0051AF10 and the other overrides); Keyboard_Process casts, options.cpp still reads it as narrow text
		virtual char const * Get_Description(void) const = 0;
		virtual bool PreventCombinationOverride(int key) const { return(true); } // RA2: slot 5 (0x0051AE70); whether the command also answers for the key with its modifiers
		virtual bool ExtraTriggerCondition(int key) const { return(true); } // RA2: slot 6 (0x0051AE80); whether Keyboard_Process runs the command for this key
		virtual bool Unknown1C(int key) const { return(false); } // RA2: slot 7 (0x0051AE90); whether Keyboard_Process then drops the key's repeats
		virtual void Execute(int key) const = 0; // RA2: slot 8; takes the key
};
