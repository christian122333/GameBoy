#include "cpu.h"
#include <stdio.h>

BYTE cartridge_memory[0x200000]; // The Game Boy cartridge holds up to 2 MB
BYTE rom[0x10000];
BYTE ram_banks[0x8000];
BYTE opcode;

WORD PC;
/*  General Memory Map
    0000-3FFF 16KB ROM Bank 00
    4000-7FFF 16KB ROM Bank 01
    8000-9FFF 8KB VRAM
    A000-BFFF 8KB External RAM
    C000-CFFF 4KB Work RAM Bank 0 (WRAM)
    D000-DFFF 4KB Work RAM Bank 1 (WRAM)
    E000-EFFF Same as C000-DFFF (ECHO)
    FE00-FE9F Sprite Attribute Table (OAM)
    FEA0-FEFF Not Usable
    FF00-FF7F I/O Ports
    FF80-FFFE High RAM (HRAM)
    FFFF Interrupt Enable Register
*/

/* There are 8 8-Bit registers from A to L, but can be paired to form 4 16-Bit registers.
The pairings are AF, BC, DE, HL. A is the accumulator and F is the flag register. */
union Register {
    WORD data;
    struct {
        BYTE lo;
        BYTE hi;
    };
}typedef Register;

Register RegAF;
Register RegBC;
Register RegDE;
Register RegHL;
Register RegSP; // Stack pointer

void cpu_init() {
    PC = 0x100;
    opcode = 0;
    RegAF.data = 0x01B0;
    RegBC.data = 0x0013;
    RegDE.data = 0x00D8;
    RegHL.data = 0x014D;
    RegSP.data = 0xFFFE;
    for (int index = 0; index < 0x10000; index++) {
        rom[index] = 0;
    }
    rom[0xFF10] = 0x80;
    rom[0xFF11] = 0xBF;
    rom[0xFF12] = 0xF3;
    rom[0xFF14] = 0xBF;
    rom[0xFF16] = 0x3F;
    rom[0xFF19] = 0xBF;
    rom[0xFF1A] = 0x7F;
    rom[0xFF1B] = 0xFF;
    rom[0xFF1C] = 0x9F;
    rom[0xFF1E] = 0xBF;
    rom[0xFF20] = 0xFF;
    rom[0xFF23] = 0xBF;
    rom[0xFF24] = 0x77;
    rom[0xFF25] = 0xF3;
    rom[0xFF26] = 0xF1;
    rom[0xFF40] = 0x91;
    rom[0xFF47] = 0xFC;
    rom[0xFF48] = 0xFF;
    rom[0xFF49] = 0xFF;
}
void load_rom(char *filename) {
    FILE *fp;
    fp = open(filename, "rb");
    fread(cartridge_memory, 1, 0x200000, fp);
    fclose(fp);
}

void execute() {
    WORD nn;

    while (1) {
    NOP:       // 0x00
        return;
    LD_BC_nn:       // 0x01
        nn = read_memory(PC++) << 8 | read_memory(PC);
        cpu_loadReg(&RegBC.data, &nn);
        return;
    LD_BC_A:       // 0x02
        cpu_loadReg(&cartridge_memory[RegBC.data], &RegAF.hi);
        return;
    INC16_BC:       // 0x03
        cpu_inc16(&RegBC.data);
        return;
    INC_B:       // 0x04
        cpu_inc(&RegBC.hi);
        return;
    DEC_B:       // 0x05
        cpu_dec(&RegBC.hi);
        return;
    LD_B_n:       // 0x06
        cpu_load(&RegAF.hi);
        return;
    RCLA:       // 0x07
        cpu_rl(&RegAF.hi);
        return;
    NOP:       // 0x08
        return;
    ADD16_HL_BC:       // 0x09
        cpu_add16(&RegHL.data, &RegBC.data);
        return;
    LD_A_BC:       // 0x0A
        cpu_loadReg(&RegAF.hi, &cartridge_memory[RegBC.data]);
        return;
    DEC_BC:       // 0x0B
        cpu_dec16(&RegBC.data);
        return;
    INC_C:       // 0x0C
        cpu_inc(&RegBC.lo);
        return;
    DEC_C:       // 0x0D
        cpu_dec(&RegBC.lo);
        return;
    LD_C_n:       // 0x0E
        cpu_load(&RegBC.lo);
        return;
    RRCA:       // 0x0F
        cpu_rr(&RegAF.hi);
        return;
    STOP:       // 0x10
        cpu_stop();
        return;
    LD_DE_nn:       // 0x11
        nn = read_memory(PC);
        nn |= read_memory(++PC) << 8;
        cpu_loadReg(&RegDE.data, &nn);
        return;
    LD_DE_A:       // 0x12
        cpu_loadReg(&cartridge_memory[RegDE.data], &RegAF.hi);
        return;
    INC16_DE:       // 0x13
        cpu_inc16(&RegDE.data);
        return;
    INC_D:       // 0x14
        cpu_inc(&RegDE.hi);
        return;
    DEC_D:       // 0x15
        cpu_dec(&RegDE.hi);
        return;
    LD_D_n:       // 0x16
        cpu_load(&RegDE.hi);
        return;
    RLA:       // 0x17
        cpu_rl(&RegAF.hi);
        return;
    JR_n:       // 0x18
        cpu_jr(NONE);
        return;
    ADD16_HL_DE:       // 0x19
        cpu_add16(&RegHL.data, &RegDE.data);
        return;
    LD_A_DE:       // 0x1A
        cpu_loadReg(&RegAF.hi, &cartridge_memory[RegDE.data]);
        return;
    DEC_DE:       // 0x1B
        cpu_dec16(&RegDE.data);
        return;
    INC_E:       // 0x1C
        cpu_inc(&RegDE.lo);
        return;
    DEC_E:       // 0x1D
        cpu_dec(&RegDE.lo);
        return;
    LD_E_n:       // 0x1E
        cpu_load(&RegDE.lo);
        return;
    RRA:       // 0x1F
        cpu_rr(&RegAF.hi);
        return;
    JR_NZ:       // 0x20
        cpu_jr(NZ);
        return;
    LD_HL_nn:       // 0x21
        nn = read_memory(PC++) << 8 | read_memory(PC);
        cpu_loadReg(&RegHL.data, &nn);
        return;
    NOP:       // 0x22
        return;
    INC16_HL:       // 0x23
        cpu_inc16(&RegHL.data);
        return;
    INC_H:       // 0x24
        cpu_inc(&RegHL.hi);
        return;
    DEC_H:       // 0x25
        cpu_dec(&RegHL.hi);
        return;
    LD_H_n:       // 0x26
        cpu_load(&RegHL.hi);
        return;
    DAA:       // 0x27
        cpu_daa();
        return;
    JR_Z:       // 0x28
        cpu_jr(Z);
        return;
    ADD16_HL_HL:       // 0x29
        cpu_add16(&RegHL.data, &RegHL.data);
        return;
    NOP:       // 0x2A
        return;
    DEC_HL:       // 0x2B
        cpu_dec16(&RegHL.data);
        return;
    INC_L:       // 0x2C
        cpu_inc(&RegHL.lo);
        return;
    DEC_L:       // 0x2D
        cpu_dec(&RegHL.lo);
        return;
    LD_L_n:       // 0x2E
        cpu_load(&RegHL.lo);
        return;
    CPL:       // 0x2F
        cpu_cpl();
        return;
    JR_NC:       // 0x30
        cpu_jr(NC);
        return;
    LD_SP_nn:       // 0x31
        nn = read_memory(PC++) << 8 | read_memory(PC);
        cpu_loadReg(&RegSP, &nn);
        return;
    NOP:       // 0x32
        return;
    INC16_SP:       // 0x33
        cpu_inc16(&RegSP.data);
        return;
    INC_HL:       // 0x34
        cpu_inc(&RegHL.lo); // EDIT
        return;
    DEC_HL:       // 0x35
        cpu_dec(&RegHL.lo); // EDIT
        return;
    NOP:       // 0x36
        return;
    SCF:       // 0x37
        cpu_scf();
        return;
    JR_C:       // 0x38
        cpu_jr(C);
        return;
    ADD16_HL_SP:       // 0x39
        cpu_add16(&RegHL.data, &RegSP.data);
        return;
    NOP:       // 0x3A
        return;
    DEC_SP:       // 0x3B
        cpu_dec16(&RegSP.data);
        return;
    INC_A:       // 0x3C
        cpu_inc(&RegAF.hi);
        return;
    DEC_A:       // 0x3D
        cpu_dec(&RegAF.hi);
        return;
    LD_A_n:       // 0x3E
        cpu_load(&RegAF.hi);
        return;
    CCF:       // 0x3F
        cpu_ccf();
        return;
    LD_B_B:       // 0x40
        cpu_loadReg(&RegBC.hi, &RegBC.hi);
        return;
    LD_B_C:       // 0x41
        cpu_loadReg(&RegBC.hi, &RegBC.lo);
        return;
    LD_B_D:       // 0x42
        cpu_loadReg(&RegBC.hi, &RegDE.hi);
        return;
    LD_B_E:       // 0x43
        cpu_loadReg(&RegBC.hi, &RegDE.lo);
        return;
    LD_B_H:       // 0x44
        cpu_loadReg(&RegBC.hi, &RegHL.hi);
        return;
    LD_B_L:       // 0x45
        cpu_loadReg(&RegBC.hi, &RegHL.lo);
        return;
    LD_B_HL:       // 0x46
        cpu_loadReg(&RegBC.hi, &cartridge_memory[RegHL.data]);
        return;
    LD_B_A:       // 0x47
        cpu_loadReg(&RegBC.hi, &RegAF.hi);
        return;
    LD_C_B:       // 0x48
        cpu_loadReg(&RegBC.lo, &RegBC.hi);
        return;
    LD_C_C:       // 0x49
        cpu_loadReg(&RegBC.lo, &RegBC.lo);
        return;
    LD_C_D:       // 0x4A
        cpu_loadReg(&RegBC.lo, &RegDE.hi);
        return;
    LD_C_E:       // 0x4B
        cpu_loadReg(&RegBC.lo, &RegDE.lo);
        return;
    LD_C_H:       // 0x4C
        cpu_loadReg(&RegBC.lo, &RegHL.hi);
        return;
    LD_C_L:       // 0x4D
        cpu_loadReg(&RegBC.lo, &RegHL.lo);
        return;
    LD_C_HL:       // 0x4E
        cpu_loadReg(&RegBC.lo, &cartridge_memory[RegHL.data]);
        return;
    LD_C_A:     // 0x4F
        cpu_loadReg(&RegBC.lo, &RegAF.hi);
        return;
    LD_D_B:       // 0x50
        cpu_loadReg(&RegDE.hi, &RegBC.hi);
        return;
    LD_D_C:       // 0x51
        cpu_loadReg(&RegDE.hi, &RegBC.lo);
        return;
    LD_D_D:       // 0x52
        cpu_loadReg(&RegDE.hi, &RegDE.hi);
        return;
    LD_D_E:       // 0x53
        cpu_loadReg(&RegDE.hi, &RegDE.lo);
        return;
    LD_D_H:       // 0x54
        cpu_loadReg(&RegDE.hi, &RegHL.hi);
        return;
    LD_D_L:       // 0x55
        cpu_loadReg(&RegDE.hi, &RegHL.lo);
        return;
    LD_D_HL:       // 0x56
        cpu_loadReg(&RegDE.hi, &cartridge_memory[RegHL.data]);
        return;
    LD_D_A:       // 0x57
        cpu_loadReg(&RegDE.hi, &RegAF.hi);
        return;
    LD_E_B:       // 0x58
        cpu_loadReg(&RegDE.lo, &RegBC.hi);
        return;
    LD_E_C:       // 0x59
        cpu_loadReg(&RegDE.lo, &RegBC.lo);
        return;
    LD_E_D:       // 0x5A
        cpu_loadReg(&RegDE.lo, &RegDE.hi);
        return;
    LD_E_E:       // 0x5B
        cpu_loadReg(&RegDE.lo, &RegDE.lo);
        return;
    LD_E_H:       // 0x5C
        cpu_loadReg(&RegDE.lo, &RegHL.hi);
        return;
    LD_E_L:       // 0x5D
        cpu_loadReg(&RegDE.lo, &RegHL.lo);
        return;
    LD_E_HL:       // 0x5E
        cpu_loadReg(&RegDE.hi, &cartridge_memory[RegHL.data]);
        return;
    LD_E_A:       // 0x5F
        cpu_loadReg(&RegDE.lo, &RegAF.hi);
        return;
    LD_H_B:       // 0x60
        cpu_loadReg(&RegHL.hi, &RegBC.hi);
        return;
    LD_H_C:       // 0x61
        cpu_loadReg(&RegHL.hi, &RegBC.lo);
        return;
    LD_H_D:       // 0x62
        cpu_loadReg(&RegHL.hi, &RegDE.hi);
        return;
    LD_H_E:       // 0x63
        cpu_loadReg(&RegHL.hi, &RegDE.lo);
        return;
    LD_H_H:       // 0x64
        cpu_loadReg(&RegHL.hi, &RegHL.hi);
        return;
    LD_H_L:       // 0x65
        cpu_loadReg(&RegHL.hi, &RegHL.lo);
        return;
    LD_H_HL:       // 0x66
        cpu_loadReg(&RegHL.hi, &cartridge_memory[RegHL.data]);
        return;
    LD_H_A:       // 0x67
        cpu_loadReg(&RegHL.hi, &RegAF.hi);
        return;
    LD_L_B:       // 0x68
        cpu_loadReg(&RegHL.lo, &RegBC.hi);
        return;
    LD_L_C:       // 0x69
        cpu_loadReg(&RegHL.lo, &RegBC.lo);
        return;
    LD_L_D:       // 0x6A
        cpu_loadReg(&RegHL.lo, &RegDE.hi);
        return;
    LD_L_E:       // 0x6B
        cpu_loadReg(&RegHL.lo, &RegDE.lo);
        return;
    LD_L_H:       // 0x6C
        cpu_loadReg(&RegHL.lo, &RegHL.hi);
        return;
    LD_L_L:       // 0x6D
        cpu_loadReg(&RegHL.lo, &RegHL.lo);
        return;
    LD_L_HL:       // 0x6E
        cpu_loadReg(&RegHL.lo, &cartridge_memory[RegHL.data]);
        return;
    LD_L_A:       // 0x6F
        cpu_loadReg(&RegHL.lo, &RegAF.hi);
        return;
    LD_HL_B:       // 0x70
        cpu_loadReg(&cartridge_memory[RegHL.data], &RegBC.hi);
        return;
    LD_HL_C:       // 0x71
        cpu_loadReg(&cartridge_memory[RegHL.data], &RegBC.lo);
        return;
    LD_HL_D:       // 0x72
        cpu_loadReg(&cartridge_memory[RegHL.data], &RegDE.hi);
        return;
    LD_HL_E:       // 0x73
        cpu_loadReg(&cartridge_memory[RegHL.data], &RegDE.lo);
        return;
    LD_HL_H:       // 0x74
        cpu_loadReg(&cartridge_memory[RegHL.data], &RegHL.hi);
        return;
    LD_HL_L:       // 0x75
        cpu_loadReg(&cartridge_memory[RegHL.data], &RegHL.lo);
        return;
    HALT:       // 0x76
        cpu_halt();
        return;
    LD_HL_A:       // 0x77
        cpu_loadReg(&cartridge_memory[RegHL.data], &RegAF.hi);
        return;
    LD_A_B:       // 0x78
        cpu_loadReg(&RegAF.hi, &RegBC.hi);
        return;
    LD_A_C:       // 0x79
        cpu_loadReg(&RegAF.hi, &RegBC.lo);
        return;
    LD_A_D:       // 0x7A
        cpu_loadReg(&RegAF.hi, &RegDE.hi);
        return;
    LD_A_E:       // 0x7B
        cpu_loadReg(&RegAF.hi, &RegDE.lo);
        return;
    LD_A_H:       // 0x7C
        cpu_loadReg(&RegAF.hi, &RegHL.hi);
        return;
    LD_A_L:       // 0x7D
        cpu_loadReg(&RegAF.hi, &RegHL.lo);
        return;
    LD_A_HL:       // 0x7E
        cpu_loadReg(&RegAF.hi, &cartridge_memory[RegHL.data]);
        return;
    LD_A_A:       // 0x7F
        cpu_loadReg(&RegAF.hi, &RegAF.hi);
        return;
    ADD_A_B:       // 0x80
        cpu_add(&RegAF.hi, &RegBC.hi);
        return;
    ADD_A_C:       // 0x81
        cpu_add(&RegAF.hi, &RegBC.lo);
        return;
    ADD_A_D:       // 0x82
        cpu_add(&RegAF.hi, &RegDE.hi);
        return;
    ADD_A_E:       // 0x83
        cpu_add(&RegAF.hi, &RegDE.lo);
        return;
    ADD_A_H:       // 0x84
        cpu_add(&RegAF.hi, &RegHL.hi);
        return;
    ADD_A_L:       // 0x85
        cpu_add(&RegAF.hi, &RegHL.lo);
        return;
    ADD_A_HL:       // 0x86
        cpu_add(&RegAF.hi, &cartridge_memory[RegHL.data]);
        return;
    ADD_A_A:       // 0x87
        cpu_add(&RegAF.hi, &RegAF.hi);
        return;
    ADC_A_B:       // 0x88 //
        cpu_adc(&RegAF.hi, &RegBC.hi);
        return;
    ADC_A_C:       // 0x89
        cpu_adc(&RegAF.hi, &RegBC.lo);
        return;
    ADC_A_D:       // 0x8A
        cpu_adc(&RegAF.hi, &RegDE.hi);
        return;
    ADC_A_E:       // 0x8B
        cpu_adc(&RegAF.hi, &RegDE.lo);
        return;
    ADC_A_H:       // 0x8C
        cpu_adc(&RegAF.hi, &RegHL.hi);
        return;
    ADC_A_L:       // 0x8D
        cpu_adc(&RegAF.hi, &RegHL.lo);
        return;
    ADC_A_HL:       // 0x8E
        cpu_adc(&RegAF.hi, &cartridge_memory[RegAF.data]);
        return;
    ADC_A_A:       // 0x8F
        cpu_adc(&RegAF.hi, &RegAF.hi);
        return;
    SUB_B:       // 0x90
        cpu_sub(&RegAF.hi, &RegBC.hi);
        return;
    SUB_C:       // 0x91
        cpu_sub(&RegAF.hi, &RegBC.lo);
        return;
    SUB_D:       // 0x92
        cpu_sub(&RegAF.hi, &RegDE.hi);
        return;
    SUB_E:       // 0x93
        cpu_sub(&RegAF.hi, &RegDE.lo);
        return;
    SUB_H:       // 0x94
        cpu_sub(&RegAF.hi, &RegHL.hi);
        return;
    SUB_L:       // 0x95
        cpu_sub(&RegAF.hi, &RegHL.lo);
        return;
    SUB_HL:       // 0x96
        cpu_sub(&RegAF.hi, &cartridge_memory[RegAF.data]); // EDIT
        return;
    SUB_A:       // 0x97
        cpu_sub(&RegAF.hi, &RegAF.hi);
        return;
    SBC_A_B:       // 0x98
        cpu_sbc(&RegAF.hi, &RegBC.hi);
        return;
    SBC_A_C:       // 0x99
        cpu_sbc(&RegAF.hi, &RegBC.lo);
        return;
    SBC_A_D:       // 0x9A
        cpu_sbc(&RegAF.hi, &RegDE.hi);
        return;
    SBC_A_E:       // 0x9B
        cpu_sbc(&RegAF.hi, &RegDE.lo);
        return;
    SBC_A_H:       // 0x9C
        cpu_sbc(&RegAF.hi, &RegHL.hi);
        return;
    SBC_A_L:       // 0x9D
        cpu_sbc(&RegAF.hi, &RegHL.lo);
        return;
    SBC_A_HL:       // 0x9E
        cpu_sbc(&RegAF.hi, &cartridge_memory[RegAF.data]); // EDIT
        return;
    SBC_A_A:       // 0x9F
        cpu_sbc(&RegAF.hi, &RegAF.hi);
        return;
    AND_B:       // 0xA0
        cpu_and(&RegAF.hi, &RegBC.hi);
        return;
    AND_C:       // 0xA1
        cpu_and(&RegAF.hi, &RegBC.lo);
        return;
    AND_D:       // 0xA2
        cpu_and(&RegAF.hi, &RegDE.hi);
        return;
    AND_E:       // 0xA3
        cpu_and(&RegAF.hi, &RegDE.lo);
        return;
    AND_H:       // 0xA4
        cpu_and(&RegAF.hi, &RegHL.hi);
        return;
    AND_L:       // 0xA5
        cpu_and(&RegAF.hi, &RegHL.lo);
        return;
    AND_HL:       // 0xA6
        cpu_and(&RegAF.hi, &cartridge_memory[RegAF.data]); // EDIT
        return;
    AND_A:       // 0xA7
        cpu_and(&RegAF.hi, &RegAF.hi);
        return;
    XOR_B:       // 0xA8
        cpu_xor(&RegAF.hi, &RegBC.hi);
        return;
    XOR_C:       // 0xA9
        cpu_xor(&RegAF.hi, &RegBC.lo);
        return;
    XOR_D:       // 0xAA
        cpu_xor(&RegAF.hi, &RegDE.hi);
        return;
    XOR_E:       // 0xAB
        cpu_xor(&RegAF.hi, &RegDE.lo);
        return;
    XOR_H:       // 0xAC
        cpu_xor(&RegAF.hi, &RegHL.hi);
        return;
    XOR_L:       // 0xAD
        cpu_xor(&RegAF.hi, &RegHL.lo);
        return;
    XOR_HL:       // 0xAE
        cpu_xor(&RegAF.hi, &RegHL.data);    // EDIT
        return;
    XOR_A:       // 0xAF
        cpu_xor(&RegAF.hi, &RegAF.hi);
        return;
    OR_B:       // 0xB0
        cpu_or(&RegAF.hi, &RegBC.hi);
        return;
    OR_C:       // 0xB1
        cpu_or(&RegAF.hi, &RegBC.lo);
        return;
    OR_D:       // 0xB2
        cpu_or(&RegAF.hi, &RegDE.hi);
        return;
    OR_E:       // 0xB3
        cpu_or(&RegAF.hi, &RegDE.lo);
        return;
    OR_H:       // 0xB4
        cpu_or(&RegAF.hi, &RegHL.hi);
        return;
    OR_L:       // 0xB5
        cpu_or(&RegAF.hi, &RegHL.lo);
        return;
    OR_HL:       // 0xB6
        cpu_or(&RegAF.hi, &RegHL.data); // EDIT
        return;
    OR_A:       // 0xB7
        cpu_or(&RegAF.hi, &RegAF.hi);
        return;
    CP_B:       // 0xB8
        cpu_cp(&RegBC.hi);
        return;
    CP_C:       // 0xB9
        cpu_cp(&RegBC.lo);
        return;
    CP_D:       // 0xBA
        cpu_cp(&RegDE.hi);
        return;
    CP_E:       // 0xBB
        cpu_cp(&RegDE.lo);
        return;
    CP_H:       // 0xBC
        cpu_cp(&RegHL.hi);
        return;
    CP_L:       // 0xBD
        cpu_cp(&RegHL.lo);
        return;
    CP_HL:       // 0xBE
        cpu_cp(&RegHL.data); // EDIT
        return;
    CP_A:       // 0xBF
        cpu_cp(&RegAF.hi);
        return;
    RET_NZ:       // 0xC0
        cpu_ret(NZ);
        return;
    POP_BC:       // 0xC1
        stack_pop(&RegBC.hi, &RegBC.lo);
        return;
    JP_NZ_nn:       // 0xC2
        cpu_jump(NZ);
        return;
    JP_nn:       // 0xC3
        cpu_jump(NONE);
        return;
    CALL_nz:       // 0xC4
        cpu_call(NZ);
        return;
    PUSH_BC:       // 0xC5
        stack_push(&RegBC.hi, &RegBC.lo);
        return;
    ADD_A_n:       // 0xC6
        cpu_add(&RegAF.hi, &RegBC.hi); // EDIT
        return;
    RST_00:       // 0xC7
        cpu_rst(0x00);
        return;
    RET_Z:       // 0xC8
        cpu_ret(Z);
        return;
    RET:       // 0xC9
        cpu_ret(NONE);
        return;
    JP_Z_nn:       // 0xCA
        cpu_jump(Z);
        return;
    CB:       // 0xCB
        cb_set();
        return;
    CALL_Z:       // 0xCC
        cpu_call(Z);
        return;
    CALL_nn:       // 0xCD
        cpu_call(NONE);
        return;
    ADC_A_n:       // 0xCE
        cpu_add(&RegAF.hi, &RegBC.hi); // EDIT
        return;
    RST_08:       // 0xCF
        cpu_rst(0x08);
        return;
    RET_NC:       // 0xD0
        cpu_ret(NC);
        return;
    POP_DE:       // 0xD1
        stack_pop(&RegDE.hi, &RegDE.lo);
        return;
    JP_NC_nn:       // 0xD2
        cpu_jump(NC);
        return;
    NOP:       // 0xD3
        return;
    CALL_NC:       // 0xD4
        cpu_call(NC);
        return;
    PUSH_DE:       // 0xD5
        stack_push(&RegDE.hi, &RegDE.lo);
        return;
    SUB_n:       // 0xD6
        cpu_sub(&RegAF.hi, &RegHL.hi); // EDIT
        return;
    RST_10:       // 0xD7
        cpu_rst(0x10);
        return;
    RET_C:       // 0xD8
        cpu_ret(C);
        return;
    RETI:       // 0xD9
        cpu_reti();
        return;
    JP_C_nn:       // 0xDA
        cpu_jump(C);
        return;
    NOP:       // 0xDB
        return;
    CALL_C:       // 0xDC
        cpu_call(C);
        return;
    NOP:       // 0xDD
        return;
    NOP:       // 0xDE
        return;
    RST_18:       // 0xDF
        cpu_rst(0x18);
        return;
    NOP:       // 0xE0
        return;
    POP_HL:       // 0xE1
        stack_pop(&RegHL.hi, &RegHL.lo);
        return;
    LD_INDEX_C_A:       // 0xE2
        cpu_loadReg(cartridge_memory[0xFF00 + RegBC.lo], &RegAF.hi);
        return;
    NOP:       // 0xE3
        return;
    NOP:       // 0xE4
        return;
    PUSH_HL:       // 0xE5
        stack_push(&RegHL.hi, &RegHL.lo);
        return;
    AND_n:       // 0xE6
        cpu_and(&RegAF.hi, &RegHL.lo); // EDIT
        return;
    RST_20:       // 0xE7
        cpu_rst(0x20);
        return;
    ADD_SP_n:       // 0xE8
        cpu_add16(&RegSP.data, &RegHL.data); // EDIT
        return;
    JP_HL:       // 0xE9
        cpu_jump(HL);
        return;
    NOP:       // 0xEA
        return;
    NOP:       // 0xEB
        return;
    NOP:       // 0xEC
        return;
    NOP:       // 0xED
        return;
    XOR_n:       // 0xEE
        cpu_xor(&RegAF.hi, &RegHL.data); // EDIT
        return;
    RST_28:       // 0xEF
        cpu_rst(0x28);
        return;
    NOP:       // 0xF0
        return;
    POP_AF:       // 0xF1
        stack_pop(&RegAF.hi, &RegAF.lo);
        return;
    LD_A_INDEX_C:       // 0xF2
        cpu_loadReg(&RegAF.hi, cartridge_memory[0xFF00 + RegBC.lo]);
        return;
    DI:       // 0xF3
        cpu_ei(0);
        return;
    NOP:       // 0xF4
        return;
    PUSH_AF:       // 0xF5
        stack_push(&RegAF.hi, &RegAF.lo);
        return;
    OR_n:       // 0xF6
        cpu_or(&RegAF.hi, &RegBC.hi); // EDIT
        return;
    RST_30:       // 0xF7
        cpu_rst(0x30);
        return;
    NOP:       // 0xF8
        return;
    LD_SP_HL:       // 0xF9
        cpu_loadReg(&RegSP.data, &RegHL.data);
        return;
    NOP:       // 0xFA
        return;
    EI:       // 0xFB
        cpu_ei(0);
        return;
    NOP:       // 0xFC
        return;
    NOP:       // 0xFD
        return;
    CP_n:       // 0xFE
        cpu_cp(&RegHL.data); // EDIT
        return;
    RST_38:       // 0xFF
        cpu_rst(0x38);
        return;
    }
}

void CB_SET(){
RLC_B:      // 00
    cpu_rl(&RegBC.hi);
    return;
RLC_C:      // 01
    cpu_rl(&RegBC.lo);
    return;
RLC_D:      // 02
    cpu_rl(&RegDE.hi);
    return;
RLC_E:      // 03
    cpu_rl(&RegDE.lo);
    return;
RLC_H:      // 04
    cpu_rl(&RegHL.hi);
    return;
RLC_L:      // 05
    cpu_rl(&RegHL.lo);
    return;
RLC_HL:     // 06
    cpu_rl(&RegBC.hi); // EDit
    return;
RLC_A:      // 07
    cpu_rl(&RegBC.hi);
    return;
SWAP_B:     // 30
    cpu_swap(&RegBC.hi);
    return;
SWAP_C:     // 31
    cpu_swap(&RegBC.lo);
    return;
SWAP_D:     // 32
    cpu_swap(&RegDE.hi);
    return;
SWAP_E:     // 33
    cpu_swap(&RegDE.lo);
    return;
SWAP_H:     // 34
    cpu_swap(&RegHL.hi);
    return;
SWAP_L:     // 35
    cpu_swap(&RegHL.lo);
    return;
SWAP_HL:    // 36
    cpu_swap(&RegHL.lo); // EDIT
    return;
SWAP_A:     // 37
    cpu_swap(&RegAF.hi);
    return;
NOP:       // 0x38
    return;
NOP:       // 0x39
    return;
NOP:       // 0x3A
    return;
NOP:       // 0x3B
    return;
NOP:       // 0x3C
    return;
NOP:       // 0x3D
    return;
NOP:       // 0x3E
    return;
NOP:       // 0x3F
    return;
BIT_0_B:       // 0x40
    cpu_test_bit(0, RegBC.hi);
    return;
BIT_0_C:      // 0x41
    cpu_test_bit(0, RegBC.lo);
    return;
BIT_0_D:       // 0x42
    cpu_test_bit(0, RegDE.hi);
    return;
BIT_0_E:       // 0x43
    cpu_test_bit(0, RegDE.lo);
    return;
BIT_0_H:       // 0x44
    cpu_test_bit(0, RegHL.hi);
    return;
BIT_0_L:       // 0x45
    cpu_test_bit(0, RegHL.lo);
    return;
BIT_0_HL:       // 0x46
    cpu_test_bit(0, RegHL.data); // edit
    return;
BIT_0_A:       // 0x47
    cpu_test_bit(0, RegAF.hi);
    return;
BIT_1_B:       // 0x48
    cpu_test_bit(1, RegBC.hi);
    return;
BIT_1_C:      // 0x49
    cpu_test_bit(1, RegBC.lo);
    return;
BIT_1_D:       // 0x4A
    cpu_test_bit(1, RegDE.hi);
    return;
BIT_1_E:       // 0x4B
    cpu_test_bit(1, RegDE.lo);
    return;
BIT_1_H:       // 0x4C
    cpu_test_bit(1, RegHL.hi);
    return;
BIT_1_L:       // 0x4D
    cpu_test_bit(1, RegHL.lo);
    return;
BIT_1_HL:       // 0x4E
    cpu_test_bit(1, RegHL.data); // edit
    return;
BIT_1_A:       // 0x4F
    cpu_test_bit(1, RegAF.hi);
    return;
BIT_2_B:       // 0x50
    cpu_test_bit(2, RegBC.hi);
    return;
BIT_2_C:      // 0x51
    cpu_test_bit(2, RegBC.lo);
    return;
BIT_2_D:       // 0x52
    cpu_test_bit(2, RegDE.hi);
    return;
BIT_2_E:       // 0x53
    cpu_test_bit(2, RegDE.lo);
    return;
BIT_2_H:       // 0x54
    cpu_test_bit(2, RegHL.hi);
    return;
BIT_2_L:       // 0x55
    cpu_test_bit(2, RegHL.lo);
    return;
BIT_2_HL:       // 0x56
    cpu_test_bit(2, RegHL.data); // edit
    return;
BIT_2_A:       // 0x57
    cpu_test_bit(2, RegAF.hi);
    return;
BIT_3_B:       // 0x58
    cpu_test_bit(3, RegBC.hi);
    return;
BIT_3_C:      // 0x59
    cpu_test_bit(3, RegBC.lo);
    return;
BIT_3_D:       // 0x5A
    cpu_test_bit(3, RegDE.hi);
    return;
BIT_3_E:       // 0x5B
    cpu_test_bit(3, RegDE.lo);
    return;
BIT_3_H:       // 0x5C
    cpu_test_bit(3, RegHL.hi);
    return;
BIT_3_L:       // 0x5D
    cpu_test_bit(3, RegHL.lo);
    return;
BIT_3_HL:       // 0x5E
    cpu_test_bit(3, RegHL.data); // edit
    return;
BIT_3_A:       // 0x5F
    cpu_test_bit(3, RegAF.hi);
    return;
BIT_4_B:       // 0x60
    cpu_test_bit(4, RegBC.hi);
    return;
BIT_4_C:      // 0x61
    cpu_test_bit(4, RegBC.lo);
    return;
BIT_4_D:       // 0x62
    cpu_test_bit(4, RegDE.hi);
    return;
BIT_4_E:       // 0x63
    cpu_test_bit(4, RegDE.lo);
    return;
BIT_4_H:       // 0x64
    cpu_test_bit(4, RegHL.hi);
    return;
BIT_4_L:       // 0x65
    cpu_test_bit(4, RegHL.lo);
    return;
BIT_4_HL:       // 0x66
    cpu_test_bit(4, RegHL.data); // edit
    return;
BIT_4_A:       // 0x67
    cpu_test_bit(4, RegAF.hi);
    return;
BIT_5_B:       // 0x68
    cpu_test_bit(5, RegBC.hi);
    return;
BIT_5_C:      // 0x69
    cpu_test_bit(5, RegBC.lo);
    return;
BIT_5_D:       // 0x6A
    cpu_test_bit(5, RegDE.hi);
    return;
BIT_5_E:       // 0x6B
    cpu_test_bit(5, RegDE.lo);
    return;
BIT_5_H:       // 0x6C
    cpu_test_bit(5, RegHL.hi);
    return;
BIT_5_L:       // 0x6D
    cpu_test_bit(5, RegHL.lo);
    return;
BIT_5_HL:       // 0x6E
    cpu_test_bit(5, RegHL.data); // edit
    return;
BIT_5_A:       // 0x6F
    cpu_test_bit(5, RegAF.hi);
    return;
BIT_6_B:       // 0x70
    cpu_test_bit(6, RegBC.hi);
    return;
BIT_6_C:      // 0x71
    cpu_test_bit(6, RegBC.lo);
    return;
BIT_6_D:       // 0x72
    cpu_test_bit(6, RegDE.hi);
    return;
BIT_6_E:       // 0x73
    cpu_test_bit(6, RegDE.lo);
    return;
BIT_6_H:       // 0x74
    cpu_test_bit(6, RegHL.hi);
    return;
BIT_6_L:       // 0x75
    cpu_test_bit(6, RegHL.lo);
    return;
BIT_6_HL:       // 0x76
    cpu_test_bit(6, RegHL.data); // edit
    return;
BIT_6_A:       // 0x77
    cpu_test_bit(6, RegAF.hi);
    return;
BIT_7_B:       // 0x78
    cpu_test_bit(7, RegBC.hi);
    return;
BIT_7_C:      // 0x79
    cpu_test_bit(7, RegBC.lo);
    return;
BIT_7_D:       // 0x7A
    cpu_test_bit(7, RegDE.hi);
    return;
BIT_7_E:       // 0x7B
    cpu_test_bit(7, RegDE.lo);
    return;
BIT_7_H:       // 0x7C
    cpu_test_bit(7, RegHL.hi);
    return;
BIT_7_L:       // 0x7D
    cpu_test_bit(7, RegHL.lo);
    return;
BIT_7_HL:       // 0x7E
    cpu_test_bit(7, RegHL.data); // edit
    return;
BIT_7_A:       // 0x7F
    cpu_test_bit(7, RegAF.hi);
    return;
RES_0_B:        // 0x80
    cpu_reset_bit(0, &RegBC.hi);
    return;
RES_0_C:        // 0x81
    cpu_reset_bit(0, &RegBC.lo);
    return;
RES_0_D:        // 0x82
    cpu_reset_bit(0, &RegDE.hi);
    return;
RES_0_E:        // 0x83
    cpu_reset_bit(0, &RegDE.lo);
    return;
RES_0_H:        // 0x84
    cpu_reset_bit(0, &RegHL.hi);
    return;
RES_0_L:        // 0x85
    cpu_reset_bit(0, &RegHL.lo);
    return;
RES_0_HL:        // 0x86
    cpu_reset_bit(0, &RegHL.data); // EDIT
    return;
RES_0_A:        // 0x87
    cpu_reset_bit(0, &RegAF.hi);
    return;
RES_1_B:        // 0x88
    cpu_reset_bit(1, &RegBC.hi);
    return;
RES_1_C:        // 0x89
    cpu_reset_bit(1, &RegBC.lo);
    return;
RES_1_D:        // 0x8A
    cpu_reset_bit(1, &RegDE.hi);
    return;
RES_1_E:        // 0x8B
    cpu_reset_bit(1, &RegDE.lo);
    return;
RES_1_H:        // 0x8C
    cpu_reset_bit(1, &RegHL.hi);
    return;
RES_1_L:        // 0x8D
    cpu_reset_bit(1, &RegHL.lo);
    return;
RES_1_HL:        // 0x8E
    cpu_reset_bit(1, &RegHL.data); // EDIT
    return;
RES_1_A:        // 0x8F
    cpu_reset_bit(1, &RegAF.hi);
    return;
RES_2_B:        // 0x90
    cpu_reset_bit(2, &RegBC.hi);
    return;
RES_2_C:        // 0x91
    cpu_reset_bit(2, &RegBC.lo);
    return;
RES_2_D:        // 0x92
    cpu_reset_bit(2, &RegDE.hi);
    return;
RES_2_E:        // 0x93
    cpu_reset_bit(2, &RegDE.lo);
    return;
RES_2_H:        // 0x94
    cpu_reset_bit(2, &RegHL.hi);
    return;
RES_2_L:        // 0x95
    cpu_reset_bit(2, &RegHL.lo);
    return;
RES_2_HL:        // 0x96
    cpu_reset_bit(2, &RegHL.data); // EDIT
    return;
RES_2_A:        // 0x97
    cpu_reset_bit(2, &RegAF.hi);
    return;
RES_3_B:        // 0x98
    cpu_reset_bit(3, &RegBC.hi);
    return;
RES_3_C:        // 0x99
    cpu_reset_bit(3, &RegBC.lo);
    return;
RES_3_D:        // 0x9A
    cpu_reset_bit(3, &RegDE.hi);
    return;
RES_3_E:        // 0x9B
    cpu_reset_bit(3, &RegDE.lo);
    return;
RES_3_H:        // 0x9C
    cpu_reset_bit(3, &RegHL.hi);
    return;
RES_3_L:        // 0x9D
    cpu_reset_bit(3, &RegHL.lo);
    return;
RES_3_HL:        // 0x9E
    cpu_reset_bit(3, &RegHL.data); // EDIT
    return;
RES_3_A:        // 0x9F
    cpu_reset_bit(3, &RegAF.hi);
    return;
RES_4_B:        // 0xA0
    cpu_reset_bit(4, &RegBC.hi);
    return;
RES_4_C:        // 0xA1
    cpu_reset_bit(4, &RegBC.lo);
    return;
RES_4_D:        // 0xA2
    cpu_reset_bit(4, &RegDE.hi);
    return;
RES_4_E:        // 0xA3
    cpu_reset_bit(4, &RegDE.lo);
    return;
RES_4_H:        // 0xA4
    cpu_reset_bit(4, &RegHL.hi);
    return;
RES_4_L:        // 0xA5
    cpu_reset_bit(4, &RegHL.lo);
    return;
RES_4_HL:        // 0xA6
    cpu_reset_bit(4, &RegHL.data); // EDIT
    return;
RES_4_A:        // 0xA7
    cpu_reset_bit(4, &RegAF.hi);
    return;
RES_5_B:        // 0xA8
    cpu_reset_bit(5, &RegBC.hi);
    return;
RES_5_C:        // 0xA9
    cpu_reset_bit(5, &RegBC.lo);
    return;
RES_5_D:        // 0xAA
    cpu_reset_bit(5, &RegDE.hi);
    return;
RES_5_E:        // 0xAB
    cpu_reset_bit(5, &RegDE.lo);
    return;
RES_5_H:        // 0xAC
    cpu_reset_bit(5, &RegHL.hi);
    return;
RES_5_L:        // 0xAD
    cpu_reset_bit(5, &RegHL.lo);
    return;
RES_5_HL:        // 0xAE
    cpu_reset_bit(5, &RegHL.data); // EDIT
    return;
RES_5_A:        // 0xAF
    cpu_reset_bit(5, &RegAF.hi);
    return;
RES_6_B:        // 0xB0
    cpu_reset_bit(6, &RegBC.hi);
    return;
RES_6_C:        // 0xB1
    cpu_reset_bit(6, &RegBC.lo);
    return;
RES_6_D:        // 0xB2
    cpu_reset_bit(6, &RegDE.hi);
    return;
RES_6_E:        // 0xB3
    cpu_reset_bit(6, &RegDE.lo);
    return;
RES_6_H:        // 0xB4
    cpu_reset_bit(6, &RegHL.hi);
    return;
RES_6_L:        // 0xB5
    cpu_reset_bit(6, &RegHL.lo);
    return;
RES_6_HL:        // 0xB6
    cpu_reset_bit(6, &RegHL.data); // EDIT
    return;
RES_6_A:        // 0xB7
    cpu_reset_bit(6, &RegAF.hi);
    return;
RES_7_B:        // 0xB8
    cpu_reset_bit(6, &RegBC.hi);
    return;
RES_7_C:        // 0xB9
    cpu_reset_bit(6, &RegBC.lo);
    return;
RES_7_D:        // 0xBA
    cpu_reset_bit(6, &RegDE.hi);
    return;
RES_7_E:        // 0xBB
    cpu_reset_bit(6, &RegDE.lo);
    return;
RES_7_H:        // 0xBC
    cpu_reset_bit(6, &RegHL.hi);
    return;
RES_7_L:        // 0xBD
    cpu_reset_bit(6, &RegHL.lo);
    return;
RES_7_HL:        // 0xBE
    cpu_reset_bit(6, &RegHL.data); // EDIT
    return;
RES_7_A:        // 0xBF
    cpu_reset_bit(6, &RegAF.hi);
    return;
SET_0_B:        // 0xC0
    cpu_set_bit(0, &RegBC.hi);
    return;
SET_0_C:        // 0xC1
    cpu_set_bit(0, &RegBC.lo);
    return;
SET_0_D:        // 0xC2
    cpu_set_bit(0, &RegDE.hi);
    return;
SET_0_E:        // 0xC3
    cpu_set_bit(0, &RegDE.lo);
    return;
SET_0_H:        // 0xC4 
    cpu_set_bit(0, &RegHL.hi);
    return;
SET_0_L:        // 0xC5 
    cpu_set_bit(0, &RegHL.lo);
    return;
SET_0_HL:       // 0xC6
    cpu_set_bit(0, &RegHL.data); // EDIT
    return;
SET_0_A:        // 0xC7
    cpu_set_bit(0, &RegAF.hi);
    return;
SET_1_B:        // 0xC8
    cpu_set_bit(1, &RegBC.hi);
    return;
SET_1_C:        // 0xC9
    cpu_set_bit(1, &RegBC.lo);
    return;
SET_1_D:        // 0xCA
    cpu_set_bit(1, &RegDE.hi);
    return;
SET_1_E:        // 0xCB
    cpu_set_bit(1, &RegDE.lo);
    return;
SET_1_H:        // 0xCC 
    cpu_set_bit(1, &RegHL.hi);
    return;
SET_1_L:        // 0xCD 
    cpu_set_bit(1, &RegHL.lo);
    return;
SET_1_HL:       // 0xCE
    cpu_set_bit(1, &RegHL.data); // EDIT
    return;
SET_1_A:        // 0xCF
    cpu_set_bit(1, &RegAF.hi);
    return;
SET_2_B:        // 0xD0
    cpu_set_bit(2, &RegBC.hi);
    return;
SET_2_C:        // 0xD1
    cpu_set_bit(2, &RegBC.lo);
    return;
SET_2_D:        // 0xD3
    cpu_set_bit(2, &RegDE.hi);
    return;
SET_2_E:        // 0xD3
    cpu_set_bit(2, &RegDE.lo);
    return;
SET_2_H:        // 0xD4 
    cpu_set_bit(2, &RegHL.hi);
    return;
SET_2_L:        // 0xD5 
    cpu_set_bit(2, &RegHL.lo);
    return;
SET_2_HL:       // 0xD6
    cpu_set_bit(2, &RegHL.data); // EDIT
    return;
SET_2_A:        // 0xD7
    cpu_set_bit(2, &RegAF.hi);
    return;
SET_3_B:        // 0xD8
    cpu_set_bit(3, &RegBC.hi);
    return;
SET_3_C:        // 0xD9
    cpu_set_bit(3, &RegBC.lo);
    return;
SET_3_D:        // 0xDA
    cpu_set_bit(3, &RegDE.hi);
    return;
SET_3_E:        // 0xDB
    cpu_set_bit(3, &RegDE.lo);
    return;
SET_3_H:        // 0xDC 
    cpu_set_bit(3, &RegHL.hi);
    return;
SET_3_L:        // 0xDD 
    cpu_set_bit(3, &RegHL.lo);
    return;
SET_3_HL:       // 0xDE
    cpu_set_bit(3, &RegHL.data); // EDIT
    return;
SET_3_A:        // 0xDF
    cpu_set_bit(3, &RegAF.hi);
    return;
SET_4_B:        // 0xE0
    cpu_set_bit(4, &RegBC.hi);
    return;
SET_4_C:        // 0xE1
    cpu_set_bit(4, &RegBC.lo);
    return;
SET_4_D:        // 0xE2
    cpu_set_bit(4, &RegDE.hi);
    return;
SET_4_E:        // 0xE3
    cpu_set_bit(4, &RegDE.lo);
    return;
SET_4_H:        // 0xE4 
    cpu_set_bit(4, &RegHL.hi);
    return;
SET_4_L:        // 0xE5 
    cpu_set_bit(4, &RegHL.lo);
    return;
SET_4_HL:       // 0xE6
    cpu_set_bit(4, &RegHL.data); // EDIT
    return;
SET_E_A:        // 0xE7
    cpu_set_bit(4, &RegAF.hi);
    return;
SET_5_B:        // 0xE8
    cpu_set_bit(5, &RegBC.hi);
    return;
SET_5_C:        // 0xE9
    cpu_set_bit(5, &RegBC.lo);
    return;
SET_5_D:        // 0xEA
    cpu_set_bit(5, &RegDE.hi);
    return;
SET_5_E:        // 0xEB
    cpu_set_bit(5, &RegDE.lo);
    return;
SET_5_H:        // 0xEC 
    cpu_set_bit(5, &RegHL.hi);
    return;
SET_5_L:        // 0xED 
    cpu_set_bit(5, &RegHL.lo);
    return;
SET_5_HL:       // 0xEE
    cpu_set_bit(5, &RegHL.data); // EDIT
    return;
SET_5_A:        // 0xEF
    cpu_set_bit(5, &RegAF.hi);
    return;
SET_6_B:        // 0xF0
    cpu_set_bit(6, &RegBC.hi);
    return;
SET_6_C:        // 0xF1
    cpu_set_bit(6, &RegBC.lo);
    return;
SET_6_D:        // 0xF2
    cpu_set_bit(6, &RegDE.hi);
    return;
SET_6_E:        // 0xF3
    cpu_set_bit(6, &RegDE.lo);
    return;
SET_6_H:        // 0xF4 
    cpu_set_bit(6, &RegHL.hi);
    return;
SET_6_L:        // 0xF5 
    cpu_set_bit(6, &RegHL.lo);
    return;
SET_6_HL:       // 0xF6
    cpu_set_bit(6, &RegHL.data); // EDIT
    return;
SET_6_A:        // 0xF7
    cpu_set_bit(6, &RegAF.hi);
    return;
SET_7_B:        // 0xF8
    cpu_set_bit(7, &RegBC.hi);
    return;
SET_7_C:        // 0xF9
    cpu_set_bit(7, &RegBC.lo);
    return;
SET_7_D:        // 0xFA
    cpu_set_bit(7, &RegDE.hi);
    return;
SET_7_E:        // 0xFB
    cpu_set_bit(7, &RegDE.lo);
    return;
SET_7_H:        // 0xFC 
    cpu_set_bit(7, &RegHL.hi);
    return;
SET_7_L:        // 0xFD 
    cpu_set_bit(7, &RegHL.lo);
    return;
SET_7_HL:       // 0xFE
    cpu_set_bit(7, &RegHL.data); // EDIT
    return;
SET_7_A:        // 0xFF
    cpu_set_bit(7, &RegAF.hi);
    return;
}


void write_memory(WORD address, BYTE data) {
    // Cannot write to ROM
    if (address < 0x8000) {
        return;
    }

    // Writes to ECHO RAM also writes in RAM
    else if ((address >= 0xE000) && (address <= 0xFDFF)) {
        rom[address] = data;
        write_memory(address - 0x2000, data);
    }
    
    // Cannot write in this address space
    else if ((address >= 0xFEA0) && (address >= 0xFEFF)) {
        return;
    }

    else {
        rom[address] = data;
    }
}

BYTE read_memory(WORD address) {
    if ((address >= 0x4000) && (address <= 0x7FFF)) {
    }

    if ((address >= 0xA000) && (address <= 0xBFFF)) {
    }
}

void cpu_load(BYTE *reg) {
    BYTE n = read_memory(PC++);
    *reg = n;
}

void cpu_loadReg(BYTE *reg1, BYTE *reg2) {
    *reg1 = *reg2;
}

void stack_push(const BYTE *hi, const BYTE *lo) {
    write_memory(RegSP.data--, hi);
    write_memory(RegSP.data--, lo);
}

void stack_pop(BYTE *hi, BYTE *lo) {
    *lo = read_memory(RegSP.data++);
    *hi = read_memory(RegSP.data++);
}

void cpu_add(BYTE *reg1, BYTE *reg2) {
    *reg1 += *reg2;
    
    // SET CARRY FLAGS HERE
}

void cpu_add16(WORD *reg1, WORD *reg2) {
}

void cpu_adc(BYTE *reg1, BYTE *reg2) {
    *reg1 += *reg2;
    *reg1 += RegAF.lo;

    // SET CARRY FLAGS HERE
}

void cpu_sub(BYTE *reg1, BYTE *reg2) {
    *reg1 -= *reg2;
    RegAF.lo |= (1 << FLAG_N); 
    if (*reg1 == 0) {
        RegAF.lo |= (1 << FLAG_Z);  
    }
}

void cpu_sbc(BYTE *reg1, BYTE *reg2){
// SET CARRY FLAGS HERE
}

void cpu_and(BYTE *reg1, BYTE *reg2) {
    *reg1 &= *reg2;
    if (*reg1 == 0) {
        cpu_set(FLAG_Z, RegAF.lo);    
    }
    cpu_reset(FLAG_N, RegAF.lo);
    cpu_set(FLAG_H, RegAF.lo);
    cpu_reset(FLAG_C, RegAF.lo);
}

void cpu_or(BYTE *reg1, BYTE *reg2) {
    *reg1 |= *reg2;
    if (*reg1 == 0) {
        cpu_set(FLAG_Z, RegAF.lo);   
    }
    cpu_reset(FLAG_N, RegAF.lo);
    cpu_reset(FLAG_H, RegAF.lo);
    cpu_reset(FLAG_C, RegAF.lo);
}

void cpu_xor(BYTE *reg1, BYTE *reg2) {
    *reg1 ^= *reg2;
    if (*reg1 == 0) {
        cpu_set(FLAG_Z, RegAF.lo);
    }
    cpu_reset(FLAG_N, RegAF.lo);
    cpu_reset(FLAG_H, RegAF.lo);
    cpu_reset(FLAG_C, RegAF.lo);
}

void cpu_cp(BYTE *reg) {
    BYTE result = RegAF.hi - *reg;
    if (result == 0) {
        cpu_set(FLAG_Z, RegAF.lo);
    }
    if (RegAF.hi > *reg) {
        cpu_set(FLAG_H, RegAF.lo);
    }
    if ((RegAF.hi & 0x0F) > (*reg & 0x0F)) {
        cpu_set(FLAG_C, RegAF.lo);
    }
    cpu_set(FLAG_N, RegAF.lo);
}

void cpu_inc(BYTE *reg) {
    if ((*reg & 0x0F) + 1 > 0x0F) {
        cpu_set(FLAG_H, RegAF.lo);
    }
    
    *reg += 1;
    
    if (*reg == 0) {
        cpu_set(FLAG_Z, RegAF.lo);
    }
    cpu_reset(FLAG_N, RegAF.lo);
}

void cpu_inc16(WORD *reg) {
    *reg += 1;
}

void cpu_dec(BYTE *reg) {
    if ((*reg & 0x0F) - 1 > 0) {
        cpu_set(FLAG_H, RegAF.lo);
    }

    *reg -= 1; 
    
    if (*reg == 0) {
        RegAF.lo |= (1 << FLAG_Z);  
    }
    RegAF.lo |= (1 << FLAG_N);

}

void cpu_dec16(WORD *reg) {
    *reg -= 1;
}

void cpu_swap(BYTE *reg) {
    *reg  = ((*reg & 0x0F) << 4) | ((*reg & 0xF0) >> 4);
}
void cpu_daa() {
}

void cpu_cpl() {
    RegAF.hi ^= 0xFF;
    cpu_set(FLAG_N, RegAF.lo);
    cpu_set(FLAG_H, RegAF.lo);
}

void cpu_ccf() {
    RegAF.lo ^= (1 << FLAG_C);
    cpu_reset(FLAG_N, RegAF.lo);
    cpu_reset(FLAG_H, RegAF.lo);
}

void cpu_scf() {
    cpu_set(FLAG_C, RegAF.lo);
    cpu_reset(FLAG_N, RegAF.lo);
    cpu_reset(FLAG_H, RegAF.lo);
}

void cpu_halt() {
}

void cpu_stop() {
}

void cpu_ei(int enable) {
    if (enable == 0) {
        rom[0xFFFF] = 0;
    }
    else {
        rom[0xFFFF] = 1;
    }
}
void cpu_rlca(BYTE *reg) {
    BYTE bit = (*reg & 0x80) >> 7; // Save most significant bit
    *reg <<= 1;
    *reg |= bit;
    
    if (bit == 1) {
        cpu_set_bit(FLAG_C, RegAF.lo);
    }
    else {
        cpu_reset_bit(FLAG_C, RegAF.lo);
    }
    
    if (*reg == 0)
    {
        cpu_set_bit(FLAG_Z, RegAF.lo);
    }
    cpu_reset_bit(FLAG_N, RegAF.lo);
    cpu_reset_bit(FLAG_H, RegAF.lo);
}

void cpu_rla(BYTE *reg) {
    BYTE bit = (*reg & 0x80 >> 7); // Save most significant bit
    BYTE carry_flag = (RegAF.lo & 0x10) >> FLAG_C;  // Save carry flag
    *reg <<= 1;
    *reg |= carry_flag; // The value of the carry flag is set to the LSB of the register
    
    if (bit == 1) {
        cpu_set_bit(FLAG_C, RegAF.lo);
    }
    else {
        cpu_reset_bit(FLAG_C, RegAF.lo);
    }
    
    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, RegAF.lo);
    }
    cpu_reset_bit(FLAG_N, RegAF.lo);
    cpu_reset_bit(FLAG_H, RegAF.lo);
}

void cpu_rrca(BYTE *reg) {
    BYTE bit = *reg & 0x01; // Save least significant bit
    *reg >>= 1;
    *reg |= (bit << 7);

    if (bit == 1) {
        cpu_set_bit(FLAG_C, RegAF.lo);
    }
    else {
        cpu_reset_bit(FLAG_C, RegAF.lo);
    }

    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, RegAF.lo);
    }
    cpu_reset_bit(FLAG_N, RegAF.lo);
    cpu_reset_bit(FLAG_H, RegAF.lo);
}

void cpu_rra(BYTE *reg) {
    BYTE bit = *reg & 0x01; // Save least significant bit
    BYTE carry_flag = (RegAF.lo & 0x10) >> FLAG_C; // Save carry flag
    *reg >>= 1;
    *reg |= (carry_flag << 7); // The value of the carry flag is set to the MSB of the register
    
    if (bit == 1) {
        cpu_set_bit(FLAG_C, RegAF.lo);
    }
    else {
        cpu_reset_bit(FLAG_C, RegAF.lo);
    }

    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, RegAF.lo);
    }
    cpu_reset_bit(FLAG_N, RegAF.lo);
    cpu_reset_bit(FLAG_H, RegAF.lo);
}

void cpu_test_bit(BYTE b, BYTE *reg) {
    BYTE test_bit = 1 << b;
    if ((*reg & test_bit) == 0) {
        RegAF.lo |= 1 << FLAG_Z; // Set Z flag
    }
    RegAF.lo &= ~(1 << FLAG_N);  // Reset N flag
    RegAF.lo |= 1 << FLAG_H;     // Set H flag
}

void cpu_set_bit(BYTE b, BYTE *reg) {
    BYTE set_bit = 1 << b;
    *reg |= set_bit;
}

void cpu_reset_bit(BYTE b, BYTE *reg) {
    BYTE reset_bit = 1 << b;
    *reg &= ~reset_bit;
}

void cpu_jump(CONDITION cond) {
    WORD nn;
    switch (cond)
    {
    NONE:
        nn = read_memory(PC++) << 8 | read_memory(PC);
        PC = nn;
        break;
    NZ: if ((RegAF.lo & (1 << FLAG_Z)) == 0) {   // Jump if Z flag is reset
            nn = read_memory(PC++) << 8 | read_memory(PC);
            PC = nn;
        }
        break;
    Z:  if ((RegAF.lo & (1 << FLAG_Z)) == 1) {     // Jump if Z flag is set 
            nn = read_memory(PC++) << 8 | read_memory(PC);
            PC = nn;
        }
        break;
    NC: if ((RegAF.lo & (1 << FLAG_C)) == 0) {   // Jump if C flag is reset
            nn = read_memory(PC++) << 8 | read_memory(PC);
            PC = nn;
        }
        break;
    C:  if ((RegAF.lo & (1 << FLAG_C)) == 1){     // Jump if C flag is set
            nn = read_memory(PC++) << 8 | read_memory(PC);
            PC = nn;
        }
        break;

    HL: if ((RegAF.lo & (1 << FLAG_C)) == 1) {    // Jump to the address contained in HL
            PC = RegHL.data;
        }
        break;
    default:
        break;
    }
}

void cpu_jr(CONDITION cond) {
    BYTE n;
    switch (cond)
    {
    NONE:
        n = read_memory(PC);
        PC = n;
        break;
    NZ: if ((RegAF.lo & (1 << FLAG_Z)) == 0) {    // Jump if Z flag is reset
            n = read_memory(PC);
            PC = n;
        }
        break;
    Z:  if ((RegAF.lo & (1 << FLAG_Z)) == 1) {     // Jump if Z flag is set 
            n = read_memory(PC);
            PC = n;
        }
        break;
    NC: if ((RegAF.lo & (1 << FLAG_C)) == 0) {   // Jump if C flag is reset
            n = read_memory(PC);
            PC = n;
        }
        break;
    C:  if ((RegAF.lo & (1 << FLAG_C)) == 1) {    // Jump if C flag is set
            n = read_memory(PC);
            PC = n;
        }
        break;
    default:
        break;
    }
}

void cpu_call(CONDITION cond) {
    WORD nn;
    switch (cond)
    {
    NONE:
        nn = read_memory(PC++) << 8 | read_memory(PC);
        write_memory(RegSP.data--, PC);
        PC = nn;
        break;
    NZ:
        if ((RegAF.lo & (1 << FLAG_Z)) == 0) {    // Jump if Z flag is reset
            nn = read_memory(PC++) << 8 | read_memory(PC);
            write_memory(RegSP.data--, PC);
            PC = nn;
        }
        break;
    Z: if ((RegAF.lo & (1 << FLAG_Z)) == 1) {     // Jump if Z flag is set 
            nn = read_memory(PC++) << 8 | read_memory(PC);
            write_memory(RegSP.data--, PC);
            PC = nn;
        }
       break;
    NC: if ((RegAF.lo & (1 << FLAG_C)) == 0) {    // Jump if C flag is reset
            nn = read_memory(PC++) << 8 | read_memory(PC);
            write_memory(RegSP.data--, PC);
            PC = nn;
        }
        break;
    C:  if ((RegAF.lo & (1 << FLAG_C)) == 1) {     // Jump if C flag is set
            nn = read_memory(PC++) << 8 | read_memory(PC);
            write_memory(RegSP.data--, PC);
            PC = nn;
        }
        break;
    default:
        break;
    }
}

void cpu_rst(BYTE n) {

}

void cpu_ret(CONDITION cond) {
    BYTE lo;
    BYTE hi;
    WORD nn;
    switch (cond)
    {
    NONE:
        stack_pop(&hi, &lo);
        nn = (hi << 4) | lo;
        PC = nn;
        break;
    NZ: if ((RegAF.lo & (1 << FLAG_Z)) == 0) {    // Jump if Z flag is reset
        stack_pop(&hi, &lo);
        nn = (hi << 4) | lo;
        PC = nn;
        }
        break;
    Z: if ((RegAF.lo & (1 << FLAG_Z)) == 1) {     // Jump if Z flag is set 
        stack_pop(&hi, &lo);
        nn = (hi << 4) | lo;
        PC = nn;
        }
       break;
    NC: if ((RegAF.lo & (1 << FLAG_C)) == 0) {    // Jump if C flag is reset
       stack_pop(&hi, &lo);
       nn = (hi << 4) | lo;
       PC = nn;
       }
       break;
    C: if ((RegAF.lo & (1 << FLAG_C)) == 1) {     // Jump if C flag is set
       stack_pop(&hi, &lo);
       nn = (hi << 4) | lo;
       PC = nn;
       }
       break;
    default:
        break;
    }
}

void cpu_reti() {
    BYTE lo;
    BYTE hi;
    stack_pop(&hi, &lo);
    WORD nn = (hi << 4) | lo;
    PC = nn;
    cpu_ei(1);
}