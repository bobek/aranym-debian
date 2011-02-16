/*
 * debug.cpp - CPU debugger
 *
 * Copyright (c) 2001-2010 Milan Jurik of ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Bernd Schmidt's UAE
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
/*
 * UAE - The Un*x Amiga Emulator
 *
 * Debugger
 *
 * (c) 1995 Bernd Schmidt
 *
 */

#include "sysdeps.h"

#include "memory.h"
#include "newcpu.h"
#include "debug.h"

#include "input.h"
#include "cpu_emulation.h"

#include "main.h"

static int debugger_active = 0;
int debugging = 0;
int irqindebug = 0;

int ignore_irq = 0;

#ifdef UAEDEBUG
static int do_skip;
static uaecptr skipaddr;
static char old_debug_cmd[80];
#endif

// MJ static FILE *logfile;

void activate_debugger (void)
{
#ifdef NEWDEBUG
	ndebug::do_skip = false;
#endif
#ifdef UAEDEBUG
	do_skip = 0;
#endif
	if (debugger_active)
		return;
	debugger_active = 1;
	SPCFLAGS_SET( SPCFLAG_BRK );
	debugging = 1;
	/* use_debugger = 1; */
}

void deactivate_debugger(void)
{
	debugging = 0;
	debugger_active = 0;
}

unsigned int firsthist = 0;
unsigned int lasthist = 0;
#ifdef NEED_TO_DEBUG_BADLY
struct regstruct history[MAX_HIST];
struct flag_struct historyf[MAX_HIST];
#else
uaecptr history[MAX_HIST];
#endif

#ifdef UAEDEBUG

static void ignore_ws (char **c)
{
	while (**c && isspace(**c)) (*c)++;
}

static uae_u32 readhex (char **c)
{
	uae_u32 val = 0;
	char nc;

	ignore_ws (c);

	while (isxdigit(nc = **c)) {
		(*c)++;
		val *= 16;
		nc = toupper(nc);
		if (isdigit(nc)) {
			val += nc - '0';
		}
		else {
			val += nc - 'A' + 10;
		}
	}
	return val;
}

static char next_char( char **c)
{
	ignore_ws (c);
	return *(*c)++;
}

static int more_params (char **c)
{
	ignore_ws (c);
	return (**c) != 0;
}

static void dumpmem (uaecptr addr, uaecptr *nxmem, int lines)
{
	broken_in = 0;
	for (;lines-- && !broken_in;) {
		int i;
		printf ("%08lx ", (unsigned long)addr);
		for (i = 0; i < 16; i++) {
			printf ("%04x ", phys_get_word(addr));
			addr += 2;
		}
		printf ("\n");
	}
	*nxmem = addr;
}

static void writeintomem (char **c)
{
	uae_u8 *p = phys_get_real_address (0);
	uae_u32 addr = 0;
	uae_u32 val = 0;
	char nc;

	ignore_ws(c);
	while (isxdigit(nc = **c)) {
		(*c)++;
		addr *= 16;
		nc = toupper(nc);
		if (isdigit(nc)) {
			addr += nc - '0';
		}
		else {
			addr += nc - 'A' + 10;
		}
	}
	ignore_ws(c);
	while (isxdigit(nc = **c)) {
		(*c)++;
		val *= 10;
		nc = toupper(nc);
		if (isdigit(nc)) {
			val += nc - '0';
		}
	}

	if (addr < RAMSize) {
		p[addr] = val>>24 & 0xff;
		p[addr+1] = val>>16 & 0xff;
		p[addr+2] = val>>8 & 0xff;
		p[addr+3] = val & 0xff;
		printf("Wrote %d at %08x\n",val,addr);
	}
	else
		printf("Invalid address %08x\n",addr);
}

static int trace_same_insn_count;
static uae_u8 trace_insn_copy[10];
static struct regstruct trace_prev_regs;
#endif

void showBackTrace(int count, bool showLast = true)
{
	unsigned int temp;
#ifdef NEED_TO_DEBUG_BADLY
	struct regstruct save_regs = regs;
	struct flag_struct save_flags = regflags;
#endif

	if (! showLast) {
		// do not show the last instruction as it causes the aranym to segfault
		if (lasthist > 0)
			lasthist--;
		else
			lasthist = MAX_HIST-1;
	}
	temp = lasthist;
	while (count-- > 0 && temp != firsthist) {
		if (temp == 0)
			temp = MAX_HIST-1;
		else
			temp--;
	}

	while (temp != lasthist) {
#ifdef NEED_TO_DEBUG_BADLY
		regs = history[temp];
		regflags = historyf[temp];
		m68k_dumpstate (NULL);
#else
		m68k_disasm (history[temp], NULL, 1);
#endif
		if (++temp == MAX_HIST)
			temp = 0;
	}

#ifdef NEED_TO_DEBUG_BADLY
	regs = save_regs;
	regflags = save_flags;
#endif
}

void debug (void)
{
	if (ignore_irq && regs.s && !regs.m ) {
		SPCFLAGS_SET( SPCFLAG_BRK );
		return;
	}
#ifdef NEWDEBUG
	ndebug::run();
#endif
#ifdef UAEDEBUG
	char input[80];
	uaecptr nextpc,nxdis,nxmem;

	input[0] = '\n';
	input[1] = '\0';

	if (do_skip && skipaddr == 0xC0DEDBAD) {
#if 0
		if (trace_same_insn_count > 0) {
			if (memcmp (trace_insn_copy, regs.pcp, 10) == 0
			        && memcmp (trace_prev_regs.regs, regs.regs, sizeof regs.regs) == 0) {
				trace_same_insn_count++;
				return;
			}
		}
		if (trace_same_insn_count > 1)
			fprintf (logfile, "[ repeated %d times ]\n", trace_same_insn_count);
#endif
		m68k_dumpstate (&nextpc);
		trace_same_insn_count = 1;
		memcpy (trace_insn_copy, phys_get_real_address(m68k_getpc()), 10);
		memcpy (&trace_prev_regs, &regs, sizeof regs);
	}

	if (do_skip && (m68k_getpc() != skipaddr/* || regs.a[0] != 0x1e558*/)) {
		SPCFLAGS_SET( SPCFLAG_BRK );
		return;
	}
	do_skip = 0;

	irqindebug = 0;

#ifndef FULL_HISTORY
#ifdef NEED_TO_DEBUG_BADLY
	history[lasthist] = regs;
	historyf[lasthist] = regflags;
#else
	history[lasthist] = m68k_getpc();
#endif
	if (++lasthist == MAX_HIST) lasthist = 0;
	if (lasthist == firsthist) {
		if (++firsthist == MAX_HIST) firsthist = 0;
	}
#endif

	m68k_dumpstate (&nextpc);
	nxdis = nextpc;
	nxmem = 0;

	// release keyboard and mouse control
	bool wasGrabbed = grabMouse(false);

	for (;;) {
		char cmd, *inptr;

		if (irqindebug) printf("i");
		if (ignore_irq) printf("I");
		printf (">");
		fflush (stdout);

		if (fgets (input, 80, stdin) == 0) {
			if (wasGrabbed) grabMouse(true);	// lock keyboard and mouse
			return;
		}

		if (input[0] == '\n') memcpy(input, old_debug_cmd, 80);
		inptr = input;
		memcpy (old_debug_cmd, input, 80);
		cmd = next_char (&inptr);
		switch (cmd) {
		case 'i':
			irqindebug = !irqindebug;
			printf("IRQ %s\n", irqindebug ? "enabled" : "disabled");
			break;
		case 'I':
			ignore_irq = !ignore_irq;
			printf("IRQ debugging %s\n", ignore_irq ? "enabled" : "disabled");
			break;
		case 'r':
			m68k_dumpstate (&nextpc);
			break;
		case 'R':
			m68k_reset();
			break;
		case 'A':
			if (more_params (&inptr)) {
				uae_u32 reg = readhex(&inptr);
				if (reg < 8)
					if (more_params (&inptr))
						m68k_areg (regs, reg) = readhex(&inptr);
			}
			break;
		case 'D':
			if (more_params (&inptr)) {
				uae_u32 reg = readhex(&inptr);
				if (reg < 8)
					if (more_params (&inptr))
						m68k_dreg (regs, reg) = readhex(&inptr);
			}
			break;
		case 'W':
			writeintomem (&inptr);
			break;
		case 'S': {
				uae_u8 *memp;
				uae_u32 src, len;
				char *name;
				FILE *fp;

				if (!more_params (&inptr))
					goto S_argh;

				name = inptr;
				while (*inptr != '\0' && !isspace (*inptr))
					inptr++;
				if (!isspace (*inptr))
					goto S_argh;

				*inptr = '\0';
				inptr++;
				if (!more_params (&inptr))
					goto S_argh;
				src = readhex (&inptr);
				if (!more_params (&inptr))
					goto S_argh;
				len = readhex (&inptr);
				if (! valid_address (src, false, len)) {
					printf ("Invalid memory block\n");
					break;
				}
				memp = phys_get_real_address (src);
				fp = fopen (name, "w");
				if (fp == NULL) {
					printf ("Couldn't open file\n");
					break;
				}
				if (fwrite (memp, 1, len, fp) != len) {
					printf ("Error writing file\n");
				}
				fclose (fp);
				break;

S_argh:
				printf ("S command needs more arguments!\n");
				break;
			}
		case 'd': {
				uae_u32 daddr;
				int count;

				if (more_params(&inptr))
					daddr = readhex(&inptr);
				else
					daddr = nxdis;
				if (more_params(&inptr))
					count = readhex(&inptr);
				else
					count = 10;
				m68k_disasm (daddr, &nxdis, count);
			}
			break;
		case 't':
			if (more_params (&inptr))
				m68k_setpc (readhex (&inptr));
			SPCFLAGS_SET( SPCFLAG_BRK );
			if (wasGrabbed) grabMouse(true);	// lock keyboard and mouse
			return;
		case 'z':
			skipaddr = nextpc;
			do_skip = 1;
			irqindebug = true;
			SPCFLAGS_SET( SPCFLAG_BRK );
			if (wasGrabbed) grabMouse(true);	// lock keyboard and mouse
			return;

		case 'f':
			skipaddr = readhex (&inptr);
			do_skip = 1;
			irqindebug = true;
			SPCFLAGS_SET( SPCFLAG_BRK );
			if (skipaddr == 0xC0DEDBAD) {
				trace_same_insn_count = 0;
				memcpy (trace_insn_copy, phys_get_real_address(m68k_getpc()), 10);
				memcpy (&trace_prev_regs, &regs, sizeof regs);
			}
			if (wasGrabbed) grabMouse(true);	// lock keyboard and mouse
			return;

		case 'q':
			QuitEmulator();
			debugger_active = 0;
			debugging = 0;
			if (wasGrabbed) grabMouse(true);	// lock keyboard and mouse
			return;

		case 'g':
			if (more_params (&inptr))
				m68k_setpc (readhex (&inptr));
			fill_prefetch_0 ();
			debugger_active = 0;
			debugging = 0;
			if (wasGrabbed) grabMouse(true);	// lock keyboard and mouse
			return;

		case 'H': {
				int count;

				if (more_params(&inptr))
					count = readhex(&inptr);
				else
					count = 10;
				if (count < 0)
					break;
				showBackTrace(count);
			}
			break;
		case 'm': {
				uae_u32 maddr;
				int lines;
				if (more_params(&inptr))
					maddr = readhex(&inptr);
				else
					maddr = nxmem;
				if (more_params(&inptr))
					lines = readhex(&inptr);
				else
					lines = 16;
				dumpmem(maddr, &nxmem, lines);
			}
			break;
		case 'h':
		case '?':
			printf ("         HELP for ARAnyM Debugger\n");
			printf ("         ------------------------\n\n");
			printf ("  g [<address>]         Start execution at the current address or <address>\n");
			printf ("  r                     Dump state of the CPU\n");
			printf ("  R                     Reset CPU & FPU\n");
			printf ("  i                     Enable/Disable IRQ\n");
			printf ("  I                     Enable/Disable IRQ debbuging\n");
			printf ("  A <number> <value>    Set Ax\n");
			printf ("  D <number> <value>    Set Dx\n");
			printf ("  m <address> <lines>   Memory dump starting at <address>\n");
			printf ("  d <address> <lines>   Disassembly starting at <address>\n");
			printf ("  t [<address>]         Step one instruction\n");
			printf ("  z                     Step through one instruction - useful for JSR, DBRA etc\n");
			printf ("  f <address>           Step forward until PC == <address>\n");
			printf ("  H <count>             Show PC history <count> instructions\n");
			printf ("  W <address> <value>   Write into ARAnyM memory\n");
			printf ("  S <file> <addr> <n>   Save a block of ARAnyM memory\n");
			printf ("  h,?                   Show this help page\n");
			printf ("  q                     Quit the emulator. You don't want to use this command.\n\n");
			break;
		}
	}
#endif /* UAEDEBUG */
}

/*
vim:ts=4:sw=4:
*/
