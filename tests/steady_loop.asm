SET R1, 10000      // loop count
SET R2, 1          // increment
SET R3, 0          // accumulator
SET R4, 1          // decrement value
loop: ADD R3, R3, R2
SUB R1, R1, R4
BEQ R1, R0, done
J loop
done: OUT 0, R3