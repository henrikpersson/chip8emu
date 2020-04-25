#include <criterion/criterion.h>
#include <criterion/assert.h>
#include <stdio.h>
#include <stdint.h>

#include "cpu.h"

#define PC_BASE 0x200

CPU* cpu = NULL;

void setup() {
    cpu = init();
}

void teardown() {
    free(cpu);
    cpu = NULL;
}

void load_program(uint8_t *program, size_t len) {
    load(cpu, program, len);
}

void reset() {
    cpu->pc = PC_BASE;
}

TestSuite(cpu, .init = setup, .fini = teardown);

Test(cpu, init) {
    cr_assert_not_null(cpu);
    cr_assert_eq(sizeof(cpu->memory), 0x1000);
    cr_assert_eq(cpu->pc, PC_BASE);
    cr_assert_eq(cpu->sp, 0xea0);
}

Test(cpu, jmp_nnn) { // case 0x1: { // JMP #$NNN
    load_program((uint8_t[]) { 0x11, 0x66 }, 2);
    exec(cpu);
    cr_assert_eq(cpu->pc, 0x166);
}

Test(cpu, movm_i_v) { // MOVM (I), V0-VX (move mem from v0-vx (inclusive) into mem[I])
   load_program((uint8_t[]) { 0xf3, 0x55 }, 2);
   cpu->registers[0] = 0xde;
   cpu->registers[1] = 0xad;
   cpu->registers[2] = 0xbe;
   cpu->registers[3] = 0xef;
   cpu->registers[4] = 0xff; // not included!

   exec(cpu);

   cr_assert_eq(cpu->memory[cpu->I],        0xde);
   cr_assert_eq(cpu->memory[cpu->I + 1],    0xad);
   cr_assert_eq(cpu->memory[cpu->I + 2],    0xbe);
   cr_assert_eq(cpu->memory[cpu->I + 3],    0xef);
   cr_assert_neq(cpu->memory[cpu->I + 4],   0xff);
}

Test(cpu, movm_v_i) { // MOVM V0-VX, (I) (move mem from mem[I] into v0-vx (inclusive))
load_program((uint8_t[]) { 0xf3, 0x65 }, 2);
   cpu->memory[cpu->I] = 0xde;
   cpu->memory[cpu->I + 1] = 0xad;
   cpu->memory[cpu->I + 2] = 0xbe;
   cpu->memory[cpu->I + 3] = 0xef;
   cpu->memory[cpu->I + 4] = 0xff; // not included! 

   exec(cpu);

   cr_assert_eq(cpu->registers[0], 0xde);
   cr_assert_eq(cpu->registers[1], 0xad);
   cr_assert_eq(cpu->registers[2], 0xbe);
   cr_assert_eq(cpu->registers[3], 0xef);
   cr_assert_neq(cpu->registers[4], 0xff);
}

Test(cpu, movbcd) { // 0xfx33 (MOVBCD Vx)
   load_program((uint8_t[]) { 0xf0, 0x33 }, 2);
   cpu->registers[0] = 0xea; // dec = 234
   exec(cpu);

   cr_assert_eq(cpu->memory[cpu->I],        2);
   cr_assert_eq(cpu->memory[cpu->I + 1],    3);
   cr_assert_eq(cpu->memory[cpu->I + 2],    4);
}

Test(cpu, reg_add) { // 0x8xy4, reg add, as supposed to immediate add
    load_program((uint8_t[]) { 0x80, 0x14 }, 2);
    cpu->registers[0] = 0xea;
    cpu->registers[1] = 0x05;
    cpu->registers[0xf] = 1; // set carry flag to test that non-overflow with set it to 0
    exec(cpu);

    cr_assert_eq(cpu->registers[0], 0xea + 0x05);
    cr_assert_eq(cpu->registers[0xf], 0); // carry flag unchanged

    cpu->registers[0] = 0x45;
    cpu->registers[1] = 0xba;
    reset();
    exec(cpu);

    cr_assert_eq(cpu->registers[0], 0x45 + 0xba); // 0xff
    cr_assert_eq(cpu->registers[0xf], 0); // carry flag unchanged

    cpu->registers[0] = 0x46;
    cpu->registers[1] = 0xba;
    reset();
    exec(cpu);

    cr_assert_eq(cpu->registers[0], 0x0); // overflow
    cr_assert_eq(cpu->registers[0xf], 1); // carry flag changed
}

Test(cpu, reg_sub) { // 0x8xy5, reg sub, as supposed to immediate add
    load_program((uint8_t[]) { 0x80, 0x15 }, 2);
    cpu->registers[0] = 0xea;
    cpu->registers[1] = 0x05;
    cpu->registers[0xf] = 0; // set carry flag to test that non-overflow with set it to 1
    exec(cpu);

    cr_assert_eq(cpu->registers[0], 0xea - 0x05);
    cr_assert_eq(cpu->registers[0xf], 1); // carry should be 1 on no-borrow

    cpu->registers[0] = 0x45;
    cpu->registers[1] = 0x45;
    reset();
    exec(cpu);

    cr_assert_eq(cpu->registers[0], 0x0);
    cr_assert_eq(cpu->registers[0xf], 1); // carry should be 1 on no-borrow

    cpu->registers[0] = 0x46;
    cpu->registers[1] = 0xba;
    reset();
    exec(cpu);
    
    // overflow
    cr_assert_eq(cpu->registers[0xf], 0); // carry flag changed, 0 == overflow
}

Test(cpu, reg_subb) { // 0x8xy7, reg subb, backwards substraction
    load_program((uint8_t[]) { 0x80, 0x17 }, 2);
    cpu->registers[0] = 0x10;
    cpu->registers[1] = 0xba;
    cpu->registers[0xf] = 0; // set carry flag to test that non-overflow with set it to 1
    exec(cpu);

    cr_assert_eq(cpu->registers[0], 0xba - 0x10);
    cr_assert_eq(cpu->registers[0xf], 1); // carry should be 1 on no-borrow

    cpu->registers[0] = 0x45;
    cpu->registers[1] = 0x45;
    reset();
    exec(cpu);

    cr_assert_eq(cpu->registers[0], 0x0);
    cr_assert_eq(cpu->registers[0xf], 1); // carry should be 1 on no-borrow

    cpu->registers[0] = 0xf0;
    cpu->registers[1] = 0xba;
    reset();
    exec(cpu);
    
    // overflow
    cr_assert_eq(cpu->registers[0xf], 0); // carry flag changed, 0 == overflow
}

Test(cpu, skipeq) { // 0x3XNN SKIPEQ Vx,#$NN
    load_program((uint8_t[]) { 0x36, 0xea }, 2);

    uint16_t saved_pc = cpu->pc;
    exec(cpu);
    cr_assert_eq(cpu->pc, saved_pc + 2);

    reset();
    saved_pc = cpu->pc;
    cpu->registers[6] = 0xea;
    exec(cpu);

    cr_assert_eq(cpu->pc, saved_pc + 4);
}

Test(cpu, mvi) { // 0x6xnn, MOVI vx, #$NN
    load_program((uint8_t[]) { 0x6a, 0xea }, 2);

    exec(cpu);

    cr_assert_eq(cpu->registers[0xa], 0xea);
}

Test(cpu, mov) { // 0x8xy0, MOV vx, vy
    load_program((uint8_t[]) { 0x8a, 0xb0 }, 2);
    cpu->registers[0xb] = 0xea;

    exec(cpu);

    cr_assert_eq(cpu->registers[0xa], 0xea);
}

Test(cpu, mvii) { // 0xannn, MOV I, $#NNN
    load_program((uint8_t[]) { 0xa3, 0x76 }, 2);
    
    exec(cpu);

    cr_assert_eq(cpu->I, 0x376);
}

Test(cpu, addi) { // 0xfx1e, ADD I, vx
    load_program((uint8_t[]) { 0xfa, 0x1e }, 2);
    cpu->I = 0x00a;
    cpu->registers[0xa] = 0x00b;
    cpu->registers[0xf] = 1; // set carry flag to test that non-overflow with set it to 0

    exec(cpu);

    cr_assert_eq(cpu->I, 0x00a + 0x00b);
    cr_assert_eq(cpu->registers[0xf], 0);

    // Test overflow
    reset();
    cpu->I = 0xff0;
    cpu->registers[0xa] = 0x10;

    exec(cpu);

    cr_assert_eq(cpu->registers[0xf], 1); // overlfow
}

Test(cpu, or) { //0x8xy1 OR vx, xy
    load_program((uint8_t[]) { 0x8a, 0xb1 }, 2);
    cpu->registers[0xa] = 0xea;
    cpu->registers[0xb] = 0x45;

    exec(cpu);

    cr_assert_eq(cpu->registers[0xa], 0xea | 0x45);
}

Test(cpu, shl) { //0x8xye SHL vx
    load_program((uint8_t[]) { 0x8a, 0xbe }, 2);
    cpu->registers[0xa] = 0xea;

    exec(cpu);

    cr_assert_eq(cpu->registers[0xf], 0xea >> 7);
    cr_assert_eq(cpu->registers[0xa], (uint8_t) (0xea << 1));
}

Test(cpu, shr) { //0x8xye SHR vx
    load_program((uint8_t[]) { 0x8a, 0xb6 }, 2);
    cpu->registers[0xa] = 0xea;

    exec(cpu);

    cr_assert_eq(cpu->registers[0xf], 0xea & 1);
    cr_assert_eq(cpu->registers[0xa], (uint8_t) (0xea >> 1));
}