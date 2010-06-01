/*
 * main_unix.cpp - Startup code for Unix
 *
 * Copyright (c) 2000-2006 ARAnyM developer team (see AUTHORS)
 *
 * Authors:
 *  MJ		Milan Jurik
 *  Joy		Petr Stehlik
 * 
 * Originally derived from Basilisk II (C) 1997-2000 Christian Bauer
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
#include "cpu_emulation.h"
#include "main.h"
#include "input.h"
#include "vm_alloc.h"
#include "hardware.h"
#include "parameters.h"
#include "newcpu.h"
#include "version.h"

#define DEBUG 0
#include "debug.h"

# include <cerrno>
# include <csignal>
# include <signal.h>
# include <cstdlib>

#ifndef HAVE_SIGHANDLER_T
typedef void (*sighandler_t)(int);
#endif

#ifdef OS_darwin
	extern void refreshMenuKeys();
#endif

static sighandler_t oldsegfault = SIG_ERR;

#ifndef HAVE_STRDUP
extern "C" char *strdup(const char *s)
{
	char *n = (char *)malloc(strlen(s) + 1);
	strcpy(n, s);
	return n;
}
#endif

#ifdef OS_mingw
# ifndef HAVE_GETTIMEOFDAY
extern "C" void gettimeofday(struct timeval *p, void *tz /*IGNORED*/)
{
        union {
                long long ns100;
                FILETIME ft;
        } _now;

        GetSystemTimeAsFileTime(&(_now.ft));
        p->tv_usec=(long)((_now.ns100/10LL)%1000000LL);
        p->tv_sec=(long)((_now.ns100-(116444736000000000LL))/10000000LL);
}
# endif
#endif

#if defined(NEWDEBUG)
static struct sigaction sigint_sa;
#endif

extern void showBackTrace(int, bool=true);

#ifdef OS_irix
void segmentationfault()
#else
void segmentationfault(int)
#endif
{
	grabMouse(false);
	panicbug("Gotcha! Illegal memory access. Atari PC = $%x", (unsigned)showPC());
#ifdef FULL_HISTORY
	showBackTrace(20, false);
	m68k_dumpstate (NULL);
#else
	panicbug("If the Full History was enabled you would see the last 20 instructions here.");
#endif
	exit(0);
}

static void allocate_all_memory()
{
#if DIRECT_ADDRESSING || FIXED_ADDRESSING
	// Initialize VM system
	vm_init();

#if FIXED_ADDRESSING
	if (vm_acquire_fixed((void *)FMEMORY, RAMSize + ROMSize + HWSize + FastRAMSize + RAMEnd) == false) {
		panicbug("Not enough free memory.");
		QuitEmulator();
	}
	RAMBaseHost = (uint8 *)FMEMORY;
	ROMBaseHost = RAMBaseHost + ROMBase;
	HWBaseHost = RAMBaseHost + HWBase;
	FastRAMBaseHost = RAMBaseHost + FastRAMBase;
# ifdef EXTENDED_SIGSEGV
	if (vm_acquire_fixed((void *)(FMEMORY + ~0xffffffL), RAMSize + ROMSize + HWSize) == false) {
		panicbug("Not enough free memory.");
		QuitEmulator();
	}

#  ifdef HW_SIGSEGV

	if ((FakeIOBaseHost = (uint8 *)vm_acquire(0x00100000)) == VM_MAP_FAILED) {
		panicbug("Not enough free memory.");
		QuitEmulator();
	}

#  endif /* HW_SISEGV */
# endif /* EXTENDED_SIGSEGV */
#else
	RAMBaseHost = (uint8*)vm_acquire(RAMSize + ROMSize + HWSize + FastRAMSize + RAMEnd);
	if (RAMBaseHost == VM_MAP_FAILED) {
		panicbug("Not enough free memory.");
		QuitEmulator();
	}

	ROMBaseHost = RAMBaseHost + ROMBase;
	HWBaseHost = RAMBaseHost + HWBase;
	FastRAMBaseHost = RAMBaseHost + FastRAMBase;
#endif
	D(bug("ST-RAM starts at %p (%08x)", RAMBaseHost, RAMBase));
	D(bug("TOS ROM starts at %p (%08x)", ROMBaseHost, ROMBase));
	D(bug("HW space starts at %p (%08x)", HWBaseHost, HWBase));
	D(bug("TT-RAM starts at %p (%08x)", FastRAMBaseHost, FastRAMBase));
# ifdef EXTENDED_SIGSEGV
#  ifdef HW_SIGSEGV
	D(panicbug("FakeIOspace %p", FakeIOBaseHost));
#  endif
#  ifdef RAMENDNEEDED
	D(bug("RAMEnd needed"));
#  endif
# endif /* EXTENDED_SIGSEGV */
#endif /* DIRECT_ADDRESSING || FIXED_ADDRESSING */
}

#ifdef EXTENDED_SIGSEGV
extern sighandler_t install_sigsegv();
#else
static sighandler_t install_sigsegv() {
	return signal(SIGSEGV, segmentationfault);
}
#endif

static void remove_sigsegv(sighandler_t orighandler) {
	if (orighandler != SIG_ERR)
		signal(SIGSEGV, orighandler);
}

static void install_signal_handler()
{
	oldsegfault = install_sigsegv();
	D(bug("Sigsegv handler installed"));

#ifdef NEWDEBUG
	if (bx_options.startup.debugger) {
		sigemptyset(&sigint_sa.sa_mask);
		sigint_sa.sa_handler = (void (*)(int))setactvdebug;
		sigint_sa.sa_flags = 0;
		sigaction(SIGINT, &sigint_sa, NULL);
	}
#endif

#ifdef EXTENDED_SIGSEGV
	if (vm_protect(ROMBaseHost, ROMSize, VM_PAGE_READ)) {
		panicbug("Couldn't protect ROM");
		exit(-1);
	}

	D(panicbug("Protected ROM (%08lx - %08lx)", ROMBaseHost, ROMBaseHost + ROMSize));

# ifdef RAMENDNEEDED
	if (vm_protect(ROMBaseHost + ROMSize + HWSize + FastRAMSize, RAMEnd, VM_PAGE_NOACCESS)) {
		panicbug("Couldn't protect RAMEnd");
		exit(-1);
	}
	D(panicbug("Protected RAMEnd (%08lx - %08lx)", ROMBaseHost + ROMSize + HWSize + FastRAMSize, ROMBaseHost + ROMSize + HWSize + FastRAMSize + RAMEnd));
# endif

# ifdef HW_SIGSEGV
	if (vm_protect(HWBaseHost, HWSize, VM_PAGE_NOACCESS)) {
		panicbug("Couldn't set HW address space");
		exit(-1);
	}

	D(panicbug("Protected HW space (%08lx - %08lx)", HWBaseHost, HWBaseHost + HWSize));

	if (vm_protect(RAMBaseHost + ~0xffffffL, 0x1000000, VM_PAGE_NOACCESS)) {
		panicbug("Couldn't set mirror address space");
		QuitEmulator();
	}

	D(panicbug("Protected mirror space (%08lx - %08lx)", RAMBaseHost + ~0xffffffL, RAMBaseHost + ~0xffffffL + RAMSize + ROMSize + HWSize));
# endif /* HW_SIGSEGV */
#endif /* EXTENDED_SIGSEGV */
}

static void remove_signal_handler()
{
	remove_sigsegv(oldsegfault);
	D(bug("Sigsegv handler removed"));
}

/*
 *  Main program
 */
int main(int argc, char **argv)
{
	// Initialize variables
	RAMBaseHost = NULL;
	ROMBaseHost = NULL;
	HWBaseHost = NULL;
	FastRAMBaseHost = NULL;

	// display version string on console (help when users provide debug info)
	infoprint("%s", VERSION_STRING);

	// remember program name
	program_name = argv[0];

#ifdef NEWDEBUG
	ndebug::init();
#endif

	// parse command line switches
	if (!decode_switches(argc, argv))
		exit(-1);

#ifdef NEWDEBUG
	signal(SIGINT, setactvdebug);
#endif

	allocate_all_memory();

	// Initialize everything
	D(bug("Initializing All Modules..."));
	if (!InitAll())
		QuitEmulator();
	D(bug("Initialization complete"));

	install_signal_handler();

#ifdef OS_darwin
	refreshMenuKeys();
#endif

	// Start 68k and jump to ROM boot routine
	D(bug("Starting emulation..."));
	Start680x0();

	// returning from emulation after the NMI
	remove_signal_handler();

	QuitEmulator();

	return 0;
}


/*
 *  Quit emulator
 */
void QuitEmulator(void)
{
	D(bug("QuitEmulator"));

#if EMULATED_68K
	// Exit 680x0 emulation
	Exit680x0();
#endif

	ExitAll();

	// Free ROM/RAM areas
#if DIRECT_ADDRESSING || FIXED_ADDRESSING
	if (RAMBaseHost != VM_MAP_FAILED) {
#ifdef RAMENDNEEDED
		vm_release(RAMBaseHost + RAMSize + ROMSize + HWSize + FastRAMSize, RAMEnd);
#endif
		vm_release(RAMBaseHost, RAMSize);
		RAMBaseHost = NULL;
	}
	if (ROMBaseHost != VM_MAP_FAILED) {
		vm_release(ROMBaseHost, ROMSize);
		ROMBaseHost = NULL;
	}
	if (HWBaseHost != VM_MAP_FAILED) {
		vm_release(HWBaseHost, HWSize);
		HWBaseHost = NULL;
	}
	if (FastRAMBaseHost !=VM_MAP_FAILED) {
		vm_release(FastRAMBaseHost, FastRAMSize);
		FastRAMBaseHost = NULL;
	}
#else
	free(RAMBaseHost);
#endif

	// Exit VM wrappers
	vm_exit();

	exit(0); // the Quit is real
}
