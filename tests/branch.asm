# Branching / control-flow sample
SET R15, 1
SET R1, 0          # accumulator
SET R2, 3          # loop counter

loop:
ADD R1, R1, R15
SUB R2, R2, R15
BEQ R2, R0, done
J loop

done:
OUT 0, R1


