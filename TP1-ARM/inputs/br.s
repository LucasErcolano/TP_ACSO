.text
ADD X0, X0, 0X4000
LSL X0, X0, 4
ADD X0, X0, 24
BR X0
B label1
HLT 0

label1:
ADD X1, X2, #10                 // Sumar 10 a X2 y almacenar en X1
BR X0                           // Saltar a la dirección almacenada en X0
ADD X2,X3,1
ADD X3,X4,1
ADD X4,X5,1
ADD X5,X6,1
HLT 0
