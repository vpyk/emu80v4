/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017
 *
 *  Based on original code by:
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

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

static const char* mnemo[0x100] =
{
    "NOP",    "LXI B,",  "STAX B", "INX B",  "INR B", "DCR B", "MVI B,", "RLC", "DB 08H", "DAD B",  "LDAX B", "DCX B",  "INR C", "DCR C", "MVI C,", "RRC",
    "DB 10H", "LXI D,",  "STAX D", "INX D",  "INR D", "DCR D", "MVI D,", "RAL", "DB 18H", "DAD D",  "LDAX D", "DCX D",  "INR E", "DCR E", "MVI E,", "RAR",
    "DB 20H", "LXI H,",  "SHLD",   "INX H",  "INR H", "DCR H", "MVI H,", "DAA", "DB 28H", "DAD H",  "LHLD",   "DCX H",  "INR L", "DCR L", "MVI L,", "CMA",
    "DB 30H", "LXI SP,", "STA",    "INX SP", "INR M", "DCR M", "MVI M,", "STC", "DB 38H", "DAD SP", "LDA",    "DCX SP", "INR A", "DCR A", "MVI A,", "CMC",

    "MOV B, B", "MOV B, C", "MOV B, D", "MOV B, E", "MOV B, H", "MOV B, L", "MOV B, M", "MOV B, A", "MOV C, B", "MOV C, C", "MOV C, D", "MOV C, E", "MOV C, H", "MOV C, L", "MOV C, M", "MOV C, A",
    "MOV D, B", "MOV D, C", "MOV D, D", "MOV D, E", "MOV D, H", "MOV D, L", "MOV D, M", "MOV D, A", "MOV E, B", "MOV E, C", "MOV E, D", "MOV E, E", "MOV E, H", "MOV E, L", "MOV E, M", "MOV E, A",
    "MOV H, B", "MOV H, C", "MOV H, D", "MOV H, E", "MOV H, H", "MOV H, L", "MOV H, M", "MOV H, A", "MOV L, B", "MOV L, C", "MOV L, D", "MOV L, E", "MOV L, H", "MOV L, L", "MOV L, M", "MOV L, A",
    "MOV M, B", "MOV M, C", "MOV M, D", "MOV M, E", "MOV M, H", "MOV M, L", "HLT",      "MOV M, A", "MOV A, B", "MOV A, C", "MOV A, D", "MOV A, E", "MOV A, H", "MOV A, L", "MOV A, M", "MOV A, A",

    "ADD B", "ADD C", "ADD D", "ADD E", "ADD H", "ADD L", "ADD M", "ADD A", "ADC B", "ADC C", "ADC D", "ADC E", "ADC H", "ADC L", "ADC M", "ADC A",
    "SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB M", "SUB A", "SBB B", "SBB C", "SBB D", "SBB E", "SBB H", "SBB L", "SBB M", "SBB A",
    "ANA B", "ANA C", "ANA D", "ANA E", "ANA H", "ANA L", "ANA M", "ANA A", "XRA B", "XRA C", "XRA D", "XRA E", "XRA H", "XRA L", "XRA M", "XRA A",
    "ORA B", "ORA C", "ORA D", "ORA E", "ORA H", "ORA L", "ORA M", "ORA A", "CMP B", "CMP C", "CMP D", "CMP E", "CMP H", "CMP L", "CMP M", "CMP A",

    "RNZ", "POP B",   "JNZ", "JMP",  "CNZ", "PUSH B",   "ADI", "RST 0", "RZ",  "RET",     "JZ",  "DB CBH",  "CZ",  "CALL",   "ACI", "RST 1",
    "RNC", "POP D",   "JNC", "OUT",  "CNC", "PUSH D",   "SUI", "RST 2", "RC",  "DB D9H",  "JC",  "IN",      "CC",  "DB DDH", "SBI", "RST 3",
    "RPO", "POP H",   "JPO", "XTHL", "CPO", "PUSH H",   "ANI", "RST 4", "RPE", "PCHL",    "JPE", "XCHG",    "CPE", "DB EDH", "XRI", "RST 5",
    "RP",  "POP PSW", "JP",  "DI",   "CP",  "PUSH PSW", "ORI", "RST 6", "RM",  "SPHL",    "JM",  "EI",      "CM",  "DB FDH", "CPI", "RST 7"
};


static const int cmd_len[0x100] =
{
    1,3,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
    1,3,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
    1,3,3,1,1,1,2,1,1,1,3,1,1,1,2,1,
    1,3,3,1,1,1,2,1,1,1,3,1,1,1,2,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,3,3,3,1,2,1,1,1,3,3,3,3,2,1,
    1,1,3,2,3,1,2,1,1,1,3,2,3,3,2,1,
    1,1,3,1,3,1,2,1,1,1,3,1,3,3,2,1,
    1,1,3,1,3,1,2,1,1,1,3,1,3,3,2,1
};

string i8080GetInstructionMnemonic(uint8_t* mem)
{
    uint8_t op = mem[0];
    ostringstream oss;
    oss << mnemo[op];

    unsigned operand;
    unsigned len = cmd_len[op];
    if (len == 2) {
        operand = mem[1];
        oss << " " << setw(2) << setfill('0');
        oss << uppercase << hex << operand;
    } else if (len == 3) {
        operand = mem[1] + ((uint16_t)mem[2] << 8);
        oss << " " << setw(4) << setfill('0');
        oss << uppercase << hex << operand;
    }
    return oss.str();
}

int i8080GetInstructionLength(uint8_t* mem)
{
    return cmd_len[*mem];
}
