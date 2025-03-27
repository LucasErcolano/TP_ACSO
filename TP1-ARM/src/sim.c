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

// Inicializa el mapa de opcodes usando como llave (longitud, opcode reducido)
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

// Formato R (ejemplo para instrucciones tipo R)
typedef struct {
    uint16_t opcode; // Bits [31..21]
    uint8_t  Rm;     // Bits [20..16]
    uint8_t  shamt;  // Bits [15..10]
    uint8_t  Rn;     // Bits [9..5]
    uint8_t  Rd;     // Bits [4..0]
} RFormat;

// Formato I (ejemplo para instrucciones tipo I)
typedef struct {
    uint16_t opcode;   // Bits [31..22]
    uint16_t ALU_imm;  // Bits [21..10]
    uint8_t  Rn;       // Bits [9..5]
    uint8_t  Rd;       // Bits [4..0]
} IFormat;

// Formato D (ejemplo para instrucciones de LOAD/STORE)
typedef struct {
    uint16_t opcode;     // Bits [31..21]
    uint16_t DT_address; // Bits [20..12]
    uint8_t  op;         // Bits [11..10]
    uint8_t  Rn;         // Bits [9..5]
    uint8_t  Rt;         // Bits [4..0]
} DFormat;

// Formato B (ejemplo para instrucciones B)
typedef struct {
    uint8_t  opcode;     // Bits [31..26]
    uint32_t BR_address; // Bits [25..0]
} BFormat;

// Formato CB (ejemplo para instrucciones condicionales CBZ, CBNZ, B.cond)
typedef struct {
    uint8_t  opcode;          // Bits [31..24]
    uint32_t COND_BR_address; // Bits [23..5]
    uint8_t  Rt;              // Bits [4..0]
} CBFormat;

// Formato IW (ejemplo para MOVZ, MOVK, etc.)
typedef struct {
    uint16_t opcode;   // Bits [31..21]
    uint16_t MOV_imm;  // Bits [20..5]
    uint8_t  Rd;       // Bits [4..0]
} IWFormat;


static inline RFormat decode_R(uint32_t instr) {
    RFormat r;
    r.opcode = (instr >> 21) & 0x7FF;  // bits [31..21]
    r.Rm     = (instr >> 16) & 0x1F;   // bits [20..16]
    r.shamt  = (instr >> 10) & 0x3F;   // bits [15..10]
    r.Rn     = (instr >> 5)  & 0x1F;   // bits [9..5]
    r.Rd     = instr & 0x1F;          // bits [4..0]
    return r;
}

static inline IFormat decode_I(uint32_t instr) {
    IFormat i;
    i.opcode   = (instr >> 22) & 0x3FF; // bits [31..22]
    i.ALU_imm  = (instr >> 10) & 0xFFF; // bits [21..10]
    i.Rn       = (instr >> 5)  & 0x1F;  // bits [9..5]
    i.Rd       = instr & 0x1F;         // bits [4..0]
    return i;
}

static inline DFormat decode_D(uint32_t instr) {
    DFormat d;
    d.opcode      = (instr >> 21) & 0x7FF; // bits [31..21]
    d.DT_address  = (instr >> 12) & 0x1FF; // bits [20..12]
    d.op          = (instr >> 10) & 0x3;   // bits [11..10]
    d.Rn          = (instr >> 5)  & 0x1F;  // bits [9..5]
    d.Rt          = instr & 0x1F;         // bits [4..0]
    return d;
}

static inline BFormat decode_B(uint32_t instr) {
    BFormat b;
    b.opcode     = (instr >> 26) & 0x3F;    // bits [31..26]
    b.BR_address = instr & 0x03FFFFFF;      // bits [25..0]
    return b;
}

static inline CBFormat decode_CB(uint32_t instr) {
    CBFormat cb;
    cb.opcode          = (instr >> 24) & 0xFF;    // bits [31..24]
    cb.COND_BR_address = (instr >> 5) & 0x7FFFF;  // bits [23..5]
    cb.Rt              = instr & 0x1F;           // bits [4..0]
    return cb;
}

static inline IWFormat decode_IW(uint32_t instr) {
    IWFormat iw;
    iw.opcode   = (instr >> 21) & 0x7FF;   // bits [31..21]
    iw.MOV_imm  = (instr >> 5)  & 0xFFFF;  // bits [20..5]
    iw.Rd       = instr & 0x1F;           // bits [4..0]
    return iw;
}
// Instrucción HLT
void handle_hlt(uint32_t instr) {
    RUN_BIT = 0;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

// ADD (inmediato) con set de banderas
void handle_adds_imm(uint32_t instr) {
    IFormat f = decode_I(instr);

    uint32_t shift = (instr >> 22) & 0x3;
    uint64_t imm   = (shift == 1) ? (f.ALU_imm << 12) : f.ALU_imm;

    uint64_t op1 = CURRENT_STATE.REGS[f.Rn];
    uint64_t res = op1 + imm;

    NEXT_STATE.REGS[f.Rd] = res;
    update_flags(res);

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// ADD (registrador) con set de banderas
void handle_adds_reg(uint32_t instr) {
    RFormat r = decode_R(instr);

    uint32_t imm3 = (instr >> 10) & 0x7;
    uint32_t opt  = (instr >> 13) & 0x7;

    uint64_t op1 = (r.Rn == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[r.Rn];
    uint64_t op2 = extend_register(CURRENT_STATE.REGS[r.Rm], opt, imm3);

    uint64_t res = op1 + op2;
    NEXT_STATE.REGS[r.Rd] = res;
    update_flags(res);

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// SUB (inmediato) con set de banderas
void handle_subs_imm(uint32_t instr) {
    IFormat f = decode_I(instr);

    uint32_t shift = (instr >> 22) & 0x3;
    uint64_t op1   = (f.Rn == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[f.Rn];
    uint64_t op2   = (shift == 1) ? (f.ALU_imm << 12) : f.ALU_imm;

    uint64_t res = op1 - op2;
    update_flags(res);

    if (f.Rd != 31) {
        NEXT_STATE.REGS[f.Rd] = res;
    }

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// SUB (registrador) con set de banderas
void handle_subs_reg(uint32_t instr) {
    RFormat r = decode_R(instr);

    uint32_t imm3 = (instr >> 10) & 0x7;
    uint32_t opt  = (instr >> 13) & 0x7;

    uint64_t op1 = (r.Rn == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[r.Rn];
    uint64_t op2 = extend_register(CURRENT_STATE.REGS[r.Rm], opt, imm3);

    uint64_t res = op1 - op2;
    update_flags(res);

    if (r.Rd != 31) {
        NEXT_STATE.REGS[r.Rd] = res;
    }

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// ANDS (ejemplo tipo R)
void handle_ands(uint32_t instr) {
    RFormat r = decode_R(instr);

    uint8_t shift = (instr >> 22) & 0x3;
    uint8_t imm6  = (instr >> 10) & 0x3F;

    uint64_t op1 = CURRENT_STATE.REGS[r.Rn];
    uint64_t op2 = CURRENT_STATE.REGS[r.Rm];

    switch (shift) {
        case 0: op2 <<= imm6; break;
        case 1: op2 >>= imm6; break;
        case 2: op2 = ((int64_t)op2) >> imm6; break;
        case 3: op2 = (op2 >> imm6) | (op2 << (64 - imm6)); break;
    }
    uint64_t res = op1 & op2;
    update_flags(res);

    NEXT_STATE.REGS[r.Rd] = res;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

// EOR (ejemplo tipo R con shift)
void handle_eor(uint32_t instr) {
    RFormat r = decode_R(instr);

    uint32_t shift = (instr >> 22) & 0x3;
    uint32_t imm6  = (instr >> 10) & 0x3F;

    uint64_t op1 = CURRENT_STATE.REGS[r.Rn];
    uint64_t op2 = CURRENT_STATE.REGS[r.Rm];

    switch (shift) {
        case 0: if (imm6) op2 <<= imm6; break;
        case 1: if (imm6) op2 >>= imm6; break;
        case 2: if (imm6) op2 = ((int64_t)op2) >> imm6; break;
        case 3: if (imm6) op2 = (op2 >> imm6) | (op2 << (64 - imm6)); break;
    }

    uint64_t res = op1 ^ op2;
    NEXT_STATE.REGS[r.Rd] = res;
    update_flags(res);

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// ORR (tipo R sencillo)
void handle_orr(uint32_t instr) {
    RFormat r = decode_R(instr);

    NEXT_STATE.REGS[r.Rd] = CURRENT_STATE.REGS[r.Rn] | CURRENT_STATE.REGS[r.Rm];
    if (!branch_taken) NEXT_STATE.PC += 4;
}

// B (branch incondicional) - formato B
void handle_b(uint32_t instr) {
    BFormat b = decode_B(instr);

    int64_t offset = ((int64_t)(b.BR_address << 6)) >> 4; 
    CURRENT_STATE.PC += offset;
    branch_taken = 1;
}

// BR (branch registrador)
void handle_br(uint32_t instr) {
    RFormat r = decode_R(instr);
    uint32_t Rn = r.Rn;

    CURRENT_STATE.PC = CURRENT_STATE.REGS[Rn];
    branch_taken = 1;
}

// STUR (Store) - formato D
void handle_stur(uint32_t instr) {
    DFormat d = decode_D(instr);

    int64_t imm9 = sign_extend(d.DT_address, 9);
    uint64_t addr = CURRENT_STATE.REGS[d.Rn] + imm9;

    mem_write_64(addr, CURRENT_STATE.REGS[d.Rt]);

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// STURB (Store byte)
void handle_sturb(uint32_t instr) {
    DFormat d = decode_D(instr);

    int64_t imm9 = sign_extend(d.DT_address, 9);
    uint64_t addr = CURRENT_STATE.REGS[d.Rn] + imm9;

    mem_write_8(addr, (uint8_t)CURRENT_STATE.REGS[d.Rt]);

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// STURH (Store halfword)
void handle_sturh(uint32_t instr) {
    DFormat d = decode_D(instr);

    int64_t imm9 = sign_extend(d.DT_address, 9);
    uint64_t addr = CURRENT_STATE.REGS[d.Rn] + imm9;

    mem_write_16(addr, (uint16_t)CURRENT_STATE.REGS[d.Rt]);

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// LDUR (Load) - formato D
void handle_ldur(uint32_t instr) {
    DFormat d = decode_D(instr);

    int64_t imm9 = sign_extend(d.DT_address, 9);
    uint64_t addr = CURRENT_STATE.REGS[d.Rn] + imm9;

    NEXT_STATE.REGS[d.Rt] = mem_read_64(addr);

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// LDURB (Load byte)
void handle_ldurb(uint32_t instr) {
    DFormat d = decode_D(instr);

    int64_t imm9 = sign_extend(d.DT_address, 9);
    uint64_t addr = CURRENT_STATE.REGS[d.Rn] + imm9;

    NEXT_STATE.REGS[d.Rt] = mem_read_8(addr);

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// LDURH (Load halfword)
void handle_ldurh(uint32_t instr) {
    DFormat d = decode_D(instr);

    int64_t imm9 = sign_extend(d.DT_address, 9);
    uint64_t addr = CURRENT_STATE.REGS[d.Rn] + imm9;

    NEXT_STATE.REGS[d.Rt] = mem_read_16(addr);

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// B.cond - formato CB
void handle_b_cond(uint32_t instr) {
    CBFormat cb = decode_CB(instr);

    int32_t imm19 = sign_extend(cb.COND_BR_address, 19);
    int64_t offset = (int64_t)imm19 << 2;

    uint32_t cond = instr & 0xF; // Por ejemplo, bits [3..0]

    int take_branch = 0;
    switch (cond) {
        case 0x0: if (CURRENT_STATE.FLAG_Z == 1) take_branch = 1; break;
        case 0x1: if (CURRENT_STATE.FLAG_Z == 0) take_branch = 1; break;
        case 0xA: if (CURRENT_STATE.FLAG_N == 0) take_branch = 1; break;
        case 0xB: if (CURRENT_STATE.FLAG_N != 0) take_branch = 1; break;
    }

    if (take_branch) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        branch_taken = 1;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

// MOVZ (ejemplo) - formato IW
void handle_movz(uint32_t instr) {
    IWFormat iw = decode_IW(instr);

    uint32_t hw = (instr >> 21) & 0x3;

    if (hw != 0) {
        printf("MOVZ: solo se implementa el caso hw == 0.\n");
    }
    NEXT_STATE.REGS[iw.Rd] = iw.MOV_imm;

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// ADD (inmediato) sin set de banderas
void handle_add_imm(uint32_t instr) {
    IFormat f = decode_I(instr);

    uint32_t shift = (instr >> 22) & 0x3;
    uint64_t imm   = (shift == 1) ? (f.ALU_imm << 12) : f.ALU_imm;

    NEXT_STATE.REGS[f.Rd] = CURRENT_STATE.REGS[f.Rn] + imm;

    if (!branch_taken) NEXT_STATE.PC += 4;
}

// ADD (registrador) sin set de banderas
void handle_add_reg(uint32_t instr) {
    RFormat r = decode_R(instr);

    uint32_t imm3 = (instr >> 10) & 0x7;
    uint32_t opt  = (instr >> 13) & 0x7;

    uint64_t op2 = extend_register(CURRENT_STATE.REGS[r.Rm], opt, imm3);
    uint64_t res = CURRENT_STATE.REGS[r.Rn] + op2;

    NEXT_STATE.REGS[r.Rd] = res;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

// MUL (ejemplo tipo R)
void handle_mul(uint32_t instr) {
    RFormat r = decode_R(instr);

    NEXT_STATE.REGS[r.Rd] = CURRENT_STATE.REGS[r.Rn] * CURRENT_STATE.REGS[r.Rm];
    if (!branch_taken) NEXT_STATE.PC += 4;
}

// CBZ - formato CB
void handle_cbz(uint32_t instr) {
    CBFormat cb = decode_CB(instr);

    int32_t imm19 = sign_extend(cb.COND_BR_address, 19);
    int64_t offset = (int64_t)imm19 << 2;

    if (CURRENT_STATE.REGS[cb.Rt] == 0) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        branch_taken = 1;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

// CBNZ - formato CB
void handle_cbnz(uint32_t instr) {
    CBFormat cb = decode_CB(instr);

    int32_t imm19 = sign_extend(cb.COND_BR_address, 19);
    int64_t offset = (int64_t)imm19 << 2;

    if (CURRENT_STATE.REGS[cb.Rt] != 0) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        branch_taken = 1;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

void handle_shifts(uint32_t instr) {
    RFormat r = decode_R(instr);

    uint32_t imm6 = r.shamt;  
    uint32_t type = (instr >> 22) & 0x1;

    uint64_t val = CURRENT_STATE.REGS[r.Rn];
    uint64_t res = (type == 0) ? (val << imm6) : (val >> imm6);

    NEXT_STATE.REGS[r.Rd] = res;
    if (!branch_taken) NEXT_STATE.PC += 4;
}