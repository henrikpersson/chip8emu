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

WINDOW* win = NULL;
WINDOW* dgb_win = NULL;
WINDOW* init_ncurses() {
    initscr();
    cbreak();
    noecho();
    timeout(0); // make getch from stdin non-blocking
    // clear();

	int starty = (LINES - SCREEN_HEIGHT) / 2;
	int startx = (COLS - SCREEN_WIDTH) / 2;
    refresh();
	win = newwin(SCREEN_HEIGHT, SCREEN_WIDTH, starty, startx);
	// box(win, 0 , 0);
	// wrefresh(win);		/* Show that box 		*/
    box(win, '|', '-');
    touchwin(win);
    // wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');

    dgb_win = newwin(SCREEN_HEIGHT, SCREEN_HEIGHT, starty, startx + SCREEN_WIDTH);
    box(dgb_win, '|', '-');
    touchwin(dgb_win);
    wrefresh(dgb_win);

    return win;
}

void render_ncurses(uint8_t *buffer) {
    for (int i = 0; i < SCREEN_PIXELS; i++) {
        uint8_t pixel = buffer[i];
        uint8_t x = i % SCREEN_WIDTH;
        uint8_t y = i / SCREEN_WIDTH;
        if (pixel) {
            mvwaddch(win, y, x, ACS_CKBOARD);
            // mvwaddch(win, y, x, ACS_DIAMOND);
            // mvwprintw(win, y, x, "█");
            // mvwvline(win, y, x, 10, 10);
        }
        wrefresh(win);
    }
}

void render_dbg(uint8_t I, uint16_t sp, uint16_t pc, uint8_t *registers, bool *key_states, char *curr_op_dissasd) {
    int y = 1;

    wclear(dgb_win);
    box(dgb_win, '|', '-');

    // TODO: render opcode
    // TODO: show # cycles

    // TODO instead of curr_op_dissasd pass the whole mem here.
    // we wanna show the a whole frame of disassembled opcodes and highlight the current one.
    // range mem[pc - 10] - mem[pc + pc] or something.

    mvwprintw(dgb_win, y++, 2, "%s", curr_op_dissasd);
    mvwprintw(dgb_win, y++, 2, "PC:\t0x%04x", pc);
    mvwprintw(dgb_win, y++, 2, "SP:\t0x%04x", sp); // TODO: print stack contents
    mvwprintw(dgb_win, y++, 2, "I:\t0x%04x", I);
    for (int i = 0; i < 16; i++) {
        // mvwprintw(dgb_win, y++, 2, "V[%01x]:\t0x%02x", i, registers[i]);
        if (registers)
        mvwprintw(dgb_win, y++, 2, "key[%01x]:\t%d", i, key_states[i]);
    }
    wrefresh(dgb_win);
}

void destroy_ncurses() {
    delwin(win);
    delwin(dgb_win);
    endwin();
}
