SET R1, 0
SET R2, 1
BEQ R1, R2, skip
SET R3, 9
J end
skip: SET R3, 42
end: OUT 0, R3