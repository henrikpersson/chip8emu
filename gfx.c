#include <SDL2/SDL.h>
#include "gfx.h"

void render(uint8_t *buffer) {
    for (int i = 0; i < SCREEN_PIXELS; i++) {
        uint8_t pixel = buffer[i];
        if (pixel) {
            printf("oo");
        } else {
            printf("  ");
        }

        if (i % SCREEN_WIDTH == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

#define SDL_BYTES_PER_PIXEL 4
#define SDL_WINDOW_WIDTH 800
#define SDL_WINDOW_HEIGHT 600

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
uint8_t pixels[SDL_BYTES_PER_PIXEL * SCREEN_WIDTH * SCREEN_HEIGHT]; // 4 == RGBA

void init_sdl() {
    SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("chip8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void render_sdl(uint8_t *buffer) {
    // clear sdl backbuffer
    SDL_RenderClear(renderer);

    memset(pixels, 0, SCREEN_PIXELS * SDL_BYTES_PER_PIXEL);

    for (int i = 0; i < SCREEN_PIXELS; i++) {
        uint8_t pixel = buffer[i];
        // uint8_t x = i % SCREEN_WIDTH;
        // uint8_t y = i / SCREEN_WIDTH;
        size_t sdl_offset = i * 4;
        if (pixel) {
            size_t sdl_offset = i * 4;
            pixels[sdl_offset] = 0xff; // R
            pixels[sdl_offset + 1] = 0xff; // G
            pixels[sdl_offset + 2] = 0xff; // B
        } 
        else {
            // clear: set to bg (black)
            pixels[sdl_offset] = 0x0; // R
            pixels[sdl_offset + 1] = 0x0; // G
            pixels[sdl_offset + 2] = 0x0; // B
        }
    }

    SDL_UpdateTexture(texture, NULL, &pixels[0], SCREEN_WIDTH * 4);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void render_dbg(uint8_t I, uint16_t sp, uint16_t pc, uint8_t *registers, bool *key_states, char *curr_op_dissasd) {
    (void)I;
    (void)sp;
    (void)pc;
    (void)registers;
    (void)key_states;
    (void)curr_op_dissasd;
}

void destroy_sdl() {
    SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
