# Load-use hazard micro-benchmark
SET R15, 1
SET R2, 0
SET R3, 42
SW R3, 0(R2)
LW R4, 0(R2)
ADD R5, R4, R15   # should forward/stall correctly
OUT 0, R5


