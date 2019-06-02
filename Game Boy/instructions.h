#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H
#include "cpu.h"

/* Put value r2 into r1 */
void cpu_load(BYTE *reg);

// ADD descrp
void cpu_loadRegSP(BYTE *reg, Register *sp);

// ADD descrp
void LDHL_SP_n();

/* Add n to A */
void cpu_add(BYTE *reg1, BYTE *reg2);

/* 16-Bit add instruction */
void cpu_add16(WORD *reg1, WORD *reg2);

/* Add n + carry flag to A */
void cpu_adc(BYTE *reg1, BYTE *reg2);

/* Add the contents of the 8-bit immediate operand to register SP */
void cpu_add_sp_n(void);

/* Subtract n from A */
void cpu_sub(BYTE *reg1, BYTE *reg2);

/* Subtract n + carry flag from A */
void cpu_sbc(BYTE *reg1, BYTE *reg2);

/* Test bit b in register r */
void cpu_test_bit(BYTE b, BYTE *reg);

/* Set bit b in register r */
void cpu_set_bit(BYTE b, BYTE *reg);

void cpu_set_bit_hl(BYTE b, WORD address);

/* Reset bit b in register r */
void cpu_reset_bit(BYTE b, BYTE *reg);

void cpu_reset_bit_hl(BYTE b, WORD address);


void cpu_loadReg16(WORD *reg1, WORD *reg2);

/* Logical AND n with A */
void cpu_and(BYTE *reg1, BYTE *reg2);

/* Logical OR n with register A */
void cpu_or(BYTE *reg1, BYTE *reg2);

/* Logical exclusive OR n with register */
void cpu_xor(BYTE *reg1, BYTE *reg2);

/* Compare register A with n */
void cpu_cp(BYTE *reg);

/* Increment 8-bit register */
void cpu_inc(BYTE *reg);

/* Jump to the address of the immediate two byte value if the condition is true  */
void cpu_jump(CONDITION cond);

/* Add the immediate signed byte value to the current address and jump if the condition is true */
void cpu_jr(CONDITION cond);

/* Push address of the next instruction onto stack and then jump to address nn if the condition is true */
void cpu_call(CONDITION cond);

/* Push current address onto stack and jump to address 0x00 + n */
void cpu_rst(BYTE n);

/* Pop two bytes from stack and jump to that address */
void cpu_ret(CONDITION cond);

/* Pop two bytes from stack and jump to that address then enable interrupts */
void cpu_reti(void);

void cpu_inc_hl(WORD address);

/* Increment 16-bit register  */
void cpu_inc16(WORD *reg);

/* Decrement 8-bit register */
void cpu_dec(BYTE *reg);

void cpu_dec_hl(WORD address);

/* Decrement 16-bit register */
void cpu_dec16(WORD *reg);

/* Swap upper & lower nibbles of n */
void cpu_swap(BYTE *reg);

void cpu_swap_hl(WORD address);

/* Decimal adjust register A */
void cpu_daa(void);

/* Complement A register */
void cpu_cpl(void);

/* Complement carry flag */
void cpu_ccf(void);

/* Set carry flag */
void cpu_scf(void);

/* Power down CPU until an interrupt occurs */
void cpu_halt(void);

/* Halt CPU & LCD display until button pressed */
void cpu_stop(void);

/* Enable or disable interrupts */
void cpu_ei(int enable);

/* Rotate register A to the left. Old bit 7 to carry flag */
void cpu_rlca(void);

/* Rotate register A left through carry flag */
void cpu_rla(void);

/* Rotate register A right. Old bit 7 to carry flag */
void cpu_rrca(void);

/* Rotate register A right through carry flag */
void cpu_rra(void);

/* Rotate register left through carry flag */
void cpu_rl(BYTE *reg);

void cpu_rl_hl(WORD address);


/* Rotate register left. Old bit 7 to carry flag */
void cpu_rlc(BYTE *reg);

void cpu_rlc_hl(WORD address);

/* Rotate register right. Old bit 7 to carry flag */
void cpu_rrc(BYTE *reg);

void cpu_rrc_hl(WORD address);

/* Rotate register right through carry flag */
void cpu_rr(BYTE *reg);

void cpu_rr_hl(WORD address);

/* Shift register left into carry. LSB of n set to 0 */
void cpu_sla(BYTE *reg);

void cpu_sla_hl(WORD address);

/* Shift register right into carry. MSB doesn't change */
void cpu_sra(BYTE *reg);

void cpu_sra_hl(WORD address);

/* Shift register right into carry. MSB set to 0 */
void cpu_srl(BYTE *reg);

void cpu_srl_hl(WORD address);

/* Push two bytes of the register pair onto stack */
void stack_push(const BYTE *hi, const BYTE *lo);

/* Pop two bytes off stack into register pair and increment Stack Pointer twice */
void stack_pop(BYTE *hi, BYTE *lo);

int test_bit(BYTE b, BYTE *reg);

#endif
