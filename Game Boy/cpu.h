#ifndef CPU_H
#define CPU_H

/* Each value is the bit in the F register for that flag */
#define FLAG_Z 7 // Zero flag
#define FLAG_N 6 // Subtract flag
#define FLAG_H 5 // Half carry flag
#define FLAG_C 4 // Carry flag


typedef unsigned char BYTE;
typedef char SIGNED_BYTE;
typedef unsigned short WORD;
typedef signed short SIGNED_WORD;

void cpu_init(void);
void load_rom(char *filename);
void fetch(void);
void execute(void);
void bank_mode(void);
void cb_set(void);
void cpu_load(BYTE *reg);
void cpu_add(BYTE *reg1, BYTE *reg2);
void cpu_add16(WORD *reg1, WORD *reg2);
void cpu_adc(BYTE *reg1, BYTE *reg2);
void cpu_sub(BYTE *reg1, BYTE *reg2);
void cpu_sbc(BYTE *reg1, BYTE *reg2);
void cpu_and(BYTE *reg1, BYTE *reg2);
void cpu_or(BYTE *reg1, BYTE *reg2);
void cpu_xor(BYTE *reg1, BYTE *reg2);
void cpu_cp(BYTE *reg);
void cpu_inc(BYTE *reg);

/* Increment register nn */
void cpu_inc16(WORD *reg);

/* Decrement register nn */
void cpu_dec(BYTE *reg);

/* Swap upper & lower nibbles of n */
void cpu_swap(BYTE *reg);

void cpu_daa(void);
void cpu_cpl(void);
void cpu_ccf(void);
void cpu_scf(void);
void cpu_halt(void);
void cpu_stop(void);
void cpu_ei(int enable);
void cpu_rl(BYTE *reg);
void cpu_rr(BYTE *reg);
void stack_push(const BYTE *hi, const BYTE *lo);
void stack_pop(BYTE *hi, BYTE *lo);
void cpu_loadReg(BYTE *reg1, BYTE *reg2);
void write_memory(WORD address, BYTE data);
BYTE read_memory(WORD address);

#endif // !1

