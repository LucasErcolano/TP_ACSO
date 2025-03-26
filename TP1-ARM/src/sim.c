#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "shell.h"
#include "hashmap.h"

typedef void (*InstructionHandler)(uint32_t);

typedef struct {
    uint32_t pattern;
    int length;
    InstructionHandler handler;
} InstructionEntry;

HashMap *opcode_map = NULL;
int branch_taken = 0;

void handle_hlt(uint32_t);
void handle_adds_imm(uint32_t);
void handle_adds_reg(uint32_t);
void handle_subs_imm(uint32_t);
void handle_subs_reg(uint32_t);
void handle_ands(uint32_t);
void handle_eor(uint32_t);
void handle_orr(uint32_t);
void handle_b(uint32_t);
void handle_br(uint32_t);
void handle_b_cond(uint32_t);
void handle_cbz(uint32_t);
void handle_cbnz(uint32_t);
void handle_ldur(uint32_t);
void handle_stur(uint32_t);
void handle_movz(uint32_t);
void handle_mul(uint32_t);
void handle_shifts(uint32_t);
void handle_sturb(uint32_t);
void handle_sturh(uint32_t);
void handle_ldurb(uint32_t);
void handle_ldurh(uint32_t);
void handle_add_reg(uint32_t);
void handle_add_imm(uint32_t);

// Retorna la máscara correspondiente al tamaño de opcode
static inline uint32_t mask_from_length(int length) {
    switch (length) {
        case 6:  return 0xFE000000;
        case 8:  return 0xFF000000;
        case 9:  return 0xFF800000;
        case 11: return 0xFFE00000;
        case 22: return 0xFFFFFC00;
        default:
            printf("Unsupported opcode length: %d\n", length);
            exit(1);
    }
}

void init_opcode_map() {
    if (opcode_map) return;
    opcode_map = hashmap_create();
    InstructionEntry entries[] = {
        {0xD4500000, 8, handle_hlt},
        {0x54000000, 8, handle_b_cond},
        {0x14000000, 6, handle_b},
        {0xD61F0000, 22, handle_br},
        {0xB1000000, 8, handle_adds_imm},
        {0xAB000000, 8, handle_adds_reg},
        {0xF1000000, 8, handle_subs_imm},
        {0xEB000000, 8, handle_subs_reg},
        {0xEA000000, 8, handle_ands},
        {0xCA000000, 8, handle_eor},
        {0xAA000000, 8, handle_orr},
        {0xD2800000, 9, handle_movz},
        {0xD3400000, 9, handle_shifts},
        {0xD3800000, 9, handle_shifts},
        {0xF8000000, 11, handle_stur},
        {0xF8400000, 11, handle_ldur},
        {0xB4000000, 8, handle_cbz},
        {0xB5000000, 8, handle_cbnz},
        {0x9B000000, 11, handle_mul},
        {0x38000000, 11, handle_sturb},
        {0x78000000, 11, handle_sturh},
        {0x38400000, 11, handle_ldurb},
        {0x78400000, 11, handle_ldurh},
        {0x8B000000, 11, handle_add_reg},
        {0x91000000, 8, handle_add_imm}
    };
    int n = sizeof(entries) / sizeof(entries[0]);
    for (int i = 0; i < n; i++) {
        uint32_t mask = mask_from_length(entries[i].length);
        hashmap_put(opcode_map, mask, entries[i].pattern & mask, entries[i].handler);
    }
}

InstructionHandler decode_instruction(uint32_t instruction) {
    if (!opcode_map) init_opcode_map();
    for (int len = 32; len >= 6; len--) {
        uint32_t mask = (0xFFFFFFFF >> (32 - len)) << (32 - len);
        uint32_t opcode = instruction & mask;
        InstructionHandler handler = hashmap_get(opcode_map, mask, opcode);
        if (handler) return handler;
    }
    return NULL;
}

void process_instruction() {
    uint32_t instr = mem_read_32(CURRENT_STATE.PC);
    InstructionHandler handler = decode_instruction(instr);
    if (handler) {
        branch_taken = 0;
        handler(instr);
    } else {
        printf("Unsupported instruction: 0x%08X\n", instr);
        exit(1);
    }
}

static inline void update_flags(int64_t result) {
    NEXT_STATE.FLAG_Z = (result == 0);
    NEXT_STATE.FLAG_N = (result < 0);
}

static inline uint64_t extend_register(uint64_t value, int option, int imm3) {
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

int64_t sign_extend(int64_t value, int bits) {
    int64_t mask = (int64_t)1 << (bits - 1);
    return (value ^ mask) - mask;
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
    if (addr & 0x7)
        printf("Warning: Unaligned 64-bit read at 0x%" PRIx64 "\n", addr);
    uint32_t low = mem_read_32(addr);
    uint32_t high = mem_read_32(addr + 4);
    return ((uint64_t)high << 32) | low;
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
    if (addr & 0x7)
        printf("Warning: Unaligned 64-bit write at 0x%" PRIx64 "\n", addr);
    mem_write_32(addr, value & 0xFFFFFFFF);
    mem_write_32(addr + 4, (value >> 32) & 0xFFFFFFFF);
}

void handle_hlt(uint32_t instr) {
    RUN_BIT = 0;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_adds_imm(uint32_t instr) {
    uint32_t imm12 = (instr >> 10) & 0xFFF;
    uint32_t shift = (instr >> 22) & 0x3;
    uint32_t d = instr & 0x1F;
    uint32_t n = (instr >> 5) & 0x1F;
    uint32_t imm = (shift == 1) ? (imm12 << 12) : imm12;
    uint32_t res = CURRENT_STATE.REGS[n] + imm;
    NEXT_STATE.REGS[d] = res;
    update_flags(res);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_adds_reg(uint32_t instr) {
    uint32_t d = instr & 0x1F;
    uint32_t n = (instr >> 5) & 0x1F;
    uint32_t imm3 = (instr >> 10) & 0x7;
    uint32_t opt = (instr >> 13) & 0x7;
    uint32_t m = (instr >> 16) & 0x1F;
    uint32_t op1 = (n == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[n];
    uint64_t op2 = extend_register(CURRENT_STATE.REGS[m], opt, imm3);
    uint64_t res = op1 + op2;
    NEXT_STATE.REGS[d] = res;
    update_flags(res);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_subs_imm(uint32_t instr) {
    uint64_t imm12 = (instr >> 10) & 0xFFF;
    uint32_t shift = (instr >> 22) & 0x3;
    uint32_t Rd = instr & 0x1F;
    uint32_t Rn = (instr >> 5) & 0x1F;
    uint64_t op1 = (Rn == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[Rn];
    uint64_t op2 = (shift == 1) ? (imm12 << 12) : imm12;
    uint64_t res = op1 - op2;
    update_flags(res);
    if (Rd != 31) NEXT_STATE.REGS[Rd] = res;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_subs_reg(uint32_t instr) {
    uint32_t Rd = instr & 0x1F;
    uint32_t Rn = (instr >> 5) & 0x1F;
    uint32_t imm3 = (instr >> 10) & 0x7;
    uint32_t opt = (instr >> 13) & 0x7;
    uint32_t Rm = (instr >> 16) & 0x1F;
    uint64_t op1 = (Rn == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[Rn];
    uint64_t op2 = extend_register(CURRENT_STATE.REGS[Rm], opt, imm3);
    uint64_t res = op1 - op2;
    update_flags(res);
    if (Rd != 31) NEXT_STATE.REGS[Rd] = res;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ands(uint32_t instr) {
    uint8_t shift = (instr >> 22) & 0x3;
    uint8_t rm = (instr >> 16) & 0x1F;
    uint8_t imm6 = (instr >> 10) & 0x3F;
    uint8_t rn = (instr >> 5) & 0x1F;
    uint8_t rd = instr & 0x1F;
    uint64_t op1 = CURRENT_STATE.REGS[rn];
    uint64_t op2 = CURRENT_STATE.REGS[rm];
    switch (shift) {
        case 0: op2 <<= imm6; break;
        case 1: op2 >>= imm6; break;
        case 2: op2 = ((int64_t)op2) >> imm6; break;
        case 3: op2 = (op2 >> imm6) | (op2 << (64 - imm6)); break;
    }
    uint64_t res = op1 & op2;
    update_flags(res);
    NEXT_STATE.REGS[rd] = res;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_eor(uint32_t instr) {
    uint32_t Rd = instr & 0x1F;
    uint32_t Rn = (instr >> 5) & 0x1F;
    uint32_t imm6 = (instr >> 10) & 0x3F;
    uint32_t Rm = (instr >> 16) & 0x1F;
    uint32_t shift = (instr >> 22) & 0x3;
    uint64_t op1 = CURRENT_STATE.REGS[Rn];
    uint64_t op2 = CURRENT_STATE.REGS[Rm];
    switch (shift) {
        case 0: op2 = (imm6 == 0) ? op2 : (op2 << imm6); break;
        case 1: op2 = (imm6 == 0) ? op2 : (op2 >> imm6); break;
        case 2: op2 = (imm6 == 0) ? op2 : ((int64_t)op2 >> imm6); break;
        case 3: op2 = (imm6 == 0) ? op2 : ((op2 >> imm6) | (op2 << (64 - imm6))); break;
    }
    uint64_t res = op1 ^ op2;
    NEXT_STATE.REGS[Rd] = res;
    update_flags(res);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_orr(uint32_t instr) {
    uint32_t Rd = instr & 0x1F;
    uint32_t Rn = (instr >> 5) & 0x1F;
    uint32_t Rm = (instr >> 16) & 0x1F;
    NEXT_STATE.REGS[Rd] = CURRENT_STATE.REGS[Rn] | CURRENT_STATE.REGS[Rm];
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_b(uint32_t instr) {
    int32_t imm26 = instr & 0x3FFFFFF;
    int64_t offset = ((int64_t)(imm26 << 6)) >> 4;
    CURRENT_STATE.PC += offset;
    branch_taken = 1;
}

void handle_br(uint32_t instr) {
    uint32_t Rn = (instr >> 5) & 0x1F;
    CURRENT_STATE.PC = CURRENT_STATE.REGS[Rn];
    branch_taken = 1;
}

void handle_stur(uint32_t instr) {
    int32_t imm9 = (instr >> 12) & 0x1FF;
    int32_t Rn = (instr >> 5) & 0x1F;
    int32_t Rt = instr & 0x1F;
    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
    mem_write_64(addr, CURRENT_STATE.REGS[Rt]);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_sturb(uint32_t instr) {
    int64_t imm9 = sign_extend((instr >> 12) & 0x1FF, 64);
    int64_t Rn = (instr >> 5) & 0x1F;
    int64_t Rt = instr & 0x1F;
    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
    mem_write_8(addr, (uint8_t)CURRENT_STATE.REGS[Rt]);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_sturh(uint32_t instr) {
    int64_t imm9 = sign_extend((instr >> 12) & 0x1FF, 64);
    int64_t Rn = (instr >> 5) & 0x1F;
    int64_t Rt = instr & 0x1F;
    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
    mem_write_16(addr, (uint16_t)CURRENT_STATE.REGS[Rt]);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ldur(uint32_t instr) {
    int32_t imm9 = (instr >> 12) & 0x1FF;
    int32_t Rn = (instr >> 5) & 0x1F;
    int32_t Rt = instr & 0x1F;
    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
    NEXT_STATE.REGS[Rt] = mem_read_64(addr);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ldurb(uint32_t instr) {
    int32_t imm9 = (instr >> 12) & 0x1FF;
    int32_t Rn = (instr >> 5) & 0x1F;
    int32_t Rt = instr & 0x1F;
    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
    NEXT_STATE.REGS[Rt] = mem_read_8(addr);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ldurh(uint32_t instr) {
    int32_t imm9 = (instr >> 12) & 0x1FF;
    int32_t Rn = (instr >> 5) & 0x1F;
    int32_t Rt = instr & 0x1F;
    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
    NEXT_STATE.REGS[Rt] = mem_read_16(addr);
    if (!branch_taken) NEXT_STATE.PC += 4;
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
    if (take_branch) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        branch_taken = 1;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

void handle_movz(uint32_t instr) {
    uint32_t rd = instr & 0x1F;
    uint32_t hw = (instr >> 21) & 0x3;
    uint32_t imm16 = (instr >> 5) & 0xFFFF;
    if (hw != 0)
        printf("MOVZ: solo se implementa el caso hw == 0.\n");
    NEXT_STATE.REGS[rd] = imm16;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_add_imm(uint32_t instr) {
    uint32_t imm12 = (instr >> 10) & 0xFFF;
    uint32_t shift = (instr >> 22) & 0x3;
    uint32_t rd = instr & 0x1F;
    uint32_t rn = (instr >> 5) & 0x1F;
    uint64_t imm = (shift == 1) ? (imm12 << 12) : imm12;
    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] + imm;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_add_reg(uint32_t instr) {
    uint32_t rd = instr & 0x1F;
    uint32_t rn = (instr >> 5) & 0x1F;
    uint32_t imm3 = (instr >> 10) & 0x7;
    uint32_t opt = (instr >> 13) & 0x7;
    uint32_t rm = (instr >> 16) & 0x1F;
    uint64_t res = CURRENT_STATE.REGS[rn] + extend_register(CURRENT_STATE.REGS[rm], opt, imm3);
    NEXT_STATE.REGS[rd] = res;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_mul(uint32_t instr) {
    uint32_t rd = instr & 0x1F;
    uint32_t rn = (instr >> 5) & 0x1F;
    uint32_t rm = (instr >> 16) & 0x1F;
    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] * CURRENT_STATE.REGS[rm];
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_cbz(uint32_t instr) {
    uint32_t rt = instr & 0x1F;
    int32_t imm = (instr >> 5) & 0x7FFFF;
    imm = sign_extend(imm, 19);
    int64_t offset = (int64_t)imm << 2;
    if (CURRENT_STATE.REGS[rt] == 0) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        branch_taken = 1;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

void handle_cbnz(uint32_t instr) {
    uint32_t rt = instr & 0x1F;
    int32_t imm = (instr >> 5) & 0x7FFFF;
    imm = sign_extend(imm, 19);
    int64_t offset = (int64_t)imm << 2;
    if (CURRENT_STATE.REGS[rt] != 0) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        branch_taken = 1;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

void handle_shifts(uint32_t instr) {
    uint32_t rd = instr & 0x1F;
    uint32_t rn = (instr >> 5) & 0x1F;
    uint32_t imm6 = (instr >> 10) & 0x3F;
    uint32_t type = (instr >> 22) & 0x1;
    uint64_t res = (type == 0) ? (CURRENT_STATE.REGS[rn] << imm6) : (CURRENT_STATE.REGS[rn] >> imm6);
    NEXT_STATE.REGS[rd] = res;
    if (!branch_taken) NEXT_STATE.PC += 4;
}
