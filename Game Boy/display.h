#ifndef DISPLAY_H
#define DISPLAY_H
#include "cpu.h"
#include "instructions.h"

#define LCD_Control 0xFF40
int display_init();
int check_state();
unsigned int get_color(int color_number);
int get_stat_mode(void);
void set_stat_mode(unsigned int mode);
void render_display();
void draw(int cycles);
void draw_scanline(void);
void draw_tile(void);
void draw_sprites(void);
#endif // !DISPLAY_H