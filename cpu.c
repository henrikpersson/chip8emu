#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "cpu.h"

// TODO_ Should not be here, in gfx??
uint8_t fontset[80] = { 
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

#define FONT_BASE 0
#define FONT_WIDTH 5

CPU* init() {
    CPU *cpu = calloc(sizeof(CPU), 1);
    cpu->pc = 0x200; // the first 512 bytes where reserved for the hardware
    cpu->sp = 0xea0; // stack has 96 bytes reserved from 0xea - 0xeff
    memcpy(cpu->memory, fontset, sizeof(fontset)); // memory 0-0x200 is unused. place fonts there.
    return cpu;
}

void load(CPU* cpu, uint8_t *program, size_t size) {
    memcpy(&cpu->memory[0x200], program, size);
}

uint16_t random0toff() {
    return (rand() % (255 - 0 + 1)) + 0;
}

void draw(uint8_t x, uint8_t y, uint8_t height, CPU *cpu) {
    // CLEAR VF
    cpu->registers[0xf] = 0;
    
    // printf("DRAW x = %d, y = %d, height = %d, I = 0x%x\n", x, y, height, cpu->I);
    // A sprite is 1 byte
    // A sprite is expanded so that each bit = 1 pixel
    for (int sprite_y = 0; sprite_y < height; sprite_y++) {
        uint8_t sprite = cpu->memory[cpu->I + sprite_y];
        for(int nbit = 0; nbit < 8; nbit++) { // nbit == x
            if((sprite & (0x80 >> nbit)) != 0) {
                int pixel = (x + nbit + ((y + sprite_y) * 64));
                if(cpu->display[pixel] == 1) {
                    cpu->registers[0xF] = 1;
                }
                cpu->display[pixel] ^= 1;
            }
        }
    }
    
    cpu->draw = true;
}

void exec(CPU* cpu) {
    // Opcodes are 2 seq bytes
    uint16_t lhs = (cpu->memory[cpu->pc] << 8);
    uint16_t rhs = (cpu->memory[cpu->pc + 1]);
    uint16_t opcode = lhs | rhs;
    uint8_t firstnib = (opcode & 0xf000) >> 12;
    switch (firstnib) {
        case 0x0: {
            switch (opcode & 0x00ff) {
                case 0xe0: { // CLEAR SCREEN
                    memset(cpu->display, 0, sizeof(cpu->display)); 
                    cpu->pc += 2;
                    cpu->draw = true;
                } break;
                case 0xee: { // RET
                    cpu->sp--;
                    uint16_t first_ret_addr_byte = cpu->memory[cpu->sp];
                    cpu->sp--;
                    uint16_t second_ret_addr_byte = cpu->memory[cpu->sp];
                    uint16_t ret_addr = (second_ret_addr_byte << 8) | first_ret_addr_byte;
                    cpu->pc = ret_addr;
                    // printf("RET = %x\n", ret_addr);
                    // assert(0);
                } break;
                case 0x00: cpu->pc += 2; break; // NOOP?
                default: assert(0);
            }
        } break;
        case 0x1: { // JMP #$NNN
            uint16_t address = opcode & 0x0fff;
            if (address == cpu->pc) {
                // printf("stuck in JMP loop, bug?? exiting...");
                cpu->exit = true;
            } else {
                cpu->pc = address;
            }
        } break;
        case 0x2: { // CALL #$NNN
            uint16_t address = opcode & 0x0fff;

            uint16_t ret = cpu->pc + 2; // ret to the instruction after this one

            // Store current PC (ret addr) in stack
            // Since mem is 8bit we store it in 2 separate bytes
            cpu->memory[cpu->sp] = ret >> 8; // Store 1st
            cpu->sp++;
            cpu->memory[cpu->sp] = ret & 0x00ff; // store 2nd
            cpu->sp++;

            // printf("in sp we have %x\n", cpu->memory[cpu->sp]);
            // printf("in sp-1 we have %x\n", cpu->memory[cpu->sp-1]);
            // printf("in sp-2 we have %x\n", cpu->memory[cpu->sp-2]);

            // call
            cpu->pc = address;
            // assert(0);
        } break;
        case 0x3: { // SKIPEQ Vx,#$NN
            uint8_t regid = (opcode & 0x0f00) >> 8;
            uint8_t val = opcode & 0x00ff;
            if (cpu->registers[regid] == val) {
                cpu->pc += 4;
            } else {
                cpu->pc += 2;
            }
        } break;
        case 0x4: { // skip next instruction if neq
            uint8_t regid = (opcode & 0x0f00) >> 8;
            uint8_t val = opcode & 0x00ff;
            if (cpu->registers[regid] != val) {
                cpu->pc += 2;
                // printf("SKIPPED ----> ");
                // disass(cpu->memory, cpu->pc);
                cpu->pc += 2;
            } else {
                cpu->pc += 2;
            }
        } break;
        case 0x5: {
            // skip next if eq
            uint8_t lhs = (opcode & 0x0f00) >> 8;
            uint8_t rhs = (opcode & 0x00f0) >> 4;
            if (cpu->registers[lhs] == cpu->registers[rhs]) {
                cpu->pc += 4;
            } else {
                cpu->pc += 2;
            }
        } break;
        case 0x6: { // MVI Vn,#$NN
            uint8_t regid = (opcode & 0x0f00) >> 8;
            uint8_t val = opcode & 0x00ff;
            cpu->registers[regid] = val;
            //assert(cpu->registers[2] == 8);
            cpu->pc += 2;
        } break;
        case 0x7: {
            // assert(cpu->registers[2] == 8);
            // assert(cpu->registers[0] == 0xf8);
            uint8_t regid = (opcode & 0x0f00) >> 8;
            uint8_t val = opcode & 0x00ff;
            cpu->registers[regid] += val;
            // printf("REG == 0x%x\n", cpu->registers[0]);
            // assert(cpu->registers[0] == 0);
            cpu->pc += 2;
        } break;
        case 0x8: { // 8XY{lastbit}
            uint8_t destreg = (opcode & 0x0f00) >> 8;
            uint8_t sourcereg = (opcode & 0x00f0) >> 4;
            uint8_t lsb = opcode & 0x000f; // "sub opcode"
            switch(lsb) {
                // MOV Vx, Vy
                case 0x0: cpu->registers[destreg] = cpu->registers[sourcereg]; cpu->pc += 2; break;
                // OR Vx, Vy
                case 0x1: cpu->registers[destreg] |= cpu->registers[sourcereg]; cpu->pc += 2; break;
                // AND Vx, Vy
                case 0x2: cpu->registers[destreg] &= cpu->registers[sourcereg]; cpu->pc += 2; break;
                // XOR Vx, Vy
                case 0x3: cpu->registers[destreg] ^= cpu->registers[sourcereg]; cpu->pc += 2; break;
                // ADD Vx, Vy
                case 0x4: {
                    if (cpu->registers[destreg] > (0xff - cpu->registers[sourcereg])) {
                        cpu->registers[0xf] = 1; // overflow, set carry flag
                    } else {
                        cpu->registers[0xf] = 0;
                    }
                    cpu->registers[destreg] += cpu->registers[sourcereg];
                    cpu->pc += 2;
                } break;
                // SUB Vx, Vy
                case 0x5: {
                    if (cpu->registers[destreg] < cpu->registers[sourcereg]) {
                        cpu->registers[0xf] = 0; // overflow (borrow), set to 0
                    } else {
                        cpu->registers[0xf] = 1; // no overflow == 1
                    }
                    cpu->registers[destreg] -= cpu->registers[sourcereg];
                    cpu->pc += 2;
                } break;
                // SHR Vx
                case 0x6: {
                    uint8_t val = cpu->registers[destreg];
                    cpu->registers[0xf] = val & 1;
                    cpu->registers[destreg] = val >> 1;
                    cpu->pc += 2;
                }; break;
                // SUBB Vx, Vy (backwards sub, vx = vy - vx)
                case 0x7: {
                    if (cpu->registers[sourcereg] < cpu->registers[destreg]) {
                        cpu->registers[0xf] = 0; // overflow (borrow), set to 0
                    } else {
                        cpu->registers[0xf] = 1; // no overflow == 1
                    }
                    cpu->registers[destreg] = cpu->registers[sourcereg] - cpu->registers[destreg];
                    cpu->pc += 2;
                } break;
                // SHL Vx
                case 0xe: {
                    uint8_t val = cpu->registers[destreg];
                    cpu->registers[0xf] = val >> 7;
                    cpu->registers[destreg] = val << 1;
                    cpu->pc += 2;
                }; break;
                default: assert(0);
            }
        }; break;
        case 0x9: { // SKIPNEQ vx, vy
            uint8_t lhs = (opcode & 0x0f00) >> 8;
            uint8_t rhs = (opcode & 0x00f0) >> 4;
            if (cpu->registers[lhs] != cpu->registers[rhs]) {
                cpu->pc += 4;
            } else {
                cpu->pc += 2;
            }
        }; break;
        case 0xa: { // MOV I, #$NNN
            cpu->I = opcode & 0x0fff;
            cpu->pc += 2; // every opcode is 2 bytes
        } break;
        case 0xb: {
            assert(0);
        } break;
        case 0xc: {
            uint8_t regid = (opcode & 0x0f00) >> 8;
            cpu->registers[regid] = random0toff() & (opcode & 0x00ff);
            cpu->pc += 2;
        } break;
        case 0xd: { // DRAW vXvYH, 0xdxyn
            uint8_t regX = (opcode & 0x0f00) >> 8;
            uint8_t x = cpu->registers[regX];
            uint8_t regY = (opcode & 0x00f0) >> 4;
            uint8_t y = cpu->registers[regY];
            uint8_t height = opcode & 0x000f;
            draw(x, y, height, cpu);
            // assert(0);
            cpu->pc += 2;
        } break;
        case 0xe: {
            uint8_t regid = (opcode & 0x0f00) >> 8;
            switch (opcode & 0x00ff) {
                case 0x9e: { // skip if key is pressed
                    uint8_t key = cpu->registers[regid];
                    if (cpu->keys[key]) {
                        cpu->pc += 4;
                    } else {
                        cpu->pc += 2;
                    }
                } break;
                case 0xa1: { // skip if key is not pressed
                    uint8_t key = cpu->registers[regid];
                    if (!cpu->keys[key]) {
                        cpu->pc += 4;
                    } else {
                        cpu->pc += 2;
                    }
                }
            }
        } break;
        case 0xf: {
            uint8_t regi = (opcode & 0x0f00) >> 8; // all 0xf... opcodes has a register ident in here
            switch(opcode & 0x00ff) {
                case 0x07: { // MOV vx, [delay]
                    cpu->registers[regi] = cpu->delay_timer;
                    cpu->pc += 2;
                } break;
                case 0x0a: { // block until a key is pressed, put keycode in vx
                    int8_t pressed_key = -1;
                    for (uint8_t i = 0; i < NUM_KEYS; i++) {
                        if (cpu->keys[i]) {
                            pressed_key = i;
                        }
                    }

                    if (pressed_key != -1) {
                        cpu->registers[regi] = pressed_key;
                        cpu->pc += 2;
                    }
                } break;
                case 0x15: { // MOV [delay], vx
                    cpu->delay_timer = cpu->registers[regi];
                    cpu->pc += 2;
                } break;
                case 0x18: { // MOV [sound], vx
                    cpu->sound_timer = cpu->registers[regi];
                    cpu->pc += 2;
                } break;
                case 0x1e: {
                    if (cpu->I > (0xFFF - cpu->registers[regi])) {
                        cpu->registers[0xf] = 1;
                    } else {
                        cpu->registers[0xf] = 0;
                    }
                    cpu->I += cpu->registers[regi];
                    cpu->pc += 2;
                } break;
                case 0x29: { // FONTSPRITE v[x] (0xfx29)
                    uint8_t character = cpu->registers[regi];
                    cpu->I = (FONT_BASE + character) * FONT_WIDTH;
                    cpu->pc += 2;
                } break;
                case 0x33: { // 0xfx33 (MOVBCD Vx)
                    uint8_t value = cpu->registers[regi];
                    uint8_t ones, tens, hundreds;
                    ones = value % 10;    
                    value = value / 10;    
                    tens = value % 10;    
                    hundreds = value / 10;
                    cpu->memory[cpu->I] = hundreds;
                    cpu->memory[cpu->I + 1] = tens;
                    cpu->memory[cpu->I + 2] = ones;
                    cpu->pc += 2;
                } break;
                case 0x55: { // MOVM (I), V0-VX (move mem from v0-vx (inclusive) into mem[I])
                    memcpy(&cpu->memory[cpu->I], cpu->registers, regi + 1); // + 1 because op is inclusive
                    cpu->pc += 2;
                } break;
                case 0x65: { // MOVM V0-VX, (I) (move mem from mem[I] into v0-vx (inclusive))
                    memcpy(cpu->registers, &cpu->memory[cpu->I], regi + 1); // +1 because op is inclusive
                    cpu->pc += 2;
                }; break;
                default: assert(0);
            }
        } break;
        default: assert(0);
    }
}
