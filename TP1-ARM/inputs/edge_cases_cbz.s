.text
MOV X0, #0
CBZ X0, label_cbz_zero
NOP

label_cbz_zero:
MOV X2, #42

MOV X1, #1
CBNZ X1, label_cbnz_nonzero
NOP

label_cbnz_nonzero:
MOV X3, #99

HLT 0
