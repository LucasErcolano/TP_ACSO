#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include "decode.h"
#include "shell.h"

void decode_i_group(uint32_t instr, uint32_t *imm12, uint32_t *shift, uint32_t *d, uint32_t *n) {
    *imm12 = (instr >> 10) & 0xFFF;
    *shift = (instr >> 22) & 0x3;
    *d = instr & 0x1F;
    *n = (instr >> 5) & 0x1F;
}

void decode_r_group(uint32_t instr, uint32_t *opt, uint32_t *imm3, uint32_t *d, uint32_t *n, uint32_t *m) {
    *opt = (instr >> 13) & 0x7;
    *imm3 = (instr >> 10) & 0x7;
    *d = instr & 0x1F;
    *n = (instr >> 5) & 0x1F;
    *m = (instr >> 16) & 0x1F;
}

void decode_shifted_register(uint32_t instr, uint32_t *shift, uint32_t *imm6, uint32_t *d, uint32_t *n, uint32_t *m) {
    *shift = (instr >> 22) & 0x3;
    *imm6 = (instr >> 10) & 0x3F;
    *d = instr & 0x1F;
    *n = (instr >> 5) & 0x1F;
    *m = (instr >> 16) & 0x1F;
}

void decode_mem_access(uint32_t instr, int32_t *imm9, int32_t *n, int32_t *t) {
    *imm9 = (instr >> 12) & 0x1FF;
    *n = (instr >> 5) & 0x1F;
    *t = instr & 0x1F;
}

void decode_conditional_branch(uint32_t instr, uint32_t *t, uint32_t *offset) {
    *t = instr & 0x1F;
    int32_t imm = (instr >> 5) & 0x7FFFF;
    imm = sign_extend(imm, 19);
    *offset = (int64_t)imm << 2;
}

void decode_lsl_lsr(uint32_t instr, bool *is_lsr, uint8_t *shift, uint8_t *rd, uint8_t *rn) {
    uint8_t immr = (instr >> 16) & 0x3F; 
    uint8_t imms = (instr >> 10) & 0x3F; 
    *rn = (instr >> 5) & 0x1F;           
    *rd = instr & 0x1F;                  

    if (imms == 63) {
        *is_lsr = true;
        *shift = immr;     
    } else {
        *is_lsr = false;
        *shift = 64 - immr; 
    }
}

int64_t sign_extend(int64_t value, int bits) {
    int64_t mask = (int64_t)1 << (bits - 1);
    return (value ^ mask) - mask;
}

uint64_t extend_register(uint64_t value, int option, int imm3) {
    uint64_t res;
    switch (option) {
        case 0: res = value & 0xFF; break;
        case 1: res = value & 0xFFFF; break;
        case 2: res = value & 0xFFFFFFFF; break;
        case 3: res = value; break;
        case 4: res = (uint64_t)(int8_t)(value & 0xFF); break;
        case 5: res = (uint64_t)(int16_t)(value & 0xFFFF); break;
        case 6: res = (uint64_t)(int32_t)(value & 0xFFFFFFFF); break;
        case 7: res = value; break;
        default: res = value; break;
    }
    return res << imm3;
}

int64_t calculate_mathOps(uint32_t n, uint32_t m, uint32_t opt, uint32_t imm3, int isSubtraction, int isImm) {
    uint64_t op1 = (n == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[n];
    uint64_t op2;
    if (isImm) {
        op2 = (opt == 1) ? (imm3 << 12) : imm3;
    } else {
        op2 = extend_register(CURRENT_STATE.REGS[m], opt, imm3);
    }
    return isSubtraction ? op1 - op2 : op1 + op2;
}

void update_flags(int64_t result) {
    NEXT_STATE.FLAG_Z = (result == 0);
    NEXT_STATE.FLAG_N = (result < 0);
}

uint8_t mem_read_8(uint64_t addr) {
    uint32_t word = mem_read_32(addr & ~0x3);
    return (word >> ((addr & 0x3) * 8)) & 0xFF;
}

uint16_t mem_read_16(uint64_t addr) {
    if (addr & 0x1)
        printf("Warning: Unaligned halfword read at 0x%" PRIx64 "\n", addr);
    uint32_t word = mem_read_32(addr & ~0x3);
    return (word >> (((addr & 0x3) / 2) * 16)) & 0xFFFF;
}

uint64_t mem_read_64(uint64_t addr) {
    uint64_t aligned_addr = addr & ~0x7;
    uint32_t offset = addr & 0x7;
    if (offset) {
        uint64_t low_part = mem_read_32(aligned_addr);
        uint64_t high_part = mem_read_32(aligned_addr + 4);
        if (offset <= 4) {
            return (high_part << (8 * (4 - offset))) | (low_part >> (8 * offset));
        } else {
            return (high_part >> (8 * (offset - 4))) | (low_part << (8 * (8 - offset)));
        }
    } else {
        uint64_t low_part = mem_read_32(addr);
        uint64_t high_part = mem_read_32(addr + 4);
        return low_part | (high_part << 32);
    }
}

void mem_write_8(uint64_t addr, uint8_t value) {
    uint64_t a = addr & ~0x3;
    uint32_t word = mem_read_32(a);
    int offset = addr & 0x3;
    uint32_t mask = 0xFF << (offset * 8);
    word = (word & ~mask) | ((value & 0xFF) << (offset * 8));
    mem_write_32(a, word);
}

void mem_write_16(uint64_t addr, uint16_t value) {
    if (addr & 0x1)
        printf("Warning: Unaligned halfword write at 0x%" PRIx64 "\n", addr);
    uint64_t a = addr & ~0x3;
    uint32_t word = mem_read_32(a);
    int offset = (addr & 0x3) / 2;
    uint32_t mask = 0xFFFF << (offset * 16);
    word = (word & ~mask) | ((value & 0xFFFF) << (offset * 16));
    mem_write_32(a, word);
}

void mem_write_64(uint64_t addr, uint64_t value) {
    uint64_t aligned_addr = addr & ~0x7;
    uint32_t offset = addr & 0x7;
    if (offset) {
        if (offset <= 4) {
            mem_write_32(aligned_addr, value & 0xFFFFFFFF);
            mem_write_32(aligned_addr + 4, (value >> (8 * (4 - offset))) | (mem_read_32(aligned_addr + 4) & ((1U << (8 * offset)) - 1)));
        } else {
            mem_write_32(aligned_addr, (mem_read_32(aligned_addr) & ((1U << (8 * (8 - offset))) - 1)) | ((value << (8 * (8 - offset))) & ~((1U << (8 * (8 - offset))) - 1)));
            mem_write_32(aligned_addr + 4, value >> (8 * (8 - offset)));
        }
    } else {
        mem_write_32(addr, value & 0xFFFFFFFF);
        mem_write_32(addr + 4, (value >> 32) & 0xFFFFFFFF);
    }
}
