.text
CMP X0, X0
BEQ label_equal

label_equal:
CMP X1, #1
BLT label_less

label_less:
CMP X2, #0
BNE label_not_equal

label_not_equal:
CMP X3, #0
BGE label_greater_equal

label_greater_equal:
CMP X4, #0
BLE label_less_equal

label_less_equal:
B label_unconditional

label_unconditional:
HLT 0
