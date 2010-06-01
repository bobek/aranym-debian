/*
	Parallel port emulation, base class

	ARAnyM (C) 2005 Patrice Mandin

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

#ifndef _PARALLEL_H
#define _PARALLEL_H

#include "sysdeps.h"

class Parallel {
	public:
		Parallel(void);
		virtual ~Parallel(void);
		virtual void reset(void);
		
		virtual void setDirection(bool out);
		virtual uint8 getData();
		virtual void setData(uint8 value);
		virtual uint8 getBusy();
		virtual void setStrobe(bool high);
};

#endif /* _PARALLEL_H */
