#include <stdio.h>
#include "cpu.h"
#include "instructions.h"

BYTE cartridge_memory[0x200000]; // The Game Boy cartridge holds up to 2 MB
BYTE rom[0x10000];
BYTE ram_banks[0x8000];
BYTE opcode;
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
    BYTE n = 0x00;
    WORD nn = 0x0000;

    switch (opcode)
    {
    case 0xD3: case 0xDB: case 0xDD: case 0xE3: case 0xE4: case 0xEB: // Invalid opcodes
    case 0xEC: case 0xED: case 0xF4: case 0xFC: case 0xFD:
        break;
    case 0x00: // NOP
        break;
    case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E: // LD nn, n
        n = read_memory(PC++);
        cpu_loadReg(Regs[(opcode - 0x06) / 8], &n);
        break;
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47: // LD B, n
    case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4F: // LD C, n 
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57: // LD D, n 
    case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5F: // LD E, n 
    case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67: // LD H, n 
    case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6F: // LD L, n  
    case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7F: // LD A, n 
        if (opcode & 0x0F < 8) {
            cpu_loadReg(Regs[((opcode & 0xF0) - 0x40) * 2], Regs[(opcode & 0xF0) % 8]);
        }
        else {
            cpu_loadReg(Regs[(((opcode & 0xF0) - 4) * 0x40) + 1], Regs[(opcode & 0xF0) % 8]);
        }
        break;
    case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E:  // LD B-L, (HL)
        n = read_memory(read_memory(RegHL.data));
        cpu_loadReg(Regs[(opcode - 0x46) / 8], &n); 
        break;
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: // LD (HL), B-L
        n = read_memory(read_memory(RegHL.data));
        cpu_loadReg(&n, Regs[opcode & 0x0F]);
        break;
    case 0x36: // LD (HL), n
        n = read_memory(PC++);
        cpu_loadReg(read_memory(&RegHL), &n);
        break;
    case 0x0A: // LD A, (BC)
        n = read_memory(RegBC.data);
        cpu_loadReg(Regs[REGA], &n);
        break;
    case 0x1A: // LD A, (DE)
        n = read_memory(RegDE.data);
        cpu_loadReg(Regs[REGA], &n);
        break;
    case 0x7E: // LD A, (HL)
        n = read_memory(RegHL.data);
        cpu_loadReg(Regs[REGA], &n);
        break;
    case 0xFA: // LD A, (nn)
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 4;
        nn = read_memory(nn);
        cpu_loadReg(Regs[REGA], &nn);
        break;
    case 0x3E: // LD A, #
        cpu_loadReg(Regs[REGA], Regs[REGA]); // Edit
        break;
    case 0x02: // LD (BC), A
        cpu_loadReg(read_memory(&RegBC), Regs[REGA]);
        break;
    case 0x12: // LD (DE), A
        cpu_loadReg(read_memory(&RegDE), Regs[REGA]);
        break;
    case 0x77: // LD (HL), A
        cpu_loadReg(read_memory(&RegHL), Regs[REGA]);
        break;
    case 0xEA: // LD (nn), A
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 4;
        nn = read_memory(nn);
        cpu_loadReg(&nn, Regs[REGA]);
        break;
    case 0xF2: // LD A, (C)
        n = read_memory(0xFF00 + *Regs[REGC]);
        cpu_loadReg(Regs[REGA], &n); 
        break;
    case 0xE2: // LD (C), A
        n = read_memory(0xFF00 + *Regs[REGC]);
        cpu_loadReg(&n, Regs[REGA]);
        break;
    case 0x3A: // LD A, (HLD)
        n = read_memory(RegHL.data);
        cpu_dec16(&RegHL.data);
        cpu_loadReg(Regs[REGA], &n);
        break;
    case 0x32: // LD (HLD), A
        n = read_memory(RegHL.data);
        cpu_dec16(&RegHL.data);
        cpu_loadReg(&n, Regs[REGA]);
        break;
    case 0x2A: // LD A, (HLI)
        n = read_memory(RegHL.data);
        cpu_inc16(&RegHL.data);
        cpu_loadReg(Regs[REGA], &n);
        break;
    case 0x22: // LD (HLI), A
        n = read_memory(RegHL.data);
        cpu_inc16(&RegHL.data);
        cpu_loadReg(&n, Regs[REGA]);
        break;
    case 0xE0: // LDH (n), A
        n = read_memory(PC++);
        n = 0xFF00 + n;
        cpu_loadReg(&n, Regs[REGA]);
        break;
    case 0xF0: // LDH A, (n)
        n = read_memory(PC++);
        n = 0xFF00 + n;
        cpu_loadReg(Regs[REGA], &n);
        break;
    case 0x01: // LDH BC, nn
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 4;
        cpu_loadReg(&RegBC.data, &nn);
        break;
    case 0x11: // LDH DE, nn
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 4;
        cpu_loadReg(&RegDE.data, &nn);
        break;
    case 0x21: // LDH HL, nn
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 4;
        cpu_loadReg(&RegHL.data, &nn);
        break;
    case 0x31: // LDH SP, nn
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 4;
        cpu_loadReg(&RegSP.data, &nn);
        break;
    case 0xF9: // LDH SP, HL
        cpu_loadReg(&RegSP.data, &RegHL.data);
        break;
    case 0xF8: // LDHL SP, n
        nn = read_memory(PC++);
        cpu_add16(&nn, &RegSP.data);
        cpu_loadReg(&RegHL.data, &nn);// flags
        cpu_reset_bit(FLAG_Z, &RegAF.lo); // Reset Z flag
        cpu_reset_bit(FLAG_N, &RegAF.lo); // Reset N flag
        break;
    case 0x08: // LD (nn), SP
        nn = read_memory(PC++);
        nn |= read_memory(PC++) << 4;
        nn = read_memory(nn);
        cpu_loadReg(&nn, &RegSP.data);
        break;
    case 0xF5: // PUSH AF
        stack_push(&RegAF.hi, &RegAF.lo);
        break;
    case 0xC5: // PUSH BC
        stack_push(&RegBC.hi, &RegBC.lo);
        break;
    case 0xD5: // PUSH DE
        stack_push(&RegDE.hi, &RegDE.lo);
        break;
    case 0xE5: // PUSH HL
        stack_push(&RegHL.hi, &RegHL.lo);
        break;
    case 0xF1: // POP AF
        stack_push(&RegAF.hi, &RegAF.lo);
        break;
    case 0xC1: // POP BC
        stack_push(&RegBC.hi, &RegBC.lo);
        break;
    case 0xD1: // POP DE
        stack_push(&RegDE.hi, &RegDE.lo);
        break;
    case 0xE1: // POP HL
        stack_push(&RegHL.hi, &RegHL.lo);
        break;
    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87: // ADD A, n
        cpu_add(Regs[REGA], Regs[(opcode & 0xF0) % 8]);
        break;
    case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F: // ADC A, n
        cpu_adc(Regs[REGA], Regs[(opcode & 0xF0) % 8]);
        break;
    case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97: // SUB A, n 
        cpu_sub(Regs[REGA], Regs[(opcode & 0xF0) % 8]);
        break;
    case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9F: // SBC A, n
        cpu_sbc(Regs[REGA], Regs[(opcode & 0xF0) % 8]);
        break;
    case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA7: // AND A, n 
        cpu_and(Regs[REGA], Regs[(opcode & 0xF0) % 8]);
        break;
    case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAF: // XOR A, n
        cpu_xor(Regs[REGA], Regs[(opcode & 0xF0) % 8]);
        break;
    case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB7: // OR A, n 
        cpu_or(Regs[REGA], Regs[(opcode & 0xF0) % 8]);
        break;
    case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBF: // CP A, n
        cpu_cp(Regs[REGA], Regs[(opcode & 0xF0) % 8]);
        break;
    case 0x3C: // INC A
        cpu_inc(Regs[REGA]);
        break;
    case 0x04: // INC B
        cpu_inc(Regs[REGB]);
        break;
    case 0x0C: // INC C
        cpu_inc(Regs[REGC]);
        break;
    case 0x14: // INC D
        cpu_inc(Regs[REGD]);
        break;
    case 0x1C: // INC E
        cpu_inc(Regs[REGE]);
        break;
    case 0x24: // INC H
        cpu_inc(Regs[REGH]);
        break;
    case 0x2C: // INC L
        cpu_inc(Regs[REGC]);
        break;
    case 0x34: // INC HL
        n = read_memory(RegHL.data);
        cpu_inc(&n);
        break;
    case 0x3D: // DEC A
        cpu_dec(Regs[REGA]);
        break;  
    case 0x05: // DEC B
        cpu_dec(Regs[REGB]);
        break;
    case 0x0D: // DEC C
        cpu_dec(Regs[REGC]);
        break;
    case 0x15: // DEC D
        cpu_dec(Regs[REGD]);
        break;
    case 0x1D: // DEC E
        cpu_dec(Regs[REGE]);
        break;
    case 0x25: // DEC H
        cpu_dec(Regs[REGH]);
        break;
    case 0x2D: // DEC L
        cpu_dec(Regs[REGL]);
        break;
    case 0x35: // DEC HL
        n = read_memory(RegHL.data);
        cpu_dec(&n);
        break;
    case 0x09: // ADD HL, BC
        cpu_add16(&RegHL, &RegBC);
        break;
    case 0x19: // ADD HL, DE
        cpu_add16(&RegHL, &RegDE);
        break;
    case 0x29: // ADD HL, HL
        cpu_add16(&RegHL, &RegHL);
        break;
    case 0x39: // ADD HL, SP
        cpu_add16(&RegHL, &RegSP);
        break;
    case 0xE8:  // ADD SP, n
        cpu_add16(&RegSP, &RegSP); // EDIT
        break;
    case 0x03: // INC BC
        cpu_inc16(&RegBC);
        break;
    case 0x13: // INC DE
        cpu_inc16(&RegDE);
        break;
    case 0x23: // INC HL
        cpu_inc16(&RegHL);
        break;
    case 0x33: // INC SP
        cpu_inc16(&RegSP);
        break;
    case 0x0B: // DEC BC 
        cpu_dec16(&RegBC);
        break;
    case 0x1B: // DEC DE
        cpu_dec16(&RegDE);
        break;
    case 0x2B: // DEC HL
        cpu_dec16(&RegHL);
        break;
    case 0x3B: // DEC SP
        cpu_dec16(&RegSP);
        break;
    case 0x27: // DAA
        cpu_daa();
        break;
    case 0x2F: // CPL
        cpu_cpl();
        break;
    case 0x3F: // CCF
        cpu_ccf();
        break;
    case 0x37: // SCF
        cpu_scf();
        break;
    case 0x76: // HALT
        cpu_halt();
        break;
    case 0x10: // STOP
        cpu_stop();
        break;
    case 0xF3: // DI
        cpu_ei(0);
        break;
    case 0xFB: // EI
        cpu_ei(1);
        break;
    case 0x07: // RLCA
        cpu_rlc(Regs[REGA]);
        break;
    case 0x17: // RLA
        cpu_rl(Regs[REGA]);
        break;
    case 0x0F: // RRCA
        cpu_rrc(Regs[REGA]);
        break;
    case 0x1F: // RRA
        cpu_rr(Regs[REGA]);
        break;
    case 0xC3: // JP nn
        cpu_jump(NONE);
        break;
    case 0xC2: // JP NZ. nn
        cpu_jump(NZ);
        break;
    case 0xCA: // JP Z. nn
        cpu_jump(Z);
        break;
    case 0xD2: // JP NC, nn
        cpu_jump(NC);
        break;
    case 0xDA: // JP C, nn
        cpu_jump(C);
        break;
    case 0xE9: // JP HL
        cpu_jump(HL);
        break;
    case 0x18: // JR n
        cpu_jr(NONE);
        break;
    case 0x20: // JR NZ,n
        cpu_jr(NZ);
        break;
    case 0x28: // JR Z,n
        cpu_jr(Z);
        break;
    case 0x30: // JR NC,n
        cpu_jr(NC);
        break;
    case 0x38: // JR C,n
        cpu_jr(C);
        break;
    case 0xCD: // CALL nn
        cpu_call(NONE);
        break;
    case 0xC4: // CALL NZ,n
        cpu_call(NZ);
        break;
    case 0xCC: // CALL Z,n
        cpu_call(Z);
        break;
    case 0xD4: // CALL NC,n
        cpu_call(NC);
        break;
    case 0xDC: // CALL C,n
        cpu_call(C);
        break;
    case 0xC7: // RST 00
        cpu_rst(0x00);
        break;
    case 0xCF: // RST 08
        cpu_rst(0x08);
        break;
    case 0xD7: // RST 10
        cpu_rst(0x10);
        break;
    case 0xDF: // RST 18
        cpu_rst(0x18);
        break;
    case 0xE7: // RST 20
        cpu_rst(0x20);
        break;
    case 0xEF: // RST 28
        cpu_rst(0x28);
        break;
    case 0xF7: // RST 30
        cpu_rst(0x30);
        break;
    case 0xFF: // RST 38
        cpu_rst(0x38);
        break;
    case 0xC9: // RET 
        cpu_ret(NONE);
        break;
    case 0xC0: // RET NZ
        cpu_ret(NZ);
        break;
    case 0xC8: // RET Z
        cpu_ret(Z);
        break;
    case 0xD0: // RET NC
        cpu_ret(NC);
        break;
    case 0xD8: // RET C
        cpu_ret(C);
        break;
    case 0xD9: // RETI C
        cpu_reti();
        break;
    default:
        break;
    }
}

void CB() {
    BYTE n = 0x00;
    switch (opcode)
    {
    case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x07: // RLC n
        cpu_rlc(Regs[opcode % 8]);
        break;
    case 0x06: // RLC (HL)
        n = read_memory(RegHL.data);
        cpu_rlc(&n);
        break;
    case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0F: // RRC n
        cpu_rrc(Regs[opcode % 8]);
        break;
    case 0x0E: // RRC (HL)
        n = read_memory(RegHL.data);
        cpu_rrc(&n);
        break;
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x17: // RL n
        cpu_rl(Regs[opcode % 8]);
        break;
    case 0x16: // RL (HL)
        n = read_memory(RegHL.data);
        cpu_rl(&n);
        break;
    case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1F: // RR n
        cpu_rr(Regs[opcode % 8]);
        break;
    case 0x1E:  // RR (HL)
        n = read_memory(RegHL.data);
        cpu_rr(&n);
        break;
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x27: // SLA n
        cpu_sla(Regs[opcode % 8]);
        break;
    case 0x26:  // SLA (HL)
        n = read_memory(RegHL.data);
        cpu_sla(&n);
        break;
    case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2F: // SRA n
        cpu_sra(Regs[opcode % 8]);
        break;
    case 0x2E:  // SRA (HL)
        n = read_memory(RegHL.data);
        cpu_sra(&n);
        break;
    case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x37: // SWAP n
        cpu_swap(Regs[opcode % 8]);
        break;
    case 0x36:  // SWAP (HL)
        n = read_memory(RegHL.data);
        cpu_swap(&n);
        break;
    case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3F: // SRL n
        cpu_srl(Regs[opcode % 8]);
        break;
    case 0x3E:  // SRL (HL)
        n = read_memory(RegHL.data);
        cpu_srl(&n);
        break;
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47: // BIT 0, n
    case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4F: // BIT 1, n
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57: // BIT 2, n
    case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5F: // BIT 3, n
    case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67: // BIT 4, n
    case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6F: // BIT 5, n
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77: // BIT 6, n
    case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7F: // BIT 7, n
        if (opcode & 0x0F < 8) {
            cpu_test_bit(((opcode & 0xF0) - 0x40) * 2, Regs[(opcode & 0xF0) % 8]);
        }
        else {
            cpu_test_bit(Regs[(((opcode & 0xF0) - 4) * 0x40) + 1], Regs[(opcode & 0xF0) % 8]);
        }
        break;
    case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E: // BIT n, HL
    case 0x76: case 0x7E:
        n = read_memory(RegHL.data);
        cpu_test_bit((opcode - 0x46) / 8, &n);
        break;
    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87: // RES 0, n
    case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F: // RES 1, n
    case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97: // RES 2, n
    case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9F: // RES 3, n
    case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA7: // RES 4, n
    case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAF: // RES 5, n
    case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB7: // RES 6, n
    case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBF: // RES 7, n
        if (opcode & 0x0F < 8) {
            cpu_reset_bit(((opcode & 0xF0) - 0x80) * 2, Regs[(opcode & 0xF0) % 8]);
        }
        else {
            cpu_teset_bit(Regs[(((opcode & 0xF0) - 4) * 0x80) + 1], Regs[(opcode & 0xF0) % 8]);
        }
        break;
    case 0x86: case 0x8E: case 0x96: case 0x9E: case 0xA6: case 0xAE: // RES n, HL
    case 0xB6: case 0xBE:
        n = read_memory(RegHL.data);
        cpu_reset_bit((opcode - 0x86) / 8, &n);
        break;
    case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC7: // SET 0, n
    case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCF: // SET 1, n
    case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD7: // SET 2, n
    case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDF: // SET 3, n
    case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE7: // SET 4, n
    case 0xE8: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEF: // SET 5, n
    case 0xF0: case 0xF1: case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF7: // SET 6, n
    case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFF: // SET 7, n
        if (opcode & 0x0F < 8) {
            cpu_set_bit(((opcode & 0xF0) - 0x80) * 2, Regs[(opcode & 0xF0) % 8]);
        }
        else {
            cpu_set_bit(Regs[(((opcode & 0xF0) - 4) * 0x80) + 1], Regs[(opcode & 0xF0) % 8]);
        }
        break;
    case 0xC6: case 0xCE: case 0xD6: case 0xDE: case 0xE6: case 0xEE: // SET n, HL
    case 0xF6: case 0xFE:
        n = read_memory(RegHL.data);
        cpu_set_bit((opcode - 0xC6) / 8, &n);
        break;
    default:
        break;
    }
}
  