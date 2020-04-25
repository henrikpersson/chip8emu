#ifndef cpu_h
#define cpu_h

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define NUM_KEYS 16

typedef struct {
    uint8_t memory[0x1000];
    uint8_t display[64*32]; // TODO: display should not be part of the CPU.
    uint8_t registers[16];
    uint16_t I; // special address register
    uint16_t sp;
    uint16_t pc;
    uint8_t sound_timer;
    uint8_t delay_timer;

    // TODO: these kinda doesn't belong here, but here they are.
    bool keys[NUM_KEYS];
    bool draw;
    bool exit;
} CPU;


CPU* init();
void load(CPU* cpu, uint8_t *program, size_t size);
void exec(CPU* cpu);

#endif
