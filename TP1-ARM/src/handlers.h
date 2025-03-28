#ifndef HANDLERS_H
#define HANDLERS_H

#include <stdint.h>

void handle_hlt(uint32_t instr);
void handle_adds_imm(uint32_t instr);
void handle_adds_reg(uint32_t instr);
void handle_subs_imm(uint32_t instr);
void handle_subs_reg(uint32_t instr);
void handle_ands(uint32_t instr);
void handle_eor(uint32_t instr);
void handle_orr(uint32_t instr);
void handle_b(uint32_t instr);
void handle_br(uint32_t instr);
void handle_b_cond(uint32_t instr);
void handle_cbz(uint32_t instr);
void handle_cbnz(uint32_t instr);
void handle_ldur(uint32_t instr);
void handle_stur(uint32_t instr);
void handle_movz(uint32_t instr);
void handle_add_imm(uint32_t instr);
void handle_add_reg(uint32_t instr);
void handle_mul(uint32_t instr);
void handle_lsl(uint32_t instr);
void handle_lsr(uint32_t instr);
void handle_sturb(uint32_t instr);
void handle_sturh(uint32_t instr);
void handle_ldurb(uint32_t instr);
void handle_ldurh(uint32_t instr);

#endif
