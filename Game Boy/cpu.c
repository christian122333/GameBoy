#include <stdio.h>
#include "display.h"
#include "cpu.h"
#include "instructions.h"

BYTE cartridge_memory[0x200000]; // The Game Boy cartridge holds up to 2 MB
BYTE rom[0x10000];
BYTE ram_banks[0x8000];
BYTE opcode;
BYTE IME = 1;
BYTE HALT = 0;
WORD PC;

Register RegAF;
Register RegBC;
Register RegDE;
Register RegHL;
Register RegSP;
BYTE *Regs[8];

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

void cpu_init() {
    PC = 0x100;
    opcode = 0;
    RegAF.data = 0x01B0;
    RegBC.data = 0x0013;
    RegDE.data = 0x00D8;
    RegHL.data = 0x014D;
    RegSP.data = 0xFFFE;
    Regs[REGB] = &RegBC.hi; // Register B
    Regs[REGC] = &RegBC.lo; // Register C
    Regs[REGD] = &RegDE.hi; // Register D
    Regs[REGE] = &RegDE.lo; // Register E
    Regs[REGH] = &RegHL.hi; // Register H
    Regs[REGL] = &RegHL.lo; // Register L
    Regs[NO_REG] = NULL;
    Regs[REGA] = &RegAF.hi; // Register A
    rom[0xFF00] = 0xCF;
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
    errno_t err;
    if (err = fopen_s(&fp, filename, "r") != 0) {
        char buf[200];
        fprintf_s(stderr, "cannot open file '%s': %s\n",
            filename, buf);
    }
    else {
        fread(rom, 0x8000, 1, fp);
        fclose(fp);
    }
}

void write_memory(WORD address, BYTE data) {
    // Cannot write to ROM
    if (address < 0x8000) {
        return;
    }

    // Can only write to VRAM in modes 0, 1, 2
    else if ((address >= 8000) && (address <= 0x9FFF)) {
        if (get_stat_mode() == 3)
            return;
        else
            rom[address] = data;
    }

    // Writes to ECHO RAM also writes in RAM
    else if ((address >= 0xE000) && (address <= 0xFDFF)) {
        rom[address] = data;
        write_memory(address - 0x2000, data);
        return;
    }


    // Can only access OAM during modes 0 and 1
    else if ((address >= 0xFE00) && (address <= 0xFE9F)) {
        if (get_stat_mode() < 2)
            rom[address] = data;
        else
            return;
    }

    // The first 4 bits of the joypad register are read only
    else if (address == 0xFF00) {
        data &= 0xF0;
        rom[address] |= data;
    }


    // The first 3 bits of the status register are read only
    else if (address == 0xFF41) {
        data &= ~0x07;
        rom[address] |= data;
    }

    // Writing to the scanline counter resets it
    else if (address == 0xFF44) {
        rom[address] = 0;
    }

    // DMA transfer
    else if (address == 0xFF46) {
        WORD address = data << 8;
        for (int i = 0; i < 0xA0; i++) {
            write_memory(0xFE00 + i, rom[address + i]);
        }
    }

    else {
        rom[address] = data;
    }

}

BYTE read_memory(WORD address) {
    return rom[address];
}


void set_halt() {
    HALT = 1;
}

void reset_halt() {
    HALT = 0;
}

int execute() {
    if (HALT)
        return 0;
    opcode = rom[PC++];
    BYTE n = 0x00;
    SIGNED_BYTE signed_n = 0x00;
    WORD nn = 0x0000;
    SIGNED_WORD signed_nn = 0x00;
    switch (opcode)
    {
    case 0xD3: case 0xDB: case 0xDD: case 0xE3: case 0xE4: case 0xEB: // Invalid opcodes
    case 0xEC: case 0xED: case 0xF4: case 0xFC: case 0xFD:
        return 0;
    case 0x00: // NOP
        return 4;
    case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E: // LD nn, n
        n = read_memory(PC++);
        cpu_loadReg(Regs[(opcode - 0x06) / 8], &n);
        return 8;
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47: // LD B, n 0
    case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4F: // LD C, n 1
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57: // LD D, n 2
    case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5F: // LD E, n 3
    case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67: // LD H, n 4
    case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6F: // LD L, n 5
    case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7F: // LD A, n 
        if ((opcode & 0x0F) < 0x08) {
            cpu_loadReg(Regs[((opcode >> 4) - 0x04) * 2], Regs[(opcode & 0x0F) % 8]);
        }
        else {
            cpu_loadReg(Regs[(((opcode >> 4) - 0x04) * 2) + 1], Regs[(opcode & 0x0F) % 8]);
        }
        return 4;
    case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E:  // LD B-L, (HL)
        n = read_memory(RegHL.data);
        cpu_loadReg(Regs[(opcode - 0x46) / 8], &n); 
        return 8;
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: // LD (HL), B-L
        write_memory(RegHL.data, *Regs[opcode & 0x0F]);
        return 8;
    case 0x36: // LD (HL), n
        n = read_memory(PC++);
        write_memory(RegHL.data, n);
        return 12;
    case 0x0A: // LD A, (BC)
        n = read_memory(RegBC.data);
        cpu_loadReg(Regs[REGA], &n);
        return 8;
    case 0x1A: // LD A, (DE)
        n = read_memory(RegDE.data);
        cpu_loadReg(Regs[REGA], &n);
        return 8;
    case 0x7E: // LD A, (HL)
        n = read_memory(RegHL.data);
        cpu_loadReg(Regs[REGA], &n);
        return 8;
    case 0xFA: // LD A, (nn)
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 8;
        n = read_memory(nn);
        cpu_loadReg(Regs[REGA], &n);
        return 16;
    case 0x3E: // LD A, #
        n = read_memory(PC++);
        cpu_loadReg(Regs[REGA], &n);
        return 8;
    case 0x02: // LD (BC), A
        write_memory(RegBC.data, *Regs[REGA]);
        return 8;
    case 0x12: // LD (DE), A
        write_memory(RegDE.data, *Regs[REGA]);
        return 8;
    case 0x77: // LD (HL), A
        write_memory(RegHL.data, *Regs[REGA]);
        return 8;
    case 0xEA: // LD (nn), A
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 8;
        write_memory(nn, *Regs[REGA]);
        return 16;
    case 0xF2: // LD A, (C)
        n = read_memory(0xFF00 + *Regs[REGC]);
        cpu_loadReg(Regs[REGA], &n); 
        return 8;
    case 0xE2: // LD (C), A
        write_memory(0xFF00 + *Regs[REGC], *Regs[REGA]);
        return 8;
    case 0x3A: // LD A, (HLD)
        n = read_memory(RegHL.data);
        cpu_loadReg(Regs[REGA], &n);
        cpu_dec16(&RegHL.data);
        return 8;
    case 0x32: // LD (HLD), A
        write_memory(RegHL.data, *Regs[REGA]);
        cpu_dec16(&RegHL.data);
        return 8;
    case 0x2A: // LD A, (HLI)
        n = read_memory(RegHL.data);
        cpu_loadReg(Regs[REGA], &n);
        cpu_inc16(&RegHL.data);
        return 8;
    case 0x22: // LD (HLI), A
        write_memory(RegHL.data, *Regs[REGA]);
        cpu_inc16(&RegHL.data);
        return 8;
    case 0xE0: // LDH (n), A
        n = read_memory(PC++);
        write_memory(0xFF00 + n, *Regs[REGA]);
        return 12;
    case 0xF0: // LDH A, (n)
        n = read_memory(PC++);
        n = read_memory(0xFF00 + n);
        cpu_loadReg(Regs[REGA], &n);
        return 12;
    case 0x01: // LDH BC, nn
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 8;
        cpu_loadReg16(&RegBC.data, &nn);
        return 12;
    case 0x11: // LDH DE, nn
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 8;
        cpu_loadReg16(&RegDE.data, &nn);
        return 12;
    case 0x21: // LDH HL, nn
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 8;
        cpu_loadReg16(&RegHL.data, &nn);
        return 12;
    case 0x31: // LDH SP, nn
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 8;
        cpu_loadReg16(&RegSP.data, &nn);
        return 12;
    case 0xF9: // LDH SP, HL
        cpu_loadReg16(&RegSP.data, &RegHL.data);
        return 8;
    case 0xF8: // LDHL SP, n
        LDHL_SP_n();
        return 12;
    case 0x08: // LD (nn), SP
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 8;
        cpu_loadRegSP(&rom[nn], &RegSP);
        return 20;
    case 0xF5: // PUSH AF
        stack_push(&RegAF.hi, &RegAF.lo);
        return 16;
    case 0xC5: // PUSH BC
        stack_push(&RegBC.hi, &RegBC.lo);
        return 16;
    case 0xD5: // PUSH DE
        stack_push(&RegDE.hi, &RegDE.lo);
        return 16;
    case 0xE5: // PUSH HL
        stack_push(&RegHL.hi, &RegHL.lo);
        return 16;
    case 0xF1: // POP AF
        stack_pop(&RegAF.hi, &RegAF.lo);
        return 12;
    case 0xC1: // POP BC
        stack_pop(&RegBC.hi, &RegBC.lo);
        return 12;
    case 0xD1: // POP DE
        stack_pop(&RegDE.hi, &RegDE.lo);
        return 12;
    case 0xE1: // POP HL
        stack_pop(&RegHL.hi, &RegHL.lo);
        return 12;
    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87: // ADD A, n
        cpu_add(Regs[REGA], Regs[(opcode & 0x0F) % 8]);
        return 4;
    case 0x86:
        n = read_memory(RegHL.data);
        cpu_add(Regs[REGA], &n);
        return 8;
    case 0xC6:
        n = read_memory(PC++);
        cpu_add(Regs[REGA], &n);
        return 8;
    case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F: // ADC A, n
        cpu_adc(Regs[REGA], Regs[(opcode & 0x0F) % 8]);
        return 4;
    case 0x8E:
        n = read_memory(RegHL.data);
        cpu_adc(Regs[REGA], &n);
        return 8;
    case 0xCE:
        n = read_memory(PC++);
        cpu_adc(Regs[REGA], &n);
        return 8;
    case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97: // SUB A, n 
        cpu_sub(Regs[REGA], Regs[(opcode & 0x0F) % 8]);
        return 4;
    case 0x96:
        n = read_memory(RegHL.data);
        cpu_sub(Regs[REGA], &n);
        return 8;
    case 0xD6:
        n = read_memory(PC++);
        cpu_sub(Regs[REGA], &n);
        return 8;
    case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9F: // SBC A, n
        cpu_sbc(Regs[REGA], Regs[(opcode & 0x0F) % 8]);
        return 4;
    case 0x9E:
        n = read_memory(RegHL.data);
        cpu_sbc(Regs[REGA], &n);
        return 8;
    case 0xDE:
        n = read_memory(PC++);
        cpu_sbc(Regs[REGA], &n);
        return 8;
    case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA7: // AND A, n 
        cpu_and(Regs[REGA], Regs[(opcode & 0x0F) % 8]);
        return 4;
    case 0xA6:
        n = read_memory(RegHL.data);
        cpu_and(Regs[REGA], &n);
        return 8;
    case 0xE6:
        n = read_memory(PC++);
        cpu_and(Regs[REGA], &n);
        return 8;
    case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAF: // XOR A, n
        cpu_xor(Regs[REGA], Regs[(opcode & 0x0F) % 8]);
        return 4;
    case 0xAE:
        n = read_memory(RegHL.data);
        cpu_xor(Regs[REGA], &n);
        return 8;
    case 0xEE:
        n = read_memory(PC++);
        cpu_xor(Regs[REGA], &n);
        return 8;
    case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB7: // OR A, n 
        cpu_or(Regs[REGA], Regs[(opcode & 0x0F) % 8]);
        return 4;
    case 0xB6:
        n = read_memory(RegHL.data);
        cpu_or(Regs[REGA], &n);
        return 8;
    case 0xF6:
        n = read_memory(PC++);
        cpu_or(Regs[REGA], &n);
        return 8;
    case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBF: // CP A, n
        cpu_cp(Regs[(opcode & 0x0F) % 8]);
        return 4;
    case 0xBE:
        cpu_cp(&rom[RegHL.data]);
        return 8;
    case 0xFE:
        n = read_memory(PC++);
        cpu_cp(&n);
        return 8;
    case 0x3C: // INC A
        cpu_inc(Regs[REGA]);
        return 4;
    case 0x04: // INC B
        cpu_inc(Regs[REGB]);
        return 4;
    case 0x0C: // INC C
        cpu_inc(Regs[REGC]);
        return 4;
    case 0x14: // INC D
        cpu_inc(Regs[REGD]);
        return 4;
    case 0x1C: // INC E
        cpu_inc(Regs[REGE]);
        return 4;
    case 0x24: // INC H
        cpu_inc(Regs[REGH]);
        return 4;
    case 0x2C: // INC L
        cpu_inc(Regs[REGL]);
        return 4;
    case 0x34: // INC (HL)
        cpu_inc_hl(RegHL.data);
        return 12;
    case 0x3D: // DEC A
        cpu_dec(Regs[REGA]);
        return 4;  
    case 0x05: // DEC B
        cpu_dec(Regs[REGB]);
        return 4;
    case 0x0D: // DEC C
        cpu_dec(Regs[REGC]);
        return 4;
    case 0x15: // DEC D
        cpu_dec(Regs[REGD]);
        return 4;
    case 0x1D: // DEC E
        cpu_dec(Regs[REGE]);
        return 4;
    case 0x25: // DEC H
        cpu_dec(Regs[REGH]);
        return 4;
    case 0x2D: // DEC L
        cpu_dec(Regs[REGL]);
        return 4;
    case 0x35: // DEC (HL)
        cpu_dec_hl(RegHL.data);
        return 12;
    case 0x09: // ADD HL, BC
        cpu_add16(&RegHL.data, &RegBC.data);
        return 8;
    case 0x19: // ADD HL, DE
        cpu_add16(&RegHL.data, &RegDE.data);
        return 8;
    case 0x29: // ADD HL, HL
        cpu_add16(&RegHL.data, &RegHL.data);
        return 8;
    case 0x39: // ADD HL, SP
        cpu_add16(&RegHL.data, &RegSP.data);
        return 8;
    case 0xE8:  // ADD SP, n
        cpu_add_sp_n();
        return 16;
    case 0x03: // INC BC
        cpu_inc16(&RegBC.data);
        return 8;
    case 0x13: // INC DE
        cpu_inc16(&RegDE.data);
        return 8;
    case 0x23: // INC HL
        cpu_inc16(&RegHL.data);
        return 8;
    case 0x33: // INC SP
        cpu_inc16(&RegSP.data);
        return 8;
    case 0x0B: // DEC BC 
        cpu_dec16(&RegBC.data);
        return 8;
    case 0x1B: // DEC DE
        cpu_dec16(&RegDE.data);
        return 8;
    case 0x2B: // DEC HL
        cpu_dec16(&RegHL.data);
        return 8;
    case 0x3B: // DEC SP
        cpu_dec16(&RegSP.data);
        return 8;
    case 0x27: // DAA
        cpu_daa();
        return 4;
    case 0x2F: // CPL
        cpu_cpl();
        return 4;
    case 0x3F: // CCF
        cpu_ccf();
        return 4;
    case 0x37: // SCF
        cpu_scf();
        return 4;
    case 0x76: // HALT
        cpu_halt();
        return 4;
    case 0x10: // STOP
        cpu_stop();
        return 4;
    case 0xF3: // DI
        cpu_ei(0);
        return 4;
    case 0xFB: // EI
        cpu_ei(1);
        return 4;
    case 0x07: // RLCA
        cpu_rlca();
        return 4;
    case 0x17: // RLA
        cpu_rla();
        return 4;
    case 0x0F: // RRCA
        cpu_rrca();
        return 4;
    case 0x1F: // RRA
        cpu_rra();
        return 4;
    case 0xC3: // JP nn
        cpu_jump(NONE);
        return 12;
    case 0xC2: // JP NZ. nn
        cpu_jump(NZ);
        return 12;
    case 0xCA: // JP Z. nn
        cpu_jump(Z);
        return 12;
    case 0xD2: // JP NC, nn
        cpu_jump(NC);
        return 12;
    case 0xDA: // JP C, nn
        cpu_jump(C);
        return 12;
    case 0xE9: // JP HL
        cpu_jump(HL);
        return 4;
    case 0x18: // JR n
        cpu_jr(NONE);
        return 18;
    case 0x20: // JR NZ,n
        cpu_jr(NZ);
        return 8;
    case 0x28: // JR Z,n
        cpu_jr(Z);
        return 8;
    case 0x30: // JR NC,n
        cpu_jr(NC);
        return 8;
    case 0x38: // JR C,n
        cpu_jr(C);
        return 8;
    case 0xCD: // CALL nn
        cpu_call(NONE);
        return 12;
    case 0xC4: // CALL NZ,n
        cpu_call(NZ);
        return 12;
    case 0xCC: // CALL Z,n
        cpu_call(Z);
        return 12;
    case 0xD4: // CALL NC,n
        cpu_call(NC);
        return 12;;
    case 0xDC: // CALL C,n
        cpu_call(C);
        return 12;
    case 0xC7: // RST 00
        cpu_rst(0x00);
        return 32;
    case 0xCF: // RST 08
        cpu_rst(0x08);
        return 32;
    case 0xD7: // RST 10
        cpu_rst(0x10);
        return 32;
    case 0xDF: // RST 18
        cpu_rst(0x18);
        return 32;
    case 0xE7: // RST 20
        cpu_rst(0x20);
        return 32;
    case 0xEF: // RST 28
        cpu_rst(0x28);
        return 32;
    case 0xF7: // RST 30
        cpu_rst(0x30);
        return 32;
    case 0xFF: // RST 38
        cpu_rst(0x38);
        return 32;
    case 0xC9: // RET 
        cpu_ret(NONE);
        return 8;
    case 0xC0: // RET NZ
        cpu_ret(NZ);
        return 8;
    case 0xC8: // RET Z
        cpu_ret(Z);
        return 8;
    case 0xD0: // RET NC
        cpu_ret(NC);
        return 8;
    case 0xD8: // RET C
        cpu_ret(C);
        return 8;
    case 0xD9: // RETI C
        cpu_reti();
        return 8;
    case 0xCB:
        return CB();
    default:
        return 0;
    }
    return 0;
}

int CB() {
    opcode = rom[PC++];
    BYTE n = 0x00;
    switch (opcode)
    {
    case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x07: // RLC n
        cpu_rlc(Regs[opcode % 8]);
        return 8;
    case 0x06: // RLC (HL)
        cpu_rlc_hl(RegHL.data);
        return 16;
    case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0F: // RRC n
        cpu_rrc(Regs[opcode % 8]);
        return 8;
    case 0x0E: // RRC (HL)
        cpu_rrc_hl(RegHL.data);
        return 16;
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x17: // RL n
        cpu_rl(Regs[opcode % 8]);
        return 8;
    case 0x16: // RL (HL)
        cpu_rl_hl(RegHL.data);
        return 16;
    case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1F: // RR n
        cpu_rr(Regs[opcode % 8]);
        return 8;
    case 0x1E:  // RR (HL)
        cpu_rr_hl(RegHL.data);
        return 16;
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x27: // SLA n 
        cpu_sla(Regs[opcode % 8]);
        return 8;
    case 0x26:  // SLA (HL)
        cpu_sla_hl(RegHL.data);
        return 16;
    case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2F: // SRA n // ALL
        cpu_sra(Regs[opcode % 8]);
        return 8;
    case 0x2E:  // SRA (HL)
        cpu_sra_hl(RegHL.data);
        return 16;
    case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x37: // SWAP n
        cpu_swap(Regs[opcode % 8]);
        return 8;
    case 0x36:  // SWAP (HL)
        cpu_swap_hl(RegHL.data);
        return 16;
    case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3F: // SRL n
        cpu_srl(Regs[opcode % 8]);
        return 8;
    case 0x3E:  // SRL (HL)
        cpu_srl_hl(RegHL.data);
        return 16;
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47: // BIT 0, n
    case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4F: // BIT 1, n
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57: // BIT 2, n
    case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5F: // BIT 3, n
    case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67: // BIT 4, n
    case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6F: // BIT 5, n
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77: // BIT 6, n
    case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7F: // BIT 7, n
        if ((opcode & 0x0F) < 8) {
            cpu_test_bit(((opcode >> 4) - 0x04) * 2, Regs[(opcode & 0x0F) % 8]);
        }
        else {
            cpu_test_bit((((opcode >> 4) - 0x04) * 2) + 1, Regs[(opcode & 0x0F) % 8]);
        }
        return 8;
    case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E: // BIT n, HL
    case 0x76: case 0x7E:
        n = read_memory(RegHL.data);
        cpu_test_bit((opcode - 0x46) / 8, &n);
        return 16;
    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87: // RES 0, n
    case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F: // RES 1, n
    case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97: // RES 2, n
    case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9F: // RES 3, n
    case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA7: // RES 4, n
    case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAF: // RES 5, n
    case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB7: // RES 6, n
    case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBF: // RES 7, n
        if ((opcode & 0x0F) < 8) {
            cpu_reset_bit(((opcode >> 4) - 0x08) * 2, Regs[(opcode & 0x0F) % 8]);
        }
        else {
            cpu_reset_bit((((opcode >> 4) - 0x08) * 2) + 1, Regs[(opcode & 0x0F) % 8]);
        }
        return 8;
    case 0x86: case 0x8E: case 0x96: case 0x9E: case 0xA6: case 0xAE: // RES n, (HL)
    case 0xB6: case 0xBE:
        cpu_reset_bit_hl((opcode - 0xC6) / 8, RegHL.data);
        return 16;
    case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC7: // SET 0, n
    case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCF: // SET 1, n
    case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD7: // SET 2, n
    case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDF: // SET 3, n
    case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE7: // SET 4, n
    case 0xE8: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEF: // SET 5, n
    case 0xF0: case 0xF1: case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF7: // SET 6, n
    case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFF: // SET 7, n
        if ((opcode & 0x0F) < 8) {
            cpu_set_bit(((opcode >> 4) - 0x0C) * 2, Regs[(opcode & 0x0F) % 8]);
        }
        else {
            cpu_set_bit((((opcode >> 4) - 0x0C) * 2) + 1, Regs[(opcode & 0x0F) % 8]);
        }
        return 8;
    case 0xC6: case 0xCE: case 0xD6: case 0xDE: case 0xE6: case 0xEE: // SET n, (HL)
    case 0xF6: case 0xFE:   
        cpu_set_bit_hl((opcode - 0xC6) / 8, RegHL.data);
        return 16;
    default:
        return 0;
    }
    return 0;
}
  