/*
	dlgKeyPress.cpp - dialog for asking to press a key

	Copyright (C) 2007 ARAnyM developer team

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

#include "parameters.h"
#include "sdlgui.h"
#include "input.h"
#include "dlgKeypress.h"

static SGOBJ presskeydlg[] =
{
	{ SGBOX, SG_BACKGROUND, 0, 0,0, 15,3, NULL },
	{ SGTEXT, 0, 0, 2,1, 11,1, "Press a key" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

DlgKeypress::DlgKeypress(SGOBJ *dlg)
	: Dialog(dlg)
{
	memset(&keysym, 0, sizeof(keysym));
}

DlgKeypress::~DlgKeypress()
{
}

void DlgKeypress::keyPress(const SDL_Event &event)
{
	if (event.type == SDL_KEYDOWN) {
		keysym.sym = event.key.keysym.sym;
		keysym.mod = (SDL_Keymod)(event.key.keysym.mod & HOTKEYS_MOD_MASK);
		if (SDLK_IS_MODIFIER(keysym.sym))
		{
			keysym.sym = SDLK_UNKNOWN;
		}
	}
}

int DlgKeypress::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;

	if (keysym.sym != SDLK_UNKNOWN) {
		// special hack: Enter key = no key needed, just modifiers
		if (keysym.sym == SDLK_RETURN) {
			keysym.sym = SDLK_UNKNOWN;
		}
		retval = Dialog::GUI_CLOSE;
	}

	return retval;
}

bx_hotkey &DlgKeypress::getPressedKey(void)
{
	return keysym;
}

Dialog *DlgKeypressOpen(void)
{
	return new DlgKeypress(presskeydlg);
}
