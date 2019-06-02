#include "instructions.h"
#include "input.h"
#include "display.h"

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
        data |= 0x0F;
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
    // Joypad Register
    if (address == 0xFF00) {
        // Select Button Keys
        if (!test_bit(5, &rom[0xFF00])) {
            return get_button_keys();
        }
        // Select Direction Keys
        else if (!test_bit(4, &rom[0xFF00])) {
            return get_direction_keys();
        }
        else {
            return 0x0F;
        }
    }
    else {
        return rom[address];
    }
}

void cpu_load(BYTE *reg) {
    BYTE n = read_memory(PC++);
    *reg = n;
}

void cpu_loadReg(BYTE *reg1, BYTE *reg2) {
    *reg1 = *reg2;
}

void cpu_loadReg16(WORD *reg1, WORD *reg2) {
    *reg1 = *reg2;
}

void cpu_loadRegSP(BYTE *reg, Register *sp) {
    *reg = sp->lo;
    *(reg + 1) = sp->hi;
}

void LDHL_SP_n() {
    SIGNED_BYTE s_n = (SIGNED_BYTE)read_memory(PC++);
    BYTE u_n = (BYTE)s_n;
    RegAF.lo = 0x00;
    
    
  /*      // Set if there is a carry from bit 15
        if ((RegSP.data) + (u_n) > 0xFFFF) {
            cpu_set_bit(FLAG_C, &RegAF.lo);
        }

        // Set if there is a carry from bit 11
        if ((RegSP.data & 0xFFF) + (u_n & 0xFF) & 0xF000) {
            cpu_set_bit(FLAG_H, &RegAF.lo);
        }
        */
        if (((BYTE)RegSP.data + u_n) > 0xFF) {
            cpu_set_bit(FLAG_C, &RegAF.lo);  // Set if carry from bit 7
        }

        if ((((BYTE)RegSP.data) & 0x0F) + (u_n & 0x0F) > 0xF) {
            cpu_set_bit(FLAG_H, &RegAF.lo);  // Set if carry from bit 3
        }
      //  WORD value = (RegSP.data + n) & 0xFFFF;
       // RegHL.data = value;
        //unsigned int v = RegSP.data + n;
        //unsigned int x =  n;
        
       // if (v > 0xFFFF)
         //   cpu_set_bit(FLAG_C, &RegAF.lo);
        
       //if ((RegSP.data & 0xF) + (n & 0xF) > 0xF)
         //       cpu_set_bit(FLAG_H,& RegAF.lo);
      //  if ((RegSP.data & 0xFFF) + (x & 0xFF) & 0xF000) {
      //  }
       RegHL.data = (s_n + RegSP.data);
}

void stack_push(const BYTE *hi, const BYTE *lo) {
    write_memory(--RegSP.data, *hi);
    write_memory(--RegSP.data, *lo);
}

void stack_pop(BYTE *hi, BYTE *lo) {
    if (lo == &RegAF.lo)
        *lo = read_memory(RegSP.data++) & 0xF0;
    else 
        *lo = read_memory(RegSP.data++);

    *hi = read_memory(RegSP.data++);
}

void cpu_add(BYTE *reg1, BYTE *reg2) {
    RegAF.lo = 0x00;

    if ((*reg1 + *reg2) > 0xFF) {
        cpu_set_bit(FLAG_C, &RegAF.lo);  // Set if carry from bit 7
    }

    if ((*reg1 & 0x0F) + (*reg2 & 0x0F) > 0x0F) {
        cpu_set_bit(FLAG_H, &RegAF.lo);  // Set if carry from bit 3
    }

    *reg1 += *reg2;

    if (*reg1 == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_add16(WORD *reg1, WORD *reg2) {
    RegAF.lo &= 0x80;

    // Set if there is a carry from bit 15
    if ((*reg1 + *reg2) > 0xFFFF) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    // Set if there is a carry from bit 11
    if (((*reg1 & 0xFFF) + (*reg2 & 0xFFF)) > 0xFFF) {
        cpu_set_bit(FLAG_H, &RegAF.lo);
    }

    *reg1 += *reg2;

    cpu_reset_bit(FLAG_N, &RegAF.lo);
}

void cpu_add_sp_n(void) {
    SIGNED_BYTE s_n = (SIGNED_BYTE)read_memory(PC++);
    BYTE u_n = (BYTE)s_n;
    RegAF.lo = 0x00;
    /*
    if ((RegSP.data + *n) > 0xFF) {
        cpu_set_bit(FLAG_C, &RegAF.lo);  // Set if carry from bit 7
    }

    if ((RegSP.data & 0x0F) + (*n & 0x0F) > 0x0F) {
        cpu_set_bit(FLAG_H, &RegAF.lo);  // Set if carry from bit 3
    }
    */
    if (((BYTE)RegSP.data + u_n) > 0xFF) {
        cpu_set_bit(FLAG_C, &RegAF.lo);  // Set if carry from bit 7
    }

    if ((((BYTE)RegSP.data) & 0x0F) + (u_n & 0x0F) > 0xF) {
        cpu_set_bit(FLAG_H, &RegAF.lo);  // Set if carry from bit 3
    }

    RegSP.data += s_n;
}

void cpu_adc(BYTE *reg1, BYTE *reg2) {
    BYTE carry_flag = (RegAF.lo & 0x10) >> FLAG_C;
    RegAF.lo = 0x00;  

    if ((*reg1 + *reg2 + carry_flag) > 0xFF) {
        cpu_set_bit(FLAG_C, &RegAF.lo);  // Set if carry from bit 7
    }
    
    if ((*reg1 & 0x0F) + (*reg2 & 0x0F) + carry_flag > 0x0F) {
        cpu_set_bit(FLAG_H, &RegAF.lo);  // Set if carry from bit 3
    }

    *reg1 += *reg2;
    *reg1 += carry_flag;

    if (*reg1 == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_sub(BYTE *reg1, BYTE *reg2) {
    RegAF.lo = 0x00;
    if ((*reg1 - *reg2) < 0) {
        cpu_set_bit(FLAG_C, &RegAF.lo);  // Set if borrow 
    }
 
    if ((*reg1 & 0x0F) - (*reg2 & 0x0F) < 0) {
        cpu_set_bit(FLAG_H, &RegAF.lo);  // Set if borrow from bit 4
    }

    *reg1 -= *reg2;

    if (*reg1 == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }

    cpu_set_bit(FLAG_N, &RegAF.lo);
}

void cpu_sbc(BYTE *reg1, BYTE *reg2) {
    BYTE carry_flag = (RegAF.lo & 0x10) >> FLAG_C;
    RegAF.lo = 0x00;
  

    if ((*reg1 - *reg2 - carry_flag) < 0) {
        cpu_set_bit(FLAG_C, &RegAF.lo);  // Set if borrow
    }

    if (((*reg1 & 0x0F) - (*reg2 & 0x0F) - carry_flag) < 0) {
        cpu_set_bit(FLAG_H, &RegAF.lo);  // Set if borrow from bit 4
    }

    *reg1 -= *reg2;
    *reg1 -= carry_flag;

    if (*reg1 == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
    cpu_set_bit(FLAG_N, &RegAF.lo);

}

void cpu_and(BYTE *reg1, BYTE *reg2) {
    RegAF.lo = 0x00;
    *reg1 &= *reg2;
    if (*reg1 == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
    cpu_set_bit(FLAG_H, &RegAF.lo);
}

void cpu_or(BYTE *reg1, BYTE *reg2) {
    RegAF.lo = 0x00;
    *reg1 |= *reg2;
    if (*reg1 == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_xor(BYTE *reg1, BYTE *reg2) {
    RegAF.lo = 0x00;
    *reg1 ^= *reg2;
    if (*reg1 == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_cp(BYTE *reg) {
    RegAF.lo = 0x00;
    BYTE result = RegAF.hi - *reg;
    if (result == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
    
    if ((RegAF.hi) < (*reg)) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }
    
    if ((RegAF.hi & 0x0F) < (*reg & 0x0F)) {
        cpu_set_bit(FLAG_H, &RegAF.lo);
    }

    cpu_set_bit(FLAG_N, &RegAF.lo);
}

void cpu_inc(BYTE *reg) {
    RegAF.lo &= 1 << FLAG_C;
    
    if ((*reg & 0x0F) == 0x0F) {
        cpu_set_bit(FLAG_H, &RegAF.lo);
    }

    *reg += 1;

    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }

}

void cpu_inc_hl(WORD address) {
    BYTE data = read_memory(address);
    RegAF.lo &= 1 << FLAG_C;

    if ((data & 0x0F) == 0x0F) {
        cpu_set_bit(FLAG_H, &RegAF.lo);
    }

    data += 1;
    write_memory(address, data);

    if (data == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }

}

void cpu_inc16(WORD *reg) {
    *reg += 1;
}

void cpu_dec(BYTE *reg) {
    RegAF.lo &= (1 << FLAG_C);

    if ((*reg & 0x0F) == 0) {
        cpu_set_bit(FLAG_H, &RegAF.lo);
    }

    *reg -= 1;

    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }

    cpu_set_bit(FLAG_N, &RegAF.lo);
}

void cpu_dec_hl(WORD address) {
    BYTE data = read_memory(address);
    RegAF.lo &= (1 << FLAG_C);

    if ((data & 0x0F) == 0) {
        cpu_set_bit(FLAG_H, &RegAF.lo);
    }

    data -= 1;
    write_memory(address, data);

    if (data == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }

    cpu_set_bit(FLAG_N, &RegAF.lo);
}

void cpu_dec16(WORD *reg) {
    *reg -= 1;
}

void cpu_swap(BYTE *reg) {
    RegAF.lo = 0x00;
    BYTE upper_nibble = (*reg & 0x0F) << 4;
    BYTE lower_nibble = (*reg & 0xF0) >> 4;
    *reg = upper_nibble | lower_nibble;
    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_swap_hl(WORD address) {
    BYTE data = read_memory(address);
    RegAF.lo = 0x00;
    BYTE upper_nibble = (data & 0x0F) << 4;
    BYTE lower_nibble = (data & 0xF0) >> 4;
    data = upper_nibble | lower_nibble;
    write_memory(address, data);
    if (data == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_daa() {
    BYTE n = 0x00;

    // Previous instruction was addition
    if (!test_bit(FLAG_N, &RegAF.lo)) {
        if (RegAF.hi > 0x99 || test_bit(FLAG_C, &RegAF.lo)) {
            cpu_set_bit(FLAG_C, &RegAF.lo);
            n |= 0x60;
        }
        if (((RegAF.hi & 0x0F) > 0x09) || test_bit(FLAG_H, &RegAF.lo)) {
            n |= 0x06;
        }
    }
    // Previous instruction was subtraction
    else{
        if (test_bit(FLAG_C, &RegAF.lo)) {
            cpu_set_bit(FLAG_C, &RegAF.lo);
            
            if (!test_bit(FLAG_H, &RegAF.lo))
                n |= 0xA0;
        }
        if (test_bit(FLAG_H, &RegAF.lo)) {
            n |= 0x0A;

            if (test_bit(FLAG_C, &RegAF.lo))
                n |= 0x90;
            else
                n |= 0xF0;
        }
    }

    RegAF.hi += n;

    if (RegAF.hi == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
    else {
        cpu_reset_bit(FLAG_Z, &RegAF.lo);
    }
    cpu_reset_bit(FLAG_H, &RegAF.lo);
}

void cpu_cpl() {
    RegAF.hi ^= 0xFF;
    cpu_set_bit(FLAG_N, &RegAF.lo);
    cpu_set_bit(FLAG_H, &RegAF.lo);
}

void cpu_ccf() {
    RegAF.lo ^= (1 << FLAG_C);
    cpu_reset_bit(FLAG_N, &RegAF.lo);
    cpu_reset_bit(FLAG_H, &RegAF.lo);
}

void cpu_scf() {
    cpu_set_bit(FLAG_C, &RegAF.lo);
    cpu_reset_bit(FLAG_N, &RegAF.lo);
    cpu_reset_bit(FLAG_H, &RegAF.lo);
}

void cpu_halt() {
    set_halt();
}

void cpu_stop() {
}

void cpu_ei(int enable) {
    if (enable == 0) {
        IME = 0;
    }
    else {
        IME = 1;
    }
}

void cpu_rlca(void) {
    RegAF.lo = 0x00;
    BYTE bit = (RegAF.hi & 0x80) >> 7; // Save most significant bit
    RegAF.hi <<= 1;
    RegAF.hi |= bit;

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }
}

void cpu_rla(void) {
    BYTE cy = test_bit(FLAG_C, &RegAF.lo);  // Save carry flag
    RegAF.lo = 0x00;
    
    BYTE msb = test_bit(7, &RegAF.hi); // Save most significant bit
    RegAF.hi <<= 1;
    RegAF.hi |= cy;
    
    // The value of the carry flag is set to the LSB of the register
    if (cy) {
      cpu_set_bit(0, &RegAF.hi);
    }
    
    // The value of the RegA msb is set to the carry flag
    if (msb) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }
}

void cpu_rrca(void) {
    RegAF.lo = 0x00;
    BYTE bit = RegAF.hi & 0x01; // Save least significant bit
    RegAF.hi >>= 1;
    RegAF.hi |= (bit << 7);

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }
}

void cpu_rra(void) {
    BYTE carry_flag = (RegAF.lo & 0x10) >> FLAG_C; // Save carry flag
    RegAF.lo = 0x00;
    BYTE bit = RegAF.hi & 0x01; // Save least significant bit
    RegAF.hi >>= 1;
    RegAF.hi |= (carry_flag << 7); // The value of the carry flag is set to the MSB of the register

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }
}


void cpu_rlc(BYTE *reg) {
    RegAF.lo = 0x00;
    BYTE bit = (*reg & 0x80) >> 7; // Save most significant bit
    *reg <<= 1;
    *reg |= bit;

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (*reg == 0)
    {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_rlc_hl(WORD address) {
    BYTE data = read_memory(address);
    RegAF.lo = 0x00;
    BYTE bit = (data & 0x80) >> 7; // Save most significant bit
    data <<= 1;
    data |= bit;
    write_memory(address, data);

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (data == 0)
    {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_rl(BYTE *reg) {
    //BYTE carry_flag = (RegAF.lo & 0x10) >> FLAG_C;  // Save carry flag
    BYTE carry_flag = test_bit(FLAG_C, &RegAF.lo);
    RegAF.lo = 0x00;
    BYTE bit = (*reg & 0x80) >> 7; // Save most significant bit
    *reg <<= 1;
    *reg |= carry_flag; // The value of the carry flag is set to the LSB of the register

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_rl_hl(WORD address) {
    BYTE data = read_memory(address);
    BYTE carry_flag = test_bit(FLAG_C, &RegAF.lo);
    RegAF.lo = 0x00;
    BYTE bit = (data & 0x80) >> 7; // Save most significant bit
    data <<= 1;
    data |= carry_flag; // The value of the carry flag is set to the LSB of the register
    write_memory(address, data);

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (data == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_rrc(BYTE *reg) {
    RegAF.lo = 0x00;
    BYTE bit = *reg & 0x01; // Save least significant bit
    *reg >>= 1;
    *reg |= (bit << 7);

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_rrc_hl(WORD address) {
    BYTE data = read_memory(address);
    RegAF.lo = 0x00;
    BYTE bit = data & 0x01; // Save least significant bit
    data >>= 1;
    data |= (bit << 7);
    write_memory(address, data);

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (data == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_rr(BYTE *reg) {
    BYTE carry_flag = test_bit(FLAG_C, &RegAF.lo); // Save carry flag
    RegAF.lo = 0x00; 
    BYTE bit = *reg & 0x01; // Save least significant bit
    *reg >>= 1;
    *reg |= (carry_flag << 7); // The value of the carry flag is set to the MSB of the register

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_rr_hl(WORD address) {
    BYTE data = read_memory(address);
    BYTE carry_flag = test_bit(FLAG_C, &RegAF.lo); // Save carry flag
    RegAF.lo = 0x00;
    BYTE bit = data & 0x01; // Save least significant bit
    data >>= 1;
    data |= (carry_flag << 7); // The value of the carry flag is set to the MSB of the register
    write_memory(address, data);

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (data == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}


void cpu_sla(BYTE *reg) {
    RegAF.lo = 0x00;
    BYTE bit = (*reg & 0x80) >> 7; // Save most significant bit
    *reg <<= 1;

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_sla_hl(WORD address) {
    BYTE data = read_memory(address);
    RegAF.lo = 0x00;
    BYTE bit = (data & 0x80) >> 7; // Save most significant bit
    data <<= 1;
    write_memory(address, data);

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (data == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}


void cpu_sra(BYTE *reg) {
    RegAF.lo = 0x00;
    BYTE lsb = *reg & 0x01; // Save least significant bit
    BYTE msb = *reg & 0x80; // Save most significant bit
    *reg >>= 1;
    *reg |= msb;

    if (lsb == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_sra_hl(WORD address) {
    BYTE data = read_memory(address);
    RegAF.lo = 0x00;
    BYTE lsb = data & 0x01; // Save least significant bit
    BYTE msb = data & 0x80; // Save most significant bit
    data >>= 1;
    data |= msb;
    write_memory(address, data);

    if (lsb == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (data == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}


void cpu_srl(BYTE *reg) {
    RegAF.lo = 0x00;
    BYTE bit = *reg & 0x01; // Save least significant bit
    *reg >>= 1;

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (*reg == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_srl_hl(WORD address) {
    BYTE data = read_memory(address);
    RegAF.lo = 0x00;
    BYTE bit = data & 0x01; // Save least significant bit
    data >>= 1;
    write_memory(address, data);

    if (bit == 1) {
        cpu_set_bit(FLAG_C, &RegAF.lo);
    }

    if (data == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
}

void cpu_test_bit(BYTE b, BYTE *reg) {
    BYTE test_bit = 1 << b;
    if ((*reg & test_bit) == 0) {
        cpu_set_bit(FLAG_Z, &RegAF.lo);
    }
    else {
        cpu_reset_bit(FLAG_Z, &RegAF.lo);
    }
    
    cpu_reset_bit(FLAG_N, &RegAF.lo); 
    cpu_set_bit(FLAG_H, &RegAF.lo);
}

void cpu_set_bit(BYTE b, BYTE *reg) {
    BYTE set_bit = 1 << b;
    *reg |= set_bit;
}

void cpu_set_bit_hl(BYTE b, WORD address) {
    BYTE data = read_memory(address);
    BYTE set_bit = 1 << b;
    data |= set_bit;
    write_memory(address, data);
}

void cpu_reset_bit(BYTE b, BYTE *reg) {
    BYTE reset_bit = 1 << b;
    *reg &= ~reset_bit;
}

void cpu_reset_bit_hl(BYTE b, WORD address) {
    BYTE data = read_memory(address);
    BYTE reset_bit = 1 << b;
    data &= ~reset_bit;
    write_memory(address, data);
}

void cpu_jump(CONDITION cond) {
    WORD nn = 0x0000;
    BYTE n = read_memory(PC++);
    nn = n;
    nn |= read_memory(PC++) << 8;
    switch (cond)
    {
    case NONE:
        PC = nn;
        break;
    case NZ: if (test_bit(FLAG_Z, &RegAF.lo) == 0) {   // Jump if Z flag is reset
        PC = nn;
    }
        break;
    case Z:  
        if (test_bit(FLAG_Z, &RegAF.lo)) {     // Jump if Z flag is set 
        PC = nn;
        }
        break;
    case NC: if (test_bit(FLAG_C, &RegAF.lo) == 0) {   // Jump if C flag is reset
        PC = nn;
    }
        break;
    case C:  if (test_bit(FLAG_C, &RegAF.lo)) {     // Jump if C flag is set
        PC = nn;
    }
        break;
    case HL:                // Jump to the address contained in HL
        PC = RegHL.data;
        break;
    default:
        break;
    }
}

void cpu_jr(CONDITION cond) {
    SIGNED_BYTE n = (SIGNED_BYTE)read_memory(PC++);
    switch (cond)
    {
    case NONE:
        PC += n;
        break;
    case NZ: if (test_bit(FLAG_Z, &RegAF.lo) == 0) {    // Jump if Z flag is reset
        PC += n;
    }
        break;
    case Z:  if (test_bit(FLAG_Z, &RegAF.lo)) {     // Jump if Z flag is set 
        PC += n;
    }
        break;
    case NC: if (test_bit(FLAG_C, &RegAF.lo) == 0) {   // Jump if C flag is reset
        PC += n;
    }
        break;
    case C:  if (test_bit(FLAG_C, &RegAF.lo)) {    // Jump if C flag is set
        PC += n;
    }
        break;
    default:
        break;
    }
}

void cpu_call(CONDITION cond) {
    WORD nn = 0x0000;
    BYTE n = read_memory(PC++);
    nn = n;
    nn |= (read_memory(PC++) << 8);
    BYTE pc_hi = ((PC & 0xFF00) >> 8);
    BYTE pc_lo = (PC & 0xFF);
    switch (cond)
    {
    case NONE:
        stack_push(&pc_hi, &pc_lo);
        PC = nn;
        break;
    case NZ:
        if (test_bit(FLAG_Z, &RegAF.lo) == 0) {    // Jump if Z flag is reset
            stack_push(&pc_hi, &pc_lo);
            PC = nn;
        }
        break;
    case Z: if (test_bit(FLAG_Z, &RegAF.lo)) {     // Jump if Z flag is set 
        stack_push(&pc_hi, &pc_lo);
        PC = nn;
    }
       break;
   case NC: if (test_bit(FLAG_C, &RegAF.lo) == 0) {    // Jump if C flag is reset
       stack_push(&pc_hi, &pc_lo);
       PC = nn;
   }
       break;
   case C:  if (test_bit(FLAG_C, &RegAF.lo)) {     // Jump if C flag is set
       stack_push(&pc_hi, &pc_lo);
       PC = nn;
   }
       break;
    default:
        break;
    }
}

void cpu_rst(BYTE n) {
    BYTE hi = (PC & 0xFF00) >> 8;
    BYTE lo = (PC & 0x00FF);
    stack_push(&hi, &lo);
    PC = n;
}

void cpu_ret(CONDITION cond) {
    BYTE lo = 0x00;
    BYTE hi = 0x00;
    WORD nn = 0x0000;
    switch (cond)
    {
    case NONE:
        stack_pop(&hi, &lo);
        nn = hi << 8;
        nn |= lo;
        PC = nn;
        break;
        
    case NZ: if (test_bit(FLAG_Z, &RegAF.lo) == 0) {    // Jump if Z flag is reset
        stack_pop(&hi, &lo);
        nn = hi << 8;
        nn |= lo;
        PC = nn;
    }
        break;
    case Z: if (test_bit(FLAG_Z, &RegAF.lo)) {     // Jump if Z flag is set 
        stack_pop(&hi, &lo);
        nn = hi << 8;
        nn |= lo;
        PC = nn;
    }
       break;
   case NC: if (test_bit(FLAG_C, &RegAF.lo) == 0) {    // Jump if C flag is reset
       stack_pop(&hi, &lo);
       nn = hi << 8;
       nn |= lo;
       PC = nn;
   }
       break;
   case C: if (test_bit(FLAG_C, &RegAF.lo)) {     // Jump if C flag is set
       stack_pop(&hi, &lo);
       nn = hi << 8;
       nn |= lo;
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
    WORD nn = (hi << 8);
    nn |= lo;
    PC = nn;
    cpu_ei(1);
}

int test_bit(BYTE b, BYTE *reg) {
    int is_set = 1;
    BYTE test_bit = 1 << b;
    if ((*reg & test_bit) == 0) {
        is_set = 0;
    }
    return is_set;
}
