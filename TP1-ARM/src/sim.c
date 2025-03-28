#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "stdbool.h"
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

void handle_hlt(uint32_t); //✅ 
void handle_adds_imm(uint32_t);//✅ 
void handle_adds_reg(uint32_t);//✅ 
void handle_subs_imm(uint32_t);//✅ 
void handle_subs_reg(uint32_t);//✅ 
void handle_ands(uint32_t);//✅ 
void handle_eor(uint32_t);//✅ 
void handle_orr(uint32_t);//✅ 
void handle_b(uint32_t);
void handle_br(uint32_t);
void handle_b_cond(uint32_t);
void handle_cbz(uint32_t);//✅ 
void handle_cbnz(uint32_t);//✅ 
void handle_ldur(uint32_t);//✅ 
void handle_stur(uint32_t);//✅ 
void handle_movz(uint32_t);//✅ 
void handle_mul(uint32_t);//✅ 
void handle_lsl(uint32_t);
void handle_lsr(uint32_t);//?
void handle_sturb(uint32_t);//✅ 
void handle_sturh(uint32_t);//✅ 
void handle_ldurb(uint32_t);//✅ 
void handle_ldurh(uint32_t);//✅ 
void handle_add_reg(uint32_t);//✅ 
void handle_add_imm(uint32_t);//✅ 

// Inicializa el mapa de opcodes usando como llave (longitud, opcode reducido)
void init_opcode_map() {
    if (opcode_map) return;
    opcode_map = hashmap_create();
    InstructionEntry entries[] = {
        {0xD4000000, 8, handle_hlt},
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
        {0xD2800000, 11, handle_movz},
        {0xD3400000, 11, handle_lsr},
        {0xD3700000, 11, handle_lsl},
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
        uint32_t len = entries[i].length;
        uint32_t opcode_key = entries[i].pattern >> (32 - len);
        hashmap_put(opcode_map, len, opcode_key, entries[i].handler);
    }
}

InstructionHandler decode_instruction(uint32_t instruction) {
    if (!opcode_map) init_opcode_map();
    static const int lengths[] = {22, 11, 9, 8, 6};
    for (int i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        int len = lengths[i];
        uint32_t opcode_key = instruction >> (32 - len);
        InstructionHandler handler = hashmap_get(opcode_map, len, opcode_key);
        if (handler)
            return handler;
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

uint64_t calculate_mathOps(uint32_t n, uint32_t m, uint32_t opt, uint32_t imm3, bool isSubtraction, bool isImm) {
    uint64_t op1 = (n == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[n];
    uint64_t op2;
    
    if (isImm) {
        op2 = (opt == 1) ? (imm3 << 12) : imm3;
    } else {
        op2 = extend_register(CURRENT_STATE.REGS[m], opt, imm3);
    }
    
    uint64_t res = isSubtraction ? op1 - op2 : op1 + op2;
    return res;
}

void handle_hlt(uint32_t instr) {
    RUN_BIT = 0;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_adds_imm(uint32_t instr) {
    uint32_t imm12, shift, d, n;
    decode_i_group(instr, &imm12, &shift, &d, &n);
    uint64_t res = calculate_mathOps(n, 0, shift, imm12, false, true);
    NEXT_STATE.REGS[d] = res;
    update_flags(res);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_adds_reg(uint32_t instr) {
    uint32_t opt, imm3, d, n, m;
    decode_r_group(instr, &opt, &imm3, &d, &n, &m);
    uint64_t res = calculate_mathOps(n, m, opt, imm3, false, false);
    NEXT_STATE.REGS[d] = res;
    update_flags(res);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_subs_imm(uint32_t instr) {
    uint32_t imm12, shift, d, n;
    decode_i_group(instr, &imm12, &shift, &d, &n);
    uint64_t res = calculate_mathOps(n, 0, shift, imm12, true, true);
    //if (d != 31)
    NEXT_STATE.REGS[d] = res;
    update_flags(res);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_subs_reg(uint32_t instr) {
    uint32_t opt, imm3, d, n, m;
    decode_r_group(instr, &opt, &imm3, &d, &n, &m);
    uint64_t res = calculate_mathOps(n, m, opt, imm3, true, false);
    //if (d != 31)
    NEXT_STATE.REGS[d] = res;
    update_flags(res);
    if (!branch_taken) NEXT_STATE.PC += 4;
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
    if (!branch_taken) NEXT_STATE.PC += 4;
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
    update_flags(res);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_orr(uint32_t instr) {
    uint32_t opt, imm3, d, n, m;
    decode_r_group(instr, &opt, &imm3, &d, &n, &m);

    NEXT_STATE.REGS[d] = CURRENT_STATE.REGS[n] | CURRENT_STATE.REGS[m];
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_b(uint32_t instr) {
    int32_t imm26 = instr & 0x3FFFFFF;
    int64_t offset = ((int64_t)(imm26 << 6)) >> 4;
    CURRENT_STATE.PC += offset;
    branch_taken = 1;
}

void handle_br(uint32_t instr) {
    uint32_t n = (instr >> 5) & 0x1F;

    CURRENT_STATE.PC = CURRENT_STATE.REGS[n];
    branch_taken = 1;
}

void handle_stur(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);

    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    mem_write_64(addr, CURRENT_STATE.REGS[t]);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_sturb(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);

    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    mem_write_8(addr, (uint8_t)CURRENT_STATE.REGS[t]);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_sturh(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);

    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    mem_write_16(addr, (uint16_t)CURRENT_STATE.REGS[t]);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ldur(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);

    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    NEXT_STATE.REGS[t] = mem_read_64(addr);
    if (!branch_taken) NEXT_STATE.PC += 4;
}           

void handle_ldurb(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);

    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    NEXT_STATE.REGS[t] = mem_read_8(addr);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ldurh(uint32_t instr) {
    int32_t imm9, n, t;
    decode_mem_access(instr, &imm9, &n, &t);

    imm9 = sign_extend(imm9, 64);
    uint64_t addr = CURRENT_STATE.REGS[n] + imm9;
    NEXT_STATE.REGS[t] = mem_read_16(addr);
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
    uint32_t d = instr & 0x1F;
    uint32_t hw = (instr >> 21) & 0x3;
    uint32_t imm16 = (instr >> 5) & 0xFFFF;

    if (hw != 0)
        printf("MOVZ: solo se implementa el caso hw == 0.\n");
    
    NEXT_STATE.REGS[d] = imm16;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_add_imm(uint32_t instr) {
    uint32_t imm12, shift, d, n;
    decode_i_group(instr, &imm12, &shift, &d, &n);

    uint64_t imm = (shift == 1) ? (imm12 << 12) : imm12;
    NEXT_STATE.REGS[d] = CURRENT_STATE.REGS[n] + imm; 
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_add_reg(uint32_t instr) {
    uint32_t opt, imm3, d, n, m;
    decode_r_group(instr, &opt, &imm3, &d, &n, &m);

    uint64_t res = CURRENT_STATE.REGS[n] + extend_register(CURRENT_STATE.REGS[m], opt, imm3);
    NEXT_STATE.REGS[d] = res;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_mul(uint32_t instr) {
    uint32_t opt, imm3, d, n, m;
    decode_r_group(instr, &opt, &imm3, &d, &n, &m);

    NEXT_STATE.REGS[d] = CURRENT_STATE.REGS[n] * CURRENT_STATE.REGS[m];
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_cbz(uint32_t instr) {
    uint32_t t, offset;
    decode_conditional_branch(instr, &t, &offset);

    if (CURRENT_STATE.REGS[t] == 0) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        branch_taken = 1;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

void handle_cbnz(uint32_t instr) {
    uint32_t t, offset;
    decode_conditional_branch(instr, &t, &offset);

    if (CURRENT_STATE.REGS[t] != 0) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        branch_taken = 1;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

void handle_lsl(uint32_t instr) {
    uint32_t imm12, d, n;
    decode_i_group(instr, &imm12, 0, &d, &n);
    uint64_t result = CURRENT_STATE.REGS[n] << imm12;
    NEXT_STATE.REGS[d] = result;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_lsr(uint32_t instr) {
    uint32_t imm12, d, n;
    decode_i_group(instr, &imm12, 0, &d, &n);
    uint64_t result = CURRENT_STATE.REGS[n] >> imm12;
    NEXT_STATE.REGS[d] = result;
    if (!branch_taken) NEXT_STATE.PC += 4;
}


