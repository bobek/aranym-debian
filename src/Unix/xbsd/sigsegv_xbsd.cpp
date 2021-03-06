/*
 * sigsegv_xbsd_x86.cpp - x86/x86_64 BSD SIGSEGV handler
 *
 * Copyright (c) 2001-2005 Milan Jurik of ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Bernie Meyer's UAE-JIT and Gwenole Beauchesne's Basilisk II-JIT
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
 *
 * 2013-06-16 : Adapted to 64 Bit Linux - Jens Heitmann
 * 2014-07-05 : Merged with 64bit version,
 *              lots of fixes - Thorsten Otto
 *
 */

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "memory.h"
#define DEBUG 0
#include "debug.h"

#include <csignal>

#ifndef HAVE_SIGHANDLER_T
typedef void (*sighandler_t)(int);
#endif

#if defined(OS_freebsd)
enum {
#ifdef CPU_i386
	REG_EDI = 0,
	REG_ESI = 1,
	REG_EBP = 2,
	REG_EBX = 4,
	REG_EDX = 5,
	REG_ECX = 6,
	REG_EAX = 7,
	REG_EIP = 10,
	REG_EFL = 12,
	REG_ESP = 13,
#endif
#if defined(CPU_x86_64)
	REG_RDI = 0,
	REG_RSI = 1,
	REG_RDX = 2,
	REG_RCX = 3,

	REG_R8  = 4,
	REG_R9  = 5,

	REG_RAX = 6,
	REG_RBX = 7,
	REG_RBP = 8,

	REG_R10 = 9,
	REG_R11 = 10,
	REG_R12 = 11,
	REG_R13 = 12,
	REG_R14 = 13,
	REG_R15 = 14,

	REG_EFL = 19,

	REG_RIP = 23,

	REG_RSP = 26,

#endif
};
#endif

#ifdef CPU_i386
#define REG_RIP REG_EIP
#define REG_RAX REG_EAX
#define REG_RBX REG_EBX
#define REG_RCX REG_ECX
#define REG_RDX REG_EDX
#define REG_RBP REG_EBP
#define REG_RSI REG_ESI
#define REG_RDI REG_EDI
#define REG_RSP REG_ESP
#endif

#if defined(CPU_i386) || defined(CPU_x86_64)
#define CONTEXT_NAME	uap
#define CONTEXT_TYPE	volatile ucontext_t
#define CONTEXT_ATYPE	CONTEXT_TYPE *
#ifdef CPU_i386
#	define CONTEXT_REGS    ((uae_u32 *)&CONTEXT_NAME->uc_mcontext.mc_edi)
#else
#	define CONTEXT_REGS    ((uae_u64 *)&CONTEXT_NAME->uc_mcontext.mc_rdi)
#endif
#define CONTEXT_AEFLAGS	CONTEXT_REGS[REG_EFL]
#define CONTEXT_AEIP	CONTEXT_REGS[REG_RIP]
#define CONTEXT_AEAX	CONTEXT_REGS[REG_RAX]
#define CONTEXT_AEBX	CONTEXT_REGS[REG_RBX]
#define CONTEXT_AECX	CONTEXT_REGS[REG_RCX]
#define CONTEXT_AEDX	CONTEXT_REGS[REG_RDX]
#define CONTEXT_AEBP	CONTEXT_REGS[REG_RBP]
#define CONTEXT_AESI	CONTEXT_REGS[REG_RSI]
#define CONTEXT_AEDI	CONTEXT_REGS[REG_RDI]
#endif

#include "sigsegv_common_x86.h"

static void segfault_vec(int /* sig */, siginfo_t *sip, void *CONTEXT_NAME)
{
	handle_access_fault((CONTEXT_ATYPE) CONTEXT_NAME, (memptr)(uintptr)((char *)sip->si_addr /* CONTEXT_REGS[REG_CR2] */ - FMEMORY));
}

void install_sigsegv() {
	struct sigaction sigsegv_sa;
	memset(&sigsegv_sa, 0, sizeof(sigsegv_sa));
	sigemptyset(&sigsegv_sa.sa_mask);
	sigsegv_sa.sa_handler = (sighandler_t) segfault_vec;
	sigsegv_sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sigsegv_sa, NULL);
}

void uninstall_sigsegv()
{
	signal(SIGSEGV, SIG_DFL);
}
