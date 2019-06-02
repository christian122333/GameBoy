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

extern BYTE cartridge_memory[0x200000]; // The Game Boy cartridge holds up to 2 MB
extern BYTE rom[0x10000];
extern BYTE ram_banks[0x8000];
extern BYTE opcode;
extern WORD PC;

/*Interrupt Master Enable Flag */
extern BYTE IME;


/* There are 8 8-Bit registers from A to L, but can be paired to form 4 16-Bit registers.
The pairings are AF, BC, DE, HL. A is the accumulator and F is the flag register. */
union Register {
    WORD data;
    struct {
        BYTE lo;
        BYTE hi;
    };
}typedef Register;

extern Register RegAF;
extern Register RegBC;
extern Register RegDE;
extern Register RegHL;
extern Register RegSP;
extern BYTE *Regs[8];



typedef enum {
    NONE, NZ, Z, NC, C, HL
}CONDITION;

typedef enum {
    REGB, REGC, REGD, REGE, REGH, REGL, NO_REG, REGA
}REG;


void cpu_init(void);
void load_rom(char *filename);
int execute(void);
int CB(void);

void cpu_loadReg(BYTE *reg1, BYTE *reg2);
void write_memory(WORD address, BYTE data);
BYTE read_memory(WORD address);
void set_halt();
void reset_halt();

#endif // !1

