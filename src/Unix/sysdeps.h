/*
 *  sysdeps.h - System dependent definitions for Unix
 *
 *  Copyright (c) 2009 ARAnyM dev team (see AUTHORS)
 *
 *  Basilisk II (C) 1997-2000 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SYSDEPS_H
#define SYSDEPS_H

#if !defined(__STDC__) && !defined(__cplusplus)
#error "Your compiler is not ANSI nor ISO. Get a real one."
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef STDC_HEADERS
#error "You don't have ANSI C header files."
#endif

#ifdef OS_irix
#define OS_INCLUDES_DEFINED

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <alloca.h>
#include <dirent.h>

#endif

#ifdef OS_openbsd
#define OS_INCLUDES_DEFINED

#include <unistd.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include <utime.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>

#endif

#ifdef OS_solaris
#define OS_INCLUDES_DEFINED

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <termios.h>
#include <utime.h>
#include <alloca.h>
#include <dirent.h>

#endif

#ifdef OS_beos
#include <stdlib.h>
#include <string.h>
#endif

#ifdef MACOSX_support

#include <Carbon/Carbon.h>

#endif /* MACOSX_support */

#ifndef OS_INCLUDES_DEFINED

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_STAT_H
# include <stat.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_TYPES_H
# include <types.h>
#endif

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif

#ifdef HAVE_UTIME_H
# include <utime.h>
#endif

#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif

#ifdef HAVE_TERMIO_H
# include <termio.h>
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#endif /* OS_INCLUDES_DEFINE */

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#else
# include "getopt.h"
#endif

/* Atari and host address space are distinct */
#ifndef DIRECT_ADDRESSING
#define DIRECT_ADDRESSING 0
#endif

#ifndef FIXED_ADDRESSING
#define FIXED_ADDRESSING 0
#endif

/* Using 68k emulator */
#define EMULATED_68K 1

/* The m68k emulator uses a prefetch buffer ? */
#define USE_PREFETCH_BUFFER 0

/* Data types */
typedef unsigned char uint8;
typedef signed char int8;
#if SIZEOF_SHORT == 2
typedef unsigned short uint16;
typedef short int16;
#elif SIZEOF_INT == 2
typedef unsigned int uint16;
typedef int int16;
#else
#error "No 2 byte type, you lose."
#endif
#if SIZEOF_INT == 4
typedef unsigned int uint32;
typedef int int32;
#elif SIZEOF_LONG == 4
typedef unsigned long uint32;
typedef long int32;
#else
#error "No 4 byte type, you lose."
#endif
#if SIZEOF_LONG == 8
#ifdef OS_darwin
#ifndef _UINT64
typedef uint64_t uint64;
#define _UINT64
#endif
#else
typedef unsigned long uint64;
#endif
typedef long int64;
#define VAL64(a) (a ## l)
#define UVAL64(a) (a ## ul)
#elif SIZEOF_LONG_LONG == 8
typedef unsigned long long uint64;
typedef long long int64;
#define VAL64(a) (a ## LL)
#define UVAL64(a) (a ## uLL)
#else
#error "No 8 byte type, you lose."
#endif
#if SIZEOF_VOID_P == 4
typedef uint32 uintptr;
typedef int32 intptr;
#elif SIZEOF_VOID_P == 8
typedef uint64 uintptr;
typedef int64 intptr;
#else
#error "Unsupported size of pointer"
#endif

#define memptr uint32

#ifndef HAVE_BOOL
# define bool int
# define true 1
# define false 0
#endif

/* Define codes for all the float formats that we know of.
 * Though we only handle IEEE format.  */
#define UNKNOWN_FLOAT_FORMAT 0
#define IEEE_FLOAT_FORMAT 1
#define VAX_FLOAT_FORMAT 2
#define IBM_FLOAT_FORMAT 3
#define C4X_FLOAT_FORMAT 4

/* UAE CPU data types */
#define uae_s8 int8
#define uae_u8 uint8
#define uae_s16 int16
#define uae_u16 uint16
#define uae_s32 int32
#define uae_u32 uint32
#define uae_s64 int64
#define uae_u64 uint64
typedef uae_u32 uaecptr;
typedef char flagtype;

/* Bochs data types */
#define Bit8u uint8
#define Bit16u uint16
#define Bit32u uint32

/* Alignment restrictions */
#if defined(CPU_i386) || defined(CPU_powerpc) || defined(CPU_m68k) || defined(CPU_x86_64)
# define CPU_CAN_ACCESS_UNALIGNED
#endif

/* UAE CPU defines */
#ifdef WORDS_BIGENDIAN

#ifdef CPU_CAN_ACCESS_UNALIGNED

/* Big-endian CPUs which can do unaligned accesses */
static inline uae_u32 do_get_mem_long(uae_u32 *a) {return *a;}
static inline uae_u32 do_get_mem_word(uae_u16 *a) {return *a;}
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {*a = v;}
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {*a = v;}

#else /* CPU_CAN_ACCESS_UNALIGNED */

#if (defined(sgi) && !defined(__GNUC__))
/* The SGI MIPSPro compilers can do unaligned accesses given enough hints.
 * They will automatically inline these routines. */
#ifdef __cplusplus
extern "C" { /* only the C compiler does unaligned accesses */
#endif
extern uae_u32 do_get_mem_long(uae_u32 *a);
extern uae_u32 do_get_mem_word(uae_u16 *a);
extern void do_put_mem_long(uae_u32 *a, uae_u32 v);
extern void do_put_mem_word(uae_u16 *a, uae_u32 v);
#ifdef __cplusplus
}
#endif

#else /* sgi && !GNUC */

/* Big-endian CPUs which can not do unaligned accesses (this is not the most efficient way to do this...) */
static inline uae_u32 do_get_mem_long(uae_u32 *a) {uint8 *b = (uint8 *)a; return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];}
static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint8 *b = (uint8 *)a; return (b[0] << 8) | b[1];}
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {uint8 *b = (uint8 *)a; b[0] = v >> 24; b[1] = v >> 16; b[2] = v >> 8; b[3] = v;}
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {uint8 *b = (uint8 *)a; b[0] = v >> 8; b[1] = v;}
#endif /* sgi */

#endif /* CPU_CAN_ACCESS_UNALIGNED */

#else /* WORDS_BIGENDIAN */

#if defined(X86_ASSEMBLY) || defined(X86_64_ASSEMBLY)

/* Intel x86 */
#define X86_PPRO_OPT
static inline uae_u32 do_get_mem_long(uae_u32 *a) {uint32 retval; __asm__ ("bswap %0" : "=r" (retval) : "0" (*a) : "cc"); return retval;}
#ifdef X86_PPRO_OPT
static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint32 retval; __asm__ ("movzwl %w1,%k0\n\tshll $16,%k0\n\tbswapl %k0\n" : "=&r" (retval) : "m" (*a) : "cc"); return retval;}
#else
static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint32 retval; __asm__ ("xorl %k0,%k0\n\tmovw %w1,%w0\n\trolw $8,%w0" : "=&r" (retval) : "m" (*a) : "cc"); return retval;}
#endif
#define HAVE_GET_WORD_UNSWAPPED
#define do_get_mem_word_unswapped(a) ((uae_u32)*((uae_u16 *)(a)))
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {__asm__ ("bswap %0" : "=r" (v) : "0" (v) : "cc"); *a = v;}
#ifdef X86_PPRO_OPT
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {__asm__ ("bswapl %0" : "=&r" (v) : "0" (v << 16) : "cc"); *a = v;}
#else
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {__asm__ ("rolw $8,%0" : "=r" (v) : "0" (v) : "cc"); *a = v;}
#endif
#define HAVE_OPTIMIZED_BYTESWAP_32
/* bswap doesn't affect condition codes */
static inline uae_u32 do_byteswap_32(uae_u32 v) {__asm__ ("bswap %0" : "=r" (v) : "0" (v)); return v;}
#define HAVE_OPTIMIZED_BYTESWAP_16
#ifdef X86_PPRO_OPT
static inline uae_u32 do_byteswap_16(uae_u32 v) {__asm__ ("bswapl %0" : "=&r" (v) : "0" (v << 16) : "cc"); return v;}
#else
static inline uae_u32 do_byteswap_16(uae_u32 v) {__asm__ ("rolw $8,%0" : "=r" (v) : "0" (v) : "cc"); return v;}
#endif

#elif defined(ARMV6_ASSEMBLY) 

// #pragma message "ARM/v6 optimized sysdeps"
static inline uae_u32 do_get_mem_long(uae_u32 *a) {uint32 retval; __asm__ (
 						"rev %0, %0"
                                                 : "=r" (retval) : "0" (*a) ); return retval;}

static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint32 retval; __asm__ (
 						"revsh %0,%0\n\t"
                                                "uxth %0,%0"
                                                : "=r" (retval) : "0" (*a) ); return retval;}

// ARM v6 and above doesn't support unaligned write, but even in 68k an unaligned access is resulting in a trap
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {__asm__ ("rev %0,%0" : "=r" (v) : "0" (v) ); *a = v;}

static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {__asm__ (
						 "revsh %0, %0"
                                                : "=r" (v) : "0" (v) ); *a = v;}

#define HAVE_OPTIMIZED_BYTESWAP_32
static inline uae_u32 do_byteswap_32(uae_u32 v) {__asm__ (
						"rev %0, %0"
                                                : "=r" (v) : "0" (v) ); return v;}

#define HAVE_OPTIMIZED_BYTESWAP_16
static inline uae_u32 do_byteswap_16(uae_u32 v) {__asm__ (
  						"revsh %0, %0\n\t"
                                                "uxth %0, %0"
                                                : "=r" (v) : "0" (v) ); return v;}

#define HAVE_GET_WORD_UNSWAPPED
#define do_get_mem_word_unswapped(a) ((uae_u32)*((uae_u16 *)(a)))

/* ARM v1 to v5 support or cross ARM support */
#elif defined(ARM_ASSEMBLY)

// #pragma message "ARM/generic optimized sysdeps"

static inline uae_u32 do_get_mem_long(uae_u32 *a) {uint32 retval; __asm__ (
						"eor r3, %0, %0, ror #16\n\t"
                                                "bic r3,r3, #0x00FF0000\n\t"
                                                "mov %0, %0, ror #8\n\t"
                                                "eor %0, %0, r3, lsr #8"
                                                 : "=r" (retval) : "0" (*a) : "r3", "cc"); return retval;}

static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint32 retval; __asm__ (
 						"bic %0, %0, #0xff000000\n\t"
                                                "bic %0, %0, #0x00ff0000\n\t"
                                                "and  r3, %0, #0xff\n\t"
                                                "mov  %0, %0, lsr #8\n\t"
                                                "orr  %0, %0, r3, lsl #8"
                                                : "=r" (retval) : "0" (*a) : "r3", "cc"); return retval;}

// ARM doesn't support unaligned write, but even in 68k an unaligned access is resulting in a trap
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {__asm__ (
						"eor r3, %0, %0, ror #16\n\t"
                                                "bic r3,r3, #0x00FF0000\n\t"
                                                "mov %0, %0, ror #8\n\t"
                                                "eor %0, %0, r3, lsr #8"
                                                : "=r" (v) : "0" (v) : "r3", "cc"); *a = v;}

static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {__asm__ (
                                                "and  r3, %0, #0xff00\n\t"
                                                "mov  %0, %0, lsl #8\n\t"
                                                "orr  %0, %0, r3, lsr #8"
                                                : "=r" (v) : "0" (v) : "r3", "cc"); *a = v;}

#define HAVE_OPTIMIZED_BYTESWAP_32
static inline uae_u32 do_byteswap_32(uae_u32 v) {__asm__ (
                                                "eor r3, %0, %0, ror #16\n\t"
                                                "bic r3,r3, #0x00FF0000\n\t"
                                                "mov %0, %0, ror #8\n\t"
                                                "eor %0, %0, r3, lsr #8"
                                                : "=r" (v) : "0" (v) : "r3", "cc"); return v;}

#define HAVE_OPTIMIZED_BYTESWAP_16
static inline uae_u32 do_byteswap_16(uae_u32 v) {__asm__ (
                                               "and  r3, %0, #0xff00\n\t"
                                                "mov  %0, %0, lsl #8\n\t"
                                                "orr  %0, %0, r3, lsr #8\n\t"
                                                "bic %0,%0, #0x00FF0000"
                                                : "=r" (v) : "0" (v) : "r3", "cc"); return v;}

#define HAVE_GET_WORD_UNSWAPPED
#define do_get_mem_word_unswapped(a) ((uae_u32)*((uae_u16 *)(a)))

#elif defined(CPU_CAN_ACCESS_UNALIGNED)

/* Other little-endian CPUs which can do unaligned accesses */
static inline uae_u32 do_get_mem_long(uae_u32 *a) {uint32 x = *a; return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);}
static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint16 x = *a; return (x >> 8) | (x << 8);}
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {*a = (v >> 24) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | (v << 24);}
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {*a = ((v >> 8) & 0xff) | (v << 8);}

#else /* CPU_CAN_ACCESS_UNALIGNED */

/* Other little-endian CPUs which can not do unaligned accesses (this needs optimization) */
static inline uae_u32 do_get_mem_long(uae_u32 *a) {uint8 *b = (uint8 *)a; return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];}
static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint8 *b = (uint8 *)a; return (b[0] << 8) | b[1];}
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {uint8 *b = (uint8 *)a; b[0] = v >> 24; b[1] = v >> 16; b[2] = v >> 8; b[3] = v;}
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {uint8 *b = (uint8 *)a; b[0] = v >> 8; b[1] = v;}

#endif /* CPU_CAN_ACCESS_UNALIGNED */

#endif /* WORDS_BIGENDIAN */

#ifndef HAVE_OPTIMIZED_BYTESWAP_32
static inline uae_u32 do_byteswap_32(uae_u32 v)
	{ return (((v >> 24) & 0xff) | ((v >> 8) & 0xff00) | ((v & 0xff) << 24) | ((v & 0xff00) << 8)); }
#endif

#ifndef HAVE_OPTIMIZED_BYTESWAP_16
static inline uae_u32 do_byteswap_16(uae_u32 v)
	{ return (((v >> 8) & 0xff) | ((v & 0xff) << 8)); }
#endif

#define do_get_mem_byte(a) ((uae_u32)*((uae_u8 *)(a)))
#define do_put_mem_byte(a, v) (*(uae_u8 *)(a) = (v))

#define call_mem_get_func(func, addr) ((*func)(addr))
#define call_mem_put_func(func, addr, v) ((*func)(addr, v))
#define CPU_EMU_SIZE 0
#undef NO_INLINE_MEMORY_ACCESS
#undef MD_HAVE_MEM_1_FUNCS
#define write_log printf

#ifdef X86_ASSEMBLY
#define ASM_SYM_FOR_FUNC(a) __asm__(a)
#else
#define ASM_SYM_FOR_FUNC(a)
#endif

#ifndef REGPARAM
# define REGPARAM
#endif
#define REGPARAM2

#undef NOT_MALLOC

#undef KNOWN_ALLOC

#if DIRECT_ADDRESSING || FIXED_ADDRESSING
# define KNOWN_ALLOC	1
#else
# define KNOWN_ALLOC	0
#endif

#ifndef HAVE_STRDUP
extern "C" char *strdup(const char *s);
#endif

#ifdef OS_mingw
#include <windows.h>

/* FIXME: O_SYNC not defined in mingw, is there a replacement value ?
 * Does it make floppy working even without it ?
 */
#define O_SYNC 0

/* FIXME: These are not defined in my current mingw setup, is it
 *  available in more uptodates mingw packages ? I took these from
 *  the wine project
 */

#ifndef FILE_DEVICE_MASS_STORAGE
# define FILE_DEVICE_MASS_STORAGE        0x0000002d
#endif
#ifndef IOCTL_STORAGE_BASE
# define IOCTL_STORAGE_BASE FILE_DEVICE_MASS_STORAGE
#endif
#ifndef IOCTL_STORAGE_EJECT_MEDIA
# define IOCTL_STORAGE_EJECT_MEDIA        CTL_CODE(IOCTL_STORAGE_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

#ifndef HAVE_GETTIMEOFDAY
extern "C" void gettimeofday(struct timeval *p, void *tz /*IGNORED*/);
#endif

#endif /* OS_mingw */

#if !HAVE_STRCHR
# define strchr index
# define strrchr rindex
char *strchr (), *strrchr ();
#endif

#if !HAVE_MEMCPY
# define memcpy(d, s, n) bcopy ((s), (d), (n))
# define memmove(d, s, n) bcopy ((s), (d), (n))
#else
# if !HAVE_BCOPY
#  define bcopy(src, dest, size)  memcpy(dest, src, size)
# endif
#endif

#if !HAVE_USLEEP
# define usleep(microseconds)    {}
#endif

#ifdef MACOSX_support

extern CFBundleRef mainBundle;



#endif /* MACOSX_support */

#ifdef HW_SIGSEGV
# define SETJMP(a)	sigsetjmp(a, 1)
# define LONGJMP(a,b)	siglongjmp(a, b)
# define JMP_BUF	sigjmp_buf
#else
# define SETJMP(a)	setjmp(a)
# define LONGJMP(a,b)	longjmp(a,b)
# define JMP_BUF	jmp_buf
#endif

#if __GNUC__ < 3
# define __builtin_expect(foo,bar) (foo)
#endif
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#define ALWAYS_INLINE	inline __attribute__((always_inline))


#endif /* SYSDEPS_H */
