#include <curses.h>
#include <stdint.h>

#define SCREEN_HEIGHT 32
#define SCREEN_WIDTH 64
#define SCREEN_PIXELS SCREEN_HEIGHT * SCREEN_WIDTH

void render(uint8_t *buffer);

void init_sdl();
void render_sdl(uint8_t *buffer);
void render_dbg(uint8_t I, uint16_t sp, uint16_t pc, uint8_t *registers, bool *key_states, char *curr_op_dissasd);
void destroy_sdl();
