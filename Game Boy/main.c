#include "cpu.h"
#include "instructions.h"
#include "input.h"
#include "interrupts.h"
#include "timer.h"
#include "display.h"
#include <stdio.h>

unsigned int num_cycles = 0;
int main(int argc, const char* argv[])
{
   cpu_init();
   load_rom("tetris.gb");
   if (display_init() == 1) {
      return 1;
   }
    while (1)
    {
        num_cycles = execute();
        draw(num_cycles);
        timer(num_cycles);
        interrupt_handler();
        handle_input();
    }
}