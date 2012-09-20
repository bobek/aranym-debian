/*
 * fpu_mpfr.cpp - emulate 68881/68040 fpu with mpfr
 *
 * Copyright (c) 2012 Andreas Schwab
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
#include <cstdio>
#include "memory.h"
#include "readcpu.h"
#include "newcpu.h"
#include "main.h"
#define FPU_IMPLEMENTATION
#include "fpu/fpu.h"

#include "fpu/flags.h"
#include "fpu/exceptions.h"
#include "fpu/rounding.h"
#include "fpu/impl.h"

#define SINGLE_PREC 24
#define SINGLE_MIN_EXP -126
#define SINGLE_MAX_EXP 127
#define SINGLE_BIAS 127
#define DOUBLE_PREC 53
#define DOUBLE_MIN_EXP -1022
#define DOUBLE_MAX_EXP 1023
#define DOUBLE_BIAS 1023
#define EXTENDED_PREC 64
#define EXTENDED_MIN_EXP -16383
#define EXTENDED_MAX_EXP 16383
#define EXTENDED_BIAS 16383

fpu_t fpu;
// The constant ROM
// Constants 48 to 63 are mapped to index 16 to 31
const int num_fpu_constants = 32;
static mpfr_t fpu_constant_rom[num_fpu_constants];
#define FPU_CONSTANT_ONE fpu_constant_rom[18]
// Exceptions generated during execution in addition to the ones
// maintained by mpfr
static uae_u32 cur_exceptions;
static uaecptr cur_instruction_address;

static void
set_format (int prec)
{
  // MPFR represents numbers as 0.m*2^e
  switch (prec)
    {
    case SINGLE_PREC:
      mpfr_set_emin (SINGLE_MIN_EXP + 1 - (SINGLE_PREC - 1));
      mpfr_set_emax (SINGLE_MAX_EXP + 1);
      break;
    case DOUBLE_PREC:
      mpfr_set_emin (DOUBLE_MIN_EXP + 1 - (DOUBLE_PREC - 1));
      mpfr_set_emax (DOUBLE_MAX_EXP + 1);
      break;
    case EXTENDED_PREC:
      mpfr_set_emin (EXTENDED_MIN_EXP + 1 - (EXTENDED_PREC - 1));
      mpfr_set_emax (EXTENDED_MAX_EXP + 1);
      break;
    }
}

static mpfr_rnd_t
get_cur_rnd ()
{
  switch (get_rounding_mode ())
    {
    default:
    case FPCR_ROUND_NEAR:
      return MPFR_RNDN;
    case FPCR_ROUND_ZERO:
      return MPFR_RNDZ;
    case FPCR_ROUND_MINF:
      return MPFR_RNDD;
    case FPCR_ROUND_PINF:
      return MPFR_RNDU;
    }
}

static mpfr_prec_t
get_cur_prec ()
{
  switch (get_rounding_precision ())
    {
    default:
    case FPCR_PRECISION_EXTENDED:
      return EXTENDED_PREC;
    case FPCR_PRECISION_SINGLE:
      return SINGLE_PREC;
    case FPCR_PRECISION_DOUBLE:
      return DOUBLE_PREC;
    }
}

void
fpu_init (bool integral_68040)
{
  fpu.is_integral = integral_68040;

  mpfr_set_default_prec (EXTENDED_PREC);
  mpfr_set_default_rounding_mode (MPFR_RNDN);
  set_format (EXTENDED_PREC);

  for (int i = 0; i < 8; i++)
    mpfr_init (fpu.registers[i].f);
  mpfr_init (fpu.result.f);

  // Initialize constant ROM
  for (int i = 0; i < num_fpu_constants; i++)
    mpfr_init (fpu_constant_rom[i]);

  // 0: pi
  mpfr_const_pi (fpu_constant_rom[0], MPFR_RNDN);
  // 11: log10 (2)
  mpfr_set_ui (fpu_constant_rom[11], 2, MPFR_RNDN);
  mpfr_log10 (fpu_constant_rom[11], fpu_constant_rom[11], MPFR_RNDN);
  // 12: e
  mpfr_set_ui (fpu_constant_rom[12], 1, MPFR_RNDN);
  mpfr_exp (fpu_constant_rom[12], fpu_constant_rom[12], MPFR_RNDN);
  // 13: log2 (e)
  mpfr_log2 (fpu_constant_rom[13], fpu_constant_rom[12], MPFR_RNDN);
  // 14: log10 (e)
  mpfr_log10 (fpu_constant_rom[14], fpu_constant_rom[12], MPFR_RNDN);
  // 15: 0
  mpfr_set_zero (fpu_constant_rom[15], 0);
  // 48: ln (2)
  mpfr_set_ui (fpu_constant_rom[16], 2, MPFR_RNDN);
  mpfr_log (fpu_constant_rom[16], fpu_constant_rom[16], MPFR_RNDN);
  // 49: ln (10)
  mpfr_set_ui (fpu_constant_rom[17], 2, MPFR_RNDN);
  mpfr_log10 (fpu_constant_rom[17], fpu_constant_rom[17], MPFR_RNDN);
  // 50 to 63: powers of 10
  mpfr_set_ui (fpu_constant_rom[18], 1, MPFR_RNDN);
  mpfr_set_ui (fpu_constant_rom[19], 10, MPFR_RNDN);
  for (int i = 20; i < 31; i++)
    mpfr_sqr (fpu_constant_rom[i], fpu_constant_rom[i - 1], MPFR_RNDN);

  fpu_reset ();
}

void
fpu_exit ()
{
  for (int i = 0; i < 8; i++)
    mpfr_clear (fpu.registers[i].f);
  mpfr_clear (fpu.result.f);
  for (int i = 0; i < num_fpu_constants; i++)
    mpfr_clear (fpu_constant_rom[i]);
}

void
fpu_reset ()
{
  set_fpcr (0);
  set_fpsr (0);
  fpu.instruction_address = 0;

  for (int i = 0; i < 8; i++)
    mpfr_set_nan (fpu.registers[i].f);
}

fpu_register::operator long double ()
{
  return mpfr_get_ld (f, MPFR_RNDN);
}

static bool
get_fp_addr (uae_u32 opcode, uae_u32 *addr, bool write)
{
  uaecptr pc;
  int mode;
  int reg;

  mode = (opcode >> 3) & 7;
  reg = opcode & 7;
  switch (mode)
    {
    case 0:
    case 1:
      return false;
    case 2:
      *addr = m68k_areg (regs, reg);
      break;
    case 3:
      *addr = m68k_areg (regs, reg);
      break;
    case 4:
      *addr = m68k_areg (regs, reg);
      break;
    case 5:
      *addr = m68k_areg (regs, reg) + (uae_s16) next_iword();
      break;
    case 6:
      *addr = get_disp_ea_020 (m68k_areg (regs, reg), next_iword());
      break;
    case 7:
      switch (reg)
	{
	case 0:
	  *addr = (uae_s16) next_iword();
	  break;
	case 1:
	  *addr = next_ilong();
	  break;
	case 2:
	  if (write)
	    return false;
	  pc = m68k_getpc ();
	  *addr = pc + (uae_s16) next_iword();
	  break;
	case 3:
	  if (write)
	    return false;
	  pc = m68k_getpc ();
	  *addr = get_disp_ea_020 (pc, next_iword());
	  break;
	default:
	  return false;
	}
    }
  return true;
}

static void
set_from_single (mpfr_t value, uae_u32 data)
{
  int s = data >> 31;
  int e = (data >> 23) & 0xff;
  uae_u32 m = data & 0x7fffff;

  if (e == 0xff)
    {
      if (m != 0)
	mpfr_set_nan (value);
      else
	mpfr_set_inf (value, 0);
    }
  else
    {
      if (e != 0)
	// Add integer bit
	m |= 0x800000;
      else
	e++;
      // Remove bias
      e -= SINGLE_BIAS;
      mpfr_set_ui_2exp (value, m, e - (SINGLE_PREC - 1), MPFR_RNDN);
    }
  mpfr_setsign (value, value, s, MPFR_RNDN);
}

static void
set_from_double (mpfr_t value, uae_u32 words[2])
{
  int s = words[0] >> 31;
  int e = (words[0] >> 20) & 0x7ff;
  uae_u32 m = words[0] & 0xfffff;

  if (e == 0x7ff)
    {
      if (m != 0)
	mpfr_set_nan (value);
      else
	mpfr_set_inf (value, 0);
    }
  else
    {
      if (e != 0)
	// Add integer bit
	m |= 0x100000;
      else
	e++;
      // Remove bias
      e -= DOUBLE_BIAS;
      mpfr_set_uj_2exp (value, ((uintmax_t) m << 32) | words[1],
			e - (DOUBLE_PREC - 1), MPFR_RNDN);
    }
  mpfr_setsign (value, value, s, MPFR_RNDN);
}

static void
set_from_extended (mpfr_t value, uae_u32 words[3])
{
  int s = words[0] >> 31;
  int e = (words[0] >> 16) & 0x7fff;

  if (e == 0x7fff)
    {
      if (((words[1] & 0x7fffffff) | words[2]) != 0)
	mpfr_set_nan (value);
      else
	mpfr_set_inf (value, 0);
    }
  else
    {
      // Remove bias
      e -= EXTENDED_BIAS;
      mpfr_set_uj_2exp (value, ((uintmax_t) words[1] << 32) | words[2],
			e - (EXTENDED_PREC - 1), MPFR_RNDN);
    }
  mpfr_setsign (value, value, s, MPFR_RNDN);
}

#define from_bcd(d) ((d) < 10 ? (d) : (d) - 10)

static void
set_from_packed (mpfr_t value, uae_u32 words[3])
{
  char str[32], *p = str;
  int sm = words[0] >> 31;
  int se = (words[0] >> 30) & 1;
  int i;

  if (((words[0] >> 16) & 0x7fff) == 0x7fff)
    {
      if ((words[1] | words[2]) != 0)
	mpfr_set_nan (value);
      else
	mpfr_set_inf (value, 0);
    }
  else
    {
      if (sm)
	*p++ = '-';
      *p++ = from_bcd (words[0] & 15) + '0';
      *p++ = '.';
      for (i = 0; i < 8; i++)
	{
	  p[i] = from_bcd ((words[1] >> (28 - i * 4)) & 15) + '0';
	  p[i + 8] = from_bcd ((words[2] >> (28 - i * 4)) & 15) + '0';
	}
      p += 16;
      *p++ = 'e';
      if (se)
	*p++ = '-';
      *p++ = from_bcd ((words[0] >> 24) & 15) + '0';
      *p++ = from_bcd ((words[0] >> 20) & 15) + '0';
      *p++ = from_bcd ((words[0] >> 16) & 15) + '0';
      *p = 0;
      mpfr_set_str (value, str, 10, MPFR_RNDN);
    }
  mpfr_setsign (value, value, sm, MPFR_RNDN);
}

static bool
get_fp_value (uae_u32 opcode, uae_u32 extra, mpfr_t value)
{
  int mode, reg, size;
  uaecptr pc;
  uae_u32 addr;
  uae_u32 words[3];
  static const int sz1[8] = {4, 4, 12, 12, 2, 8, 1, 0};
  static const int sz2[8] = {4, 4, 12, 12, 2, 8, 2, 0};

  if ((extra & 0x4000) == 0)
    {
      mpfr_set (value, fpu.registers[(extra >> 10) & 7].f, MPFR_RNDN);
      return true;
    }
  mode = (opcode >> 3) & 7;
  reg = opcode & 7;
  size = (extra >> 10) & 7;
  switch (mode)
    {
    case 0:
      switch (size)
	{
	case 6:
	  mpfr_set_si (value, (uae_s8) m68k_dreg (regs, reg), MPFR_RNDN);
	  break;
	case 4:
	  mpfr_set_si (value, (uae_s16) m68k_dreg (regs, reg), MPFR_RNDN);
	  break;
	case 0:
	  mpfr_set_si (value, (uae_s32) m68k_dreg (regs, reg), MPFR_RNDN);
	  break;
	case 1:
	  set_from_single (value, m68k_dreg (regs, reg));
	  break;
	default:
	  return false;
	}
      return true;
    case 1:
      return false;
    case 2:
    case 3:
      addr = m68k_areg (regs, reg);
      break;
    case 4:
      addr = m68k_areg (regs, reg) - (reg == 7 ? sz2[size] : sz1[size]);
      break;
    case 5:
      addr = m68k_areg (regs, reg) + (uae_s16) next_iword ();
      break;
    case 6:
      addr = get_disp_ea_020 (m68k_areg (regs, reg), next_iword ());
      break;
    case 7:
      switch (reg)
	{
	case 0:
	  addr = (uae_s16) next_iword ();
	  break;
	case 1:
	  addr = next_ilong ();
	  break;
	case 2:
	  pc = m68k_getpc ();
	  addr = pc + (uae_s16) next_iword ();
	  break;
	case 3:
	  pc = m68k_getpc ();
	  addr = get_disp_ea_020 (pc, next_iword ());
	  break;
	case 4:
	  addr = m68k_getpc ();
	  m68k_incpc (sz2[size]);
	  if (size == 6) // Immediate byte
	    addr++;
	  break;
	default:
	  return false;
	}
    }

  switch (size)
    {
    case 0:
      mpfr_set_si (value, (uae_s32) get_long (addr), MPFR_RNDN);
      break;
    case 1:
      set_from_single (value, get_long (addr));
      break;
    case 2:
      words[0] = get_long (addr);
      words[1] = get_long (addr + 4);
      words[2] = get_long (addr + 8);
      set_from_extended (value, words);
      break;
    case 3:
      words[0] = get_long (addr);
      words[1] = get_long (addr + 4);
      words[2] = get_long (addr + 8);
      set_from_packed (value, words);
      break;
    case 4:
      mpfr_set_si (value, (uae_s16) get_word (addr), MPFR_RNDN);
      break;
    case 5:
      words[0] = get_long (addr);
      words[1] = get_long (addr + 4);
      set_from_double (value, words);
      break;
    case 6:
      mpfr_set_si (value, (uae_s8) get_byte (addr), MPFR_RNDN);
      break;
    default:
      return false;
    }

  switch (mode)
    {
    case 3:
      m68k_areg (regs, reg) += reg == 7 ? sz2[size] : sz1[size];
      break;
    case 4:
      m68k_areg (regs, reg) -= reg == 7 ? sz2[size] : sz1[size];
      break;
    }

  return true;
}

static void
update_exceptions ()
{
  uae_u32 exc, aexc;

  exc = cur_exceptions;
  // Add any mpfr detected exceptions
  if (mpfr_underflow_p ())
    exc |= FPSR_EXCEPTION_UNFL;
  if (mpfr_overflow_p ())
    exc |= FPSR_EXCEPTION_OVFL;
  if (mpfr_inexflag_p ())
    exc |= FPSR_EXCEPTION_INEX2;
  set_exception_status (exc);

  aexc = get_accrued_exception ();
  if (exc & (FPSR_EXCEPTION_SNAN|FPSR_EXCEPTION_OPERR))
    aexc |= FPSR_ACCR_IOP;
  if (exc & FPSR_EXCEPTION_OVFL)
    aexc |= FPSR_ACCR_OVFL;
  if ((exc & (FPSR_EXCEPTION_UNFL|FPSR_EXCEPTION_INEX2))
      == (FPSR_EXCEPTION_UNFL|FPSR_EXCEPTION_INEX2))
    aexc |= FPSR_ACCR_UNFL;
  if (exc & FPSR_EXCEPTION_DZ)
    aexc |= FPSR_ACCR_DZ;
  if (exc & (FPSR_EXCEPTION_INEX1|FPSR_EXCEPTION_INEX2|FPSR_EXCEPTION_OVFL))
    aexc |= FPSR_ACCR_INEX;
  set_accrued_exception (aexc);

  if ((fpu.fpcr.exception_enable & exc) != 0)
    {
      fpu.instruction_address = cur_instruction_address;
      // TODO: raise exceptions
      // Problem: FPSP040 depends on proper FPU stack frames, it would suffer
      // undefined behaviour with our dummy FSAVE implementation
    }
}

static void
set_fp_register (int reg, mpfr_t value, mpfr_prec_t prec, mpfr_rnd_t rnd, bool do_flags)
{
  int t;

  switch (prec)
    {
    case SINGLE_PREC:
      {
	MPFR_DECL_INIT (single, SINGLE_PREC);
	// Round to single
	set_format (SINGLE_PREC);
	t = mpfr_set (single, value, rnd);
	t = mpfr_check_range (single, t, rnd);
	mpfr_subnormalize (single, t, rnd);
	set_format (EXTENDED_PREC);
	mpfr_set (fpu.registers[reg].f, single, rnd);
      }
      break;
    case DOUBLE_PREC:
      {
	MPFR_DECL_INIT (dbl, DOUBLE_PREC);
	// Round to double
	set_format (DOUBLE_PREC);
	t = mpfr_set (dbl, value, rnd);
	t = mpfr_check_range (dbl, t, rnd);
	mpfr_subnormalize (dbl, t, rnd);
	set_format (EXTENDED_PREC);
	mpfr_set (fpu.registers[reg].f, dbl, rnd);
      }
      break;
    case EXTENDED_PREC:
      mpfr_set (fpu.registers[reg].f, value, rnd);
      break;
    }
  if (do_flags)
    {
      uae_u32 flags = 0;

      if (mpfr_zero_p (fpu.registers[reg].f))
	flags |= FPSR_CCB_ZERO;
      if (mpfr_signbit (fpu.registers[reg].f))
	flags |= FPSR_CCB_NEGATIVE;
      if (mpfr_nan_p (fpu.registers[reg].f))
	flags |= FPSR_CCB_NAN;
      if (mpfr_inf_p (fpu.registers[reg].f))
	flags |= FPSR_CCB_INFINITY;
      set_fpccr (flags);
    }
}

static uae_u32
extract_to_single (mpfr_t value)
{
  uae_u32 word;
  int t;
  mpfr_rnd_t rnd = get_cur_rnd ();
  MPFR_DECL_INIT (single, SINGLE_PREC);

  set_format (SINGLE_PREC);
  // Round to single
  t = mpfr_set (single, value, rnd);
  t = mpfr_check_range (single, t, rnd);
  mpfr_subnormalize (single, t, rnd);
  set_format (EXTENDED_PREC);

  if (mpfr_inf_p (single))
    word = 0x7f800000;
  else if (mpfr_nan_p (single))
    word = 0x7fffffff;
  else if (mpfr_zero_p (single))
    word = 0;
  else
    {
      int e;
      mpz_t f;
      mpz_init (f);
      word = 0;
      // Get exponent and mantissa
      e = mpfr_get_z_2exp (f, single);
      // Move binary point
      e += SINGLE_PREC - 1;
      // Add bias
      e += SINGLE_BIAS;
      if (e <= 0)
	{
	  // Denormalized number
	  mpz_tdiv_q_2exp (f, f, -e + 1);
	  e = 0;
	}
      mpz_export (&word, 0, 1, 4, 0, 0, f);
      // Remove integer bit
      word &= 0x7fffff;
      word |= e << 23;
      mpz_clear (f);
    }
  if (mpfr_signbit (single))
    word |= 0x80000000;
  return word;
}

static void
extract_to_double (mpfr_t value, uint32_t *words)
{
  int t;
  mpfr_rnd_t rnd = get_cur_rnd ();
  MPFR_DECL_INIT (dbl, DOUBLE_PREC);

  set_format (DOUBLE_PREC);
  // Round to double
  t = mpfr_set (dbl, value, rnd);
  t = mpfr_check_range (dbl, t, rnd);
  mpfr_subnormalize (dbl, t, rnd);
  set_format (EXTENDED_PREC);

  if (mpfr_inf_p (dbl))
    {
      words[0] = 0x7ff00000;
      words[1] = 0;
    }
  else if (mpfr_nan_p (dbl))
    {
      words[0] = 0x7fffffff;
      words[1] = 0xffffffff;
    }
  else if (mpfr_zero_p (dbl))
    {
      words[0] = 0;
      words[1] = 0;
    }
  else
    {
      int e, off = 0;
      mpz_t f;
      mpz_init (f);
      words[0] = words[1] = 0;
      // Get exponent and mantissa
      e = mpfr_get_z_2exp (f, dbl);
      // Move binary point
      e += DOUBLE_PREC - 1;
      // Add bias
      e += DOUBLE_BIAS;
      if (e <= 0)
	{
	  // Denormalized number
	  mpz_tdiv_q_2exp (f, f, -e + 1);
	  if (e <= -20)
	    // No more than 32 bits left
	    off = 1;
	  e = 0;
	}
      mpz_export (&words[off], 0, 1, 4, 0, 0, f);
      // Remove integer bit
      words[0] &= 0xfffff;
      words[0] |= e << 20;
      mpz_clear (f);
    }
  if (mpfr_signbit (dbl))
    words[0] |= 0x80000000;
}

static void
extract_to_extended (mpfr_t value, uint32_t *words)
{
  if (mpfr_inf_p (value))
    {
      words[0] = 0x7fff0000;
      words[1] = 0;
      words[2] = 0;
    }
  else if (mpfr_nan_p (value))
    {
      words[0] = 0x7fff0000;
      words[1] = 0xffffffff;
      words[2] = 0xffffffff;
    }
  else if (mpfr_zero_p (value))
    {
      words[0] = 0;
      words[1] = 0;
      words[2] = 0;
    }
  else
    {
      int e, off = 0;
      mpz_t f;

      mpz_init (f);
      words[0] = words[1] = words[2] = 0;
      // Get exponent and mantissa
      e = mpfr_get_z_2exp (f, value);
      // Move binary point
      e += EXTENDED_PREC - 1;
      // Add bias
      e += EXTENDED_BIAS;
      if (e < 0)
	{
	  // Denormalized number
	  mpz_tdiv_q_2exp (f, f, -e);
	  if (e <= -32)
	    // No more than 32 bits left
	    off = 1;
	  e = 0;
	}
      mpz_export (&words[1 + off], 0, 1, 4, 0, 0, f);
      words[0] = e << 16;
      mpz_clear (f);
    }
  if (mpfr_signbit (value))
    words[0] |= 0x80000000;
}

static void
extract_to_packed (mpfr_t value, int k, uae_u32 *words)
{
  if (mpfr_inf_p (value))
    {
      words[0] = 0x7fff0000;
      words[1] = 0;
      words[2] = 0;
    }
  else if (mpfr_nan_p (value))
    {
      words[0] = 0x7fff0000;
      words[1] = 0xffffffff;
      words[2] = 0xffffffff;
    }
  else if (mpfr_zero_p (value))
    {
      words[0] = 0;
      words[1] = 0;
      words[2] = 0;
    }
  else
    {
      char str[100], *p = str;
      mpfr_exp_t e;
      mpfr_rnd_t rnd = get_cur_rnd ();

      words[0] = words[1] = words[2] = 0;
      if (k >= 64)
	k -= 128;
      else if (k >= 18)
	cur_exceptions |= FPSR_EXCEPTION_OPERR;
      if (k <= 0)
	{
	  MPFR_DECL_INIT (temp, 16);

	  mpfr_log10 (temp, value, rnd);
	  k = mpfr_get_si (temp, MPFR_RNDZ) - k + 1;
	}
      if (k <= 0)
	k = 1;
      else if (k >= 18)
	k = 17;
      mpfr_get_str (str, &e, 10, k, value, rnd);
      e--;
      if (*p == '-')
	p++;
      // Pad to 17 digits
      while (k < 17)
	p[k++] = '0';
      if (e < 0)
	{
	  words[0] |= 0x40000000;
	  e = -e;
	}
      words[0] |= (e % 10) << 16;
      e /= 10;
      words[0] |= (e % 10) << 20;
      e /= 10;
      words[0] |= (e % 10) << 24;
      e /= 10;
      if (e)
	cur_exceptions |= FPSR_EXCEPTION_OPERR;
      words[0] |= e << 12;
      words[0] |= *p++ & 15;
      for (k = 0; k < 8; k++)
	words[1] = (words[1] << 4) | (*p++ & 15);
      for (k = 0; k < 8; k++)
	words[2] = (words[2] << 4) | (*p++ & 15);
	  
    }  
  if (mpfr_signbit (value))
    words[0] |= 0x80000000;
}

static long
extract_to_integer (mpfr_t value, long min, long max)
{
  long result;
  mpfr_rnd_t rnd = get_cur_rnd ();

  if (mpfr_fits_slong_p (value, rnd))
    {
      result = mpfr_get_si (value, rnd);
      if (result > max)
	{
	  result = max;
	  cur_exceptions |= FPSR_EXCEPTION_OPERR;
	}
      else if (result < min)
	{
	  result = min;
	  cur_exceptions |= FPSR_EXCEPTION_OPERR;
	}
    }
  else
    {
      if (!mpfr_signbit (value))
	result = max;
      else
	result = min;
      cur_exceptions |= FPSR_EXCEPTION_OPERR;
    }
  return result;
}

static bool
fpuop_fmove_memory (uae_u32 opcode, uae_u32 extra)
{
  int mode, reg, size;
  uaecptr pc;
  uae_u32 addr;
  uae_u32 words[3];
  mpfr_t *value;
  static const int sz1[8] = {4, 4, 12, 12, 2, 8, 1, 0};
  static const int sz2[8] = {4, 4, 12, 12, 2, 8, 2, 0};

  mpfr_clear_flags ();
  cur_exceptions = 0;
  mode = (opcode >> 3) & 7;
  reg = opcode & 7;
  size = (extra >> 10) & 7;
  value = &fpu.registers[(extra >> 7) & 7].f;

  switch (mode)
    {
    case 0:
      switch (size)
	{
	case 0:
	  m68k_dreg (regs, reg) = extract_to_integer (*value, -0x7fffffff-1, 0x7fffffff);
	  break;
	case 1:
	  m68k_dreg (regs, reg) = extract_to_single (*value);
	  break;
	case 4:
	  m68k_dreg (regs, reg) &= ~0xffff;
	  m68k_dreg (regs, reg) |= extract_to_integer (*value, -32768, 32767) & 0xffff;
	  break;
	case 6:
	  m68k_dreg (regs, reg) &= ~0xff;
	  m68k_dreg (regs, reg) |= extract_to_integer (*value, -128, 127) & 0xff;
	  break;
	default:
	  return false;
	}
      update_exceptions ();
      return true;
    case 1:
      return false;
    case 2:
      addr = m68k_areg (regs, reg);
      break;
    case 3:
      addr = m68k_areg (regs, reg);
      break;
    case 4:
      addr = m68k_areg (regs, reg) - (reg == 7 ? sz2[size] : sz1[size]);
      break;
    case 5:
      addr = m68k_areg (regs, reg) + (uae_s16) next_iword();
      break;
    case 6:
      addr = get_disp_ea_020 (m68k_areg (regs, reg), next_iword());
      break;
    case 7:
      switch (reg)
	{
	case 0:
	  addr = (uae_s16) next_iword();
	  break;
	case 1:
	  addr = next_ilong();
	  break;
	case 2:
	  pc = m68k_getpc ();
	  addr = pc + (uae_s16) next_iword();
	  break;
	case 3:
	  pc = m68k_getpc ();
	  addr = get_disp_ea_020 (pc, next_iword ());
	  break;
	case 4:
	  addr = m68k_getpc ();
	  m68k_incpc (sz2[size]);
	  break;
	default:
	  return false;
	}
    }

  switch (size)
    {
    case 0:
      put_long (addr, extract_to_integer (*value, -0x7fffffff-1, 0x7fffffff));
      break;
    case 1:
      put_long (addr, extract_to_single (*value));
      break;
    case 2:
      extract_to_extended (*value, words);
      put_long (addr, words[0]);
      put_long (addr + 4, words[1]);
      put_long (addr + 8, words[2]);
      break;
    case 3:
      extract_to_packed (*value, extra & 0x7f, words);
      put_long (addr, words[0]);
      put_long (addr + 4, words[1]);
      put_long (addr + 8, words[2]);
      break;
    case 4:
      put_word (addr, extract_to_integer (*value, -32768, 32767));
      break;
    case 5:
      extract_to_double (*value, words);
      put_long (addr, words[0]);
      put_long (addr + 4, words[1]);
      break;
    case 6:
      put_byte (addr, extract_to_integer (*value, -128, 127));
      break;
    case 7:
      extract_to_packed (*value, m68k_dreg (regs, (extra >> 4) & 7) & 0x7f, words);
      put_long (addr, words[0]);
      put_long (addr + 4, words[1]);
      put_long (addr + 8, words[2]);
      break;
    }

  switch (mode)
    {
    case 3:
      m68k_areg (regs, reg) += reg == 7 ? sz2[size] : sz1[size];
      break;
    case 4:
      m68k_areg (regs, reg) -= reg == 7 ? sz2[size] : sz1[size];
      break;
    }

  update_exceptions ();
  return true;
}

static bool
fpuop_fmovem_control (uae_u32 opcode, uae_u32 extra)
{
  int list, mode, reg;
  uae_u32 addr;

  list = (extra >> 10) & 7;
  mode = (opcode >> 3) & 7;
  reg = opcode & 7;

  if (list == 0)
    return false;

  if (extra & 0x2000)
    {
      // FMOVEM to <ea>
      if (mode == 0)
	{
	  switch (list)
	    {
	    case 1:
	      m68k_dreg (regs, reg) = fpu.instruction_address;
	      break;
	    case 2:
	      m68k_dreg (regs, reg) = get_fpsr ();
	      break;
	    case 4:
	      m68k_dreg (regs, reg) = get_fpcr ();
	      break;
	    default:
	      return false;
	    }
	}
      else if (mode == 1)
	{
	  if (list != 1)
	    return false;
	  m68k_areg (regs, reg) = fpu.instruction_address;
	}
      else
	{
	  int nwords;

	  if (!get_fp_addr (opcode, &addr, true))
	    return false;
	  nwords = (list & 1) + ((list >> 1) & 1) + ((list >> 2) & 1);
	  if (mode == 4)
	    addr -= nwords * 4;
	  if (list & 4)
	    {
	      put_long (addr, get_fpcr ());
	      addr += 4;
	    }
	  if (list & 2)
	    {
	      put_long (addr, get_fpsr ());
	      addr += 4;
	    }
	  if (list & 1)
	    {
	      put_long (addr, fpu.instruction_address);
	      addr += 4;
	    }
	  if (mode == 4)
	    m68k_areg (regs, reg) = addr - nwords * 4;
	  else if (mode == 3)
	    m68k_areg (regs, reg) = addr;
	}
    }
  else
    {
      // FMOVEM from <ea>

      if (mode == 0)
	{
	  switch (list)
	    {
	    case 1:
	      fpu.instruction_address = m68k_dreg (regs, reg);
	      break;
	    case 2:
	      set_fpsr (m68k_dreg (regs, reg));
	      break;
	    case 4:
	      set_fpcr (m68k_dreg (regs, reg));
	      break;
	    default:
	      return false;
	    }
	}
      else if (mode == 1)
	{
	  if (list != 1)
	    return false;
	  fpu.instruction_address = m68k_areg (regs, reg);
	}
      else if ((opcode & 077) == 074)
	{
	  switch (list)
	    {
	    case 1:
	      fpu.instruction_address = next_ilong ();
	      break;
	    case 2:
	      set_fpsr (next_ilong ());
	      break;
	    case 4:
	      set_fpcr (next_ilong ());
	      break;
	    default:
	      return false;
	    }
	}
      else
	{
	  int nwords;

	  if (!get_fp_addr (opcode, &addr, false))
	    return false;
	  nwords = (list & 1) + ((list >> 1) & 1) + ((list >> 2) & 1);
	  if (mode == 4)
	    addr -= nwords * 4;
	  if (list & 4)
	    {
	      set_fpcr (get_long (addr));
	      addr += 4;
	    }
	  if (list & 2)
	    {
	      set_fpsr (get_long (addr));
	      addr += 4;
	    }
	  if (list & 1)
	    {
	      fpu.instruction_address = get_long (addr);
	      addr += 4;
	    }
	  if (mode == 4)
	    m68k_areg (regs, reg) = addr - nwords * 4;
	  else if (mode == 3)
	    m68k_areg (regs, reg) = addr;
	}
    }
	  
  return true;
}

static bool
fpuop_fmovem_register (uae_u32 opcode, uae_u32 extra)
{
  uae_u32 addr;
  uae_u32 words[3];
  int list;
  int i;

  if (!get_fp_addr (opcode, &addr, extra & 0x2000))
    return false;
  if (extra & 0x800)
    list = m68k_dreg (regs, (extra >> 4) & 7) & 0xff;
  else
    list = extra & 0xff;

  if (extra & 0x2000)
    {
      // FMOVEM to memory

      switch (opcode & 070)
	{
	case 030:
	  return false;
	case 040:
	  if (extra & 0x1000)
	    return false;
	  for (i = 7; i >= 0; i--)
	    if (list & (1 << i))
	      {
		extract_to_extended (fpu.registers[i].f, words);
		addr -= 12;
		put_long (addr, words[0]);
		put_long (addr + 4, words[1]);
		put_long (addr + 8, words[2]);
	      }
	  m68k_areg (regs, opcode & 7) = addr;
	  break;
	default:
	  if ((extra & 0x1000) == 0)
	    return false;
	  for (i = 0; i < 8; i++)
	    if (list & (0x80 >> i))
	      {
		extract_to_extended (fpu.registers[i].f, words);
		put_long (addr, words[0]);
		put_long (addr + 4, words[1]);
		put_long (addr + 8, words[2]);
		addr += 12;
	      }
	  if ((opcode & 070) == 030)
	    m68k_areg (regs, opcode & 7) = addr;
	  break;
	}
    }
  else
    {
      // FMOVEM from memory

      if ((opcode & 070) == 040)
	return false;

      if ((extra & 0x1000) == 0)
	return false;
      for (i = 0; i < 8; i++)
	if (list & (0x80 >> i))
	  {
	    words[0] = get_long (addr);
	    words[1] = get_long (addr + 4);
	    words[2] = get_long (addr + 8);
	    addr += 12;
	    set_from_extended (fpu.registers[i].f, words);
	  }
      if ((opcode & 070) == 030)
	m68k_areg (regs, opcode & 7) = addr;
    }
  return true;
}

static void
do_getexp (mpfr_t value)
{
  if (mpfr_inf_p (value))
    {
      mpfr_set_nan (value);
      cur_exceptions |= FPSR_EXCEPTION_OPERR;
    }
  else if (!mpfr_nan_p (value) && !mpfr_zero_p (value))
    mpfr_set_si (value, mpfr_get_exp (value) - 1, MPFR_RNDN);
}

static void
do_getman (mpfr_t value)
{
  if (mpfr_inf_p (value))
    {
      mpfr_set_nan (value);
      cur_exceptions |= FPSR_EXCEPTION_OPERR;
    }
  else if (!mpfr_nan_p (value) && !mpfr_zero_p (value))
    mpfr_set_exp (value, 1);
}

static void
do_scale (mpfr_t value, mpfr_t reg, mpfr_rnd_t rnd)
{
  long scale;

  if (mpfr_nan_p (value))
    ;
  else if (mpfr_inf_p (value))
    {
      mpfr_set_nan (value);
      cur_exceptions |= FPSR_EXCEPTION_OPERR;
    }
  else if (mpfr_fits_slong_p (value, rnd))
    {
      scale = mpfr_get_si (value, MPFR_RNDZ);
      mpfr_clear_inexflag ();
      mpfr_mul_2si (value, reg, scale, rnd);
    }
  else
    mpfr_set_inf (value, -mpfr_signbit (value));
}

static void
do_remainder (mpfr_t value, mpfr_t reg, mpfr_rnd_t rnd)
{
  long quo;

  if (mpfr_nan_p (value) || mpfr_nan_p (reg))
    ;
  else if (mpfr_zero_p (value) || mpfr_inf_p (reg))
    cur_exceptions |= FPSR_EXCEPTION_OPERR;
  mpfr_remquo (value, &quo, reg, value, rnd);
  if (quo < 0)
    quo = (-quo & 0x7f) | 0x80;
  else
    quo &= 0x7f;
  fpu.fpsr.quotient = quo << 16;
}

// Unfortunately, mpfr_fmod does not return the quotient bits, so we
// have to reimplement it here
static int
mpfr_rem1 (mpfr_t rem, int *quo, mpfr_t x, mpfr_t y, mpfr_rnd_t rnd)
{
  mpfr_exp_t ex, ey;
  int inex, sign, signx = mpfr_signbit (x);
  mpz_t mx, my, r;

  mpz_init (mx);
  mpz_init (my);
  mpz_init (r);

  ex = mpfr_get_z_2exp (mx, x);  /* x = mx*2^ex */
  ey = mpfr_get_z_2exp (my, y);  /* y = my*2^ey */

  /* to get rid of sign problems, we compute it separately:
     quo(-x,-y) = quo(x,y), rem(-x,-y) = -rem(x,y)
     quo(-x,y) = -quo(x,y), rem(-x,y)  = -rem(x,y)
     thus quo = sign(x/y)*quo(|x|,|y|), rem = sign(x)*rem(|x|,|y|) */
  sign = (signx != mpfr_signbit (y));
  mpz_abs (mx, mx);
  mpz_abs (my, my);

  /* divide my by 2^k if possible to make operations mod my easier */
  {
    unsigned long k = mpz_scan1 (my, 0);
    ey += k;
    mpz_fdiv_q_2exp (my, my, k);
  }

  if (ex <= ey)
    {
      /* q = x/y = mx/(my*2^(ey-ex)) */
      mpz_mul_2exp (my, my, ey - ex);   /* divide mx by my*2^(ey-ex) */
      /* 0 <= |r| <= |my|, r has the same sign as mx */
      mpz_tdiv_qr (mx, r, mx, my);
      /* mx is the quotient */
      mpz_tdiv_r_2exp (mx, mx, 7);
      *quo = mpz_get_si (mx);
    }
  else                          /* ex > ey */
    {
      /* to get the low 7 more bits of the quotient, we first compute
	 R = X mod Y*2^7, where X and Y are defined below. Then the
	 low 7 of the quotient are floor(R/Y). */
      mpz_mul_2exp (my, my, 7);     /* 2^7*Y */

      mpz_set_ui (r, 2);
      mpz_powm_ui (r, r, ex - ey, my);  /* 2^(ex-ey) mod my */
      mpz_mul (r, r, mx);
      mpz_mod (r, r, my);

      /* now 0 <= r < 2^7*Y */
      mpz_fdiv_q_2exp (my, my, 7);   /* back to Y */
      mpz_tdiv_qr (mx, r, r, my);
      /* oldr = mx*my + newr */
      *quo = mpz_get_si (mx);

      /* now 0 <= |r| < |my| */
    }

  if (mpz_cmp_ui (r, 0) == 0)
    {
      inex = mpfr_set_ui (rem, 0, MPFR_RNDN);
      /* take into account sign of x */
      if (signx)
	mpfr_neg (rem, rem, MPFR_RNDN);
    }
  else
    {
      /* take into account sign of x */
      if (signx)
	mpz_neg (r, r);
      inex = mpfr_set_z_2exp (rem, r, ex > ey ? ey : ex, rnd);
    }

  if (sign)
    *quo |= 0x80;

  mpz_clear (mx);
  mpz_clear (my);
  mpz_clear (r);

  return inex;
}

static void
do_fmod (mpfr_t value, mpfr_t reg, mpfr_rnd_t rnd)
{
  if (mpfr_nan_p (value) || mpfr_nan_p (reg))
    mpfr_set_nan (value);
  else if (mpfr_zero_p (value) || mpfr_inf_p (reg))
    {
      mpfr_set_nan (value);
      cur_exceptions |= FPSR_EXCEPTION_OPERR;
    }
  else if (mpfr_zero_p (reg) || mpfr_inf_p (value))
    {
      fpu.fpsr.quotient = 0;
      mpfr_set (value, reg, rnd);
    }
  else
    {
      int quo;

      mpfr_rem1 (value, &quo, reg, value, rnd);
      fpu.fpsr.quotient = quo << 16;
    }
}

static void
do_fcmp (mpfr_t source, mpfr_t dest)
{
  uae_u32 flags = 0;

  if (mpfr_nan_p (source) || mpfr_nan_p (dest))
    flags |= FPSR_CCB_NAN;
  else
    {
      int cmp = mpfr_cmp (dest, source);
      if (cmp < 0)
	flags |= FPSR_CCB_NEGATIVE;
      else if (cmp == 0)
	{
	  flags |= FPSR_CCB_ZERO;
	  if ((mpfr_zero_p (dest) || mpfr_inf_p (dest)) && mpfr_signbit (dest))
	    flags |= FPSR_CCB_NEGATIVE;
	}
    }
  set_fpccr (flags);
}

static void
do_ftst (mpfr_t value)
{
  uae_u32 flags = 0;

  if (mpfr_signbit (value))
    flags |= FPSR_CCB_NEGATIVE;
  if (mpfr_nan_p (value))
    flags |= FPSR_CCB_NAN;
  else if (mpfr_zero_p (value))
    flags |= FPSR_CCB_ZERO;
  else if (mpfr_inf_p (value))
    flags |= FPSR_CCB_INFINITY;
  set_fpccr (flags);
}

static bool
fpuop_general (uae_u32 opcode, uae_u32 extra)
{
  MPFR_DECL_INIT (value, EXTENDED_PREC);
  mpfr_prec_t prec = get_cur_prec ();
  mpfr_rnd_t rnd = get_cur_rnd ();
  int reg = (extra >> 7) & 7;

  mpfr_clear_flags ();
  cur_exceptions = 0;
  cur_instruction_address = m68k_getpc () - 4;
  if ((extra & 0xfc00) == 0x5c00)
    {
      // FMOVECR
      int rom_index = extra & 0x7f;
      if (rom_index == 0 || (rom_index >= 11 && rom_index <= 15))
	mpfr_set (value, fpu_constant_rom[rom_index], rnd);
      else if (rom_index >= 48 && rom_index <= 63)
	mpfr_set (value, fpu_constant_rom[rom_index - 32], rnd);
      else
	mpfr_set_zero (value, 0);
      set_fp_register (reg, value, prec, rnd, true);
    }
  else if (extra & 0x40)
    {
      static const char valid[64] =
	{
	  1, 1, 0, 0, 1, 1, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  1, 0, 1, 0, 1, 0, 1, 0,
	  1, 0, 1, 1, 1, 0, 1, 1,
	  1, 0, 0, 0, 1, 0, 0, 0
	};

      if (!fpu.is_integral)
	return false;
      if (!valid[extra & 0x3b])
	return false;
      if (!get_fp_value (opcode, extra, value))
	return false;

      switch (extra & 0x3f)
	{
	case 0: // FSMOVE
	case 4: // FDMOVE
	  break;
	case 1: // FSSQRT
	case 5: // FDSQRT
	  if (mpfr_sgn (value) < 0)
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_sqrt (value, value, rnd);
	  break;
	case 24: // FSABS
	case 28: // FDABS
	  mpfr_abs (value, value, rnd);
	  break;
	case 26: // FSNEG
	case 30: // FDNEG
	  mpfr_neg (value, value, rnd);
	  break;
	case 32: // FSDIV
	case 36: // FDDIV
	  if (mpfr_zero_p (value))
	    {
	      if (mpfr_regular_p (fpu.registers[reg].f))
		cur_exceptions |= FPSR_EXCEPTION_DZ;
	      else if (mpfr_zero_p (fpu.registers[reg].f))
		cur_exceptions |= FPSR_EXCEPTION_OPERR;
	    }
	  else if (mpfr_inf_p (value) && mpfr_inf_p (fpu.registers[reg].f))
		cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_div (value, fpu.registers[reg].f, value, rnd);
	  break;
	case 34: // FSADD
	case 38: // FDADD
	  if (mpfr_inf_p (fpu.registers[reg].f) && mpfr_inf_p (value)
	      && mpfr_signbit (fpu.registers[reg].f) != mpfr_signbit (value))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_add (value, fpu.registers[reg].f, value, rnd);
	  break;
	case 35: // FSMUL
	case 39: // FDMUL
	  if ((mpfr_zero_p (value) && mpfr_inf_p (fpu.registers[reg].f))
	      || (mpfr_inf_p (value) && mpfr_zero_p (fpu.registers[reg].f)))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_mul (value, fpu.registers[reg].f, value, rnd);
	  break;
	case 40: // FSSUB
	case 44: // FDSUB
	  if (mpfr_inf_p (fpu.registers[reg].f) && mpfr_inf_p (value)
	      && mpfr_signbit (fpu.registers[reg].f) == mpfr_signbit (value))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_sub (value, fpu.registers[reg].f, value, rnd);
	  break;
	default:
	  return false;
	}
      if (extra & 4)
	// FD...
	prec = DOUBLE_PREC;
      else
	// FS...
	prec = SINGLE_PREC;
      set_fp_register (reg, value, prec, rnd, true);
    }
  else if ((extra & 0x30) == 0x30)
    {
      if ((extra & 15) > 10 || (extra & 15) == 9)
	return false;
      if (!get_fp_value (opcode, extra, value))
	return false;

      if ((extra & 15) < 8)
	{
	  // FSINCOS
	  int reg2 = extra & 7;
	  MPFR_DECL_INIT (value2, EXTENDED_PREC);

	  if (mpfr_inf_p (value))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_sin_cos (value, value2, value, rnd);
	  if (reg2 != reg)
	    set_fp_register (reg2, value2, prec, rnd, false);
	  set_fp_register (reg, value, prec, rnd, true);
	}
      else if ((extra & 15) == 8)
	// FCMP
	do_fcmp (value, fpu.registers[reg].f);
      else
	// FTST
	do_ftst (value);
    }
  else
    {
      static const char valid[64] =
	{
	  1, 1, 1, 1, 1, 0, 1, 0,
	  1, 1, 1, 0, 1, 1, 1, 1,
	  1, 1, 1, 0, 1, 1, 1, 0,
	  1, 1, 1, 0, 1, 1, 1, 1,
	  1, 1, 1, 1, 1, 1, 1, 1,
	  1 
	};
      if (!valid[extra & 0x3f])
	return false;
      if (!get_fp_value (opcode, extra, value))
	return false;

      switch (extra & 0x3f)
	{
	case 0: // FMOVE
	  break;
	case 1: // FINT
	  mpfr_rint (value, value, rnd);
	  break;
	case 2: // FSINH
	  mpfr_sinh (value, value, rnd);
	  break;
	case 3: // FINTRZ
	  mpfr_rint (value, value, MPFR_RNDZ);
	  break;
	case 4: // FSQRT
	  if (mpfr_sgn (value) < 0)
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_sqrt (value, value, rnd);
	  break;
	case 6: // FLOGNP1
	  if (!mpfr_nan_p (value))
	    {
	      int cmp = mpfr_cmp_si (value, -1);
	      if (cmp == 0)
		cur_exceptions |= FPSR_EXCEPTION_DZ;
	      else if (cmp < 0)
		cur_exceptions |= FPSR_EXCEPTION_OPERR;
	    }
	  mpfr_log1p (value, value, rnd);
	  break;
	case 8: // FETOXM1
	  mpfr_expm1 (value, value, rnd);
	  break;
	case 9: // FTANH
	  mpfr_tanh (value, value, rnd);
	  break;
	case 10: // FATAN
	  mpfr_atan (value, value, rnd);
	  break;
	case 12: // FASIN
	  if (mpfr_cmpabs (value, FPU_CONSTANT_ONE) > 0)
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_asin (value, value, rnd);
	  break;
	case 13: // FATANH
	  if (mpfr_cmpabs (value, FPU_CONSTANT_ONE) > 0)
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_atanh (value, value, rnd);
	  break;
	case 14: // FSIN
	  if (mpfr_inf_p (value))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_sin (value, value, rnd);
	  break;
	case 15: // FTAN
	  if (mpfr_inf_p (value))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_tan (value, value, rnd);
	  break;
	case 16: // FETOX
	  mpfr_exp (value, value, rnd);
	  break;
	case 17: // FTWOTOX
	  mpfr_ui_pow (value, 2, value, rnd);
	  break;
	case 18: // FTENTOX
	  mpfr_ui_pow (value, 10, value, rnd);
	  break;
	case 20: // FLOGN
	  if (mpfr_zero_p (value))
	    cur_exceptions |= FPSR_EXCEPTION_DZ;
	  else if (mpfr_sgn (value) < 0)
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_log (value, value, rnd);
	  break;
	case 21: // FLOG10
	  if (mpfr_zero_p (value))
	    cur_exceptions |= FPSR_EXCEPTION_DZ;
	  else if (mpfr_sgn (value) < 0)
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_log10 (value, value, rnd);
	  break;
	case 22: // FLOG2
	  if (mpfr_zero_p (value))
	    cur_exceptions |= FPSR_EXCEPTION_DZ;
	  else if (mpfr_sgn (value) < 0)
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_log2 (value, value, rnd);
	  break;
	case 24: // FABS
	  mpfr_abs (value, value, rnd);
	  break;
	case 25: // FCOSH
	  mpfr_cosh (value, value, rnd);
	  break;
	case 26: // FNEG
	  mpfr_neg (value, value, rnd);
	  break;
	case 28: // FACOS
	  if (mpfr_cmpabs (value, FPU_CONSTANT_ONE) > 0)
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_acos (value, value, rnd);
	  break;
	case 29: // FCOS
	  if (mpfr_inf_p (value))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_cos (value, value, rnd);
	  break;
	case 30: // FGETEXP
	  do_getexp (value);
	  break;
	case 31: // FGETMAN
	  do_getman (value);
	  break;
	case 32: // FDIV
	  if (mpfr_zero_p (value))
	    {
	      if (mpfr_regular_p (fpu.registers[reg].f))
		cur_exceptions |= FPSR_EXCEPTION_DZ;
	      else if (mpfr_zero_p (fpu.registers[reg].f))
		cur_exceptions |= FPSR_EXCEPTION_OPERR;
	    }
	  else if (mpfr_inf_p (value) && mpfr_inf_p (fpu.registers[reg].f))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_div (value, fpu.registers[reg].f, value, rnd);
	  break;
	case 33: // FMOD
	  do_fmod (value, fpu.registers[reg].f, rnd);
	  break;
	case 34: // FADD
	  if (mpfr_inf_p (fpu.registers[reg].f) && mpfr_inf_p (value)
	      && mpfr_signbit (fpu.registers[reg].f) != mpfr_signbit (value))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_add (value, fpu.registers[reg].f, value, rnd);
	  break;
	case 35: // FMUL
	  if ((mpfr_zero_p (value) && mpfr_inf_p (fpu.registers[reg].f))
	      || (mpfr_inf_p (value) && mpfr_zero_p (fpu.registers[reg].f)))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_mul (value, fpu.registers[reg].f, value, rnd);
	  break;
	case 36: // FSGLDIV
	  if (mpfr_zero_p (value))
	    {
	      if (mpfr_regular_p (fpu.registers[reg].f))
		cur_exceptions |= FPSR_EXCEPTION_DZ;
	      else if (mpfr_zero_p (fpu.registers[reg].f))
		cur_exceptions |= FPSR_EXCEPTION_OPERR;
	    }
	  else if (mpfr_inf_p (value) && mpfr_inf_p (fpu.registers[reg].f))
		cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_div (value, fpu.registers[reg].f, value, rnd);
	  prec = SINGLE_PREC;
	  break;
	case 37: // FREM
	  do_remainder (value, fpu.registers[reg].f, rnd);
	  break;
	case 38: // FSCALE
	  do_scale (value, fpu.registers[reg].f, rnd);
	  break;
	case 39: // FSGLMUL
	  if ((mpfr_zero_p (value) && mpfr_inf_p (fpu.registers[reg].f))
	      || (mpfr_inf_p (value) && mpfr_zero_p (fpu.registers[reg].f)))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_mul (value, fpu.registers[reg].f, value, rnd);
	  prec = SINGLE_PREC;
	  break;
	case 40: // FSUB
	  if (mpfr_inf_p (fpu.registers[reg].f) && mpfr_inf_p (value)
	      && mpfr_signbit (fpu.registers[reg].f) == mpfr_signbit (value))
	    cur_exceptions |= FPSR_EXCEPTION_OPERR;
	  mpfr_sub (value, fpu.registers[reg].f, value, rnd);
	  break;
	}
      set_fp_register (reg, value, prec, rnd, true);
    }
  update_exceptions ();
  return true;
}

void
fpuop_arithmetic (uae_u32 opcode, uae_u32 extra)
{
  bool valid;

  switch ((extra >> 13) & 7)
    {
    case 3:
      valid = fpuop_fmove_memory (opcode, extra);
      break;
    case 4:
    case 5:
      valid = fpuop_fmovem_control (opcode, extra);
      break;
    case 6:
    case 7:
      valid = fpuop_fmovem_register (opcode, extra);
      break;
    case 0:
    case 2:
      valid = fpuop_general (opcode, extra);
      break;
    default:
      valid = false;
      break;
    }

  if (!valid)
    {
      m68k_setpc (m68k_getpc () - 4);
      op_illg (opcode);
    }
}

static bool
check_fp_cond (uae_u32 pred)
{
  uae_u32 fpcc = get_fpccr ();

  if ((pred & 16) != 0 && (fpcc & FPSR_CCB_NAN) != 0)
    {
      // IEEE non-aware test
      set_exception_status (get_exception_status () | FPSR_EXCEPTION_BSUN);
      set_accrued_exception (get_accrued_exception () | FPSR_ACCR_IOP);
    }

  switch (pred & 15)
    {
    case 0: // F / SF
      return false;
    case 1: // EQ /SEQ
      return (fpcc & FPSR_CCB_ZERO) != 0;
    case 2: // OGT / GT
      return (fpcc & (FPSR_CCB_NAN | FPSR_CCB_ZERO | FPSR_CCB_NEGATIVE)) == 0;
    case 3: // OGE / GE
      return (fpcc & FPSR_CCB_ZERO) != 0 || (fpcc & (FPSR_CCB_NAN | FPSR_CCB_NEGATIVE)) == 0;
    case 4: // OLT / LT
      return (fpcc & (FPSR_CCB_NEGATIVE | FPSR_CCB_NAN | FPSR_CCB_ZERO)) == FPSR_CCB_NEGATIVE;
    case 5: // OLE / LE
      return (fpcc & FPSR_CCB_ZERO) != 0 || (fpcc & (FPSR_CCB_NEGATIVE | FPSR_CCB_NAN)) == FPSR_CCB_NEGATIVE;
    case 6: // OGL / GL
      return (fpcc & (FPSR_CCB_NAN | FPSR_CCB_ZERO)) == 0;
    case 7: // OR / GLE
      return (fpcc & FPSR_CCB_NAN) == 0;
    case 8: // UN / NGLE
      return (fpcc & FPSR_CCB_NAN) != 0;
    case 9: // UEQ / NGL
      return (fpcc & (FPSR_CCB_NAN | FPSR_CCB_ZERO)) != 0;
    case 10: // UGT / NLE
      return (fpcc & FPSR_CCB_NAN) != 0 || (fpcc & (FPSR_CCB_NEGATIVE | FPSR_CCB_ZERO)) == 0;
    case 11: // UGE / NLT
      return (fpcc & (FPSR_CCB_NEGATIVE | FPSR_CCB_NAN | FPSR_CCB_ZERO)) != FPSR_CCB_NEGATIVE;
    case 12: // ULT / NGE
      return (fpcc & FPSR_CCB_NAN) != 0 || (fpcc & (FPSR_CCB_NEGATIVE | FPSR_CCB_ZERO)) == FPSR_CCB_NEGATIVE;
    case 13: // ULE / NGT
      return (fpcc & (FPSR_CCB_NAN | FPSR_CCB_ZERO | FPSR_CCB_NEGATIVE)) != 0;
    case 14: // NE / SNE
      return (fpcc & FPSR_CCB_ZERO) == 0;
    case 15: // T / ST
      return true;
    default:
      return false;
    }
}

void
fpuop_bcc (uae_u32 opcode, uaecptr pc, uae_u32 disp)
{
  if (check_fp_cond (opcode))
    m68k_setpc (pc + (uae_s16) disp);
}

void
fpuop_scc (uae_u32 opcode, uae_u32 extra)
{
  uae_u32 addr;
  int value = check_fp_cond (extra) ? 0xff : 0;
  if ((opcode & 070) == 0)
    {
      int reg = opcode & 7;
      m68k_dreg (regs, reg) = (m68k_dreg (regs, reg) & ~0xff) | value;
    }
  else if (!get_fp_addr (opcode, &addr, true))
    {
      m68k_setpc (m68k_getpc () - 4);
      op_illg (opcode);
    }
  else
    {
      switch (opcode & 070)
	{
	case 030:
	  m68k_areg (regs, opcode & 7) += (opcode & 7) == 7 ? 2 : 1;
	  break;
	case 040:
	  addr -= (opcode & 7) == 7 ? 2 : 1;
	  m68k_areg (regs, opcode & 7) = addr;
	}
      put_byte (addr, value);
    }
}

void
fpuop_dbcc (uae_u32 opcode, uae_u32 extra)
{
  uaecptr pc = m68k_getpc ();
  uae_s16 disp = next_iword ();

  if (!check_fp_cond (extra))
    {
      int reg = opcode & 7;
      uae_u16 cnt = (m68k_dreg (regs, reg) & 0xffff) - 1;
      m68k_dreg (regs, reg) = (m68k_dreg (regs, reg) & ~0xffff) | cnt;
      if (cnt != 0xffff)
	m68k_setpc (pc + disp);
    }
}

void
fpuop_trapcc (uae_u32, uaecptr oldpc, uae_u32 extra)
{
  if (check_fp_cond (extra))
    Exception (7, oldpc - 2);
}

void
fpuop_save (uae_u32 opcode)
{
  uae_u32 addr;

  if ((opcode & 070) == 030
      || !get_fp_addr (opcode, &addr, true))
    {
      m68k_setpc (m68k_getpc () - 2);
      op_illg (opcode);
      return;
    }

  if (fpu.is_integral)
    {
      // 4 byte 68040 IDLE frame
      // FIXME: generate proper FPU stack frames that does not result
      // in undefined behaviour from FPSP040
      if ((opcode & 070) == 040)
	{
	  addr -= 4;
	  m68k_areg (regs, opcode & 7) = addr;
	}
      put_long (addr, 0x41000000);
    }
  else
    {
      // 28 byte 68881 IDLE frame
      if ((opcode & 070) == 040)
	{
	  addr -= 28;
	  m68k_areg (regs, opcode & 7) = addr;
	}
      put_long (addr, 0x1f180000);
      for (int i = 0; i < 6; i++)
	{
	  addr += 4;
	  put_long (addr, 0);
	}
    }
}

void
fpuop_restore (uae_u32 opcode)
{
  uae_u32 addr;
  uae_u32 format;

  if ((opcode & 070) == 040
      || !get_fp_addr (opcode, &addr, false))
    {
      m68k_setpc (m68k_getpc () - 2);
      op_illg (opcode);
      return;
    }

  format = get_long (addr);
  addr += 4;
  if ((format & 0xff000000) == 0)
    // NULL frame
    fpu_reset ();
  else
    addr += (format & 0xff0000) >> 16;
  if ((opcode & 070) == 030)
    m68k_areg (regs, opcode & 7) = addr;
}
