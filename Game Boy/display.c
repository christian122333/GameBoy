#include <SDL.h>
#include <stdio.h>
#include "display.h"
#include "interrupts.h"
#include <stdbool.h>

#define WIDTH 960
#define HEIGHT 864
#define LY 0xFF44
#define LYC 0xFF45
#define STATUS 0xFF41

BYTE *lcd_ctrl = &rom[0xFF40];
BYTE display[160][144];
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Rect pixel;
int pixelW;
int pixelH;
int prev_mode = 0;
int mode_clock = 0;

/* 0xFF41 LCDC Status (R/W)
    Bit 6 - LYC=LY Coincidence Interrupt (1 = Enable) (R/W)
    Bit 5 - Mode 2 OAM Interrupt (1 = Enable) (R/W)
    Bit 4 - Mode 1 V-Blank Interrupt (1 = Enable) (R/W)
    Bit 3 - Mode 0 H-Blank Interrupt (1 = Enable) (R/W)
    Bit 2 - Coincidence Flag (0: LYC<>LY, 1: LYC = LY) (R Only)
    Bit 1-0 - Mode Flag (Mode 0-3) (R Only)
        0: During H-Blank
        1: During V-Blank
        2: During Searching OAM-RAM
        3: During Transferring Data to LCD Driver
 */

void draw(int cycles) {
   if (test_bit(7, lcd_ctrl) == 0) {
        rom[LY] = 0;
        rom[STATUS] &= 252;
        rom[STATUS] &= ~1;
        return;
   }

    mode_clock += cycles;
    switch (get_stat_mode()) {

        // Scanning OAM
    case 2:
        if (mode_clock >= 80) {
            mode_clock = 0;
            set_stat_mode(3);
        }
        break;

        // Reading OAM and VRAM
    case 3:
        if (mode_clock >= 172) {
            mode_clock = 0;
            set_stat_mode(0);
            draw_scanline();
        }
        break;

        // Horizontal blanking
    case 0:
        if (mode_clock >= 204) {
            mode_clock = 0;
            rom[LY] += 1;
            
            if (rom[LY] == 143) {
                set_stat_mode(1);
                render_display();
            }
            else {
                set_stat_mode(2);
            }
        }
        break;

        // Vertical blanking
    case 1:
        if (mode_clock >= 456) {
            // Request V-Blank Interrupt
            if (rom[LY] == 144) {
                request_interrupt(0);
            }
            mode_clock = 0;
            rom[LY] += 1;
            
            if (rom[LY] > 153) {
                set_stat_mode(2);
                rom[LY] = 0;
            }
        }
        break;
    }

    // Change mode flag in LCD Status Register
    if (prev_mode != get_stat_mode()) {
        prev_mode = get_stat_mode();

        // If interrupt is enabled for mode 0,1,or 2, then request lcd interrupt
        if (get_stat_mode() != 3) {
            BYTE b = test_bit(get_stat_mode() + 3, &rom[STATUS]);
            if (b)
                request_interrupt(LCD);
        }
    }

    if (rom[LY] == rom[LYC]) {
        rom[STATUS] |= 0x04;
        if(test_bit(6, &rom[STATUS]))
            request_interrupt(LCD);
    }
    else {
        rom[STATUS] &= ~0x04;
    }
}

/*  0xFF40 LCD Control (R/W)
    Bit 7 - LCD Display Enable  (0 = Off, 1 = On)
    Bit 6 - Window Tile Map Display Select (0 = 9800-9BFF, 1 = 9C00-9FFF)
    Bit 5 - Window Display Enable   (0 = Off, 1 = On)
    Bit 4 - BG & Window Tile Data Select  (0 = 8800-97FF, 1 = 8000-8FFF)
    Bit 3 - BG Tile Map Display Select  (0 = 9800-9BFF, 1 = 9C00-9FFF)
    Bit 2 - OBJ (Sprite) Size   (0 = 8x8, 1 = 8x16)
    Bit 1 - OBJ (Sprite) Display Enable (0 = Off, 1 = On)
    Bit 0 - BG Display  (0 = Off, 1 = On)
*/

void draw_scanline() {
  if (test_bit(0, lcd_ctrl) == 1) {
       draw_tile();
  }
  if (test_bit(1, lcd_ctrl) == 1) {
      draw_sprites();
  }
}

void draw_tile() {
    /*  Specifies the position in the 256x256 pixels BG map where the upper left corner of the LCD is to be displayed */
    int scrollY = read_memory(0xFF42);
    int scrollX = read_memory(0xFF43);
    
    int bg_map_addr, tile_data_addr;

    
    if (test_bit(6, lcd_ctrl)) {
        bg_map_addr = 0x9C00;
    }
    else {
       bg_map_addr = 0x9800;
    }

    if (test_bit(4, lcd_ctrl)) {
        tile_data_addr = 0x8000;
    }
    else {
        tile_data_addr = 0x8800;
    }

    int scanline = read_memory(0xFF44);
    int yPos = scrollY + scanline;

    /* The screen can wrap around bg map, so % 256 is used to set the position at the top if its greater than 256. 
       Then it's divided by 8 to show which of the 32 tiles the display should be rendering. Each row has 32 bytes,
       so multiply the by 32*/
    int vertical_tile = ((yPos % 256) / 8) * 32;

    /* Set the pixels in the display for the current scanline */
    for (int x = 0; x < 20; x++) {
        
        int xPos = scrollX + x;
        int horizontal_tile = (xPos % 256) / 8;
        
        /* Retrieve index of tile to render */
        int tile_num = read_memory(bg_map_addr + vertical_tile + horizontal_tile);
        
        /* The tile data is 8x8 pixels. Eight rows that contain two bytes of data */
        int tile_data_row = (yPos % 8) * 2;
        
        /* Retrieve the tile data lower byte */
        BYTE tile_data_lb = read_memory((tile_num * 16) + tile_data_row + tile_data_addr);
        /* Retrieve the tile data upper byte */
        BYTE tile_data_ub = read_memory((tile_num * 16) + tile_data_row + tile_data_addr + 1);

        // Draw 8 pixels for the row of tile data
        for (int i = 7; i >= 0; i--) {
            int color = 0;
            if (test_bit(i, &tile_data_lb) == 0 && test_bit(i, &tile_data_ub) == 0) {
                color = 0;  // Black
            }
            else if (test_bit(i, &tile_data_lb) == 0 && test_bit(i, &tile_data_ub) == 1) {
                color = 1; // Dark Grey
            }
            else if (test_bit(i, &tile_data_lb) == 1 && test_bit(i, &tile_data_ub) == 0) {
                color = 2; // Light Grey
            }
            else if (test_bit(i, &tile_data_lb) == 1 && test_bit(i, &tile_data_ub) == 1) {
                color = 3;  // White
            }
            display[(x * 8) + i][scanline] = color;
        }
    }
}

void draw_sprite() {
    int scanline = read_memory(0xFF44);
    int sprite_size;

    if (test_bit(2, lcd_ctrl) == 1) {
        sprite_size = 16;
    }
    else {
        sprite_size = 8;
    }

    for (int i = 0; i < 40; i++) {
        /* There can be 40 sprites in the scene, and each sprite attribute is 4 bytes long */
        BYTE yPos = read_memory(0xFE00 + ((i * 4) + 0));
        BYTE xPos = read_memory(0xFE00 + ((i * 4) + 1));
        BYTE tile_num = read_memory(0xFE00 + ((i * 4) + 2));
        BYTE flags = read_memory(0xFE00 + ((i * 4) + 3));
        
        if (sprite_size = 8) {
            if (scanline < (yPos - 16) || scanline > (yPos - 16 + 7)) {
                continue;
            }
            if (xPos == 0 || xPos >= 168) {
                continue;
            }
        }
        else if (sprite_size = 16) {
            if (scanline < (yPos - 16) || scanline > (yPos - 16 + 15)) {
                continue;
            }
            if (xPos == 0 || xPos >= 168) {
                continue;
            }
        }

        int sprite_row = scanline - (yPos - 16);
        /* Retrieve the tile data lower byte */
        BYTE tile_data_lb = read_memory((tile_num * 16) + sprite_row + 0x8000);
        /* Retrieve the tile data upper byte */
        BYTE tile_data_ub = read_memory((tile_num * 16) + sprite_row + 0x8000 + 1);

        // Draw 8 pixels for the row of tile data
        for (int i = 7; i >= 0; i--) {
            if ((xPos - 8) + i < 0 || (xPos - 8) + i > 159) {
                continue;
            }
            int color = 0;
            if (test_bit(i, &tile_data_lb) == 0 && test_bit(i, &tile_data_ub) == 0) {
                color = 0;  // Black
            }
            else if (test_bit(i, &tile_data_lb) == 0 && test_bit(i, &tile_data_ub) == 1) {
                color = 1; // Dark Grey
            }
            else if (test_bit(i, &tile_data_lb) == 1 && test_bit(i, &tile_data_ub) == 0) {
                color = 2; // Light Grey
            }
            else if (test_bit(i, &tile_data_lb) == 1 && test_bit(i, &tile_data_ub) == 1) {
                color = 3;  // White
            }
            display[(xPos - 8) + i][scanline] = color;
        }
    }


}

int get_stat_mode(void) {
    return (rom[STATUS] & 0x03);
}

void set_stat_mode(unsigned int mode) {
    if (mode > 3) {
        return;
    }
    // Clear mode
    rom[STATUS] &= ~(0x03);
    // Set mode
    rom[STATUS] |= mode;
}

int display_init() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Game Boy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WIDTH, HEIGHT,  SDL_WINDOW_MOUSE_FOCUS);
    if (window == NULL) {
        printf("Could no create window: %s\n", SDL_GetError());
        return 1;
    }
    renderer = SDL_CreateRenderer(window, -1, 0);

    pixelW = WIDTH / 160;
    pixelH = HEIGHT / 144;
    pixel.w = pixelW;
    pixel.h = pixelH;

    for (int y = 0; y < 144; y++) {
        for (int x = 0; x < 160; x++) {
            display[x][y] = 0;
        }
    }
    for (int y = 140; y < 144; y++) {
        for (int x = 0; x < 160; x++) {
            display[x][y] = 1;
        }
    }

    return 0;
}

void render_display() {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int y = 0; y < 144; y++) {
        for (int x = 0; x < 160; x++) {

            if (display[x][y] == 0) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }
            else if (display[x][y] == 1) {
                SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
            }
            else if (display[x][y] == 2) {
                SDL_SetRenderDrawColor(renderer, 96, 96, 96, 255);
            }
            else if (display[x][y] == 3) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }
            pixel.x = pixelW * x;
            pixel.y = pixelH * y;
            SDL_RenderFillRect(renderer, &pixel);
        }
    }
    SDL_RenderPresent(renderer);
}
