#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "shell.h"
#include "hashmap.h"
#include "decode.h"
#include "handlers.h"

typedef void (*InstructionHandler)(uint32_t);

typedef struct {
    uint32_t pattern;
    int length;
    InstructionHandler handler;
} InstructionEntry;

HashMap *opcode_map = NULL;
int branch_taken = 0;

// Inicializa el mapa de opcodes usando (longitud, opcode reducido)
void init_opcode_map() {
    if (opcode_map) return;
    opcode_map = hashmap_create();
    InstructionEntry entries[] = {
        {0xD61F00, 22, handle_br},
        {0x380, 11, handle_sturb},
        {0x384, 11, handle_ldurb},
        {0x780, 11, handle_sturh},
        {0x784, 11, handle_ldurh},
        {0x8B0, 11, handle_add_reg},
        {0x9B0, 11, handle_mul},
        {0xF80, 11, handle_stur},
        {0xF84, 11, handle_ldur},
        {0xD28, 11, handle_movz}, 
        {0xD34, 10, handle_shift},
        {0x54, 8, handle_b_cond},
        {0x91, 8, handle_add_imm},
        {0xAA, 8, handle_orr},
        {0xAB, 8, handle_adds_reg},
        {0xB1, 8, handle_adds_imm},
        {0xB4, 8, handle_cbz},
        {0xB5, 8, handle_cbnz},
        {0xCA, 8, handle_eor},
        {0xEA, 8, handle_ands},
        {0xEB, 8, handle_subs_reg},
        {0xF1, 8, handle_subs_imm},
        {0xD4, 8, handle_hlt},
        {0x14, 6, handle_b}
    };
    int n = sizeof(entries) / sizeof(entries[0]);
    for (int i = 0; i < n; i++) {
        uint32_t len = entries[i].length;
        uint32_t opcode_key = entries[i].pattern;
        hashmap_put(opcode_map, len, opcode_key, entries[i].handler);
    }
}

InstructionHandler decode_instruction(uint32_t instruction) {
    if (!opcode_map) init_opcode_map();
    static const int lengths[] = {22, 11, 10, 8, 6};
    for (int i = 0; i < sizeof(lengths)/sizeof(lengths[0]); i++) {
        int len = lengths[i];
        uint32_t opcode_key = instruction >> (32 - len);
        opcode_key <<= ((32 - len) % 4);
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
