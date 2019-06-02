#include "cpu.h"
#include "instructions.h"

void request_interrupt(BYTE bit) {
    // Request an interrupt by setting the corresponding bit in the Interrupt Flag
    cpu_set_bit(bit, &rom[0xFF0F]);
    // Resume cpu execution if halt was set
    reset_halt();
}

void execute_interrupt(int interrupt) {
    // Disable interrupt master flag while executing an interrupt
    IME = 0;

    // Push current PC onto stack
    BYTE hi = (PC & 0xFF00) >> 8;
    BYTE lo = (PC & 0x00FF);
    stack_push(&hi, &lo);

    cpu_reset_bit(interrupt, &rom[0xFF0F]);
    // Set PC to corresponding ISR
    switch (interrupt) {
    case 0:
        PC = 0x40;  // V-Blank
        break;
    case 1:
        PC = 0x48;  // LCD STAT
        break;
    case 2:
        PC = 0x50;  // Timer
        break;
    case 3:
        PC = 0x58;  // Serial
        break;
    case 4:
        PC = 0x60;  // Joypad
        break;
    }
}

void interrupt_handler() {
    if (IME == 1) {
        BYTE interrupt_flag = rom[0xFF0F];
        if (interrupt_flag > 0) {
            for (int i = 4; i >= 0; i--) {
                // Check if interrupt flag is set for x bit
                if (test_bit(i, &rom[0xFF0F]) == 1) {
                    // If interrupt is enabled, then execute it
                    if (test_bit(i, &rom[0xFFFF])) {
                        execute_interrupt(i);
                    }
                }
            }
        }
    }
}
