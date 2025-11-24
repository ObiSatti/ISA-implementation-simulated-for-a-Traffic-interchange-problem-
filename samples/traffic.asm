start:
    # Traffic-light: R1=timer, R2=state (0=NS green,1=EW green), R15=1 (constant)
    SET R15, 1
    SET R1, 3
    SET R2, 0
loop:
    OUT 0, R2
    SUB R1, R1, R15    # decrement timer
    BEQ R1, R0, toggle
    J loop
toggle:
    BEQ R2, R0, set1
    SET R2, 0
    SET R1, 3
    J loop
set1:
    SET R2, 1
    SET R1, 3
    J loop

