#include "cpu.h"
#include "timer.h"
#include "interrupts.h"

const WORD DIV = 0xFF04;    // Divider Register
const WORD TIMA = 0xFF05;   // Timer Counter
const WORD TMA = 0xFF06;    // Timer Modulo
const WORD TAC = 0xFF07;    // Timer Control

int timer_cycles = 0;
int divider_cycles = 0;

int timer_clock;



void timer(int cycles) {
    
    // Divider is incremented at a rate of 16384 Hz. 256 = cpu speed / frequency 
    if ((divider_cycles += cycles) >= 256) {
        divider_cycles = 0;
        rom[DIV] += 1;
    }
    
    // If timer is not enabled, then return
    if (!timer_enable())
        return;
    
    // Increment the timer by one according to the frequency in the timer control
    if ((timer_cycles += cycles) >= timer_clock) {
        rom[TIMA] += 1;
        
        // Once the timer equals 255, request an interrupt, change the timer frequency, and reset the timer to the value in modulo
        if (rom[TIMA] == 255) {
            request_interrupt(TIMER);
            set_clock();
            rom[TIMA] = rom[TMA];
        }
    }
}

void set_clock() {
    switch (rom[TAC] & 0x03) {
    // timer clock = clock speed / timer frequency
    case 0x00:  // 4096 Hz
        timer_clock = 1024;
        break;
    case 0x01:  // 262144 Hz
        timer_clock = 16;
        break;
    case 0x02:  // 65536 Hz
        timer_clock = 64;
        break;  
    case 0x03:  // 16384 Hz
        timer_clock = 256;
        break;
    }
}

int timer_enable() {
    // Check Bit 2 of the timer control to see if clock is enabled
    if ((rom[TAC] & 0x04) > 0)
        return 1;
    else 
        return 0;
}