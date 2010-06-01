/*
	ROM / OS loader, EmuTOS

	ARAnyM (C) 2005-2006 Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "bootos_emutos.h"
#include "aranym_exception.h"

#define DEBUG 0
#include "debug.h"

/*	EmuTOS ROM class */

EmutosBootOs::EmutosBootOs(void) throw (AranymException)
{
	if (strlen(bx_options.emutos_path) == 0)
		throw AranymException("Path to EmuTOS ROM image file undefined");

	load(bx_options.emutos_path);

	infoprint("EmuTOS %02x%02x/%02x/%02x loading from '%s'... [OK]",
		ROMBaseHost[0x18], ROMBaseHost[0x19],
		ROMBaseHost[0x1a], ROMBaseHost[0x1b],
		bx_options.emutos_path
	);

	init();
}
/* vim:ts=4:sw=4
 */
