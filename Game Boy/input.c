#include <SDL.h>
#include "cpu.h"
#include "instructions.h"

SDL_Event user_event;
BYTE direction_keys = 0x0F;
BYTE input_keys = 0x0F;

void handle_input() {
    while (SDL_PollEvent(&user_event)) {
        if (user_event.type == SDL_KEYDOWN) {
            switch (user_event.key.keysym.sym) {
                // Input Right
            case SDLK_d:
                cpu_reset_bit(0, &direction_keys);
                break;
                // Input Left
            case SDLK_a:
                cpu_reset_bit(1, &direction_keys);
                break;
                // Input Up
            case SDLK_w:
                cpu_reset_bit(2, &direction_keys);
                break;
                // Input Down
            case SDLK_s:
                cpu_reset_bit(3, &direction_keys);
                break;
               
                // Button A
            case SDLK_LEFT:
                cpu_reset_bit(0, &input_keys);
                break;
                // Button B
            case SDLK_UP:
                cpu_reset_bit(0, &input_keys);
                break;
                // Select
            case SDLK_SPACE:
                cpu_reset_bit(0, &input_keys);
                break;
                // Start
            case SDLK_RETURN:
                cpu_reset_bit(0, &input_keys);
                break;
            }

        }
        else if (user_event.type == SDL_KEYUP) {
            switch (user_event.key.keysym.sym) {
                // Input Right
            case SDLK_d:
                cpu_set_bit(0, &direction_keys);
                break;
                // Input Left
            case SDLK_a:
                cpu_set_bit(1, &direction_keys);
                break;
                // Input Up
            case SDLK_w:
                cpu_set_bit(2, &direction_keys);
                break;
                // Input Down
            case SDLK_s:
                cpu_set_bit(3, &direction_keys);
                break;
                           
                // Button A
            case SDLK_LEFT:
                cpu_set_bit(0, &input_keys);
                break;
                // Button B
            case SDLK_UP:
                cpu_set_bit(1, &input_keys);
                break;
                // Select
            case SDLK_SPACE:
                cpu_set_bit(2, &input_keys);
                break;
                // Start
            case SDLK_RETURN:
                cpu_set_bit(3, &input_keys);
                break;
            }
        }
    }
}

BYTE get_direction_keys(void) {
    return direction_keys;
}

BYTE get_button_keys(void) {
    return direction_keys;
}