/*
 * Simple ASP simulator: single-cycle and 5-stage pipelined
 * Supports a minimal Traffic-ASP ISA described in ../isa.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_PROG 1024
#define MAX_LINE 256
#define REGS 16
#define MEM_WORDS 1024
#define MAX_PORTS 16

typedef enum { I_NOP, I_ADD, I_SUB, I_LW, I_SW, I_BEQ, I_J, I_OUT, I_SET } InstType;

typedef struct {
    InstType type;
    int rd, rs, rt;
    int imm;
    char label[64];
    char raw[128];
} Instr;

Instr prog[MAX_PROG];
int prog_len = 0;

int labels_count = 0;
typedef struct { char name[64]; int addr; } Label;
Label labels[256];

int regfile[REGS];
int memory[MEM_WORDS];

// simple output ports capture
int port_out[MAX_PORTS];

typedef struct {
    int valid;
    int pc;
    Instr ins;
} IFID;

typedef struct {
    int valid;
    int pc;
    Instr ins;
    int rs_val;
    int rt_val;
    int rd_val;
} IDEX;

typedef struct {
    int valid;
    int pc;
    Instr ins;
    int alu;
    int store_val;
} EXMEM;

typedef struct {
    int valid;
    int pc;
    Instr ins;
    int result;
} MEMWB;

static bool writes_back(const Instr *ins){ //Hazard detection and forwarding
    switch(ins->type){
        case I_ADD:
        case I_SUB:
        case I_LW:
        case I_SET:
            return true;
        default:
            return false;
    }
}

static int dest_reg(const Instr *ins){
    if (!writes_back(ins)) return -1;
    return ins->rd;
}

static bool uses_reg_source(const Instr *ins, int reg){ //Load-use hazard detection
    if (reg < 0) return false;
    switch(ins->type){
        case I_ADD:
        case I_SUB:
        case I_BEQ:
            return ins->rs == reg || ins->rt == reg;
        case I_LW:
            return ins->rs == reg;
        case I_SW:
            return ins->rs == reg || ins->rd == reg; // base in rd
        case I_OUT:
            return ins->rs == reg;
        default:
            return false;
    }
}

static void reset_state(void){ //clear the state of the pipeline
    memset(regfile,0,sizeof(regfile));
    memset(memory,0,sizeof(memory));
    memset(port_out,0,sizeof(port_out));
    regfile[0]=0;
}

static void trim(char *s){
    // trim leading/trailing whitespace
    char *p = s; while(isspace((unsigned char)*p)) p++;
    memmove(s, p, strlen(p)+1);
    for(int i=strlen(s)-1;i>=0 && isspace((unsigned char)s[i]);--i) s[i]='\0';
}

int reg_index(const char *tok){
    if (tok[0]=='R' || tok[0]=='r'){
        int v = atoi(tok+1);
        if (v<0) v=0; if (v>=REGS) v=REGS-1;
        return v;
    }
    return -1;
}

int find_label(const char *name){ //find the address of a label
    for(int i=0;i<labels_count;i++) if (strcmp(labels[i].name,name)==0) return labels[i].addr;
    return -1;
}

void add_label(const char *name,int addr){ //add a label to the list of labels
    strncpy(labels[labels_count].name,name,63);
    labels[labels_count].addr = addr;
    labels_count++;
}

void parse_file(const char *path){ //parse the file and add the labels to the list of labels
    labels_count = 0;
    FILE *f = fopen(path,"r");
    if (!f){ perror(path); exit(1);} 
    char line[MAX_LINE];
    // first pass: collect labels and raw lines
    char rawlines[MAX_PROG][MAX_LINE];
    int rawc=0;
    while(fgets(line,sizeof(line),f)){
        char copy[MAX_LINE]; strcpy(copy,line);
        trim(copy);
        if (copy[0]=='\0' || copy[0]=='#') continue;
        // label?
        char *colon = strchr(copy,':');
        if (colon){
            *colon='\0'; trim(copy);
            add_label(copy, rawc);
            char *after = colon+1; trim(after);
            if (after[0]=='\0') continue;
            strcpy(rawlines[rawc++], after);
        } else {
            strcpy(rawlines[rawc++], copy);
        }
    }
    fclose(f);
    // second pass: parse instructions
    prog_len = 0;
    for(int i=0;i<rawc;i++){
        char *s = rawlines[i];
        Instr ins; memset(&ins,0,sizeof(ins)); strcpy(ins.raw,s);
        // strip comments
        char *hash = strchr(s,'#'); if (hash) *hash='\0'; trim(s);
        if (strlen(s)==0) continue;
        // tokenize operation
        char op[32]; int p=0; while(s[p] && !isspace((unsigned char)s[p])) p++;
        strncpy(op,s,p); op[p]='\0';
        char *rest = s + p; trim(rest);
        for(char *c=op; *c; ++c) *c = toupper((unsigned char)*c);

        if (strcmp(op,"ADD")==0){ ins.type=I_ADD; int r1,r2,r3; sscanf(rest," R%d , R%d , R%d",&r1,&r2,&r3); ins.rd=r1; ins.rs=r2; ins.rt=r3; }
        else if (strcmp(op,"SUB")==0){ ins.type=I_SUB; int r1,r2,r3; sscanf(rest," R%d , R%d , R%d",&r1,&r2,&r3); ins.rd=r1; ins.rs=r2; ins.rt=r3; }
        else if (strcmp(op,"LW")==0){ ins.type=I_LW; // rd, imm(rs)
            int rd,imm,rs; if (sscanf(rest," R%d , %d ( R%d )",&rd,&imm,&rs)==3){ ins.rd=rd; ins.rs=rs; ins.imm=imm; }
        }
        else if (strcmp(op,"SW")==0){ ins.type=I_SW; int rs,imm,rd; if (sscanf(rest," R%d , %d ( R%d )",&rs,&imm,&rd)==3){ ins.rs=rs; ins.imm=imm; ins.rd=rd; }}
        else if (strcmp(op,"BEQ")==0){ ins.type=I_BEQ; char lb[64]; int r1,r2; if (sscanf(rest," R%d , R%d , %63s",&r1,&r2,lb)==3){ ins.rs=r1; ins.rt=r2; strncpy(ins.label,lb,63);} }
        else if (strcmp(op,"J")==0){ ins.type=I_J; char lb[64]; if (sscanf(rest," %63s",lb)==1) strncpy(ins.label,lb,63); }
        else if (strcmp(op,"OUT")==0){ ins.type=I_OUT; int port, r; sscanf(rest," %d , R%d",&port,&r); ins.imm=port; ins.rs=r; }
        else if (strcmp(op,"SET")==0){ ins.type=I_SET; int r,v; sscanf(rest," R%d , %d",&r,&v); ins.rd=r; ins.imm=v; }
        else if (strcmp(op,"NOP")==0){ ins.type=I_NOP; }
        else { // unknown -> treat as NOP
            ins.type=I_NOP;
        }
        prog[prog_len++] = ins;
    }
}

const char *iname(InstType t){
    switch(t){case I_ADD: return "ADD"; case I_SUB: return "SUB"; case I_LW: return "LW"; case I_SW: return "SW"; case I_BEQ: return "BEQ"; case I_J: return "J"; case I_OUT: return "OUT"; case I_SET: return "SET"; default: return "NOP"; }
}

void dump_regs(){
    for(int i=0;i<REGS;i++) printf("R%02d=%d ",i,regfile[i]);
    printf("\n");
}

void run_single(long max_cycles){
    reset_state();
    int pc=0; long cycles=0; int executed=0;
    printf("--- Single-cycle execution trace ---\n");
    while(pc < prog_len){
        Instr *ins = &prog[pc];
        printf("PC=%02d: %s\n", pc, ins->raw);
        switch(ins->type){
            case I_ADD: regfile[ins->rd] = regfile[ins->rs] + regfile[ins->rt]; pc++; break;
            case I_SUB: regfile[ins->rd] = regfile[ins->rs] - regfile[ins->rt]; pc++; break;
            case I_LW: {
                int addr = regfile[ins->rs] + ins->imm;
                int val = 0;
                if (addr % 4 == 0){
                    int idx = addr / 4;
                    if (idx >= 0 && idx < MEM_WORDS) val = memory[idx];
                }
                regfile[ins->rd] = val;
                pc++;
            } break;
            case I_SW: {
                int addr = regfile[ins->rd] + ins->imm;
                if (addr % 4 == 0){
                    int idx = addr / 4;
                    if (idx >= 0 && idx < MEM_WORDS) memory[idx] = regfile[ins->rs];
                }
                pc++;
            } break;
            case I_BEQ: { if (regfile[ins->rs] == regfile[ins->rt]){ int a = find_label(ins->label); if (a>=0) pc = a; else pc++; } else pc++; } break;
            case I_J: { int a = find_label(ins->label); if (a>=0) pc = a; else pc++; } break;
            case I_OUT: { port_out[ins->imm] = regfile[ins->rs]; printf("  OUT port %d <= %d\n", ins->imm, port_out[ins->imm]); pc++; } break;
            case I_SET: regfile[ins->rd] = ins->imm; pc++; break;
            case I_NOP: default: pc++; break;
        }
        cycles++; executed++;
        regfile[0]=0;
        if (cycles >= max_cycles){
            printf("Reached single-cycle cap (%ld cycles). Stopping early.\n", max_cycles);
            break;
        }
    }
    printf("--- Final registers ---\n"); dump_regs();
    if (executed == 0) executed = 1;
    printf("Cycles=%ld, Instructions=%d, CPI=%.2f\n", cycles, executed, (double)cycles/executed);
}

static void print_stage_instr(const Instr *if_ins, const IFID *ifid, const IDEX *idex, const EXMEM *exmem, const MEMWB *memwb){
    const char *sif = (if_ins? if_ins->raw: "-");
    const char *sid = (ifid->valid? ifid->ins.raw: "-");
    const char *sex = (idex->valid? idex->ins.raw: "-");
    const char *sm = (exmem->valid? exmem->ins.raw: "-");
    const char *swb = (memwb->valid? memwb->ins.raw: "-");
    printf("IF:[%s] | ID:[%s] | EX:[%s] | MEM:[%s] | WB:[%s]\n", sif, sid, sex, sm, swb);
}

void run_pipelined(long max_cycles){ //WB → MEM → EX → ID → IF

    reset_state();
    IFID ifid = {0};
    IDEX idex = {0};
    EXMEM exmem = {0};
    MEMWB memwb = {0};
    int pc = 0;
    long cycle = 0;
    long retired = 0;
    printf("--- Pipelined execution trace ---\n");
    while (cycle < max_cycles){
        bool empty = !ifid.valid && !idex.valid && !exmem.valid && !memwb.valid;
        if (empty && pc >= prog_len) break;

        Instr *if_fetch = (pc < prog_len)? &prog[pc] : NULL;
        printf("Cycle %ld: ", cycle+1);
        print_stage_instr(if_fetch, &ifid, &idex, &exmem, &memwb);

        IFID cur_ifid = ifid;
        IDEX cur_idex = idex;
        EXMEM cur_exmem = exmem;
        MEMWB cur_memwb = memwb;

        IFID next_ifid = cur_ifid; // default hold (stall)
        IDEX next_idex = (IDEX){0};
        EXMEM next_exmem = (EXMEM){0};
        MEMWB next_memwb = (MEMWB){0};

        bool branch_taken = false;
        int branch_target = -1;
        bool load_use_stall = false;

        // WB stage
        if (cur_memwb.valid){
            Instr *ins = &cur_memwb.ins;
            switch(ins->type){
                case I_ADD:
                case I_SUB:
                case I_SET:
                case I_LW:
                    regfile[ins->rd] = cur_memwb.result;
                    break;
                case I_OUT:
                    port_out[ins->imm] = cur_memwb.result;
                    printf("  OUT port %d <= %d\n", ins->imm, port_out[ins->imm]);
                    break;
                default:
                    break;
            }
            retired++;
            regfile[0] = 0;
        }

        // MEM stage
        if (cur_exmem.valid){
            next_memwb.valid = 1;
            next_memwb.pc = cur_exmem.pc;
            next_memwb.ins = cur_exmem.ins;
            switch(cur_exmem.ins.type){
                case I_LW: {
                    int addr = cur_exmem.alu;
                    int val = 0;
                    if (addr % 4 == 0){
                        int idx = addr / 4;
                        if (idx >= 0 && idx < MEM_WORDS) val = memory[idx];
                    }
                    next_memwb.result = val;
                } break;
                case I_SW: {
                    int addr = cur_exmem.alu;
                    if (addr % 4 == 0){
                        int idx = addr / 4;
                        if (idx >= 0 && idx < MEM_WORDS){
                            memory[idx] = cur_exmem.store_val;
                        }
                    }
                    next_memwb.result = 0;
                } break;
                default:
                    next_memwb.result = cur_exmem.alu;
                    break;
            }
        }

        // EX stage
        if (cur_idex.valid){
            next_exmem.valid = 1;
            next_exmem.pc = cur_idex.pc;
            next_exmem.ins = cur_idex.ins;
            int vrs = cur_idex.rs_val;
            int vrt = cur_idex.rt_val;
            int vrd = cur_idex.rd_val;

            if (cur_exmem.valid){
                int ex_dest = dest_reg(&cur_exmem.ins);
                if (ex_dest >= 0 && cur_exmem.ins.type != I_LW){
                    if (cur_idex.ins.rs == ex_dest) vrs = cur_exmem.alu;
                    if (cur_idex.ins.rt == ex_dest) vrt = cur_exmem.alu;
                    if (cur_idex.ins.rd == ex_dest) vrd = cur_exmem.alu;
                }
            }
            if (cur_memwb.valid){
                int wb_dest = dest_reg(&cur_memwb.ins);
                if (wb_dest >= 0){
                    if (cur_idex.ins.rs == wb_dest) vrs = cur_memwb.result;
                    if (cur_idex.ins.rt == wb_dest) vrt = cur_memwb.result;
                    if (cur_idex.ins.rd == wb_dest) vrd = cur_memwb.result;
                }
            }

            switch(cur_idex.ins.type){
                case I_ADD: next_exmem.alu = vrs + vrt; break;
                case I_SUB: next_exmem.alu = vrs - vrt; break;
                case I_SET: next_exmem.alu = cur_idex.ins.imm; break;
                case I_LW: next_exmem.alu = vrs + cur_idex.ins.imm; break;
                case I_SW:
                    next_exmem.alu = vrd + cur_idex.ins.imm;
                    next_exmem.store_val = vrs;
                    break;
                case I_OUT: next_exmem.alu = vrs; break;
                case I_BEQ:
                    next_exmem.alu = 0;
                    if (vrs == vrt){
                        int tgt = find_label(cur_idex.ins.label);
                        if (tgt >= 0){
                            branch_taken = true;
                            branch_target = tgt;
                        }
                    }
                    break;
                case I_J: {
                    int tgt = find_label(cur_idex.ins.label);
                    if (tgt >= 0){
                        branch_taken = true;
                        branch_target = tgt;
                    }
                } break;
                default:
                    next_exmem.alu = 0;
                    break;
            }
        }

        // ID stage (hazard detection + decode)
        if (branch_taken){
            next_idex.valid = 0;
        } else if (cur_ifid.valid){
            bool hazard = false;
            if (cur_idex.valid && cur_idex.ins.type == I_LW){
                int load_dest = cur_idex.ins.rd;
                if (uses_reg_source(&cur_ifid.ins, load_dest)){
                    hazard = true;
                }
            }
            if (hazard){
                load_use_stall = true;
                next_idex.valid = 0;
            } else {
                next_idex.valid = 1;
                next_idex.pc = cur_ifid.pc;
                next_idex.ins = cur_ifid.ins;
                next_idex.rs_val = regfile[cur_ifid.ins.rs];
                next_idex.rt_val = regfile[cur_ifid.ins.rt];
                next_idex.rd_val = regfile[cur_ifid.ins.rd];
            }
        }

        // IF stage
        if (branch_taken){
            next_ifid.valid = 0;
            if (branch_target >= 0){
                pc = branch_target;
            }
        } else if (load_use_stall){
            printf("  (stall inserted due to load-use)\n");
            next_ifid = cur_ifid;
        } else {
            if (pc < prog_len){
                next_ifid.valid = 1;
                next_ifid.pc = pc;
                next_ifid.ins = prog[pc];
                pc++;
            } else {
                next_ifid.valid = 0;
            }
        }

        ifid = next_ifid;
        idex = next_idex;
        exmem = next_exmem;
        memwb = next_memwb;

        cycle++;
    }
    if (cycle >= max_cycles){
        printf("Reached pipeline cap (%ld cycles). Stopping early.\n", max_cycles);
    }
    printf("--- Final registers ---\n");
    dump_regs();
    if (retired == 0) retired = 1;
    printf("Cycles=%ld, Completed instructions=%ld, CPI=%.2f\n", cycle, retired, (double)cycle/retired);
}

void usage(const char *p){ printf("Usage: %s -i <file> [-c maxCycles] -s|-p\n", p); }

int main(int argc, char **argv){
    if (argc < 3){ usage(argv[0]); return 1; }
    const char *inpath = NULL;
    int do_single = 0, do_pipe = 0;
    long max_cycles = 200;
    for(int i=1;i<argc;i++){
        if (strcmp(argv[i],"-i")==0 && i+1<argc){
            inpath = argv[++i];
        } else if (strcmp(argv[i],"-s")==0){
            do_single = 1;
        } else if (strcmp(argv[i],"-p")==0){
            do_pipe = 1;
        } else if (strcmp(argv[i],"-c")==0 && i+1<argc){
            max_cycles = strtol(argv[++i], NULL, 10);
            if (max_cycles <= 0) max_cycles = 1;
        }
    }
    if (!inpath || (!do_single && !do_pipe)){
        usage(argv[0]);
        return 1;
    }
    parse_file(inpath);
    if (do_single) run_single(max_cycles);
    if (do_pipe) run_pipelined(max_cycles);
    return 0;
}
