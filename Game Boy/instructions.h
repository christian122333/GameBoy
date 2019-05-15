#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H
#include "cpu.h"

/* Put value r2 into r1 */
void cpu_load(BYTE *reg);

/* Add n to A */
void cpu_add(BYTE *reg1, BYTE *reg2);

/* 16-Bit add instruction */
void cpu_add16(WORD *reg1, WORD *reg2);

/* Add n + carry flag to A */
void cpu_adc(BYTE *reg1, BYTE *reg2);

/* Subtract n from A */
void cpu_sub(BYTE *reg1, BYTE *reg2);

/* Subtract n + carry flag from A */
void cpu_sbc(BYTE *reg1, BYTE *reg2);

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

/* Test bit b in register r */
void cpu_test_bit(BYTE b, BYTE *reg);

/* Set bit b in register r */
void cpu_set_bit(BYTE b, BYTE *reg);

/* Reset bit b in register r */
void cpu_reset_bit(BYTE b, BYTE *reg);

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

/* Increment 16-bit register  */
void cpu_inc16(WORD *reg);

/* Decrement 8-bit register */
void cpu_dec(BYTE *reg);

/* Decrement 16-bit register */
void cpu_dec16(WORD *reg);

/* Swap upper & lower nibbles of n */
void cpu_swap(BYTE *reg);

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

/* Rotate register left through carry flag */
void cpu_rl(BYTE *reg);

/* Rotate register left. Old bit 7 to carry flag */
void cpu_rlc(BYTE *reg);

/* Rotate register right. Old bit 7 to carry flag */
void cpu_rrc(BYTE *reg);

/* Rotate register right through carry flag */
void cpu_rr(BYTE *reg);

/* Shift register left into carry. LSB of n set to 0 */
void cpu_sla(BYTE *reg);

/* Shift register right into carry. MSB doesn't change */
void cpu_sra(BYTE *reg);

/* Shift register right into carry. MSB set to 0 */
void cpu_srl(BYTE *reg);

/* Push two bytes of the register pair onto stack */
void stack_push(const BYTE *hi, const BYTE *lo);

/* Pop two bytes off stack into register pair and increment Stack Pointer twice */
void stack_pop(BYTE *hi, BYTE *lo);
#endif