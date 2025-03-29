#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "shell.h"
#include "decode.h"
#include "handlers.h"

void handle_hlt(uint32_t instr) {
    RUN_BIT = 0;
}

void handle_adds_imm(uint32_t instr) {
    uint32_t imm12, shift, d, n;
    decode_i_group(instr, &imm12, &shift, &d, &n);
    uint64_t res = calculate_mathOps(n, 0, shift, imm12, 0, 1);
    NEXT_STATE.REGS[d] = res;
    update_flags(res);
}

void handle_adds_reg(uint32_t instr) {
    uint32_t opt, imm3, d, n, m;
    decode_r_group(instr, &opt, &imm3, &d, &n, &m);
    uint64_t res = calculate_mathOps(n, m, opt, imm3, 0, 0);
    NEXT_STATE.REGS[d] = res;
    update_flags(res);
}

void handle_subs_imm(uint32_t instr) {
    uint32_t imm12, shift, d, n;
    decode_i_group(instr, &imm12, &shift, &d, &n);
    uint64_t res = calculate_mathOps(n, 0, shift, imm12, 1, 1);
    update_flags(res);
    if (d != 31) {
        NEXT_STATE.REGS[d] = res;
    }
}

void handle_subs_reg(uint32_t instr) {
    uint32_t opt, imm3, d, n, m;
    decode_r_group(instr, &opt, &imm3, &d, &n, &m);
    uint64_t res = calculate_mathOps(n, m, opt, imm3, 1, 0);
    update_flags(res);
    if (d != 31) {
        NEXT_STATE.REGS[d] = res;
    }
}

void handle_ands(uint32_t instr) {
    uint32_t shift, imm6, d, n, m;
    decode_shifted_register(instr, &shift, &imm6, &d, &n, &m);
    uint64_t op1 = CURRENT_STATE.REGS[n];
    uint64_t op2 = CURRENT_STATE.REGS[m];
    switch (shift) {
        case 0: op2 <<= imm6; break;
        case 1: op2 >>= imm6; break;
        case 2: op2 = ((int64_t)op2) >> imm6; break;
        case 3: op2 = (op2 >> imm6) | (op2 << (64 - imm6)); break;
    }
    uint64_t res = op1 & op2;
    NEXT_STATE.REGS[d] = res;
    update_flags(res);
}

void handle_eor(uint32_t instr) {
    uint32_t shift, imm6, d, n, m;
    decode_shifted_register(instr, &shift, &imm6, &d, &n, &m);
    uint64_t op1 = CURRENT_STATE.REGS[n];
    uint64_t op2 = CURRENT_STATE.REGS[m];
    switch (shift) {
        case 0: op2 = (imm6 == 0) ? op2 : (op2 << imm6); break;
        case 1: op2 = (imm6 == 0) ? op2 : (op2 >> imm6); break;
        case 2: op2 = (imm6 == 0) ? op2 : ((int64_t)op2 >> imm6); break;
        case 3: op2 = (imm6 == 0) ? op2 : ((op2 >> imm6) | (op2 << (64 - imm6))); break;
    }
    uint64_t res = op1 ^ op2;
    NEXT_STATE.REGS[d] = res;
}

void handle_orr(uint32_t instr) {
    uint32_t opt, imm3, d, n, m;
    decode_r_group(instr, &opt, &imm3, &d, &n, &m);
    NEXT_STATE.REGS[d] = CURRENT_STATE.REGS[n] | CURRENT_STATE.REGS[m];
}

void handle_b(uint32_t instr) {
    int32_t imm26 = instr & 0x3FFFFFF;
    int64_t offset = ((int64_t)(imm26 << 6)) >> 4;
    NEXT_STATE.PC += offset - 4;
}

void handle_br(uint32_t instr) {
    uint32_t n = (instr >> 5) & 0x1F;
    NEXT_STATE.PC = CURRENT_STATE.REGS[n] - 4;
}

void handle_stur(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);
    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    mem_write_64(addr, CURRENT_STATE.REGS[t]);
}

void handle_sturb(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);
    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    mem_write_8(addr, (uint8_t)CURRENT_STATE.REGS[t]);
}

void handle_sturh(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);
    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    mem_write_16(addr, (uint16_t)CURRENT_STATE.REGS[t]);
}

void handle_ldur(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);
    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    NEXT_STATE.REGS[t] = mem_read_64(addr);
}

void handle_ldurb(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);
    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    NEXT_STATE.REGS[t] = mem_read_8(addr);
}

void handle_ldurh(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);
    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    NEXT_STATE.REGS[t] = mem_read_16(addr);
}

void handle_b_cond(uint32_t instr) {
    int32_t imm19 = (instr >> 5) & 0x7FFFF;
    imm19 = sign_extend(imm19, 19);
    int64_t offset = (int64_t)imm19 << 2;
    uint32_t cond = instr & 0xF;
    int take_branch = 0;
    switch (cond) {
        case 0x0: if (CURRENT_STATE.FLAG_Z == 1) take_branch = 1; break;
        case 0x1: if (CURRENT_STATE.FLAG_Z == 0) take_branch = 1; break;
        case 0xA: if (CURRENT_STATE.FLAG_N == 0) take_branch = 1; break;
        case 0xB: if (CURRENT_STATE.FLAG_N != 0) take_branch = 1; break;
        case 0xC: if ((CURRENT_STATE.FLAG_Z == 0) && (CURRENT_STATE.FLAG_N == 0)) take_branch = 1; break;
        case 0xD: if ((CURRENT_STATE.FLAG_Z == 1) || (CURRENT_STATE.FLAG_N != 0)) take_branch = 1; break;
    }
    if (take_branch)
        NEXT_STATE.PC = CURRENT_STATE.PC + offset - 4;
}

void handle_movz(uint32_t instr) {
    uint32_t d = instr & 0x1F;
    uint32_t hw = (instr >> 21) & 0x3;
    uint32_t imm16 = (instr >> 5) & 0xFFFF;
    if (hw != 0)
        printf("MOVZ: solo se implementa el caso hw == 0.\n");
    NEXT_STATE.REGS[d] = imm16;
}

void handle_add_imm(uint32_t instr) {
    uint32_t imm12, shift, d, n;
    decode_i_group(instr, &imm12, &shift, &d, &n);
    uint64_t imm = (shift == 1) ? (imm12 << 12) : imm12;
    NEXT_STATE.REGS[d] = CURRENT_STATE.REGS[n] + imm; 
}

void handle_add_reg(uint32_t instr) {
    uint32_t opt, imm3, d, n, m;
    decode_r_group(instr, &opt, &imm3, &d, &n, &m);
    uint64_t res = CURRENT_STATE.REGS[n] + extend_register(CURRENT_STATE.REGS[m], opt, imm3);
    NEXT_STATE.REGS[d] = res;
}

void handle_mul(uint32_t instr) {
    uint32_t opt, imm3, d, n, m;
    decode_r_group(instr, &opt, &imm3, &d, &n, &m);
    NEXT_STATE.REGS[d] = CURRENT_STATE.REGS[n] * CURRENT_STATE.REGS[m];
}

void handle_cbz(uint32_t instr) {
    uint32_t t, offset;
    decode_conditional_branch(instr, &t, &offset);
    if (CURRENT_STATE.REGS[t] == 0)
        NEXT_STATE.PC = CURRENT_STATE.PC + offset - 4;
}

void handle_cbnz(uint32_t instr) {
    uint32_t t, offset;
    decode_conditional_branch(instr, &t, &offset);
    if (CURRENT_STATE.REGS[t] != 0)
        NEXT_STATE.PC = CURRENT_STATE.PC + offset - 4;
}

void handle_shift(uint32_t instr) {
    bool is_lsr;
    uint8_t shift_amt, rd, rn;
    decode_lsl_lsr(instr, &is_lsr, &shift_amt, &rd, &rn);

    if (is_lsr) {
        NEXT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rn] >> shift_amt);
    } else {
        NEXT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rn] << shift_amt);
    }
}

