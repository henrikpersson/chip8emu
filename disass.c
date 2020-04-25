#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "disass.h"

// $ = HEX
// #$ = HEX VALUE
// JMP #$123 = jump to addr $123
// JMP $123 = jump to the address which is stored in addr $123
// (V0) = the value stored in register(var?) 0

void disass(char *output, uint8_t *mem, int pc) {
    uint8_t *opcode = &mem[pc];

    char raw_header[DISASS_OP_SIZE];
    sprintf(raw_header, "%04x %02x %02x\t", pc, opcode[0], opcode[1]);

    uint8_t firstnib = (opcode[0] >> 4);
    char mnemonic[DISASS_OP_SIZE];
    switch (firstnib) {
        case 0x0:
            switch (opcode[1]) {
                case 0xe0: sprintf(mnemonic, "CLEAR SCREEN"); break;
                case 0xee: sprintf(mnemonic, "RET"); break;
                case 0x00: sprintf(mnemonic, "NOOP"); break;
                default: sprintf(mnemonic, "CALL RCA 1802 <-- PROB NOT RIGHT");
            }
            break;
        case 0x1: {
            uint8_t firstaddrnib = (opcode[0] & 0x0f); // clear first nibble (1NNN)
            //                              N          NN
            sprintf(mnemonic, "JMP #$%01x%02x", firstaddrnib, opcode[1]);
        } break;
        case 0x2: {
            uint8_t firstaddrnib = (opcode[0] & 0x0f);
            sprintf(mnemonic, "CALL    #$%01x%02x", firstaddrnib, opcode[1]);
        } break;
        case 0x3: {
            uint8_t regident = (opcode[0] & 0x0f);
            // skip next if eq
            sprintf(mnemonic, "SKIPEQ V%01x,#$%02x", regident, opcode[1]);
        } break;
        case 0x4: {
            uint8_t regident = (opcode[0] & 0x0f);
            // skip next if neq
            sprintf(mnemonic, "SKIPNEQ    V%01x,#$%02x", regident, opcode[1]);
        } break;
        case 0x5: {
            // skip next if eq
            uint8_t regidentleft = (opcode[0] & 0x0f);
            uint8_t regidentright = (opcode[1] >> 4);
            sprintf(mnemonic, "SKIPEQRR    V%01x,V%01x", regidentleft, regidentright);
        } break;
        case 0x6: {
            uint8_t regident = (opcode[0] & 0x0f);
            sprintf(mnemonic, "MVI V%01x,#$%02x", regident, opcode[1]);
        } break;
        case 0x7: {
            uint8_t regident = (opcode[0] & 0x0f);
            sprintf(mnemonic, "ADD V%01x,#$%02x", regident, opcode[1]);
        } break;
        case 0x8: { // register op, 8XY0
            uint8_t destreg = opcode[0] & 0x0f;
            uint8_t sourcereg = (opcode[1] & 0xf0) >> 4;
            uint8_t lastnib = opcode[1] & 0x0f;
            switch(lastnib) {
                case 0x0: sprintf(mnemonic, "MOV V%x, V%x", destreg, sourcereg); break;
                case 0x1: sprintf(mnemonic, "OR V%x, V%x", destreg, sourcereg); break;
                case 0x2: sprintf(mnemonic, "AND V%x, V%x", destreg, sourcereg); break;
                case 0x3: sprintf(mnemonic, "XOR V%x, V%x", destreg, sourcereg); break;
                case 0x4: sprintf(mnemonic, "ADD V%x, V%x", destreg, sourcereg); break;
                case 0x5: sprintf(mnemonic, "SUB V%x, V%x", destreg, sourcereg); break;
                case 0x6: sprintf(mnemonic, "SHR V%x", destreg); break;
                case 0x7: sprintf(mnemonic, "SUBB V%x, V%x", destreg, sourcereg); break; // backward substract
                case 0xe: sprintf(mnemonic, "SHL V%x", destreg); break;
                default: sprintf(mnemonic, "??? UNKOWN 0x8 register op");
            }
        } break;
        case 0x9: { // SKIPNEQ Vx, Vy
            uint8_t regx = opcode[0] & 0x0f;
            uint8_t regy = (opcode[1] & 0xf0) >> 4;
            sprintf(mnemonic, "SKIPNEQ V%x, V%x", regx, regy);
        } break;
        case 0xa: {
            uint8_t firstaddrnib = (opcode[0] & 0x0f);
            sprintf(mnemonic, "MVI I,#$%01x%02x", firstaddrnib, opcode[1]);
        } break;
        case 0xb: {
            uint8_t firstvaluenib = (opcode[0] & 0x0f);
            sprintf(mnemonic, "JMP #$%01x%02x+(V0)", firstvaluenib, opcode[1]);
        } break;
        case 0xc: { // 0xCXNN
            uint8_t regident = (opcode[0] & 0x0f);
            sprintf(mnemonic, "MOV V%x, rand()&%x", regident, opcode[1]);
        } break;
        case 0xd: {
            sprintf(mnemonic, "DRAW");
        } break;
        case 0xe: {
            uint8_t reg = opcode[0] & 0x0f;
            switch (opcode[1]) {
                case 0x9e: sprintf(mnemonic, "SKIPKEQ V%x", reg); break; // skip if key is pressed
                case 0xa1: sprintf(mnemonic, "SKIPKNEQ V%x", reg); break; // skip if key is not pressed
            }
        } break;
        case 0xf: {
            uint8_t regident = (opcode[0] & 0x0f); // all 0xFxxx gets a register identifier here
            switch(opcode[1]) {
                case 0x07: { // set vx to delay timer 
                    sprintf(mnemonic, "MOV V%x, [delay]", regident);
                } break;
                case 0x0a: { // block until a key is pressed, put keycode in vx
                    sprintf(mnemonic, "KEY V%x", regident);
                } break;
                case 0x15: { // set delay timer to vx
                    sprintf(mnemonic, "MOV [delay], V%x", regident);
                } break;
                case 0x18: { // set sound timer to vx
                    sprintf(mnemonic, "MOV [sound], V%x", regident);
                } break;
                case 0x1e: {
                    sprintf(mnemonic, "ADD I,V%01x", regident);
                } break;
                case 0x29: {
                    sprintf(mnemonic, "FONTSPRITE V%x", regident);
                }; break;
                case 0x33: {
                    sprintf(mnemonic, "MOVBCD V%x", regident);
                } break;
                case 0x55: { // 0xFX55
                    sprintf(mnemonic, "MOVMEM (I), V0-V%x", regident);
                } break;
                case 0x65: { 
                    sprintf(mnemonic, "MOVMEM V0-V%x, (I)", regident);
                } break;
                default: sprintf(mnemonic, "????");
            }
        } break;
        default: sprintf(mnemonic, "??? (nib = 0x%01x)", firstnib);
    }

    strcat(raw_header, mnemonic);
    memcpy(output, raw_header, DISASS_OP_SIZE);
}

/*
0000000 00 e0 a2 20 62 08 60 f8 70 08 61 10 40 20 12 0e
0000010 d1 08 f2 1e 71 08 41 30 12 08 12 10 00 00 00 00
*/
