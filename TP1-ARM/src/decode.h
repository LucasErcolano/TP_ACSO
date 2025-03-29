#ifndef DECODE_H
#define DECODE_H

#include <stdint.h>

void decode_i_group(uint32_t instr, uint32_t *imm12, uint32_t *shift, uint32_t *d, uint32_t *n);
void decode_r_group(uint32_t instr, uint32_t *opt, uint32_t *imm3, uint32_t *d, uint32_t *n, uint32_t *m);
void decode_shifted_register(uint32_t instr, uint32_t *shift, uint32_t *imm6, uint32_t *d, uint32_t *n, uint32_t *m);
void decode_mem_access(uint32_t instr, int32_t *imm9, int32_t *n, int32_t *t);
void decode_conditional_branch(uint32_t instr, uint32_t *t, uint32_t *offset);
void decode_lsl_lsr(uint32_t instr, bool *is_lsr, uint8_t *shift, uint8_t *rd, uint8_t *rn);

int64_t sign_extend(int64_t value, int bits);
uint64_t extend_register(uint64_t value, int option, int imm3);
int64_t calculate_mathOps(uint32_t n, uint32_t m, uint32_t opt, uint32_t imm3, int isSubtraction, int isImm);
void update_flags(int64_t result);

// Funciones de acceso a memoria
uint8_t mem_read_8(uint64_t addr);
uint16_t mem_read_16(uint64_t addr);
uint64_t mem_read_64(uint64_t addr);
void mem_write_8(uint64_t addr, uint8_t value);
void mem_write_16(uint64_t addr, uint16_t value);
void mem_write_64(uint64_t addr, uint64_t value);

#endif
