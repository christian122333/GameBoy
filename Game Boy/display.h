#ifndef DISPLAY_H
#define DISPLAY_H

#include "cpu.h"
#include "instructions.h"

/*
#define BLACK 0xFF000000
#define DARK_GRAY 0xFF555555
#define LIGHT_GRAY 0xFFAAAAAA
#define WHITE 0xFFFFFFFF
*/
enum {
    BLACK, DARK_GRAY, LIGHT_GRAY, WHITE
}COLOUR;

#define LCD_Control 0xFF40
extern BYTE display[160][144];
int display_init();
int get_stat_mode(void);
void set_stat_mode(unsigned int mode);
void render_display();
void draw(int cycles);
void draw_scanline(void);
void draw_tile(void);
void draw_sprite(void);
#endif // !DISPLAY_H