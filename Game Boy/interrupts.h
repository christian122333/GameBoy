#ifndef INTERRUPTS_H
#define INTERRUPTS_H

enum{
    VBLANK, LCD, TIMER, SERIAL, JOYPAD
} INTERRUPT;

void request_interrupt(BYTE bit);
void interrupt_handler();
void execute_interrupt(int interrupt);
#endif