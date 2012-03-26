/*
 * dlgMain.cpp - Main Setup dialog 
 *
 * Copyright (c) 2002-2008 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"
#include "file.h"
#include "parameters.h"			// load/saveSettings()
#include "tools.h"
#include "version.h"
#include "sdlgui.h"
#include "dlgFileSelect.h"
#include "dlgMain.h"
#include "dlgAlert.h"
#include "dlgOs.h"
#include "dlgVideo.h"
#include "dlgKeyboard.h"
#include "dlgNetwork.h"
#include "dlgPartition.h"
#include "dlgHotkeys.h"
#include "dlgDisk.h"
#include "dlgUsb.h"
#include "bootos.h" // bootOs ptr

#ifdef OS_darwin
	extern void refreshMenuKeys();
#endif

/* The main dialog: */
enum MAINDLG {
	box_main,
	box_setup,
	text_main,
	ABOUT,
	DISCS,
	HOTKEYS,
	KEYBOARD,
	OS,
	VIDEO,
	NETWORK,
	PARTITIONS,
	USB,
	text_conffile,
	LOAD,
	SAVE,
	box_hotkeys,
	text_hotkeys,
	REBOOT,
	SHUTDOWN,
	FULLSCREEN,
	SCREENSHOT,
	HELP,
	CLOSE
};

SGOBJ maindlg[] = {
	{SGBOX, SG_BACKGROUND, 0, 0, 0, 76, 25, NULL},
	{SGBOX, 0, 0, 17, 2, 44, 15, NULL},
	{SGTEXT, 0, 0, 32, 1, 14, 1, " ARAnyM SETUP "},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 19, 3, 18, 1, "About"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 19, 5, 18, 1, "Disks"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 19, 7, 18, 1, "Hotkeys"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 19, 9, 18, 1, "Keyboard + mouse"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 19, 11, 18, 1, "Operating System"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 41, 3, 18, 1, "Video"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 41, 5, 18, 1, "Networking"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 41, 7, 18, 1, "Partitions"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 41, 9, 18, 1, "USB"},

	{SGTEXT, 0, 0, 22, 15, 12, 1, "Config file:"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 35, 15, 6, 1, "Load"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 43, 15, 6, 1, "Save"},
	
	{SGBOX, 0, 0, 17, 19, 44, 5, NULL},
	{SGTEXT, 0, 0, 27, 18, 22, 1, " Quick Access Buttons "},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 19, 20, 10, 1, "Reboot"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 19, 22, 10, 1, "Shutdown"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 32, 20, 12, 1, "Fullscreen"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 32, 22, 12, 1, "Screenshot"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 47, 20, 12, 1, "Help"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 47, 22, 12, 1, "Close"},
	{-1, 0, 0, 0, 0, 0, 0, NULL}
};

static const char *ABOUT_TEXT =
"            " VERSION_STRING "\n"
"            =============\n"
"\n"
"ARAnyM as an Open Source project has\n"
"been developed by a loosely knit team\n"
"of hackers accross the Net. Please see\n"
"the doc file AUTHORS for more info.\n"
"\n"
"This program is free software; you can\n"
"redistribute it and/or modify it under\n"
"the terms of the GNU General Public\n"
"License as published by the Free Soft-\n"
"ware Foundation; either version 2 of\n"
"the License, or (at your option) any\n"
"later version.\n"
"\n"
"This program is distributed in the\n"
"hope that it will be useful, but\n"
"WITHOUT ANY WARRANTY. See the GNU Ge-\n"
"neral Public License for more details.";

static const char *HELP_TEXT = 
"          ARAnyM SETUP menu:\n"
"          ------------------\n"
"Changes must be confirmed with APPLY first. Note that all changes are applied immediately to a running ARAnyM.\n"
"Some changes require system reboot in order to take effect. It's actually safest to always reboot after any change.";

static void setState(int index, int bits, bool set)
{
	if (set)
		maindlg[index].state |= bits;
	else
		maindlg[index].state &= ~bits;
}

DlgMain::DlgMain(SGOBJ *dlg)
	: Dialog(dlg), state(STATE_MAIN), dlgFileSelect(NULL)
{
	memset(path, 0, sizeof(path));
	// if no bootOs available then disable the Reboot button
	setState(REBOOT, SG_DISABLED, bootOs == NULL);
}

DlgMain::~DlgMain()
{
#ifdef OS_darwin
	refreshMenuKeys();
#endif
}

int DlgMain::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;

	switch(return_obj) {
		case ABOUT:
			SDLGui_Open(DlgAlertOpen(ABOUT_TEXT, ALERT_OK));
			break;
		case DISCS:
			SDLGui_Open(DlgDiskOpen());
			break;
		case KEYBOARD:
			SDLGui_Open(DlgKeyboardOpen());
			break;
		case HOTKEYS:
			SDLGui_Open(DlgHotkeysOpen());
			break;
		case OS:
			SDLGui_Open(DlgOsOpen());
			break;
		case VIDEO:
			SDLGui_Open(DlgVideoOpen());
			break;
		case NETWORK:
			SDLGui_Open(DlgNetworkOpen());
			break;
		case PARTITIONS:
			SDLGui_Open(DlgPartitionOpen());
			break;
		case USB:
			SDLGui_Open(DlgUsbOpen());
			break;
		case LOAD:
			LoadSettings();
			break;
		case SAVE:
			// make sure users understand which setting they're saving
			// best by allowing this Save button only after the "Apply" was used.
			SaveSettings();
			break;
		case REBOOT:
			if (bootOs != NULL)
				retval = Dialog::GUI_REBOOT;
			break;
		case SHUTDOWN:
			retval = Dialog::GUI_SHUTDOWN;
			break;
		case FULLSCREEN:
			bx_options.video.fullscreen = !bx_options.video.fullscreen;
			retval = Dialog::GUI_CLOSE;
			break;
		case SCREENSHOT:
			host->video->doScreenshot();
			retval = Dialog::GUI_CLOSE;
			break;
		case HELP:
			SDLGui_Open(DlgAlertOpen(HELP_TEXT, ALERT_OK));
			break;
		case CLOSE:
			retval = Dialog::GUI_CLOSE;
			break;
	}

	return retval;
}

void DlgMain::LoadSettings(void)
{
	if (strlen(path) == 0) {
		safe_strncpy(path, getConfigFile(), sizeof(path));
	}
	dlgFileSelect = (DlgFileSelect *) DlgFileSelectOpen(path, false);
	SDLGui_Open(dlgFileSelect);
	state = STATE_LOADSETTINGS;
}

void DlgMain::SaveSettings(void)
{
	if (strlen(path) == 0) {
		safe_strncpy(path, getConfigFile(), sizeof(path));
	}
	dlgFileSelect = (DlgFileSelect *) DlgFileSelectOpen(path, true);
	SDLGui_Open(dlgFileSelect);
	state = STATE_SAVESETTINGS;
}

void DlgMain::processResult(void)
{
	/* We called either a fileselector, to load/save
		or an alert box
	*/
	switch(state) {
		case STATE_LOADSETTINGS:
			if (dlgFileSelect) {
				/* Load setting if pressed OK in fileselector */
				if (dlgFileSelect->pressedOk()) {
					loadSettings(path);
				}
			}
			break;
		case STATE_SAVESETTINGS:
			if (dlgFileSelect) {
				/* Save setting if pressed OK in fileselector */
				if (dlgFileSelect->pressedOk()) {
					saveSettings(path);
				}
			}
			break;
	}
	dlgFileSelect = NULL;
	state = STATE_MAIN;
}

DlgMain *DlgMainOpen(void)
{
	return new DlgMain(maindlg);
}

/*
vim:ts=4:sw=4:
*/
