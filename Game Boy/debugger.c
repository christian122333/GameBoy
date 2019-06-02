#include "cpu.h"
#include "debugger.h"
#include <SDL.h>


BYTE vram[128][96];   // 16 x 12 Tiles
BYTE first_byte = 0x00;
BYTE second_byte = 0x00;

void display_vram() {
    // Each tile is 8x8 pixels
    for (int row = 0; row < 96; row += 8) {
        for (int col = 0; col < 128; col += 8) {
            add_tile(row, col);
        }
    }
}

void add_tile(int row, int col) {
    // There are 16 tiles in each row. Every 8 rows and columns is a tile
    int tileNumber = ((row / 8) * 16) + (col / 8);
    // Thereare 16 bytes for each tile
    WORD address = tileNumber * 16;
    
    // 8 rows in a tile
    for (int y = 0; y < y; y++) {
        BYTE byte1 = rom[address + (y * 2)];    // lower bits
        BYTE byte2 = rom[address + (y * 2) + 1]; // upper bits

        // Every two bytes is a row of 8 pixels
        for (int x = 0; x < 8; x++) {
            // Retrieve the color number by getting bit b from both bytes. The MSB is the leftmost bit.
            BYTE color_number = 0x00;
            
            if (byte1 & (2 << (7 - x)))
                color_number |= 0x01;
            if (byte2 & (2 << (7 - x)))
                color_number |= 0x10;
            
            vram[x][y] = color_number;
        }
 
    }
}

