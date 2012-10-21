#include "asm/symbol.h"
#include "asm/lexer.h"
#include "asm/rpn.h"

#include "../asmy.h"

struct sLexInitString localstrings[] = {
	{"adc", T_Z80_ADC},
	{"add", T_Z80_ADD},
	{"and", T_Z80_AND},
	{"bit", T_Z80_BIT},
	{"call", T_Z80_CALL},
	{"ccf", T_Z80_CCF},
	{"cpl", T_Z80_CPL},
	{"cp", T_Z80_CP},
	{"daa", T_Z80_DAA},
	{"dec", T_Z80_DEC},
	{"di", T_Z80_DI},
	{"ei", T_Z80_EI},
	{"ex", T_Z80_EX},
	{"halt", T_Z80_HALT},
	{"inc", T_Z80_INC},
	{"jp", T_Z80_JP},
	{"jr", T_Z80_JR},
	{"ld", T_Z80_LD},
	{"ldi", T_Z80_LDI},
	{"ldd", T_Z80_LDD},
	{"ldio", T_Z80_LDIO},
	{"ldh", T_Z80_LDIO},
	{"nop", T_Z80_NOP},
	{"or", T_Z80_OR},
	{"pop", T_Z80_POP},
	{"push", T_Z80_PUSH},
	{"res", T_Z80_RES},
	{"reti", T_Z80_RETI},
	{"ret", T_Z80_RET},
	{"rlca", T_Z80_RLCA},
	{"rlc", T_Z80_RLC},
	{"rla", T_Z80_RLA},
	{"rl", T_Z80_RL},
	{"rrc", T_Z80_RRC},
	{"rrca", T_Z80_RRCA},
	{"rra", T_Z80_RRA},
	{"rr", T_Z80_RR},
	{"rst", T_Z80_RST},
	{"sbc", T_Z80_SBC},
	{"scf", T_Z80_SCF},

	/* Handled by globallex.c */
	/* { "set", T_POP_SET }, */

	{"sla", T_Z80_SLA},
	{"sra", T_Z80_SRA},
	{"srl", T_Z80_SRL},
	{"stop", T_Z80_STOP},
	{"sub", T_Z80_SUB},
	{"swap", T_Z80_SWAP},
	{"xor", T_Z80_XOR},

	{"nz", T_CC_NZ},
	{"z", T_CC_Z},
	{"nc", T_CC_NC},
	/* { "c", T_MODE_C }, */

	{"[hl]", T_MODE_HL_IND},
	{"[hl+]", T_MODE_HL_INDINC},
	{"[hl-]", T_MODE_HL_INDDEC},
	{"[hli]", T_MODE_HL_INDINC},
	{"[hld]", T_MODE_HL_INDDEC},
	{"hl", T_MODE_HL},
	{"af", T_MODE_AF},
	{"[bc]", T_MODE_BC_IND},
	{"bc", T_MODE_BC},
	{"[de]", T_MODE_DE_IND},
	{"de", T_MODE_DE},
	{"[sp]", T_MODE_SP_IND},
	{"sp", T_MODE_SP},
	{"a", T_MODE_A},
	{"b", T_MODE_B},
	{"[$ff00+c]", T_MODE_C_IND},
	{"[c]", T_MODE_C_IND},
	{"c", T_MODE_C},
	{"d", T_MODE_D},
	{"e", T_MODE_E},
	{"h", T_MODE_H},
	{"l", T_MODE_L},

	{NULL, 0}
};
