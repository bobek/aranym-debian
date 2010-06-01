/*
	Falcon VIDEL emulation, with zoom

	(C) 2006-2007 ARAnyM developer team

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

#ifndef VIDELZOOM_H
#define VIDELZOOM_H

class VIDEL;
class HostSurface;

class VidelZoom : public VIDEL
{
	private:
		HostSurface *surface;
		int zoomWidth, zoomHeight;
		int prevVidelWidth, prevVidelHeight, prevVidelBpp;
		int prevWidth, prevHeight, prevBpp;
		int *xtable, *ytable;

		void refreshScreen(void);

	public:
		VidelZoom(memptr, uint32);
		virtual ~VidelZoom(void);

		void reset(void);

		HostSurface *getSurface(void);
		void forceRefresh(void);
};

#endif /* VIDELZOOM_H */
