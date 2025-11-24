ASP Traffic-Light Simulator
===========================

This project implements a small Application-Specific Processor (ASP) simulator tailored to a traffic-light interchange domain. It includes both a single-cycle model and a 5-stage pipelined model with hazard handling so that students can observe instruction-level behavior and compare performance.

Highlights
----------
- Custom ISA with 8 core instructions (see `isa.txt`).
- Single-cycle engine with instruction/memory trace.
- 5-stage pipeline (IF/ID/EX/MEM/WB) with:
  - Forwarding from EX/MEM and MEM/WB.
  - Automatic load-use stalling.
  - Control-hazard flush on BEQ/J.
- Cycle-by-cycle pipeline trace plus CPI reporting for both modes.
- Configurable cycle cap (`-c`) to explore finite portions of long-running control programs (e.g., the included `samples/traffic.asm` traffic controller).

Building
--------
Requires a C compiler (tested with `gcc`):

```
cd E:\work\conpa
gcc -O2 -o asp_sim src/main.c
```

Running
-------
```
.\asp_sim -i <program.asm> [-c maxCycles] -s     # single-cycle
.\asp_sim -i <program.asm> [-c maxCycles] -p     # pipelined
```

Use `-c` to limit the number of simulated cycles when running programs that never halt (default: 200 cycles). Both modes print register dumps and CPI so you can compare single-cycle vs. pipelined throughput.

Sample programs:
- `samples/traffic.asm`: infinite traffic controller (run with `-c` to limit trace).
- `tests/arthmetic.asm`: short arithmetic sanity test.
- `tests/branch.asm`: loop/branch control sample.
- `tests/hazard.asm`: load-use forwarding/stall micro-benchmark.

Example comparison:
```
.\asp_sim -i tests/branch.asm -s
.\asp_sim -i tests/branch.asm -p
```

Use the reported `Cycles`, `Instructions`, and `CPI` lines to quantify speedup between the two models.

Documentation
-------------
- `isa.txt`: instruction encodings, semantics, and addressing conventions.
- `samples/` & `tests/`: ready-to-run assembly workloads demonstrating arithmetic, control, and hazard scenarios.

For class reports/demos, capture excerpts of the traces (single-cycle vs. pipelined) alongside CPI numbers to highlight architectural differences.
