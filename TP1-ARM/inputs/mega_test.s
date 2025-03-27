.teXt
adds X0, X0, 10
adds X1, X0, X2

subs X3, X0, 5
subs X4, X3, X2

ands X0, X10, X11

eor X0, X10, X11

orr X7, X5, X6

stur X10, [X1, 0x0]
sturb W10, [X1, 0x6]
sturH W10, [X1, 0x6]

ldur X14, [X1, 0x4]
ldurb W15, [X1, 0x6]
ldurH W15, [X1, 0x6]

HLT 0
