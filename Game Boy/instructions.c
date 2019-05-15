#include "instructions.h"

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
    if ((*reg1 + *reg2) > 0xFF) {
        cpu_set_bit(FLAG_C, RegAF.lo);  // Set if carry from bit 7
    }
    if ((*reg1 & 0x0F) + (*reg2 & 0x0F) > 0x0F) {
        cpu_set_bit(FLAG_H, RegAF.lo);  // Set if carry from bit 3
    }

    *reg1 += *reg2;

    if (*reg1 == 0) {
        cpu_set_bit(FLAG_Z, RegAF.lo);
    }
    cpu_reset_bit(FLAG_N, RegAF.lo);
}

void cpu_add16(WORD *reg1, WORD *reg2) {
}

void cpu_adc(BYTE *reg1, BYTE *reg2) {
    BYTE carry_flag = (RegAF.lo & 0x10) >> FLAG_C;

    if ((*reg1 + *reg2 + carry_flag) > 0xFF) {
        cpu_set_bit(FLAG_C, RegAF.lo);  // Set if carry from bit 7
    }
    if ((*reg1 & 0x0F) + (*reg2 & 0x0F) + carry_flag > 0x0F) {
        cpu_set_bit(FLAG_H, RegAF.lo);  // Set if carry from bit 3
    }

    *reg1 += *reg2;
    *reg1 += carry_flag;

    if (*reg1 == 0) {
        cpu_set_bit(FLAG_Z, RegAF.lo);
    }
    cpu_reset_bit(FLAG_N, RegAF.lo);

}

void cpu_sub(BYTE *reg1, BYTE *reg2) {
    if ((*reg1 - *reg2) > 0) {
        cpu_set_bit(FLAG_C, RegAF.lo);  // Set if no borrow 
    }
    if ((*reg1 & 0x0F) - (*reg2 & 0x0F) > 0) {
        cpu_set_bit(FLAG_H, RegAF.lo);  // Set if no borrow from bit 4
    }

    *reg1 -= *reg2;

    if (*reg1 == 0) {
        cpu_set_bit(FLAG_Z, RegAF.lo);
    }
    cpu_set_bit(FLAG_N, RegAF.lo);
}

void cpu_sbc(BYTE *reg1, BYTE *reg2) {
    BYTE carry_flag = (RegAF.lo & 0x10) >> FLAG_C;

    if ((*reg1 - *reg2 - carry_flag) > 0) {
        cpu_set_bit(FLAG_C, RegAF.lo);  // Set if no borrow
    }
    if ((*reg1 & 0x0F) - (*reg2 & 0x0F) - carry_flag > 0) {
        cpu_set_bit(FLAG_H, RegAF.lo);  // Set if no borrow from bit 4
    }

    *reg1 -= *reg2;
    *reg1 -= carry_flag;

    if (*reg1 == 0) {
        cpu_set_bit(FLAG_Z, RegAF.lo);
    }
    cpu_set_bit(FLAG_N, RegAF.lo);

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
    *reg = ((*reg & 0x0F) << 4) | ((*reg & 0xF0) >> 4);
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
void cpu_rlc(BYTE *reg) {
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

void cpu_rl(BYTE *reg) {
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

void cpu_rrc(BYTE *reg) {
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

void cpu_rr(BYTE *reg) {
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

void cpu_sla(BYTE *reg) {
    BYTE bit = (*reg & 0x80 >> 7); // Save most significant bit
    *reg <<= 1;

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


void cpu_sra(BYTE *reg) {
    BYTE lsb = *reg & 0x01; // Save least significant bit
    BYTE msb = *reg & 0x08; // Save most significant bit
    *reg >>= 1;
    *reg |= msb;

    if (lsb == 1) {
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


void cpu_srl(BYTE *reg) {
    BYTE bit = *reg & 0x01; // Save least significant bit
    *reg >>= 1;

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
    C:  if ((RegAF.lo & (1 << FLAG_C)) == 1) {     // Jump if C flag is set
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