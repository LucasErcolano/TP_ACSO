.text
MOVZ X0, 0x400008
B label1
HLT 0

label1:
ADD X1, X2, 10
BR X0
HLT 0
