#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <SDL2/SDL.h>

#define SDL 0
#define DRAW 1
#define DEBUG 0

#include "cpu.h"

#if DEBUG
#include "disass.h"
#endif

#include "gfx.h"

#define TICKS_PER_FRAME 15 // how many??
#define FRAMES_PER_SECOND 60

int keymap[] = {
    SDLK_KP_0,
    SDLK_KP_1,
    SDLK_KP_2,
    SDLK_KP_3,
    SDLK_KP_4,
    SDLK_KP_5,
    SDLK_KP_6,
    SDLK_KP_7,
    SDLK_KP_8,
    SDLK_KP_9,
    SDLK_a,
    SDLK_b,
    SDLK_c,
    SDLK_d,
    SDLK_e,
    SDLK_f,
};

uint64_t gettimestamp() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t) 1000000 * tv.tv_sec + tv.tv_usec;
}

bool handle_input(SDL_Event *event, CPU *cpu) {
    bool user_exit = false;
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT) {
            user_exit = true;
        }

        switch (event->type) {
            case SDL_KEYDOWN: {
                if (event->key.keysym.sym == SDLK_ESCAPE || event->key.keysym.sym == SDLK_q) {
                    user_exit = true;
                    break;
                }

                for (int i = 0; i < NUM_KEYS; i++) {
                    if (keymap[i] == event->key.keysym.sym) {
                        cpu->keys[i] = true;
                    }
                }
            } break;
            case SDL_KEYUP: {
                for (int i = 0; i < NUM_KEYS; i++) {
                    if (keymap[i] == event->key.keysym.sym) {
                        cpu->keys[i] = false;
                    }
                }
            } break;
            case SDL_QUIT: user_exit = true; break;
        }
    }
    return user_exit;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: %s rom.ch8", argv[0]);
        exit(1);
    }

    srand(time(0));

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        printf("fopen");
        exit(1);
    }

    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    CPU *cpu = init();

    uint8_t program[fsize];
    fread(program, fsize, 1, f);
    load(cpu, program, fsize);

    fclose(f);

    #if SDL
        init_sdl();
    #endif

    SDL_Event event;
    bool user_exit = false;
    while(!cpu->exit && !user_exit) { 
        uint64_t frame_start_ts = gettimestamp();

        #if DEBUG 
        char curr_op_dissasd[128];
        #endif

        for (int tick = 0; tick < TICKS_PER_FRAME; tick++) {
            if (cpu->exit || user_exit) break;

            user_exit = handle_input(&event, cpu);

            // TODO: use ncurses for nice gdb output, sdl for graphx output
            #if DEBUG
                disass(curr_op_dissasd, cpu->memory, cpu->pc);
                printf("%s\n", curr_op_dissasd);
            #endif

            // tick
            exec(cpu);
        }

        if (cpu->draw) {
            #if SDL
                render_sdl(cpu->display);
            #elif DRAW
                render(cpu->display);
            #endif

            cpu->draw = false;
        }

        // Timers, supposed to go down at 60hz so this simple decr 1 will work as long as the the framerate is 60fps
        if (cpu->sound_timer > 0) {
            // TODO: BEEP!
            cpu->sound_timer--;
        }

        if (cpu->delay_timer > 0) {
            cpu->delay_timer--;
        }

        while(gettimestamp() < (frame_start_ts + (1000000 / FRAMES_PER_SECOND)));
        // SDL_Delay(sleeptime/1000);
    }

    // free(cpu->memory);
    printf("cleaning up!");
    free(cpu);

    #if SDL
        destroy_sdl();
    #endif

    return 0;
}
