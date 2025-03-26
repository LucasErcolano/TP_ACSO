.text
cbnz X3, nouno
cbz X3, uno
adds X0, X1, 1

nouno:
adds X1, X2, 1
HLT 0

nodos:
adds X2, X3, 1
HLT 0

uno:
adds X3, X3, 1
cbz X3, nodos
cbnz X3, dos
HLT 0

dos:
adds X4, X5, 1
HLT 0
