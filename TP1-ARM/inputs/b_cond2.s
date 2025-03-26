.text
cmp X11, X11
bge uno
adds X1, X0, 10

nouno:
adds X5, X0, 10
HLT 0

nodos:
adds X6, X0, 10
HLT 0

uno:
adds X2, X0, 1
adds X11, X0, 10
cmp X11, X12
ble nouno
cmp X11, X12
bge dos
HLT 0

dos:
adds X3, X0, 1
cmp X12, X11
ble nodos
cmp X12, X11
bge tres
HLT 0

tres:
adds X4, X0, 1
HLT 0