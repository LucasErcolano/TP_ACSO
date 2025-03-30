.text
// Inicializar registros
MOV X11, #10
MOV X12, #20

cmp X11, X11
bne nouno
cmp X11, X11
beq uno
adds X1, X0, 10
HLT 0

nouno:
adds X5, X0, 10
HLT 0

nodos:
adds X6, X0, 10
HLT 0

notres:
adds X7, X0, 10
HLT 0

uno:
adds X2, X0, 1
adds X11, X0, 10
cmp X11, X12
beq nodos
cmp X11, X12
bne dos
HLT 0

dos:
adds X3, X0, 1
cmp X11, X12
blt notres
cmp X11, X12
bgt tres
HLT 0

tres:
adds X4, X0, 1
cmp X12, X11
bgt notres
cmp X12, X11
blt end
HLT 0

end:
HLT 0
