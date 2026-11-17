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

/* $Header: /CounterStrike/GETCPU.CPP 1     3/03/97 10:24a Joe_bostic $*/
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : GETCPU                                                       *
 *                                                                                             *
 *                    File Name : GETCPU.CPP                                                   *
 *                                                                                             *
 *                   Programmer : Steve Tall                                                   *
 *                                                                                             *
 *                   Start Date : 6/26/96                                                      *
 *                                                                                             *
 *                  Last Update : June 26th 1996 [ST]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview:                                                                                   *
 *   Example of interface to assembly language code to find CPU type                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 *   Get_CPU_Type -- interface to ASM detection code                                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "getcpu.h"

#include <cstdio>
#include <cstring>

/*
**	Prototypes for linkage to assembly module
*/
extern "C" {
	bool __cdecl Detect_MMX_Availability (void);
	bool __cdecl Detect_CMOV_Availability (void);

	extern char CPUType;
	extern char VendorID;
}


/***********************************************************************************************
 * Get_CPU_Type -- Find out what kind of CPU we are running on                                 *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    int   - reference to cpu type                                                     *
 *           bool  - reference to mmx availability flag                                        *
 *           char* - ptr to buffer to receive chip vendor info                                 *
 *           int   - length of above buffer                                                    *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    6/26/96 10:15AM ST : Created                                                             *
 *=============================================================================================*/
void Get_CPU_Type(int & cpu_type, bool & mmx, char * vendor_id, int vendor_id_length)
{
	/*
	**	Call the asm CPU detection code
	*/
	mmx = Detect_MMX_Availability();
	Detect_CMOV_Availability();

	/*
	**	Return the promised results
	*/
	cpu_type = (int)CPUType;

	if (vendor_id != NULL) {
		char * vendor_ptr = &VendorID;
		strncpy(vendor_id, vendor_ptr, vendor_id_length);
	}
}


extern "C" { // RA2: as above
	extern char CPUModel; // RA2: 0x008015A4 (between CPUType and VendorID), the CPU model number, read as a signed byte
}

/***********************************************************************************************
 * Get_CPU_Type -- the CPU class as a CPUType value                                            *
 *=============================================================================================*/
#pragma auto_inline(off) // RA2: Get_CPU_Type_Name (0x004D2B80) calls this one out of line
int Get_CPU_Type(void) // RA2: not in TibSun (0x004D2A60); classifies the result of the detection code above
{
	int family; // RA2: as above
	bool mmx; // RA2: as above
	char vendor[64]; // RA2: as above

	Get_CPU_Type(family, mmx, vendor, sizeof(vendor)); // RA2: as above

	if (family == 0 || CPUModel == 0) { // RA2: as above
		return CPU_UNKNOWN; // RA2: as above
	}

	if (stricmp(vendor, "AuthenticAMD") == 0) { // RA2: as above
		return CPU_AMD; // RA2: as above
	}
	if (stricmp(vendor, "CyrixInstead") == 0) { // RA2: as above
		return CPU_CYRIX; // RA2: as above
	}

	switch (family) { // RA2: as above
		case 4: // RA2: as above
			return CPU_486; // RA2: as above

		case 5: // RA2: as above
			return CPU_PENTIUM; // RA2: as above

		case 6: // RA2: as above
			switch (CPUModel) { // RA2: as above
				case 3: // RA2: as above
				case 5: // RA2: as above
					return CPU_PII; // RA2: as above

				case 6: // RA2: as above
					return CPU_CELERON; // RA2: as above

				case 7: // RA2: as above
				case 8: // RA2: as above
				case 10: // RA2: as above
					return CPU_PIII; // RA2: as above

				default: // RA2: as above
					return CPU_PPRO; // RA2: as above
			}

		default: // RA2: as above
			return CPU_PIV; // RA2: as above
	}
}

#pragma auto_inline(on) // RA2: as above

char const * Get_CPU_Type_Name(void) // RA2: not in TibSun (0x004D2B80)
{
	static char const * const names[CPU_COUNT] = { // RA2: the table at 0x007D8838
		"486", // RA2: as above
		"Pentium", // RA2: as above
		"Pentium Pro", // RA2: as above
		"Pentium II", // RA2: as above
		"Pentium III", // RA2: as above
		"Celeron", // RA2: as above
		"Pentium 4", // RA2: as above
		"AMD", // RA2: as above
		"Cyrix" // RA2: as above
	};

	int type = Get_CPU_Type(); // RA2: as above
	if (type > CPU_UNKNOWN && type < CPU_COUNT) { // RA2: as above
		return names[type]; // RA2: as above
	}
	return "Unknown"; // RA2: as above
}
