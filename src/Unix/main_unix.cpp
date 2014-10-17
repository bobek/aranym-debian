
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

#if defined _WIN32 || defined(OS_cygwin)
# define SDL_MAIN_HANDLED
#endif

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "input.h"
#include "vm_alloc.h"
#include "hardware.h"
#include "parameters.h"
#include "newcpu.h"
#include "version.h"

#define USE_VALGRIND 0
#if USE_VALGRIND
#include <valgrind/memcheck.h>
#endif

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

#ifdef OS_irix
void segmentationfault()
#else
void segmentationfault(int)
#endif
{
	grabMouse(SDL_FALSE);
	panicbug("Gotcha! Illegal memory access. Atari PC = $%x", (unsigned)showPC());
#ifdef FULL_HISTORY
	ndebug::showHistory(20, false);
	m68k_dumpstate (stderr, NULL);
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
		panicbug("Not enough free memory (ST-RAM 0x%08x + TT-RAM 0x%08x).", RAMSize, FastRAMSize);
		QuitEmulator();
	}
	RAMBaseHost = (uint8 *)FMEMORY;
	ROMBaseHost = RAMBaseHost + ROMBase;
	HWBaseHost = RAMBaseHost + HWBase;
	FastRAMBaseHost = RAMBaseHost + FastRAMBase;
# ifdef EXTENDED_SIGSEGV
	if (vm_acquire_fixed((void *)(FMEMORY + ~0xffffffL), RAMSize + ROMSize + HWSize) == false) {
		panicbug("Not enough free memory (protected mirror RAM 0x%08x).", RAMSize);
		QuitEmulator();
	}

#  ifdef HW_SIGSEGV

	if ((FakeIOBaseHost = (uint8 *)vm_acquire(0x00100000)) == VM_MAP_FAILED) {
		panicbug("Not enough free memory (Shadow IO).");
		QuitEmulator();
	}

#  endif /* HW_SISEGV */
# endif /* EXTENDED_SIGSEGV */
#else
	RAMBaseHost = (uint8*)vm_acquire(RAMSize + ROMSize + HWSize + FastRAMSize + RAMEnd);
	if (RAMBaseHost == VM_MAP_FAILED) {
		panicbug("Not enough free memory (ST-RAM 0x%08x + TT-RAM 0x%08x).", RAMSize, FastRAMSize);
		QuitEmulator();
	}

	ROMBaseHost = RAMBaseHost + ROMBase;
	HWBaseHost = RAMBaseHost + HWBase;
	FastRAMBaseHost = RAMBaseHost + FastRAMBase;
#endif
	InitMEM();
	D(bug("ST-RAM     at %p - %p (0x%08x - 0x%08x)", RAMBaseHost, RAMBaseHost + RAMSize, RAMBase, RAMBase + RAMSize));
	D(bug("TOS ROM    at %p - %p (0x%08x - 0x%08x)", ROMBaseHost, ROMBaseHost + ROMSize, ROMBase, ROMBase + ROMSize));
	D(bug("HW space   at %p - %p (0x%08x - 0x%08x)", HWBaseHost, HWBaseHost + HWSize, HWBase, HWBase + HWSize));
	D(bug("TT-RAM     at %p - %p (0x%08x - 0x%08x)", FastRAMBaseHost, FastRAMBaseHost + FastRAMSize, FastRAMBase, FastRAMBase + FastRAMSize));
	if (VideoRAMBaseHost) {
	D(bug("Video-RAM  at %p - %p (0x%08x - 0x%08x)", VideoRAMBaseHost, VideoRAMBaseHost + ARANYMVRAMSIZE, VideoRAMBase, VideoRAMBase + ARANYMVRAMSIZE));
	}
# ifdef EXTENDED_SIGSEGV
#  ifdef HW_SIGSEGV
	D(bug("FakeIOspace %p", FakeIOBaseHost));
#  endif
#  ifdef RAMENDNEEDED
	D(bug("RAMEnd needed"));
#  endif
# endif /* EXTENDED_SIGSEGV */
#endif /* DIRECT_ADDRESSING || FIXED_ADDRESSING */
}

#ifndef EXTENDED_SIGSEGV
void install_sigsegv() {
	signal(SIGSEGV, segmentationfault);
}

void uninstall_sigsegv() {
	signal(SIGSEGV, SIG_DFL);
}
#endif

static void install_signal_handler()
{
	install_sigsegv();
	D(bug("Sigsegv handler installed"));

#ifdef HAVE_SIGACTION
	{
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sigemptyset(&sa.sa_mask);
		sa.sa_handler = (void (*)(int))setactvdebug;
		sa.sa_flags = 0;
		sigaction(SIGINT, &sa, NULL);
	}
#else
	signal(SIGINT, (void (*)(int))setactvdebug);
#endif

#ifdef EXTENDED_SIGSEGV
	if (vm_protect(ROMBaseHost, ROMSize, VM_PAGE_READ)) {
		panicbug("Couldn't protect ROM");
		exit(-1);
	}

	D(bug("Protected ROM          (%p - %p)", ROMBaseHost, ROMBaseHost + ROMSize));
#if USE_VALGRIND
	VALGRIND_MAKE_MEM_DEFINED(ROMBaseHost, ROMSize);
#endif

# ifdef RAMENDNEEDED
	if (vm_protect(ROMBaseHost + ROMSize + HWSize + FastRAMSize, RAMEnd, VM_PAGE_NOACCESS)) {
		panicbug("Couldn't protect RAMEnd");
		exit(-1);
	}
	D(bug("Protected RAMEnd       (%p - %p)", ROMBaseHost + ROMSize + HWSize + FastRAMSize, ROMBaseHost + ROMSize + HWSize + FastRAMSize + RAMEnd));
#if USE_VALGRIND
	VALGRIND_MAKE_MEM_DEFINED(ROMBaseHost + ROMSize + HWSize + FastRAMSize, RAMEnd);
#endif
# endif

# ifdef HW_SIGSEGV
	if (vm_protect(HWBaseHost, HWSize, VM_PAGE_NOACCESS)) {
		panicbug("Couldn't set HW address space");
		exit(-1);
	}

	D(bug("Protected HW space     (%p - %p)", HWBaseHost, HWBaseHost + HWSize));

	if (vm_protect(RAMBaseHost + ~0xffffffUL, 0x1000000, VM_PAGE_NOACCESS)) {
		panicbug("Couldn't set mirror address space");
		QuitEmulator();
	}

	D(bug("Protected mirror space (%p - %p)", RAMBaseHost + ~0xffffffUL, RAMBaseHost + ~0xffffffUL + RAMSize + ROMSize + HWSize));
#if USE_VALGRIND
	VALGRIND_MAKE_MEM_DEFINED(HWBaseHost, HWSize);
	VALGRIND_MAKE_MEM_DEFINED(RAMBaseHost + ~0xffffffUL, 0x1000000);
#endif
# endif /* HW_SIGSEGV */

#ifdef HAVE_SBRK
	D(bug("Program break           %p", sbrk(0)));
#endif

#endif /* EXTENDED_SIGSEGV */
}

static void remove_signal_handler()
{
	uninstall_sigsegv();
	D(bug("Sigsegv handler removed"));
}

/*
 *  Main program
 */
int main(int argc, char **argv)
{
#if defined _WIN32 || defined(OS_cygwin)
	SDL_SetMainReady();
#endif

	// Initialize variables
	RAMBaseHost = NULL;
	ROMBaseHost = NULL;
	HWBaseHost = NULL;
	FastRAMBaseHost = NULL;

	// remember program name
	program_name = argv[0];

#ifdef DEBUGGER
	ndebug::init();
#endif

	// display version string on console (help when users provide debug info)
	infoprint("%s", VERSION_STRING);

	// parse command line switches
	if (!decode_switches(argc, argv))
		exit(-1);

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
