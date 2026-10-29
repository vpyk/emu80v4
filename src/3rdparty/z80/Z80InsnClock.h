/* Z80InsnClock API _________________________________________________________
|                                                                            |
|  ________ ________ _________                                               |
| |____   /|   __   |   __   |\                                              |
|  \__/  / |  |\_|  |  |\_|  | |                                             |
|    /  / /|   __   |  | ||  | |                                             |
|   /  /_/_|  |\_|  |  |_||  | | Instruction Clock                           |
|  /_______|________|________| | Copyright (C) 2021-2025 Sofía Ortega Sosa.  |
|  \________\________\________\|                                             |
|                                                                            |
|  Permission to use, copy, modify, and/or distribute this software for any  |
|  purpose with or without fee is hereby granted.                            |
|                                                                            |
|  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES  |
|  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF          |
|  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR   |
|  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    |
|  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN     |
|  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF   |
|  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.            |
|                                                                            |
'===========================================================================*/

#ifndef Z80InsnClock_H
#define Z80InsnClock_H

#ifdef Z80_INSN_CLOCK_EXTERNAL_HEADER
#	include Z80_EXTERNAL_HEADER
#else
#	include <Z/macros/language.h>
#	include <Z/types/bitwise.h>
#endif

#ifndef Z80_INSN_CLOCK_API
#	if defined(Z80_INSN_CLOCK_STATIC) || defined(__DOXYGEN__)
#		define Z80_INSN_CLOCK_API
#	else
#		define Z80_INSN_CLOCK_API Z_API_IMPORT
#	endif
#endif

/** @brief Major version number of the Z80InsnClock library. */

#define Z80_INSN_CLOCK_LIBRARY_VERSION_MAJOR 0

/** @brief Minor version number of the Z80InsnClock library. */

#define Z80_INSN_CLOCK_LIBRARY_VERSION_MINOR 1

/** @brief Micro version number of the Z80InsnClock library. */

#define Z80_INSN_CLOCK_LIBRARY_VERSION_MICRO 0

/** @brief String literal with the version number of the Z80InsnClock library. */

#define Z80_INSN_CLOCK_LIBRARY_VERSION_STRING "0.1"

typedef zuint8 (* Z80InsnClockRead)(void *context, zuint16 address);

typedef struct {
	zuint32 pattern;
	zuint8 cycles;
	zuint8 prefix;
	void *context;
	Z80InsnClockRead read;
	ZInt16 *af;
	ZInt16 *bc;
	ZInt16 *hl;
} Z80InsnClock;

#define Z80_INSN_CLOCK_PREFIX_NONE 0
#define Z80_INSN_CLOCK_PREFIX_CB 1
#define Z80_INSN_CLOCK_PREFIX_XY 2
#define Z80_INSN_CLOCK_PREFIX_ED 3

#define Z80_INSN_CLOCK_RESUME_HALT   1
#define Z80_INSN_CLOCK_RESUME_XY     2
#define Z80_INSN_CLOCK_RESUME_IM0_XY 3

Z_EXTERN_C_BEGIN

Z80_INSN_CLOCK_API extern zuint32 const z80_insn_clock_table[4][256];
Z80_INSN_CLOCK_API extern zuint32 const z80_insn_clock_extra_table[4][256];

static Z_ALWAYS_INLINE
void z80_insn_clock_initialize(
	Z80InsnClock*    self,
	ZInt16*          af,
	ZInt16*          bc,
	ZInt16*          hl,
	void*            context,
	Z80InsnClockRead read
)
	{
	self->af      = af;
	self->bc      = bc;
	self->hl      = hl;
	self->context = context;
	self->read    = read;
	}


static Z_ALWAYS_INLINE
void z80_insn_clock_start(Z80InsnClock *self, zuint8 resume)
	{
	self->pattern = self->cycles = 0;
	self->prefix = resume & 2;
	}


static Z_ALWAYS_INLINE
zuint8 z80_insn_clock_m1(Z80InsnClock *self, zuint8 opcode)
	{
	zuint32 p = z80_insn_clock_table[self->prefix][opcode];

	self->prefix = p & 3;
	self->pattern = p >> 4;
	return (self->cycles = p >> 28);
	}


static Z_ALWAYS_INLINE
zuint8 z80_insn_clock_inta(Z80InsnClock *self, zuint8 im, zuint8 ird)
	{
	zuint32 p;

	if (im)	{
		self->pattern = Z_UINT32(0x333370);
		return (self->cycles = self->prefix = 0);
		}

	p = z80_insn_clock_table[self->prefix][ird];
	self->prefix = p & 3;
	self->pattern = (p >> 4) + 2;
	return (self->cycles = (p >> 28) ? 6 : 0);
	}


static Z_ALWAYS_INLINE
void z80_insn_clock_nmia(Z80InsnClock *self)
	{
	self->pattern = 0x3350;
	self->cycles = self->prefix = 0;
	}


static Z_ALWAYS_INLINE
zuint8 z80_insn_clock_m(Z80InsnClock *self)
	{return (self->cycles += (self->pattern >>= 4) & 0xF);}


Z80_INSN_CLOCK_API
zuint8 z80_insn_clock_extra_m1(Z80InsnClock *self, zuint8 opcode);


static Z_ALWAYS_INLINE
zuint8 z80_insn_clock_extra_inta(Z80InsnClock *self, zuint8 im, zuint8 ird)
	{
	zuint8 extra_cycles;

	if (im)	{
		self->pattern = self->cycles = self->prefix = 0;
		return 1;
		}

	extra_cycles = z80_insn_clock_extra_m1(self, ird);
	if (self->cycles) self->cycles += 2;
	return extra_cycles;
	}


static Z_ALWAYS_INLINE
void z80_insn_clock_extra_nmia_fast(Z80InsnClock *self)
	{
	self->pattern = self->prefix = 0;
	self->cycles = 5;
	}


static Z_ALWAYS_INLINE
void z80_insn_clock_extra_nmia(Z80InsnClock *self)
	{self->pattern = self->cycles = self->prefix = 0;}


static Z_ALWAYS_INLINE
zuint8 z80_insn_clock_extra_m(Z80InsnClock *self)
	{return (self->pattern >>= 4) & 0xF;}


static Z_ALWAYS_INLINE
zuint8 z80_insn_clock_extra_out(Z80InsnClock *self)
	{return (self->pattern >> 4) & 0xF;}


static Z_ALWAYS_INLINE
void z80_insn_clock_extra_add_m1(Z80InsnClock *self, zuint8 extra_cycles)
	{self->cycles += 4 + extra_cycles;}


static Z_ALWAYS_INLINE
void z80_insn_clock_extra_add_inta(Z80InsnClock *self, zuint8 extra_cycles)
	{self->cycles += 4 + 2 + extra_cycles;}


static Z_ALWAYS_INLINE
void z80_insn_clock_extra_add_nmia(Z80InsnClock *self)
	{self->cycles += 5;}


static Z_ALWAYS_INLINE
void z80_insn_clock_extra_add_m(Z80InsnClock *self, zuint8 extra_cycles)
	{self->cycles += 3 + extra_cycles;}


static Z_ALWAYS_INLINE
void z80_insn_clock_extra_add_in(Z80InsnClock *self)
	{self->cycles += 4;}


Z_EXTERN_C_END

#endif /* Z80InsnClock_H */
