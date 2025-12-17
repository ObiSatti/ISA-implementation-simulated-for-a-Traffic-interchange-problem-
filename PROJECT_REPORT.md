# Traffic Light Processor Simulator - Complete Project Report

---

## Table of Contents
1. [What is This Project?](#1-what-is-this-project)
2. [Basic Concepts Explained Simply](#2-basic-concepts-explained-simply)
3. [The Custom Instruction Set (ISA)](#3-the-custom-instruction-set-isa)
4. [Project File Structure](#4-project-file-structure)
5. [The Main Code Explained](#5-the-main-code-explained)
6. [Sample Programs Explained](#6-sample-programs-explained)
7. [Test Programs Explained](#7-test-programs-explained)
8. [How to Build and Run](#8-how-to-build-and-run)
9. [Understanding the Output](#9-understanding-the-output)
10. [Performance Comparison](#10-performance-comparison)
11. [Key Concepts for Demo](#11-key-concepts-for-demo)

---

## 1. What is This Project?

This project is a **CPU Simulator** written in C that mimics how a real processor works. It's specifically designed to control **traffic lights** at a road intersection.

### In Simple Words:
- Imagine a tiny computer chip that controls traffic signals
- This project creates a **pretend version** of that chip in software
- It can run programs written in **assembly language** (a very basic programming language)
- It shows you **step-by-step** how a CPU processes instructions

### Why Two Modes?
The simulator has **two different ways** to process instructions:
1. **Single-Cycle Mode**: Completes one instruction completely before starting the next (like finishing one task before starting another)
2. **Pipelined Mode**: Works on multiple instructions at the same time (like an assembly line in a factory)

---

## 2. Basic Concepts Explained Simply

### What is a CPU?
A CPU (Central Processing Unit) is the "brain" of a computer. It:
- Reads instructions (like "add these two numbers")
- Executes them
- Stores results

### What is Assembly Language?
- The most basic programming language that a CPU understands
- Each line is ONE simple instruction
- Example: `ADD R1, R2, R3` means "add R2 and R3, put result in R1"

### What is a Register?
- A register is like a **small storage box inside the CPU**
- Very fast to access
- This project has **16 registers** named R0 to R15
- **R0 is special**: it ALWAYS contains 0 (cannot be changed)

### What is Memory?
- Memory is like a **bigger storage area** outside the CPU
- Slower to access than registers
- Used to store data that doesn't fit in registers

---

### Single-Cycle vs Pipelined - The Key Difference

#### Single-Cycle (Non-Pipelined)
Think of a **laundry machine** that does EVERYTHING for one load before starting the next:

```
Shirt 1: [Wash → Dry → Fold] 
                              Shirt 2: [Wash → Dry → Fold]
                                                            Shirt 3: [Wash → Dry → Fold]
```
- Each instruction finishes COMPLETELY before the next starts
- Simple, but slow
- **CPI (Cycles Per Instruction) = 1.00** always

#### Pipelined
Think of a **car factory assembly line**:

```
         Cycle 1  Cycle 2  Cycle 3  Cycle 4  Cycle 5
Car 1:   [Frame]  [Engine] [Paint]  [Wheels] [Done!]
Car 2:            [Frame]  [Engine] [Paint]  [Wheels]
Car 3:                     [Frame]  [Engine] [Paint]
```
- Multiple cars being worked on simultaneously
- More complex, but faster overall
- **5 stages in this project**: IF → ID → EX → MEM → WB

### The 5 Pipeline Stages:
| Stage | Name | What It Does |
|-------|------|--------------|
| **IF** | Instruction Fetch | Gets the instruction from memory |
| **ID** | Instruction Decode | Figures out what the instruction means |
| **EX** | Execute | Does the actual calculation |
| **MEM** | Memory Access | Reads/writes to memory if needed |
| **WB** | Write Back | Saves the result to a register |

---

### Pipeline Hazards (Problems)

When using a pipeline, sometimes there are **conflicts**:

#### 1. Data Hazard (Load-Use Hazard)
**Problem**: An instruction needs data that isn't ready yet
```assembly
LW R2, 0(R1)     # Load value from memory into R2
ADD R3, R2, R4   # PROBLEM! R2 isn't ready yet!
```
**Solution**: The simulator automatically **stalls** (waits) for 1 cycle

#### 2. Control Hazard (Branch Hazard)
**Problem**: A branch/jump changes where the program goes, but we already started other instructions
```assembly
BEQ R1, R2, skip  # If R1 equals R2, jump to "skip"
ADD R3, R4, R5    # Already started this... but maybe we shouldn't!
```
**Solution**: The simulator **flushes** (cancels) wrong instructions

#### 3. Forwarding (Data Bypassing)
**Solution to some hazards**: Pass results directly between stages without waiting for write-back
```
EX result → directly to next EX stage (bypasses register file)
```

---

## 3. The Custom Instruction Set (ISA)

**ISA** = Instruction Set Architecture (the "language" the CPU understands)

This project has **9 instructions**:

| Instruction | Format | What It Does | Example |
|-------------|--------|--------------|---------|
| **ADD** | `ADD rd, rs, rt` | rd = rs + rt | `ADD R3, R1, R2` → R3 = R1 + R2 |
| **SUB** | `SUB rd, rs, rt` | rd = rs - rt | `SUB R3, R1, R2` → R3 = R1 - R2 |
| **LW** | `LW rd, imm(rs)` | Load word from memory | `LW R2, 4(R1)` → R2 = Memory[R1 + 4] |
| **SW** | `SW rs, imm(rd)` | Store word to memory | `SW R2, 4(R1)` → Memory[R1 + 4] = R2 |
| **BEQ** | `BEQ rs, rt, label` | Branch if equal | `BEQ R1, R2, loop` → if R1==R2, go to loop |
| **J** | `J label` | Jump unconditionally | `J start` → go to start |
| **SET** | `SET rd, imm` | Set register to value | `SET R1, 10` → R1 = 10 |
| **OUT** | `OUT port, rs` | Output to port | `OUT 0, R1` → send R1 value to port 0 |
| **NOP** | `NOP` | Do nothing | Used for timing/delays |

### Labels
Labels are **markers** in the code that you can jump to:
```assembly
start:          # This is a label called "start"
    ADD R1, R2, R3
    J start     # Jump back to the "start" label (creates a loop)
```

---

## 4. Project File Structure

```
📁 ISA-implementation-simulated-for-a-Traffic-interchange-problem-/
│
├── 📄 README.md                    # Quick project overview
├── 📄 isa.txt                      # Instruction set documentation
├── 📄 performance_analysis.txt     # Detailed comparison report
├── 📄 single_cycle_output.txt      # Sample output from single-cycle mode
├── 📄 pipelined_output.txt         # Sample output from pipelined mode
│
├── 📁 src/
│   └── 📄 main.c                   # THE MAIN CODE - the actual simulator
│
├── 📁 samples/
│   ├── 📄 traffic.asm              # Simple traffic light program
│   ├── 📄 traffic_interchange.asm  # Complex 4-way intersection program
│   └── 📄 assembly.txt             # (Empty file)
│
├── 📁 tests/
│   ├── 📄 arthmetic.asm            # Tests ADD instruction
│   ├── 📄 branch.asm               # Tests BEQ and J instructions
│   ├── 📄 hazard.asm               # Tests load-use hazard detection
│   └── 📄 steady_loop.asm          # Tests loop performance
│
└── 📁 scripts/
    └── 📄 compare.ps1              # PowerShell script to compare both modes
```

---

## 5. The Main Code Explained

The main code is in [src/main.c](src/main.c). Here's what each part does:

### Key Data Structures:

```c
// Instruction types
typedef enum { I_NOP, I_ADD, I_SUB, I_LW, I_SW, I_BEQ, I_J, I_OUT, I_SET } InstType;

// An instruction contains:
typedef struct {
    InstType type;    // What kind of instruction (ADD, SUB, etc.)
    int rd, rs, rt;   // Register numbers
    int imm;          // Immediate value (a number)
    char label[64];   // For jumps/branches
    char raw[128];    // The original text
} Instr;
```

### Pipeline Registers (for pipelined mode):
```c
IFID  // Between IF and ID stages
IDEX  // Between ID and EX stages
EXMEM // Between EX and MEM stages
MEMWB // Between MEM and WB stages
```

### Key Functions:

| Function | Purpose |
|----------|---------|
| `parse_file()` | Reads the .asm file and converts it to instructions |
| `run_single()` | Runs the single-cycle simulator |
| `run_pipelined()` | Runs the 5-stage pipelined simulator |
| `writes_back()` | Checks if instruction writes to a register |
| `uses_reg_source()` | Checks if instruction reads from a register |
| `dest_reg()` | Gets the destination register of an instruction |
| `dump_regs()` | Prints all register values |

### How Single-Cycle Works:
```c
while(pc < prog_len) {
    // 1. Get instruction at current PC
    // 2. Execute it completely
    // 3. Move PC to next instruction
    // 4. Repeat
}
```

### How Pipelined Works:
```c
while(cycle < max_cycles) {
    // 5. WB Stage  - Write results back
    // 4. MEM Stage - Access memory
    // 3. EX Stage  - Execute ALU operations
    // 2. ID Stage  - Decode instruction, detect hazards
    // 1. IF Stage  - Fetch next instruction
    // (Done in reverse order to avoid overwriting!)
}
```

---

## 6. Sample Programs Explained

### Sample 1: traffic.asm (Simple Traffic Light)

```assembly
start:
    SET R15, 1          # R15 = 1 (constant for subtraction)
    SET R1, 3           # R1 = timer (counts down from 3)
    SET R2, 0           # R2 = state (0=North-South green, 1=East-West green)

loop:
    OUT 0, R2           # Output current state to port 0
    SUB R1, R1, R15     # timer = timer - 1
    BEQ R1, R0, toggle  # If timer reaches 0, toggle the light
    J loop              # Otherwise, continue loop

toggle:
    BEQ R2, R0, set1    # If state is 0, go to set1
    SET R2, 0           # Else, set state to 0 (NS green)
    SET R1, 3           # Reset timer
    J loop

set1:
    SET R2, 1           # Set state to 1 (EW green)
    SET R1, 3           # Reset timer
    J loop
```

**What it does**: Alternates between North-South (0) and East-West (1) green lights every 3 cycles. Runs forever!

---

### Sample 2: traffic_interchange.asm (Complex 4-Way Intersection)

This is a **208-line program** that simulates a real 4-way traffic junction with:
- North-South main road
- East-West main road
- Left turn lanes for both
- Pedestrian crossings
- Emergency vehicle priority!

**Memory Layout**:
| Address | Contents |
|---------|----------|
| MEM[0]  | NS main road timer |
| MEM[4]  | EW main road timer |
| MEM[8]  | NS left turn timer |
| MEM[12] | EW left turn timer |
| MEM[16] | Pedestrian crossing timer |
| MEM[20] | Current phase (0-4) |
| MEM[24] | Emergency mode flag |
| MEM[28] | Cycle counter |

**Phases**:
- Phase 0: North-South green (5 cycles)
- Phase 1: East-West green (5 cycles)
- Phase 2: NS left turn (3 cycles)
- Phase 3: EW left turn (3 cycles)
- Phase 4: Pedestrian crossing (2 cycles)

---

## 7. Test Programs Explained

### Test 1: arthmetic.asm
```assembly
SET R1, 10          # R1 = 10
SET R2, 20          # R2 = 20
ADD R3, R1, R2      # R3 = 10 + 20 = 30
OUT 0, R3           # Output 30 to port 0
```
**Purpose**: Tests if ADD works correctly.

---

### Test 2: branch.asm
```assembly
SET R1, 0           # R1 = 0
SET R2, 1           # R2 = 1
BEQ R1, R2, skip    # If R1 == R2, skip (they're NOT equal, so don't skip)
SET R3, 9           # R3 = 9
J end               # Jump to end
skip: 
    SET R3, 42      # R3 = 42 (this won't run)
end: 
    OUT 0, R3       # Output R3 (should be 9)
```
**Purpose**: Tests if BEQ and J work correctly.

---

### Test 3: hazard.asm
```assembly
SET R1, 0           # R1 = 0 (base address)
SET R5, 7           # R5 = 7
SW R5, 0(R1)        # Store 7 to memory[0]
LW R2, 0(R1)        # Load from memory[0] into R2 (R2 = 7)
ADD R3, R2, R2      # R3 = R2 + R2 = 14 (HAZARD! R2 just loaded!)
OUT 0, R3           # Output 14
```
**Purpose**: Tests if the pipeline detects load-use hazard and stalls correctly.

---

### Test 4: steady_loop.asm
```assembly
SET R1, 10000       # Counter = 10000
SET R2, 1           # Increment value
SET R3, 0           # Accumulator
SET R4, 1           # Decrement value

loop: 
    ADD R3, R3, R2  # accumulator += 1
    SUB R1, R1, R4  # counter -= 1
    BEQ R1, R0, done # If counter = 0, done
    J loop          # Repeat

done: 
    OUT 0, R3       # Output final sum (should be 10000)
```
**Purpose**: Tests loop performance with 10,000 iterations.

---

## 8. How to Build and Run

### Step 1: Compile the Program

**On Linux/Mac:**
```bash
cd /root/ISA-implementation-simulated-for-a-Traffic-interchange-problem-
gcc -O2 -o asp_sim src/main.c
```

**On Windows:**
```cmd
cd E:\path\to\project
gcc -O2 -o asp_sim.exe src/main.c
```

### Step 2: Run the Simulator

**Basic Command Format:**
```bash
./asp_sim -i <assembly_file> [-c max_cycles] -s|-p
```

**Options:**
| Option | Meaning |
|--------|---------|
| `-i <file>` | Input assembly file (required) |
| `-s` | Run in single-cycle mode |
| `-p` | Run in pipelined mode |
| `-c <number>` | Maximum cycles to run (default: 200) |

### Examples:

**Run arithmetic test in single-cycle mode:**
```bash
./asp_sim -i tests/arthmetic.asm -s
```

**Run arithmetic test in pipelined mode:**
```bash
./asp_sim -i tests/arthmetic.asm -p
```

**Run traffic simulation with 100 cycle limit:**
```bash
./asp_sim -i samples/traffic.asm -c 100 -s
```

**Compare both modes on branch test:**
```bash
./asp_sim -i tests/branch.asm -s
./asp_sim -i tests/branch.asm -p
```

---

## 9. Understanding the Output

### Single-Cycle Output:
```
--- Single-cycle execution trace ---
PC=00: SET R15, 1          # Constant 1
PC=01: SET R14, 5          # Main road green duration
PC=02: SET R13, 3          # Left turn duration
...
  OUT port 0 <= 0
...
--- Final registers ---
R00=0 R01=0 R02=5 R03=0 R04=0 R05=0 ...
Cycles=150, Instructions=150, CPI=1.00
```

**What it shows:**
- Each line shows the PC (Program Counter) and the instruction being executed
- `OUT port X <= Y` shows output to ports
- Final register values
- Statistics: total cycles, instructions executed, CPI

---

### Pipelined Output:
```
--- Pipelined execution trace ---
Cycle 1: IF:[SET R15, 1] | ID:[-] | EX:[-] | MEM:[-] | WB:[-]
Cycle 2: IF:[SET R14, 5] | ID:[SET R15, 1] | EX:[-] | MEM:[-] | WB:[-]
Cycle 3: IF:[SET R13, 3] | ID:[SET R14, 5] | EX:[SET R15, 1] | MEM:[-] | WB:[-]
...
  (stall inserted due to load-use)
...
Cycles=150, Completed instructions=95, CPI=1.58
```

**What it shows:**
- Each cycle shows what's in EACH pipeline stage
- `-` means empty stage
- "stall inserted" means a hazard was detected
- CPI is higher because of stalls and flushes

---

## 10. Performance Comparison

### For the Traffic Interchange Program (150 cycles):

| Metric | Single-Cycle | Pipelined |
|--------|-------------|-----------|
| Cycles | 150 | 150 |
| Instructions Completed | 150 | 95 |
| CPI | 1.00 | 1.58 |
| Pipeline Stalls | N/A | 8 load-use stalls |

### Why Pipelined Has Higher CPI Here:
1. **Pipeline warm-up**: Takes 4-5 cycles to fill the pipeline initially
2. **Load-use stalls**: 8 stalls detected (LW followed by instruction using that register)
3. **Branch flushes**: When branches are taken, pipeline must be cleared
4. **Short test duration**: Only 150 cycles - overhead is more visible

### In Longer Programs:
- Single-cycle: CPI stays at 1.00
- Pipelined: CPI drops to ~1.10-1.15 (much better!)
- Pipelined can potentially run at higher clock speeds

---

## 11. Key Concepts for Demo

### Things You MUST Know:

1. **What is this project?**
   - A CPU simulator for traffic light control
   - Written in C
   - Supports single-cycle and pipelined execution

2. **What is Single-Cycle?**
   - One instruction completes fully before next starts
   - CPI is always 1.00
   - Simple but slower

3. **What is Pipelined?**
   - 5 stages: IF → ID → EX → MEM → WB
   - Multiple instructions in-flight simultaneously
   - Can have hazards (data and control)

4. **What are Pipeline Hazards?**
   - **Data hazard**: Need data that isn't ready (solved by stalling or forwarding)
   - **Control hazard**: Branch changes program flow (solved by flushing)

5. **What does the traffic program do?**
   - Controls a 4-way intersection
   - Has 5 phases (NS, EW, NS-left, EW-left, pedestrian)
   - Uses timers to switch between phases
   - Has emergency vehicle priority

6. **How to run it?**
   ```bash
   gcc -O2 -o asp_sim src/main.c
   ./asp_sim -i samples/traffic.asm -c 50 -s
   ./asp_sim -i samples/traffic.asm -c 50 -p
   ```

7. **What does CPI mean?**
   - Cycles Per Instruction
   - Lower is better
   - Single-cycle = 1.00 (ideal)
   - Pipelined = varies (1.0 to 2.0+ depending on hazards)

### Demo Flow Suggestion:

1. **Start simple**: Run `tests/arthmetic.asm` in both modes
2. **Show branching**: Run `tests/branch.asm` 
3. **Show hazards**: Run `tests/hazard.asm` - point out the stall message
4. **Show real application**: Run `samples/traffic.asm` with `-c 50`
5. **Compare CPI**: Show the statistics at the end

---

## Quick Reference Commands

```bash
# Build
gcc -O2 -o asp_sim src/main.c

# Simple test
./asp_sim -i tests/arthmetic.asm -s
./asp_sim -i tests/arthmetic.asm -p

# Hazard test (shows stalls)
./asp_sim -i tests/hazard.asm -p

# Traffic light (limited cycles)
./asp_sim -i samples/traffic.asm -c 50 -s
./asp_sim -i samples/traffic.asm -c 50 -p

# Complex traffic (limited cycles)
./asp_sim -i samples/traffic_interchange.asm -c 100 -s
./asp_sim -i samples/traffic_interchange.asm -c 100 -p
```

---

## Glossary

| Term | Simple Definition |
|------|-------------------|
| **ISA** | Instruction Set Architecture - the "language" of the CPU |
| **Register** | Small, fast storage inside CPU (R0-R15) |
| **Memory** | Larger, slower storage outside CPU |
| **PC** | Program Counter - which instruction we're on |
| **CPI** | Cycles Per Instruction - efficiency measure |
| **Pipeline** | Overlapping instruction execution like assembly line |
| **Hazard** | A problem that prevents smooth pipeline operation |
| **Stall** | Pausing the pipeline to wait for data |
| **Flush** | Clearing wrong instructions from pipeline |
| **Forwarding** | Passing data directly between stages |

---

**Good luck with your demo! 🚦**
