/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2024
 *
 *  Based on original code by:
 *  Frank D. Cringle (YAZE project, GPL v.2 licence)
 *  Juergen Buchmueller (MESS project, BSD-3-clause licence)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Globals.h"
#include "CpuZ80.h"
#include "CpuHook.h"
#include "CpuWaits.h"
#include "Emulation.h"
#include "PlatformCore.h"

using namespace std;

#define GetBYTE(addr) (as_input(addr))
#define GetWORD(addr) ((GetBYTE((addr+1) & 0xFFFF) << 8) | GetBYTE(addr))

#define PutBYTE(addr, value) as_output(addr, value)
#define PutWORD(addr, value) PutBYTE(addr, value & 0xff);PutBYTE((addr + 1) & 0xFFFF, (value >> 8) & 0xff);

#define PC pc
#define AF af[af_sel]
#define BC regs[regs_sel].bc
#define DE regs[regs_sel].de
#define HL regs[regs_sel].hl
#define SP sp

#define FLAG_C  1
#define FLAG_N  2
#define FLAG_P  4
#define FLAG_H  16
#define FLAG_Z  64
#define FLAG_S  128

#define SETFLAG(f,c)    AF = (c) ? AF | FLAG_ ## f : AF & ~FLAG_ ## f
#define TSTFLAG(f)  ((AF & FLAG_ ## f) != 0)

#define ldig(x)     ((x) & 0xf)
#define hdig(x)     (((x)>>4)&0xf)
#define lreg(x)     ((x)&0xff)
#define hreg(x)     (((x)>>8)&0xff)

#define Setlreg(x, v)   x = (((x)&0xff00) | ((v)&0xff))
#define Sethreg(x, v)   x = (((x)&0xff) | (((v)&0xff) << 8))

static const unsigned char partab[256] = {
    4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
    0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
    0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
    4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
    0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
    4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
    4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
    0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
    0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
    4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
    4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
    0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
    4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
    0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
    0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
    4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
};

#define parity(x)   partab[(x)&0xff]

#define POP(x)  do {                                \
    m_stackOperation = true;                        \
    unsigned y = GetBYTE(SP); SP++;                 \
    x = y + (GetBYTE(SP) << 8); SP++;               \
    m_stackOperation = false;                       \
} while (0)

#define PUSH(x) do {                                \
    m_stackOperation = true;                        \
    --SP; PutBYTE(SP, (x) >> 8);                    \
    --SP; PutBYTE(SP, x);                           \
    m_stackOperation = false;                       \
} while (0)

#define JPC(cond) {                                 \
    if (cond) {                                     \
      PC = GetWORD(PC);                             \
      cycles += cc_ex[op];                          \
    } else {                                        \
      PC += 2;                                      \
    }                                               \
}

#define CALLC(cond) {                               \
    if (cond) {                                     \
      unsigned adrr = GetWORD(PC);                  \
      PUSH(PC+2);                                   \
      PC = adrr;                                    \
      cycles += cc_ex[op];                          \
    } else {                                        \
      PC += 2;                                      \
    }                                               \
}

static const uint8_t cc_op[0x100] = {
    4,10, 7, 6, 4, 4, 7, 4, 4,11, 7, 6, 4, 4, 7, 4,
    8,10, 7, 6, 4, 4, 7, 4,12,11, 7, 6, 4, 4, 7, 4,
    7,10,16, 6, 4, 4, 7, 4, 7,11,16, 6, 4, 4, 7, 4,
    7,10,13, 6,11,11,10, 4, 7,11,13, 6, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    7, 7, 7, 7, 7, 7, 4, 7, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    5,10,10,10,10,11, 7,11, 5,10,10, 0,10,17, 7,11, /* cb -> cc_cb */
    5,10,10,11,10,11, 7,11, 5, 4,10,11,10, 0, 7,11, /* dd -> cc_xy */
    5,10,10,19,10,11, 7,11, 5, 4,10, 4,10, 0, 7,11, /* ed -> cc_ed */
    5,10,10, 4,10,11, 7,11, 5, 6,10, 4,10, 0, 7,11  /* fd -> cc_xy */
};

static const uint8_t cc_cb[0x100] = {
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
    8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
    8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
    8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
    8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
    8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8
};

static const uint8_t cc_ed[0x100] = {
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    12,12,15,20,8,14, 8, 9,12,12,15,20, 8,14, 8, 9,
    12,12,15,20,8,14, 8, 9,12,12,15,20, 8,14, 8, 9,
    12,12,15,20,8,14, 8,18,12,12,15,20, 8,14, 8,18,
    12,12,15,20,8,14, 8, 8,12,12,15,20, 8,14, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    16,16,16,16,8, 8, 8, 8,16,16,16,16, 8, 8, 8, 8,
    16,16,16,16,8, 8, 8, 8,16,16,16,16, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

static const uint8_t cc_xy[0x100] = {
    4+4,10+4, 7+4, 6+4, 4+4, 4+4, 7+4, 4+4, 4+4,11+4, 7+4, 6+4, 4+4, 4+4, 7+4, 4+4,
    8+4,10+4, 7+4, 6+4, 4+4, 4+4, 7+4, 4+4,12+4,11+4, 7+4, 6+4, 4+4, 4+4, 7+4, 4+4,
    7+4,10+4,16+4, 6+4, 4+4, 4+4, 7+4, 4+4, 7+4,11+4,16+4, 6+4, 4+4, 4+4, 7+4, 4+4,
    7+4,10+4,13+4, 6+4,  23,  23,  19, 4+4, 7+4,11+4,13+4, 6+4, 4+4, 4+4, 7+4, 4+4,
    4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4,
    4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4,
    4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4,
     19,  19,  19,  19,  19,  19, 4+4,  19, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4,
    4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4,
    4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4,
    4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4,
    4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4, 4+4,  19, 4+4,
    5+4,10+4,10+4,10+4,10+4,11+4, 7+4,11+4, 5+4,10+4,10+4,   0,10+4,17+4, 7+4,11+4, /* cb -> cc_xycb */
    5+4,10+4,10+4,11+4,10+4,11+4, 7+4,11+4, 5+4, 4+4,10+4,11+4,10+4,   4, 7+4,11+4, /* dd -> cc_xy again */
    5+4,10+4,10+4,19+4,10+4,11+4, 7+4,11+4, 5+4, 4+4,10+4, 4+4,10+4,   4, 7+4,11+4, /* ed -> cc_ed */
    5+4,10+4,10+4, 4+4,10+4,11+4, 7+4,11+4, 5+4, 6+4,10+4, 4+4,10+4,   4, 7+4,11+4  /* fd -> cc_xy again */
};

/* extra cycles if jr/jp/call taken and 'interrupt latency' on rst 0-7 */
static const uint8_t cc_ex[0x100] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* DJNZ */
    5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, /* JR NZ/JR Z */
    5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, /* JR NC/JR C */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5, 0, 0, 0, 0, /* LDIR/CPIR/INIR/OTIR LDDR/CPDR/INDR/OTDR */
    6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2,
    6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2,
    6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2,
    6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2
};

unsigned CpuZ80::cb_prefix(unsigned adr)
{
    unsigned temp = 0, acu, op, cbits;

        op = GetBYTE(PC);
        unsigned cycles = cc_cb[op];

        switch (op & 7) {
        case 0: ++PC; acu = hreg(BC); break;
        case 1: ++PC; acu = lreg(BC); break;
        case 2: ++PC; acu = hreg(DE); break;
        case 3: ++PC; acu = lreg(DE); break;
        case 4: ++PC; acu = hreg(HL); break;
        case 5: ++PC; acu = lreg(HL); break;
        case 6: ++PC; acu = GetBYTE(adr);  break;
        case 7: ++PC; acu = hreg(AF); break;
        }
        switch (op & 0xc0) {
        case 0x00:      /* shift/rotate */
            switch (op & 0x38) {
            case 0x00:  /* RLC */
                temp = (acu << 1) | (acu >> 7);
                cbits = temp & 1;
                goto cbshflg1;
            case 0x08:  /* RRC */
                temp = (acu >> 1) | (acu << 7);
                cbits = temp & 0x80;
                goto cbshflg1;
            case 0x10:  /* RL */
                temp = (acu << 1) | TSTFLAG(C);
                cbits = acu & 0x80;
                goto cbshflg1;
            case 0x18:  /* RR */
                temp = (acu >> 1) | (TSTFLAG(C) << 7);
                cbits = acu & 1;
                goto cbshflg1;
            case 0x20:  /* SLA */
                temp = acu << 1;
                cbits = acu & 0x80;
                goto cbshflg1;
            case 0x28:  /* SRA */
                temp = (acu >> 1) | (acu & 0x80);
                cbits = acu & 1;
                goto cbshflg1;
            case 0x30:  /* SLIA */
                temp = (acu << 1) | 1;
                cbits = acu & 0x80;
                goto cbshflg1;
            case 0x38:  /* SRL */
                temp = acu >> 1;
                cbits = acu & 1;
            cbshflg1:
                AF = (AF & ~0xff) | (temp & 0xa8) |
                    (((temp & 0xff) == 0) << 6) |
                    parity(temp) | !!cbits;
            }
            break;
        case 0x40:      /* BIT */
            if (acu & (1 << ((op >> 3) & 7)))
                AF = (AF & ~0xfe) | 0x10 |
                (((op & 0x38) == 0x38) << 7);
            else
                AF = (AF & ~0xfe) | 0x54;
            if ((op&7) != 6)
                AF |= (acu & 0x28);
            temp = acu;
            break;
        case 0x80:      /* RES */
            temp = acu & ~(1 << ((op >> 3) & 7));
            break;
        case 0xc0:      /* SET */
            temp = acu | (1 << ((op >> 3) & 7));
            break;
        }
        switch (op & 7) {
            case 0: Sethreg(BC, temp); break;
            case 1: Setlreg(BC, temp); break;
            case 2: Sethreg(DE, temp); break;
            case 3: Setlreg(DE, temp); break;
            case 4: Sethreg(HL, temp); break;
            case 5: Setlreg(HL, temp); break;
            case 6: PutBYTE(adr, temp);break;
            case 7: Sethreg(AF, temp); break;
        }
    return cycles;
}

unsigned CpuZ80::dfd_prefix(uint16_t& IXY)
{
    unsigned temp, adr, acu, op, sum, cbits;

        op = GetBYTE(PC);
        ++PC;
        unsigned cycles = cc_xy[op];

        switch (op) {
        case 0x09:          /* ADD IXY,BC */
            IXY &= 0xffff;
            BC &= 0xffff;
            sum = IXY + BC;
            cbits = (IXY ^ BC ^ sum) >> 8;
            IXY = sum;
            AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) |
                (cbits & 0x10) | ((cbits >> 8) & 1);
            break;
        case 0x19:          /* ADD IXY,DE */
            IXY &= 0xffff;
            DE &= 0xffff;
            sum = IXY + DE;
            cbits = (IXY ^ DE ^ sum) >> 8;
            IXY = sum;
            AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) |
                (cbits & 0x10) | ((cbits >> 8) & 1);
            break;
        case 0x21:          /* LD IXY,nnnn */
            IXY = GetWORD(PC);
            PC += 2;
            break;
        case 0x22:          /* LD (nnnn),IXY */
            temp = GetWORD(PC);
            PutWORD(temp, IXY);
            PC += 2;
            break;
        case 0x23:          /* INC IXY */
            ++IXY;
            break;
        case 0x24:          /* INC IXYH */
            IXY += 0x100;
            temp = hreg(IXY);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                (((temp & 0xf) == 0) << 4) |
                ((temp == 0x80) << 2);
            break;
        case 0x25:          /* DEC IXYH */
            IXY -= 0x100;
            temp = hreg(IXY);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                (((temp & 0xf) == 0xf) << 4) |
                ((temp == 0x7f) << 2) | 2;
            break;
        case 0x26:          /* LD IXYH,nn */
            Sethreg(IXY, GetBYTE(PC)); ++PC;
            break;
        case 0x29:          /* ADD IXY,IXY */
            IXY &= 0xffff;
            sum = IXY + IXY;
            cbits = (IXY ^ IXY ^ sum) >> 8;
            IXY = sum;
            AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) |
                (cbits & 0x10) | ((cbits >> 8) & 1);
            break;
        case 0x2A:          /* LD IXY,(nnnn) */
            temp = GetWORD(PC);
            IXY = GetWORD(temp);
            PC += 2;
            break;
        case 0x2B:          /* DEC IXY */
            --IXY;
            break;
        case 0x2C:          /* INC IXYL */
            temp = lreg(IXY)+1;
            Setlreg(IXY, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                (((temp & 0xf) == 0) << 4) |
                ((temp == 0x80) << 2);
            break;
        case 0x2D:          /* DEC IXYL */
            temp = lreg(IXY)-1;
            Setlreg(IXY, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                (((temp & 0xf) == 0xf) << 4) |
                ((temp == 0x7f) << 2) | 2;
            break;
        case 0x2E:          /* LD IXYL,nn */
            Setlreg(IXY, GetBYTE(PC)); ++PC;
            break;
        case 0x34:          /* INC (IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            temp = GetBYTE(adr)+1;
            PutBYTE(adr, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                (((temp & 0xf) == 0) << 4) |
                ((temp == 0x80) << 2);
            break;
        case 0x35:          /* DEC (IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            temp = GetBYTE(adr)-1;
            PutBYTE(adr, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                (((temp & 0xf) == 0xf) << 4) |
                ((temp == 0x7f) << 2) | 2;
            break;
        case 0x36:          /* LD (IXY+dd),nn */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            PutBYTE(adr, GetBYTE(PC)); ++PC;
            break;
        case 0x39:          /* ADD IXY,SP */
            IXY &= 0xffff;
            SP &= 0xffff;
            sum = IXY + SP;
            cbits = (IXY ^ SP ^ sum) >> 8;
            IXY = sum;
            AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) |
                (cbits & 0x10) | ((cbits >> 8) & 1);
            break;
        case 0x44:          /* LD B,IXYH */
            Sethreg(BC, hreg(IXY));
            break;
        case 0x45:          /* LD B,IXYL */
            Sethreg(BC, lreg(IXY));
            break;
        case 0x46:          /* LD B,(IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            Sethreg(BC, GetBYTE(adr));
            break;
        case 0x4C:          /* LD C,IXYH */
            Setlreg(BC, hreg(IXY));
            break;
        case 0x4D:          /* LD C,IXYL */
            Setlreg(BC, lreg(IXY));
            break;
        case 0x4E:          /* LD C,(IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            Setlreg(BC, GetBYTE(adr));
            break;
        case 0x54:          /* LD D,IXYH */
            Sethreg(DE, hreg(IXY));
            break;
        case 0x55:          /* LD D,IXYL */
            Sethreg(DE, lreg(IXY));
            break;
        case 0x56:          /* LD D,(IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            Sethreg(DE, GetBYTE(adr));
            break;
        case 0x5C:          /* LD E,H */
            Setlreg(DE, hreg(IXY));
            break;
        case 0x5D:          /* LD E,L */
            Setlreg(DE, lreg(IXY));
            break;
        case 0x5E:          /* LD E,(IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            Setlreg(DE, GetBYTE(adr));
            break;
        case 0x60:          /* LD IXYH,B */
            Sethreg(IXY, hreg(BC));
            break;
        case 0x61:          /* LD IXYH,C */
            Sethreg(IXY, lreg(BC));
            break;
        case 0x62:          /* LD IXYH,D */
            Sethreg(IXY, hreg(DE));
            break;
        case 0x63:          /* LD IXYH,E */
            Sethreg(IXY, lreg(DE));
            break;
        case 0x64:          /* LD IXYH,IXYH */
            /* nop */
            break;
        case 0x65:          /* LD IXYH,IXYL */
            Sethreg(IXY, lreg(IXY));
            break;
        case 0x66:          /* LD H,(IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            Sethreg(HL, GetBYTE(adr));
            break;
        case 0x67:          /* LD IXYH,A */
            Sethreg(IXY, hreg(AF));
            break;
        case 0x68:          /* LD IXYL,B */
            Setlreg(IXY, hreg(BC));
            break;
        case 0x69:          /* LD IXYL,C */
            Setlreg(IXY, lreg(BC));
            break;
        case 0x6A:          /* LD IXYL,D */
            Setlreg(IXY, hreg(DE));
            break;
        case 0x6B:          /* LD IXYL,E */
            Setlreg(IXY, lreg(DE));
            break;
        case 0x6C:          /* LD IXYL,IXYH */
            Setlreg(IXY, hreg(IXY));
            break;
        case 0x6D:          /* LD IXYL,IXYL */
            /* nop */
            break;
        case 0x6E:          /* LD L,(IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            Setlreg(HL, GetBYTE(adr));
            break;
        case 0x6F:          /* LD IXYL,A */
            Setlreg(IXY, hreg(AF));
            break;
        case 0x70:          /* LD (IXY+dd),B */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            PutBYTE(adr, hreg(BC));
            break;
        case 0x71:          /* LD (IXY+dd),C */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            PutBYTE(adr, lreg(BC));
            break;
        case 0x72:          /* LD (IXY+dd),D */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            PutBYTE(adr, hreg(DE));
            break;
        case 0x73:          /* LD (IXY+dd),E */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            PutBYTE(adr, lreg(DE));
            break;
        case 0x74:          /* LD (IXY+dd),H */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            PutBYTE(adr, hreg(HL));
            break;
        case 0x75:          /* LD (IXY+dd),L */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            PutBYTE(adr, lreg(HL));
            break;
        case 0x77:          /* LD (IXY+dd),A */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            PutBYTE(adr, hreg(AF));
            break;
        case 0x7C:          /* LD A,IXYH */
            Sethreg(AF, hreg(IXY));
            break;
        case 0x7D:          /* LD A,IXYL */
            Sethreg(AF, lreg(IXY));
            break;
        case 0x7E:          /* LD A,(IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            Sethreg(AF, GetBYTE(adr));
            break;
        case 0x84:          /* ADD A,IXYH */
            temp = hreg(IXY);
            acu = hreg(AF);
            sum = acu + temp;
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                ((cbits >> 8) & 1);
            break;
        case 0x85:          /* ADD A,IXYL */
            temp = lreg(IXY);
            acu = hreg(AF);
            sum = acu + temp;
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                ((cbits >> 8) & 1);
            break;
        case 0x86:          /* ADD A,(IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            temp = GetBYTE(adr);
            acu = hreg(AF);
            sum = acu + temp;
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                ((cbits >> 8) & 1);
            break;
        case 0x8C:          /* ADC A,IXYH */
            temp = hreg(IXY);
            acu = hreg(AF);
            sum = acu + temp + TSTFLAG(C);
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                ((cbits >> 8) & 1);
            break;
        case 0x8D:          /* ADC A,IXYL */
            temp = lreg(IXY);
            acu = hreg(AF);
            sum = acu + temp + TSTFLAG(C);
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                ((cbits >> 8) & 1);
            break;
        case 0x8E:          /* ADC A,(IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            temp = GetBYTE(adr);
            acu = hreg(AF);
            sum = acu + temp + TSTFLAG(C);
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                ((cbits >> 8) & 1);
            break;
        case 0x94:          /* SUB IXYH */
            temp = hreg(IXY);
            acu = hreg(AF);
            sum = acu - temp;
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
                ((cbits >> 8) & 1);
            break;
        case 0x95:          /* SUB IXYL */
            temp = lreg(IXY);
            acu = hreg(AF);
            sum = acu - temp;
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
                ((cbits >> 8) & 1);
            break;
        case 0x96:          /* SUB (IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            temp = GetBYTE(adr);
            acu = hreg(AF);
            sum = acu - temp;
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
                ((cbits >> 8) & 1);
            break;
        case 0x9C:          /* SBC A,IXYH */
            temp = hreg(IXY);
            acu = hreg(AF);
            sum = acu - temp - TSTFLAG(C);
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
                ((cbits >> 8) & 1);
            break;
        case 0x9D:          /* SBC A,IXYL */
            temp = lreg(IXY);
            acu = hreg(AF);
            sum = acu - temp - TSTFLAG(C);
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
                ((cbits >> 8) & 1);
            break;
        case 0x9E:          /* SBC A,(IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            temp = GetBYTE(adr);
            acu = hreg(AF);
            sum = acu - temp - TSTFLAG(C);
            cbits = acu ^ temp ^ sum;
            AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
                (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
                ((cbits >> 8) & 1);
            break;
        case 0xA4:          /* AND IXYH */
            sum = ((AF & (IXY)) >> 8) & 0xff;
            AF = (sum << 8) | (sum & 0xa8) |
                ((sum == 0) << 6) | 0x10 | partab[sum];
            break;
        case 0xA5:          /* AND IXYL */
            sum = ((AF >> 8) & IXY) & 0xff;
            AF = (sum << 8) | (sum & 0xa8) | 0x10 |
                ((sum == 0) << 6) | partab[sum];
            break;
        case 0xA6:          /* AND (IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            sum = ((AF >> 8) & GetBYTE(adr)) & 0xff;
            AF = (sum << 8) | (sum & 0xa8) | 0x10 |
                ((sum == 0) << 6) | partab[sum];
            break;
        case 0xAC:          /* XOR IXYH */
            sum = ((AF ^ (IXY)) >> 8) & 0xff;
            AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
            break;
        case 0xAD:          /* XOR IXYL */
            sum = ((AF >> 8) ^ IXY) & 0xff;
            AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
            break;
        case 0xAE:          /* XOR (IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            sum = ((AF >> 8) ^ GetBYTE(adr)) & 0xff;
            AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
            break;
        case 0xB4:          /* OR IXYH */
            sum = ((AF | (IXY)) >> 8) & 0xff;
            AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
            break;
        case 0xB5:          /* OR IXYL */
            sum = ((AF >> 8) | IXY) & 0xff;
            AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
            break;
        case 0xB6:          /* OR (IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            sum = ((AF >> 8) | GetBYTE(adr)) & 0xff;
            AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
            break;
        case 0xBC:          /* CP IXYH */
            temp = hreg(IXY);
            AF = (AF & ~0x28) | (temp & 0x28);
            acu = hreg(AF);
            sum = acu - temp;
            cbits = acu ^ temp ^ sum;
            AF = (AF & ~0xff) | (sum & 0x80) |
                (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
                (cbits & 0x10) | ((cbits >> 8) & 1);
            break;
        case 0xBD:          /* CP IXYL */
            temp = lreg(IXY);
            AF = (AF & ~0x28) | (temp & 0x28);
            acu = hreg(AF);
            sum = acu - temp;
            cbits = acu ^ temp ^ sum;
            AF = (AF & ~0xff) | (sum & 0x80) |
                (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
                (cbits & 0x10) | ((cbits >> 8) & 1);
            break;
        case 0xBE:          /* CP (IXY+dd) */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            temp = GetBYTE(adr);
            AF = (AF & ~0x28) | (temp & 0x28);
            acu = hreg(AF);
            sum = acu - temp;
            cbits = acu ^ temp ^ sum;
            AF = (AF & ~0xff) | (sum & 0x80) |
                (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
                (cbits & 0x10) | ((cbits >> 8) & 1);
            break;
        case 0xCB:          /* CB prefix */
            adr = IXY + (signed char) GetBYTE(PC); ++PC;
            cycles = cb_prefix(adr);
            break;
        case 0xE1:          /* POP IXY */
            POP(IXY);
            break;
        case 0xE3:          /* EX (SP),IXY */
            temp = IXY; POP(IXY); PUSH(temp);
            break;
        case 0xE5:          /* PUSH IXY */
            PUSH(IXY);
            break;
        case 0xE9:          /* JP (IXY) */
            PC = IXY;
            break;
        case 0xF9:          /* LD SP,IXY */
            SP = IXY;
            break;
        default:            /* ignore DD */
            PC--;
            cycles = 0;
        }
    return cycles;
}

unsigned CpuZ80::simz80()
{
    unsigned temp, acu, sum, cbits;
    unsigned op;
    unsigned cycles;

    m_stackOperation = false;

    op = GetBYTE(PC);
    ++PC;
    cycles = cc_op[op];

    switch(op) {
    case 0x00:          /* NOP */
        break;
    case 0x01:          /* LD BC,nnnn */
        BC = GetWORD(PC);
        PC += 2;
        break;
    case 0x02:          /* LD (BC),A */
        PutBYTE(BC, hreg(AF));
        break;
    case 0x03:          /* INC BC */
        ++BC;
        break;
    case 0x04:          /* INC B */
        BC += 0x100;
        temp = hreg(BC);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0) << 4) |
            ((temp == 0x80) << 2);
        break;
    case 0x05:          /* DEC B */
        BC -= 0x100;
        temp = hreg(BC);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0xf) << 4) |
            ((temp == 0x7f) << 2) | 2;
        break;
    case 0x06:          /* LD B,nn */
        Sethreg(BC, GetBYTE(PC)); ++PC;
        break;
    case 0x07:          /* RLCA */
        AF = ((AF >> 7) & 0x0128) | ((AF << 1) & ~0x1ff) |
            (AF & 0xc4) | ((AF >> 15) & 1);
        break;
    case 0x08:          /* EX AF,AF' */
        af_sel = 1 - af_sel;
        break;
    case 0x09:          /* ADD HL,BC */
        HL &= 0xffff;
        BC &= 0xffff;
        sum = HL + BC;
        cbits = (HL ^ BC ^ sum) >> 8;
        HL = sum;
        AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0x0A:          /* LD A,(BC) */
        Sethreg(AF, GetBYTE(BC));
        break;
    case 0x0B:          /* DEC BC */
        --BC;
        break;
    case 0x0C:          /* INC C */
        temp = lreg(BC)+1;
        Setlreg(BC, temp);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0) << 4) |
            ((temp == 0x80) << 2);
        break;
    case 0x0D:          /* DEC C */
        temp = lreg(BC)-1;
        Setlreg(BC, temp);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0xf) << 4) |
            ((temp == 0x7f) << 2) | 2;
        break;
    case 0x0E:          /* LD C,nn */
        Setlreg(BC, GetBYTE(PC)); ++PC;
        break;
    case 0x0F:          /* RRCA */
        temp = hreg(AF);
        sum = temp >> 1;
        AF = ((temp & 1) << 15) | (sum << 8) |
            (sum & 0x28) | (AF & 0xc4) | (temp & 1);
        break;
    case 0x10:          /* DJNZ dd */
        if ((BC -= 0x100) & 0xff00) {
            PC += (signed char) GetBYTE(PC) + 1;
            cycles += cc_ex[op];                          \
        } else
            PC++;
        break;

    case 0x11:          /* LD DE,nnnn */
        DE = GetWORD(PC);
        PC += 2;
        break;
    case 0x12:          /* LD (DE),A */
        PutBYTE(DE, hreg(AF));
        break;
    case 0x13:          /* INC DE */
        ++DE;
        break;
    case 0x14:          /* INC D */
        DE += 0x100;
        temp = hreg(DE);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0) << 4) |
            ((temp == 0x80) << 2);
        break;
    case 0x15:          /* DEC D */
        DE -= 0x100;
        temp = hreg(DE);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0xf) << 4) |
            ((temp == 0x7f) << 2) | 2;
        break;
    case 0x16:          /* LD D,nn */
        Sethreg(DE, GetBYTE(PC)); ++PC;
        break;
    case 0x17:          /* RLA */
        AF = ((AF << 8) & 0x0100) | ((AF >> 7) & 0x28) | ((AF << 1) & ~0x01ff) |
            (AF & 0xc4) | ((AF >> 15) & 1);
        break;
    case 0x18:          /* JR dd */
        PC += (1) ? (signed char) GetBYTE(PC) + 1 : 1;
        break;
    case 0x19:          /* ADD HL,DE */
        HL &= 0xffff;
        DE &= 0xffff;
        sum = HL + DE;
        cbits = (HL ^ DE ^ sum) >> 8;
        HL = sum;
        AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0x1A:          /* LD A,(DE) */
        Sethreg(AF, GetBYTE(DE));
        break;
    case 0x1B:          /* DEC DE */
        --DE;
        break;
    case 0x1C:          /* INC E */
        temp = lreg(DE)+1;
        Setlreg(DE, temp);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0) << 4) |
            ((temp == 0x80) << 2);
        break;
    case 0x1D:          /* DEC E */
        temp = lreg(DE)-1;
        Setlreg(DE, temp);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0xf) << 4) |
            ((temp == 0x7f) << 2) | 2;
        break;
    case 0x1E:          /* LD E,nn */
        Setlreg(DE, GetBYTE(PC)); ++PC;
        break;
    case 0x1F:          /* RRA */
        temp = hreg(AF);
        sum = temp >> 1;
        AF = ((AF & 1) << 15) | (sum << 8) |
            (sum & 0x28) | (AF & 0xc4) | (temp & 1);
        break;
    case 0x20:          /* JR NZ,dd */
        if (!TSTFLAG(Z)) {
            PC += (signed char) GetBYTE(PC) + 1;
            cycles += cc_ex[op];                          \
        } else
            PC++;
        break;
    case 0x21:          /* LD HL,nnnn */
        HL = GetWORD(PC);
        PC += 2;
        break;
    case 0x22:          /* LD (nnnn),HL */
        temp = GetWORD(PC);
        PutWORD(temp, HL);
        PC += 2;
        break;
    case 0x23:          /* INC HL */
        ++HL;
        break;
    case 0x24:          /* INC H */
        HL += 0x100;
        temp = hreg(HL);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0) << 4) |
            ((temp == 0x80) << 2);
        break;
    case 0x25:          /* DEC H */
        HL -= 0x100;
        temp = hreg(HL);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0xf) << 4) |
            ((temp == 0x7f) << 2) | 2;
        break;
    case 0x26:          /* LD H,nn */
        Sethreg(HL, GetBYTE(PC)); ++PC;
        break;
    case 0x27:          /* DAA */
        acu = hreg(AF);
        temp = ldig(acu);
        cbits = TSTFLAG(C);
        if (TSTFLAG(N)) {   /* last operation was a subtract */
            int hd = cbits || acu > 0x99;
            if (TSTFLAG(H) || (temp > 9)) { /* adjust low digit */
                if (temp > 5)
                    SETFLAG(H, 0);
                acu -= 6;
                acu &= 0xff;
            }
            if (hd)     /* adjust high digit */
                acu -= 0x160;
        }
        else {          /* last operation was an add */
            if (TSTFLAG(H) || (temp > 9)) { /* adjust low digit */
                SETFLAG(H, (temp > 9));
                acu += 6;
            }
            if (cbits || ((acu & 0x1f0) > 0x90)) /* adjust high digit */
                acu += 0x60;
        }
        cbits |= (acu >> 8) & 1;
        acu &= 0xff;
        AF = (acu << 8) | (acu & 0xa8) | ((acu == 0) << 6) |
            (AF & 0x12) | partab[acu] | cbits;
        break;
    case 0x28:          /* JR Z,dd */
        if (TSTFLAG(Z)) {
            PC += (signed char) GetBYTE(PC) + 1;
            cycles += cc_ex[op];                          \
        } else
            PC++;
        break;
    case 0x29:          /* ADD HL,HL */
        HL &= 0xffff;
        sum = HL + HL;
        cbits = (HL ^ HL ^ sum) >> 8;
        HL = sum;
        AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0x2A:          /* LD HL,(nnnn) */
        temp = GetWORD(PC);
        HL = GetWORD(temp);
        PC += 2;
        break;
    case 0x2B:          /* DEC HL */
        --HL;
        break;
    case 0x2C:          /* INC L */
        temp = lreg(HL)+1;
        Setlreg(HL, temp);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0) << 4) |
            ((temp == 0x80) << 2);
        break;
    case 0x2D:          /* DEC L */
        temp = lreg(HL)-1;
        Setlreg(HL, temp);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0xf) << 4) |
            ((temp == 0x7f) << 2) | 2;
        break;
    case 0x2E:          /* LD L,nn */
        Setlreg(HL, GetBYTE(PC)); ++PC;
        break;
    case 0x2F:          /* CPL */
        AF = (~AF & ~0xff) | (AF & 0xc5) | ((~AF >> 8) & 0x28) | 0x12;
        break;
    case 0x30:          /* JR NC,dd */
        if (!TSTFLAG(C)) {
            PC += (signed char) GetBYTE(PC) + 1;
            cycles += cc_ex[op];                          \
        } else
            PC++;
        break;
    case 0x31:          /* LD SP,nnnn */
        SP = GetWORD(PC);
        PC += 2;
        break;
    case 0x32:          /* LD (nnnn),A */
        temp = GetWORD(PC);
        PutBYTE(temp, hreg(AF));
        PC += 2;
        break;
    case 0x33:          /* INC SP */
        ++SP;
        break;
    case 0x34:          /* INC (HL) */
        temp = GetBYTE(HL)+1;
        PutBYTE(HL, temp);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0) << 4) |
            ((temp == 0x80) << 2);
        break;
    case 0x35:          /* DEC (HL) */
        temp = GetBYTE(HL)-1;
        PutBYTE(HL, temp);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0xf) << 4) |
            ((temp == 0x7f) << 2) | 2;
        break;
    case 0x36:          /* LD (HL),nn */
        PutBYTE(HL, GetBYTE(PC)); ++PC;
        break;
    case 0x37:          /* SCF */
        AF = (AF&~0x3b)|((AF>>8)&0x28)|1;
        break;
    case 0x38:          /* JR C,dd */
        if (TSTFLAG(C)) {
            PC += (signed char) GetBYTE(PC) + 1;
            cycles += cc_ex[op];                          \
        } else
            PC++;
        break;
    case 0x39:          /* ADD HL,SP */
        HL &= 0xffff;
        SP &= 0xffff;
        sum = HL + SP;
        cbits = (HL ^ SP ^ sum) >> 8;
        HL = sum;
        AF = (AF & ~0x3b) | ((sum >> 8) & 0x28) |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0x3A:          /* LD A,(nnnn) */
        temp = GetWORD(PC);
        Sethreg(AF, GetBYTE(temp));
        PC += 2;
        break;
    case 0x3B:          /* DEC SP */
        --SP;
        break;
    case 0x3C:          /* INC A */
        AF += 0x100;
        temp = hreg(AF);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0) << 4) |
            ((temp == 0x80) << 2);
        break;
    case 0x3D:          /* DEC A */
        AF -= 0x100;
        temp = hreg(AF);
        AF = (AF & ~0xfe) | (temp & 0xa8) |
            (((temp & 0xff) == 0) << 6) |
            (((temp & 0xf) == 0xf) << 4) |
            ((temp == 0x7f) << 2) | 2;
        break;
    case 0x3E:          /* LD A,nn */
        Sethreg(AF, GetBYTE(PC)); ++PC;
        break;
    case 0x3F:          /* CCF */
        AF = (AF&~0x3b)|((AF>>8)&0x28)|((AF&1)<<4)|(~AF&1);
        break;
    case 0x40:          /* LD B,B */
        /* nop */
        break;
    case 0x41:          /* LD B,C */
        BC = (BC & 255) | ((BC & 255) << 8);
        break;
    case 0x42:          /* LD B,D */
        BC = (BC & 255) | (DE & ~255);
        break;
    case 0x43:          /* LD B,E */
        BC = (BC & 255) | ((DE & 255) << 8);
        break;
    case 0x44:          /* LD B,H */
        BC = (BC & 255) | (HL & ~255);
        break;
    case 0x45:          /* LD B,L */
        BC = (BC & 255) | ((HL & 255) << 8);
        break;
    case 0x46:          /* LD B,(HL) */
        Sethreg(BC, GetBYTE(HL));
        break;
    case 0x47:          /* LD B,A */
        BC = (BC & 255) | (AF & ~255);
        break;
    case 0x48:          /* LD C,B */
        BC = (BC & ~255) | ((BC >> 8) & 255);
        break;
    case 0x49:          /* LD C,C */
        /* nop */
        break;
    case 0x4A:          /* LD C,D */
        BC = (BC & ~255) | ((DE >> 8) & 255);
        break;
    case 0x4B:          /* LD C,E */
        BC = (BC & ~255) | (DE & 255);
        break;
    case 0x4C:          /* LD C,H */
        BC = (BC & ~255) | ((HL >> 8) & 255);
        break;
    case 0x4D:          /* LD C,L */
        BC = (BC & ~255) | (HL & 255);
        break;
    case 0x4E:          /* LD C,(HL) */
        Setlreg(BC, GetBYTE(HL));
        break;
    case 0x4F:          /* LD C,A */
        BC = (BC & ~255) | ((AF >> 8) & 255);
        break;
    case 0x50:          /* LD D,B */
        DE = (DE & 255) | (BC & ~255);
        break;
    case 0x51:          /* LD D,C */
        DE = (DE & 255) | ((BC & 255) << 8);
        break;
    case 0x52:          /* LD D,D */
        /* nop */
        break;
    case 0x53:          /* LD D,E */
        DE = (DE & 255) | ((DE & 255) << 8);
        break;
    case 0x54:          /* LD D,H */
        DE = (DE & 255) | (HL & ~255);
        break;
    case 0x55:          /* LD D,L */
        DE = (DE & 255) | ((HL & 255) << 8);
        break;
    case 0x56:          /* LD D,(HL) */
        Sethreg(DE, GetBYTE(HL));
        break;
    case 0x57:          /* LD D,A */
        DE = (DE & 255) | (AF & ~255);
        break;
    case 0x58:          /* LD E,B */
        DE = (DE & ~255) | ((BC >> 8) & 255);
        break;
    case 0x59:          /* LD E,C */
        DE = (DE & ~255) | (BC & 255);
        break;
    case 0x5A:          /* LD E,D */
        DE = (DE & ~255) | ((DE >> 8) & 255);
        break;
    case 0x5B:          /* LD E,E */
        /* nop */
        break;
    case 0x5C:          /* LD E,H */
        DE = (DE & ~255) | ((HL >> 8) & 255);
        break;
    case 0x5D:          /* LD E,L */
        DE = (DE & ~255) | (HL & 255);
        break;
    case 0x5E:          /* LD E,(HL) */
        Setlreg(DE, GetBYTE(HL));
        break;
    case 0x5F:          /* LD E,A */
        DE = (DE & ~255) | ((AF >> 8) & 255);
        break;
    case 0x60:          /* LD H,B */
        HL = (HL & 255) | (BC & ~255);
        break;
    case 0x61:          /* LD H,C */
        HL = (HL & 255) | ((BC & 255) << 8);
        break;
    case 0x62:          /* LD H,D */
        HL = (HL & 255) | (DE & ~255);
        break;
    case 0x63:          /* LD H,E */
        HL = (HL & 255) | ((DE & 255) << 8);
        break;
    case 0x64:          /* LD H,H */
        /* nop */
        break;
    case 0x65:          /* LD H,L */
        HL = (HL & 255) | ((HL & 255) << 8);
        break;
    case 0x66:          /* LD H,(HL) */
        Sethreg(HL, GetBYTE(HL));
        break;
    case 0x67:          /* LD H,A */
        HL = (HL & 255) | (AF & ~255);
        break;
    case 0x68:          /* LD L,B */
        HL = (HL & ~255) | ((BC >> 8) & 255);
        break;
    case 0x69:          /* LD L,C */
        HL = (HL & ~255) | (BC & 255);
        break;
    case 0x6A:          /* LD L,D */
        HL = (HL & ~255) | ((DE >> 8) & 255);
        break;
    case 0x6B:          /* LD L,E */
        HL = (HL & ~255) | (DE & 255);
        break;
    case 0x6C:          /* LD L,H */
        HL = (HL & ~255) | ((HL >> 8) & 255);
        break;
    case 0x6D:          /* LD L,L */
        /* nop */
        break;
    case 0x6E:          /* LD L,(HL) */
        Setlreg(HL, GetBYTE(HL));
        break;
    case 0x6F:          /* LD L,A */
        HL = (HL & ~255) | ((AF >> 8) & 255);
        break;
    case 0x70:          /* LD (HL),B */
        PutBYTE(HL, hreg(BC));
        break;
    case 0x71:          /* LD (HL),C */
        PutBYTE(HL, lreg(BC));
        break;
    case 0x72:          /* LD (HL),D */
        PutBYTE(HL, hreg(DE));
        break;
    case 0x73:          /* LD (HL),E */
        PutBYTE(HL, lreg(DE));
        break;
    case 0x74:          /* LD (HL),H */
        PutBYTE(HL, hreg(HL));
        break;
    case 0x75:          /* LD (HL),L */
        PutBYTE(HL, lreg(HL));
        break;
    case 0x76:          /* HALT */
        PC--;
        if (m_debugOnHalt)
            g_emulation->debugRequest(this);
        //return PC&0xffff;
        break; //!!!
    case 0x77:          /* LD (HL),A */
        PutBYTE(HL, hreg(AF));
        break;
    case 0x78:          /* LD A,B */
        AF = (AF & 255) | (BC & ~255);
        break;
    case 0x79:          /* LD A,C */
        AF = (AF & 255) | ((BC & 255) << 8);
        break;
    case 0x7A:          /* LD A,D */
        AF = (AF & 255) | (DE & ~255);
        break;
    case 0x7B:          /* LD A,E */
        AF = (AF & 255) | ((DE & 255) << 8);
        break;
    case 0x7C:          /* LD A,H */
        AF = (AF & 255) | (HL & ~255);
        break;
    case 0x7D:          /* LD A,L */
        AF = (AF & 255) | ((HL & 255) << 8);
        break;
    case 0x7E:          /* LD A,(HL) */
        Sethreg(AF, GetBYTE(HL));
        break;
    case 0x7F:          /* LD A,A */
        /* nop */
        break;
    case 0x80:          /* ADD A,B */
        temp = hreg(BC);
        acu = hreg(AF);
        sum = acu + temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x81:          /* ADD A,C */
        temp = lreg(BC);
        acu = hreg(AF);
        sum = acu + temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x82:          /* ADD A,D */
        temp = hreg(DE);
        acu = hreg(AF);
        sum = acu + temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x83:          /* ADD A,E */
        temp = lreg(DE);
        acu = hreg(AF);
        sum = acu + temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x84:          /* ADD A,H */
        temp = hreg(HL);
        acu = hreg(AF);
        sum = acu + temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x85:          /* ADD A,L */
        temp = lreg(HL);
        acu = hreg(AF);
        sum = acu + temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x86:          /* ADD A,(HL) */
        temp = GetBYTE(HL);
        acu = hreg(AF);
        sum = acu + temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x87:          /* ADD A,A */
        temp = hreg(AF);
        acu = hreg(AF);
        sum = acu + temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x88:          /* ADC A,B */
        temp = hreg(BC);
        acu = hreg(AF);
        sum = acu + temp + TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x89:          /* ADC A,C */
        temp = lreg(BC);
        acu = hreg(AF);
        sum = acu + temp + TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x8A:          /* ADC A,D */
        temp = hreg(DE);
        acu = hreg(AF);
        sum = acu + temp + TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x8B:          /* ADC A,E */
        temp = lreg(DE);
        acu = hreg(AF);
        sum = acu + temp + TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x8C:          /* ADC A,H */
        temp = hreg(HL);
        acu = hreg(AF);
        sum = acu + temp + TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x8D:          /* ADC A,L */
        temp = lreg(HL);
        acu = hreg(AF);
        sum = acu + temp + TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x8E:          /* ADC A,(HL) */
        temp = GetBYTE(HL);
        acu = hreg(AF);
        sum = acu + temp + TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x8F:          /* ADC A,A */
        temp = hreg(AF);
        acu = hreg(AF);
        sum = acu + temp + TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        break;
    case 0x90:          /* SUB B */
        temp = hreg(BC);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x91:          /* SUB C */
        temp = lreg(BC);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x92:          /* SUB D */
        temp = hreg(DE);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x93:          /* SUB E */
        temp = lreg(DE);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x94:          /* SUB H */
        temp = hreg(HL);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x95:          /* SUB L */
        temp = lreg(HL);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x96:          /* SUB (HL) */
        temp = GetBYTE(HL);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x97:          /* SUB A */
        temp = hreg(AF);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x98:          /* SBC A,B */
        temp = hreg(BC);
        acu = hreg(AF);
        sum = acu - temp - TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x99:          /* SBC A,C */
        temp = lreg(BC);
        acu = hreg(AF);
        sum = acu - temp - TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x9A:          /* SBC A,D */
        temp = hreg(DE);
        acu = hreg(AF);
        sum = acu - temp - TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x9B:          /* SBC A,E */
        temp = lreg(DE);
        acu = hreg(AF);
        sum = acu - temp - TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x9C:          /* SBC A,H */
        temp = hreg(HL);
        acu = hreg(AF);
        sum = acu - temp - TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x9D:          /* SBC A,L */
        temp = lreg(HL);
        acu = hreg(AF);
        sum = acu - temp - TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x9E:          /* SBC A,(HL) */
        temp = GetBYTE(HL);
        acu = hreg(AF);
        sum = acu - temp - TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0x9F:          /* SBC A,A */
        temp = hreg(AF);
        acu = hreg(AF);
        sum = acu - temp - TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        break;
    case 0xA0:          /* AND B */
        sum = ((AF & (BC)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) |
            ((sum == 0) << 6) | 0x10 | partab[sum];
        break;
    case 0xA1:          /* AND C */
        sum = ((AF >> 8) & BC) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | 0x10 |
            ((sum == 0) << 6) | partab[sum];
        break;
    case 0xA2:          /* AND D */
        sum = ((AF & (DE)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) |
            ((sum == 0) << 6) | 0x10 | partab[sum];
        break;
    case 0xA3:          /* AND E */
        sum = ((AF >> 8) & DE) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | 0x10 |
            ((sum == 0) << 6) | partab[sum];
        break;
    case 0xA4:          /* AND H */
        sum = ((AF & (HL)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) |
            ((sum == 0) << 6) | 0x10 | partab[sum];
        break;
    case 0xA5:          /* AND L */
        sum = ((AF >> 8) & HL) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | 0x10 |
            ((sum == 0) << 6) | partab[sum];
        break;
    case 0xA6:          /* AND (HL) */
        sum = ((AF >> 8) & GetBYTE(HL)) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | 0x10 |
            ((sum == 0) << 6) | partab[sum];
        break;
    case 0xA7:          /* AND A */
        sum = ((AF & (AF)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) |
            ((sum == 0) << 6) | 0x10 | partab[sum];
        break;
    case 0xA8:          /* XOR B */
        sum = ((AF ^ (BC)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xA9:          /* XOR C */
        sum = ((AF >> 8) ^ BC) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xAA:          /* XOR D */
        sum = ((AF ^ (DE)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xAB:          /* XOR E */
        sum = ((AF >> 8) ^ DE) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xAC:          /* XOR H */
        sum = ((AF ^ (HL)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xAD:          /* XOR L */
        sum = ((AF >> 8) ^ HL) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xAE:          /* XOR (HL) */
        sum = ((AF >> 8) ^ GetBYTE(HL)) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xAF:          /* XOR A */
        sum = ((AF ^ (AF)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xB0:          /* OR B */
        sum = ((AF | (BC)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xB1:          /* OR C */
        sum = ((AF >> 8) | BC) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xB2:          /* OR D */
        sum = ((AF | (DE)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xB3:          /* OR E */
        sum = ((AF >> 8) | DE) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xB4:          /* OR H */
        sum = ((AF | (HL)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xB5:          /* OR L */
        sum = ((AF >> 8) | HL) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xB6:          /* OR (HL) */
        sum = ((AF >> 8) | GetBYTE(HL)) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xB7:          /* OR A */
        sum = ((AF | (AF)) >> 8) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        break;
    case 0xB8:          /* CP B */
        temp = hreg(BC);
        AF = (AF & ~0x28) | (temp & 0x28);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = (AF & ~0xff) | (sum & 0x80) |
            (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0xB9:          /* CP C */
        temp = lreg(BC);
        AF = (AF & ~0x28) | (temp & 0x28);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = (AF & ~0xff) | (sum & 0x80) |
            (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0xBA:          /* CP D */
        temp = hreg(DE);
        AF = (AF & ~0x28) | (temp & 0x28);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = (AF & ~0xff) | (sum & 0x80) |
            (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0xBB:          /* CP E */
        temp = lreg(DE);
        AF = (AF & ~0x28) | (temp & 0x28);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = (AF & ~0xff) | (sum & 0x80) |
            (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0xBC:          /* CP H */
        temp = hreg(HL);
        AF = (AF & ~0x28) | (temp & 0x28);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = (AF & ~0xff) | (sum & 0x80) |
            (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0xBD:          /* CP L */
        temp = lreg(HL);
        AF = (AF & ~0x28) | (temp & 0x28);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = (AF & ~0xff) | (sum & 0x80) |
            (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0xBE:          /* CP (HL) */
        temp = GetBYTE(HL);
        AF = (AF & ~0x28) | (temp & 0x28);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = (AF & ~0xff) | (sum & 0x80) |
            (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0xBF:          /* CP A */
        temp = hreg(AF);
        AF = (AF & ~0x28) | (temp & 0x28);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = (AF & ~0xff) | (sum & 0x80) |
            (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        break;
    case 0xC0:          /* RET NZ */
        if (!TSTFLAG(Z)) { POP(PC); cycles += cc_ex[op];}
        break;
    case 0xC1:          /* POP BC */
        POP(BC);
        break;
    case 0xC2:          /* JP NZ,nnnn */
        JPC(!TSTFLAG(Z));
        break;
    case 0xC3:          /* JP nnnn */
        JPC(1);
        break;
    case 0xC4:          /* CALL NZ,nnnn */
        CALLC(!TSTFLAG(Z));
        break;
    case 0xC5:          /* PUSH BC */
        PUSH(BC);
        break;
    case 0xC6:          /* ADD A,nn */
        temp = GetBYTE(PC);
        acu = hreg(AF);
        sum = acu + temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        ++PC;
        break;
    case 0xC7:          /* RST 0 */
        PUSH(PC); PC = 0;
        break;
    case 0xC8:          /* RET Z */
        if (TSTFLAG(Z)) { POP(PC); cycles += cc_ex[op];}
        break;
    case 0xC9:          /* RET */
        POP(PC);
        break;
    case 0xCA:          /* JP Z,nnnn */
        JPC(TSTFLAG(Z));
        break;
    case 0xCB:          /* CB prefix */
        cycles = cb_prefix(HL);
        break;
    case 0xCC:          /* CALL Z,nnnn */
        CALLC(TSTFLAG(Z));
        break;
    case 0xCD:          /* CALL nnnn */
        CALLC(1);
        break;
    case 0xCE:          /* ADC A,nn */
        temp = GetBYTE(PC);
        acu = hreg(AF);
        sum = acu + temp + TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) |
            ((cbits >> 8) & 1);
        ++PC;
        break;
    case 0xCF:          /* RST 8 */
        PUSH(PC); PC = 8;
        break;
    case 0xD0:          /* RET NC */
        if (!TSTFLAG(C)) { POP(PC); cycles += cc_ex[op]; }
        break;
    case 0xD1:          /* POP DE */
        POP(DE);
        break;
    case 0xD2:          /* JP NC,nnnn */
        JPC(!TSTFLAG(C));
        break;
    case 0xD3:          /* OUT (nn),A */
        io_output(GetBYTE(PC), hreg(AF)); ++PC;
        break;
    case 0xD4:          /* CALL NC,nnnn */
        CALLC(!TSTFLAG(C));
        break;
    case 0xD5:          /* PUSH DE */
        PUSH(DE);
        break;
    case 0xD6:          /* SUB nn */
        temp = GetBYTE(PC);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        ++PC;
        break;
    case 0xD7:          /* RST 10H */
        PUSH(PC); PC = 0x10;
        break;
    case 0xD8:          /* RET C */
        if (TSTFLAG(C)) { POP(PC); cycles += cc_ex[op]; }
        break;
    case 0xD9:          /* EXX */
        regs_sel = 1 - regs_sel;
        break;
    case 0xDA:          /* JP C,nnnn */
        JPC(TSTFLAG(C));
        break;
    case 0xDB:          /* IN A,(nn) */
        Sethreg(AF, io_input(GetBYTE(PC))); ++PC;
        break;
    case 0xDC:          /* CALL C,nnnn */
        CALLC(TSTFLAG(C));
        break;
    case 0xDD:          /* DD prefix */
        cycles = dfd_prefix(ix);
        break;
    case 0xDE:          /* SBC A,nn */
        temp = GetBYTE(PC);
        acu = hreg(AF);
        sum = acu - temp - TSTFLAG(C);
        cbits = acu ^ temp ^ sum;
        AF = ((sum & 0xff) << 8) | (sum & 0xa8) |
            (((sum & 0xff) == 0) << 6) | (cbits & 0x10) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            ((cbits >> 8) & 1);
        ++PC;
        break;
    case 0xDF:          /* RST 18H */
        PUSH(PC); PC = 0x18;
        break;
    case 0xE0:          /* RET PO */
        if (!TSTFLAG(P)) { POP(PC); cycles += cc_ex[op]; }
        break;
    case 0xE1:          /* POP HL */
        POP(HL);
        break;
    case 0xE2:          /* JP PO,nnnn */
        JPC(!TSTFLAG(P));
        break;
    case 0xE3:          /* EX (SP),HL */
        temp = HL; POP(HL); PUSH(temp);
        break;
    case 0xE4:          /* CALL PO,nnnn */
        CALLC(!TSTFLAG(P));
        break;
    case 0xE5:          /* PUSH HL */
        PUSH(HL);
        break;
    case 0xE6:          /* AND nn */
        sum = ((AF >> 8) & GetBYTE(PC)) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | 0x10 |
            ((sum == 0) << 6) | partab[sum];
        ++PC;
        break;
    case 0xE7:          /* RST 20H */
        PUSH(PC); PC = 0x20;
        break;
    case 0xE8:          /* RET PE */
        if (TSTFLAG(P)) { POP(PC); cycles += cc_ex[op]; }
        break;
    case 0xE9:          /* JP (HL) */
        PC = HL;
        break;
    case 0xEA:          /* JP PE,nnnn */
        JPC(TSTFLAG(P));
        break;
    case 0xEB:          /* EX DE,HL */
        temp = HL; HL = DE; DE = temp;
        break;
    case 0xEC:          /* CALL PE,nnnn */
        CALLC(TSTFLAG(P));
        break;
    case 0xED:          /* ED prefix */
        op = GetBYTE(PC);
        ++PC;
        cycles = cc_ed[op];
        switch (op) {
        case 0x40:          /* IN B,(C) */
            temp = io_input(lreg(BC));
            Sethreg(BC, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                parity(temp);
            break;
        case 0x41:          /* OUT (C),B */
            io_output(lreg(BC), hreg(BC));
            break;
        case 0x42:          /* SBC HL,BC */
            HL &= 0xffff;
            BC &= 0xffff;
            sum = HL - BC - TSTFLAG(C);
            cbits = (HL ^ BC ^ sum) >> 8;
            HL = sum;
            AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) |
                (((sum & 0xffff) == 0) << 6) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                (cbits & 0x10) | 2 | ((cbits >> 8) & 1);
            break;
        case 0x43:          /* LD (nnnn),BC */
            temp = GetWORD(PC);
            PutWORD(temp, BC);
            PC += 2;
            break;
        case 0x44:          /* NEG */
            temp = hreg(AF);
            AF = (-(AF & 0xff00) & 0xff00);
            AF |= ((AF >> 8) & 0xa8) | (((AF & 0xff00) == 0) << 6) |
                (((temp & 0x0f) != 0) << 4) | ((temp == 0x80) << 2) |
                2 | (temp != 0);
            break;
        case 0x45:          /* RETN */
            IFF |= IFF >> 1;
            POP(PC);
            break;
        case 0x46:          /* IM 0 */
            IM = 0;
            break;
        case 0x47:          /* LD I,A */
            ir = (ir & 255) | (AF & ~255);
            break;
        case 0x48:          /* IN C,(C) */
            temp = io_input(lreg(BC));
            Setlreg(BC, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                parity(temp);
            break;
        case 0x49:          /* OUT (C),C */
            io_output(lreg(BC), lreg(BC));
            break;
        case 0x4A:          /* ADC HL,BC */
            HL &= 0xffff;
            BC &= 0xffff;
            sum = HL + BC + TSTFLAG(C);
            cbits = (HL ^ BC ^ sum) >> 8;
            HL = sum;
            AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) |
                (((sum & 0xffff) == 0) << 6) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                (cbits & 0x10) | ((cbits >> 8) & 1);
            break;
        case 0x4B:          /* LD BC,(nnnn) */
            temp = GetWORD(PC);
            BC = GetWORD(temp);
            PC += 2;
            break;
        case 0x4D:          /* RETI */
            IFF |= IFF >> 1;
            POP(PC);
            break;
        case 0x4F:          /* LD R,A */
            ir = (ir & ~255) | ((AF >> 8) & 255);
            break;
        case 0x50:          /* IN D,(C) */
            temp = io_input(lreg(BC));
            Sethreg(DE, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                parity(temp);
            break;
        case 0x51:          /* OUT (C),D */
            io_output(lreg(BC), hreg(DE));
            break;
        case 0x52:          /* SBC HL,DE */
            HL &= 0xffff;
            DE &= 0xffff;
            sum = HL - DE - TSTFLAG(C);
            cbits = (HL ^ DE ^ sum) >> 8;
            HL = sum;
            AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) |
                (((sum & 0xffff) == 0) << 6) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                (cbits & 0x10) | 2 | ((cbits >> 8) & 1);
            break;
        case 0x53:          /* LD (nnnn),DE */
            temp = GetWORD(PC);
            PutWORD(temp, DE);
            PC += 2;
            break;
        case 0x56:          /* IM 1 */
            IM = 1;
            break;
        case 0x57:          /* LD A,I */
            AF = (AF & 0x29) | (ir & ~255) | ((ir >> 8) & 0x80) | (((ir & ~255) == 0) << 6) | ((IFF & 2) << 1);
            break;
        case 0x58:          /* IN E,(C) */
            temp = io_input(lreg(BC));
            Setlreg(DE, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                parity(temp);
            break;
        case 0x59:          /* OUT (C),E */
            io_output(lreg(BC), lreg(DE));
            break;
        case 0x5A:          /* ADC HL,DE */
            HL &= 0xffff;
            DE &= 0xffff;
            sum = HL + DE + TSTFLAG(C);
            cbits = (HL ^ DE ^ sum) >> 8;
            HL = sum;
            AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) |
                (((sum & 0xffff) == 0) << 6) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                (cbits & 0x10) | ((cbits >> 8) & 1);
            break;
        case 0x5B:          /* LD DE,(nnnn) */
            temp = GetWORD(PC);
            DE = GetWORD(temp);
            PC += 2;
            break;
        case 0x5E:          /* IM 2 */
            IM = 2;
            break;
        case 0x5F:          /* LD A,R */
            AF = (AF & 0x29) | ((ir & 255) << 8) | (ir & 0x80) | (((ir & 255) == 0) << 6) | ((IFF & 2) << 1);
            break;
        case 0x60:          /* IN H,(C) */
            temp = io_input(lreg(BC));
            Sethreg(HL, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                parity(temp);
            break;
        case 0x61:          /* OUT (C),H */
            io_output(lreg(BC), hreg(HL));
            break;
        case 0x62:          /* SBC HL,HL */
            HL &= 0xffff;
            sum = HL - HL - TSTFLAG(C);
            cbits = (HL ^ HL ^ sum) >> 8;
            HL = sum;
            AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) |
                (((sum & 0xffff) == 0) << 6) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                (cbits & 0x10) | 2 | ((cbits >> 8) & 1);
            break;
        case 0x63:          /* LD (nnnn),HL */
            temp = GetWORD(PC);
            PutWORD(temp, HL);
            PC += 2;
            break;
        case 0x67:          /* RRD */
            temp = GetBYTE(HL);
            acu = hreg(AF);
            PutBYTE(HL, hdig(temp) | (ldig(acu) << 4));
            acu = (acu & 0xf0) | ldig(temp);
            AF = (acu << 8) | (acu & 0xa8) | (((acu & 0xff) == 0) << 6) |
                partab[acu] | (AF & 1);
            break;
        case 0x68:          /* IN L,(C) */
            temp = io_input(lreg(BC));
            Setlreg(HL, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                parity(temp);
            break;
        case 0x69:          /* OUT (C),L */
            io_output(lreg(BC), lreg(HL));
            break;
        case 0x6A:          /* ADC HL,HL */
            HL &= 0xffff;
            sum = HL + HL + TSTFLAG(C);
            cbits = (HL ^ HL ^ sum) >> 8;
            HL = sum;
            AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) |
                (((sum & 0xffff) == 0) << 6) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                (cbits & 0x10) | ((cbits >> 8) & 1);
            break;
        case 0x6B:          /* LD HL,(nnnn) */
            temp = GetWORD(PC);
            HL = GetWORD(temp);
            PC += 2;
            break;
        case 0x6F:          /* RLD */
            temp = GetBYTE(HL);
            acu = hreg(AF);
            PutBYTE(HL, (ldig(temp) << 4) | ldig(acu));
            acu = (acu & 0xf0) | hdig(temp);
            AF = (acu << 8) | (acu & 0xa8) | (((acu & 0xff) == 0) << 6) |
                partab[acu] | (AF & 1);
            break;
        case 0x70:          /* IN (C) */
            temp = io_input(lreg(BC));
            Setlreg(temp, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                parity(temp);
            break;
        case 0x71:          /* OUT (C),0 */
            io_output(lreg(BC), lreg(0));
            break;
        case 0x72:          /* SBC HL,SP */
            HL &= 0xffff;
            SP &= 0xffff;
            sum = HL - SP - TSTFLAG(C);
            cbits = (HL ^ SP ^ sum) >> 8;
            HL = sum;
            AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) |
                (((sum & 0xffff) == 0) << 6) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                (cbits & 0x10) | 2 | ((cbits >> 8) & 1);
            break;
        case 0x73:          /* LD (nnnn),SP */
            temp = GetWORD(PC);
            PutWORD(temp, SP);
            PC += 2;
            break;
        case 0x78:          /* IN A,(C) */
            temp = io_input(lreg(BC));
            Sethreg(AF, temp);
            AF = (AF & ~0xfe) | (temp & 0xa8) |
                (((temp & 0xff) == 0) << 6) |
                parity(temp);
            break;
        case 0x79:          /* OUT (C),A */
            io_output(lreg(BC), hreg(AF));
            break;
        case 0x7A:          /* ADC HL,SP */
            HL &= 0xffff;
            SP &= 0xffff;
            sum = HL + SP + TSTFLAG(C);
            cbits = (HL ^ SP ^ sum) >> 8;
            HL = sum;
            AF = (AF & ~0xff) | ((sum >> 8) & 0xa8) |
                (((sum & 0xffff) == 0) << 6) |
                (((cbits >> 6) ^ (cbits >> 5)) & 4) |
                (cbits & 0x10) | ((cbits >> 8) & 1);
            break;
        case 0x7B:          /* LD SP,(nnnn) */
            temp = GetWORD(PC);
            SP = GetWORD(temp);
            PC += 2;
            break;
        case 0xA0:          /* LDI */
            acu = GetBYTE(HL); ++HL;
            PutBYTE(DE, acu); ++DE;
            acu += hreg(AF);
            AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4) |
                (((--BC & 0xffff) != 0) << 2);
            break;
        case 0xA1:          /* CPI */
            acu = hreg(AF);
            temp = GetBYTE(HL); ++HL;
            sum = acu - temp;
            cbits = acu ^ temp ^ sum;
            AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |
                (((sum - ((cbits&16)>>4))&2) << 4) | (cbits & 16) |
                ((sum - ((cbits >> 4) & 1)) & 8) |
                ((--BC & 0xffff) != 0) << 2 | 2;
            if ((sum & 15) == 8 && (cbits & 16) != 0)
                AF &= ~8;
            break;
        case 0xA2:          /* INI */
            PutBYTE(HL, io_input(lreg(BC))); ++HL;
            SETFLAG(N, 1);
            Sethreg(BC, lreg(BC) - 1);
            SETFLAG(Z, lreg(BC) == 0);
            break;
        case 0xA3:          /* OUTI */
            io_output(lreg(BC), GetBYTE(HL)); ++HL;
            SETFLAG(N, 1);
            Sethreg(BC, lreg(BC) - 1);
            SETFLAG(Z, lreg(BC) == 0);
            break;
        case 0xA8:          /* LDD */
            acu = GetBYTE(HL); --HL;
            PutBYTE(DE, acu); --DE;
            acu += hreg(AF);
            AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4) |
                (((--BC & 0xffff) != 0) << 2);
            break;
        case 0xA9:          /* CPD */
            acu = hreg(AF);
            temp = GetBYTE(HL); --HL;
            sum = acu - temp;
            cbits = acu ^ temp ^ sum;
            AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |
                (((sum - ((cbits&16)>>4))&2) << 4) | (cbits & 16) |
                ((sum - ((cbits >> 4) & 1)) & 8) |
                ((--BC & 0xffff) != 0) << 2 | 2;
            if ((sum & 15) == 8 && (cbits & 16) != 0)
                AF &= ~8;
            break;
        case 0xAA:          /* IND */
            PutBYTE(HL, io_input(lreg(BC))); --HL;
            SETFLAG(N, 1);
            Sethreg(BC, lreg(BC) - 1);
            SETFLAG(Z, lreg(BC) == 0);
            break;
        case 0xAB:          /* OUTD */
            io_output(lreg(BC), GetBYTE(HL)); --HL;
            SETFLAG(N, 1);
            Sethreg(BC, lreg(BC) - 1);
            SETFLAG(Z, lreg(BC) == 0);
            break;
        case 0xB0:          /* LDIR */
            acu = GetBYTE(HL++);
            PutBYTE(DE++, acu);
            acu += hreg(AF);
            AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4);
            if (--BC) {
                cycles += cc_ex[op];
                PC -= 2;
            }
            break;
        case 0xB1:          /* CPIR */
        {
            acu = hreg(AF);
            temp = GetBYTE(HL++);
            bool cond = --BC != 0;
            sum = acu - temp;
            cbits = acu ^ temp ^ sum;
            AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |
                    (((sum - ((cbits&16)>>4))&2) << 4) |
                    (cbits & 16) | ((sum - ((cbits >> 4) & 1)) & 8) |
                    cond << 2 | 2;
            if ((sum & 15) == 8 && (cbits & 16) != 0)
                AF &= ~8;
            if (cond && sum) {
                cycles += cc_ex[op];
                PC -= 2;
            }
        }
            break;
        case 0xB2:          /* INIR */
            PutBYTE(HL++, io_input(lreg(BC)));
            BC -= 0x100;
            SETFLAG(N, 1);
            SETFLAG(Z, 1);
            if (hreg(BC)) {
                cycles += cc_ex[op];
                PC -= 2;
            }
            break;
        case 0xB3:          /* OTIR */
            temp = hreg(BC);
            do {
                io_output(lreg(BC), GetBYTE(HL)); ++HL;
            } while (--temp);
            Sethreg(BC, 0);
            SETFLAG(N, 1);
            SETFLAG(Z, 1);
            break;
        case 0xB8:          /* LDDR */
            acu = GetBYTE(HL--);
            PutBYTE(DE--, acu);
            acu += hreg(AF);
            AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4);
            if (--BC) {
                cycles += cc_ex[op];
                PC -= 2;
            }
            break;
        case 0xB9:          /* CPDR */
        {
            acu = hreg(AF);
            temp = GetBYTE(HL--);
            bool cond = --BC != 0;
            sum = acu - temp;
            cbits = acu ^ temp ^ sum;
            AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |
                    (((sum - ((cbits&16)>>4))&2) << 4) |
                    (cbits & 16) | ((sum - ((cbits >> 4) & 1)) & 8) |
                    cond << 2 | 2;
            if ((sum & 15) == 8 && (cbits & 16) != 0)
                AF &= ~8;
            if (cond && sum) {
                cycles += cc_ex[op];
                PC -= 2;
            }
        }
            break;
        case 0xBA:          /* INDR */
            PutBYTE(HL--, io_input(lreg(BC)));
            BC -= 0x100;
            SETFLAG(N, 1);
            SETFLAG(Z, 1);
            if (hreg(BC)) {
                cycles += cc_ex[op];
                PC -= 2;
            }
            break;
        case 0xBB:          /* OTDR */
            temp = hreg(BC);
            do {
                io_output(lreg(BC), GetBYTE(HL)); --HL;
            } while (--temp);
            Sethreg(BC, 0);
            SETFLAG(N, 1);
            SETFLAG(Z, 1);
            break;
        default: if (0x40 <= op && op <= 0x7f) PC--;        /* ignore ED */
        }
        break;
    case 0xEE:          /* XOR nn */
        sum = ((AF >> 8) ^ GetBYTE(PC)) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        ++PC;
        break;
    case 0xEF:          /* RST 28H */
        PUSH(PC); PC = 0x28;
        break;
    case 0xF0:          /* RET P */
        if (!TSTFLAG(S)) { POP(PC); cycles += cc_ex[op]; }
        break;
    case 0xF1:          /* POP AF */
        POP(AF);
        break;
    case 0xF2:          /* JP P,nnnn */
        JPC(!TSTFLAG(S));
        break;
    case 0xF3:          /* DI */
        IFF = 0;
        m_core->inte(false);
        break;
    case 0xF4:          /* CALL P,nnnn */
        CALLC(!TSTFLAG(S));
        break;
    case 0xF5:          /* PUSH AF */
        PUSH(AF);
        break;
    case 0xF6:          /* OR nn */
        sum = ((AF >> 8) | GetBYTE(PC)) & 0xff;
        AF = (sum << 8) | (sum & 0xa8) | ((sum == 0) << 6) | partab[sum];
        ++PC;
        break;
    case 0xF7:          /* RST 30H */
        PUSH(PC); PC = 0x30;
        break;
    case 0xF8:          /* RET M */
        if (TSTFLAG(S)) { POP(PC); cycles += cc_ex[op]; }
        break;
    case 0xF9:          /* LD SP,HL */
        SP = HL;
        break;
    case 0xFA:          /* JP M,nnnn */
        JPC(TSTFLAG(S));
        break;
    case 0xFB:          /* EI */
        m_iffPendingCnt = 2;
        IFF = 0;
        m_core->inte(true);  // there is no INTE output in Z80 though
        break;
    case 0xFC:          /* CALL M,nnnn */
        CALLC(TSTFLAG(S));
        break;
    case 0xFD:          /* FD prefix */
        cycles = dfd_prefix(iy);
        break;
    case 0xFE:          /* CP nn */
        temp = GetBYTE(PC);
        AF = (AF & ~0x28) | (temp & 0x28);
        acu = hreg(AF);
        sum = acu - temp;
        cbits = acu ^ temp ^ sum;
        AF = (AF & ~0xff) | (sum & 0x80) |
            (((sum & 0xff) == 0) << 6) | (temp & 0x28) |
            (((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
            (cbits & 0x10) | ((cbits >> 8) & 1);
        ++PC;
        break;
    case 0xFF:          /* RST 38H */
        PUSH(PC); PC = 0x38;
    }

    if (m_iffPendingCnt)
        if (!--m_iffPendingCnt) {
            IFF = 3;
            m_core->inte(true);
        }

    return cycles;
}


CpuZ80::CpuZ80()
{
    // reset
}


/*void CpuZ80::exec(int nCmds)
{
    for (int i = 0; i < nCmds; i++)
        simz80();
}*/


void CpuZ80::operate()
{
    if (!m_hooksDisabled) {
        bool retFlag = false;
        list<CpuHook*>* hookList = m_hookArray[PC];
        if (hookList) {
            for (auto it = hookList->begin(); it != hookList->end(); it++)
                retFlag = retFlag || (*it)->hookProc();
        }
        if (retFlag)
            return;
    }

    if (m_waits) {
        int tag;
        int opcode = m_addrSpace->readByteEx(PC, tag) + (m_addrSpace->readByte(uint16_t(PC + 1)) << 8); // 2 bytes for z80
        int clocks = simz80();
        m_curClock += m_kDiv * (clocks + m_waits->getCpuWaitStates(tag, opcode, clocks));
    } else
        m_curClock += m_kDiv * simz80();

    if (m_stepReq) {
        m_stepReq = false;
        g_emulation->debugRequest(this);
    }
}


void CpuZ80::reset() {
    af_sel = 0;
    regs_sel = 0;

    for (int i = 0; i < 2; i++) {
        af[i] = 0;
        regs[i].bc = 0;
        regs[i].de = 0;
        regs[i].hl = 0;
    }

    ir = 0;
    ix = 0;
    iy = 0;
    sp = 0;
    IM = 0;

    pc = m_startAddr;

    m_iffPendingCnt = 0;

    IFF = 0;
    m_core->inte(false);
}


void CpuZ80::intRst(int vect)
{
    if (IFF != 0) {
        IFF = 0;
        m_core->inte(false);
        if (GetBYTE(PC) == 0x76) {
            PC++;
            m_curClock = g_emulation->getCurClock();
        }
        PUSH(PC);
        if (IM != 2)
            PC = vect * 8;
        else {
            PC = GetWORD((ir | 0xFF)); // r is ignored
            m_curClock += m_kDiv * 19;
            /*if (m_waits)
                m_curClock += m_kDiv * m_waits->getCpuWaitStates(0, 0xE3, 19);*/ // similar to xthl
        }
        m_curClock += m_kDiv * 11; // revise!
    }
}


void CpuZ80::intCall(uint16_t addr)
{
    if (IFF != 0) {
        IFF = 0;
        m_core->inte(false);
        if (GetBYTE(PC) == 0x76) {
            PC++;
            m_curClock = g_emulation->getCurClock();
        }
        PUSH(PC);
        PC = addr;
        m_curClock += m_kDiv * 19;
        /*if (m_waits)
            m_curClock += m_kDiv * m_waits->getCpuWaitStates(0, 0xCD, 17);*/
    }
}


void CpuZ80::ret() {
    POP(PC);
}


uint16_t CpuZ80::getAF() {
    return AF;
}


uint16_t CpuZ80::getBC() {
    return BC;
}


uint16_t CpuZ80::getDE() {
    return DE;
}


uint16_t CpuZ80::getHL() {
    return HL;
}


uint16_t CpuZ80::getSP() {
    return SP;
}


uint16_t CpuZ80::getPC() {
    return PC;
}


bool CpuZ80::getInte() {
    return IFF;
}


void CpuZ80::setBC(uint16_t value)
{
    BC = value;
}


void CpuZ80::setDE(uint16_t value)
{
    DE = value;
}


void CpuZ80::setHL(uint16_t value)
{
    HL = value;
}


void CpuZ80::setSP(uint16_t value)
{
    SP = value;
}


void CpuZ80::setPC(uint16_t value)
{
    PC = value;
}


void CpuZ80::setAF(uint16_t value)
{
    AF = value;
}


uint16_t CpuZ80::getAF2() {
    return af[1 - af_sel];
}


uint16_t CpuZ80::getBC2() {
    return regs[1 - regs_sel].bc;
}


uint16_t CpuZ80::getDE2() {
    return regs[1 - regs_sel].de;
}


uint16_t CpuZ80::getHL2() {
    return regs[1 - regs_sel].hl;
}


uint16_t CpuZ80::getIX() {
    return ix;
}


uint16_t CpuZ80::getIY() {
    return iy;
}


uint8_t CpuZ80::getIM() {
    return IM;
}


uint8_t CpuZ80::getI() {
    return ir >> 8;
}


uint8_t CpuZ80::getIFF() {
    return IFF;
}


void CpuZ80::setAF2(uint16_t value)
{
    af[1 - af_sel] = value;
}


void CpuZ80::setBC2(uint16_t value)
{
    regs[1 - regs_sel].bc = value;
}


void CpuZ80::setDE2(uint16_t value)
{
    regs[1 - regs_sel].de = value;
}


void CpuZ80::setHL2(uint16_t value)
{
    regs[1 - regs_sel].hl = value;
}


void CpuZ80::setIX(uint16_t value)
{
    ix = value;
}


void CpuZ80::setIY(uint16_t value)
{
    iy = value;
}


void CpuZ80::setIFF(bool iff)
{
    IFF = iff ? 3 : 0;
}
