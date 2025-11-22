import os
import shutil 

# Configuration
BASE_DIR = "assembly_multi_hazard"
HAZARD_DIR = os.path.join(BASE_DIR, "gapped")

# Dependent registers
DEP_REGS = ["x0", "x1"]

# Non Dependent Registers
OTHER_REGS = ["x0", "x2"] 

# Other registers
OTHER_REG_RD = "x3"
OTHER_REG_BASE = "x4"
NEUTRAL_REG = "x5"

# Register to test for correct jal jump
JUMP_TEST_REG = "x29"

# VmConfig constants
DATA_SECTION_LUI_VAL = "0x10000"
DATA_SECTION_OFFSET = 128 

PRODUCERS = ["add", "addi", "lw", "lui", "jal"]
CONSUMERS = ["add", "addi", "lw", "sw"]

def format_test_file(preamble_lines, test_lines):

    output_asm = ".text\n"
    
    for line in preamble_lines:
        output_asm += f"    {line}\n"
        
    output_asm += "\n    # Test Instructions\n"
    
    for line in test_lines:
        if line.strip().endswith(':'):
            output_asm += f"{line}\n"
        else:
            output_asm += f"    {line}\n"

    output_asm += "\nend_program:\n"
    return output_asm

def generate_gapped_hazards():
    count = 0
    os.makedirs(HAZARD_DIR, exist_ok=True)
    
    # Preamble to initialize registers
    preamble = [
        "addi x1, x0, 10",
        "addi x2, x0, 20",
        f"lui {OTHER_REG_BASE}, {DATA_SECTION_LUI_VAL}",
        f"addi {OTHER_REG_BASE}, {OTHER_REG_BASE}, {DATA_SECTION_OFFSET}",
        f"sw x1, 0({OTHER_REG_BASE})", 
        f"addi {JUMP_TEST_REG}, x0, 0",
        f"addi {NEUTRAL_REG}, x0, 0"
    ]
    n_line = f"addi {NEUTRAL_REG}, {NEUTRAL_REG}, 1"
    for producer_instr in PRODUCERS:
        for consumer_instr in CONSUMERS:
            
            pair_name = f"{producer_instr}_{consumer_instr}"
            pair_dir = os.path.join(HAZARD_DIR, pair_name)
            os.makedirs(pair_dir, exist_ok=True)
            pair_count = 0

            for p_rd in DEP_REGS:        
                for c_rs_other in OTHER_REGS: 
                
                    # Generate the Producer Instruction
                    p_line = ""
                    if producer_instr == "add":
                        p_line = f"add {p_rd}, x1, x2"
                    elif producer_instr == "addi":
                        p_line = f"addi {p_rd}, x1, 5"
                    elif producer_instr == "lw":
                        p_line = f"lw {p_rd}, 0({OTHER_REG_BASE})"
                    elif producer_instr == "lui":
                        p_line = f"lui {p_rd}, 0xABC"
                    elif producer_instr == "jal":
                        p_line = f"jal {p_rd}, jal_jump"
                    
                    # Generate the Consumer Instruction
                    
                    # Case 1: rs1 Dependency
                    c_line = None
                    if consumer_instr == "add":
                        c_line = f"{consumer_instr} {OTHER_REG_RD}, {p_rd}, {c_rs_other}"
                    elif consumer_instr == "addi":
                        c_line = f"addi {OTHER_REG_RD}, {p_rd}, 5" 
                    elif consumer_instr == "lw":
                        c_line = f"{consumer_instr} {OTHER_REG_RD}, 0({p_rd})"
                    elif consumer_instr == "sw":
                        c_line = f"sw {c_rs_other}, 0({p_rd})"
                    
                    if c_line:
                        test_lines = [p_line, n_line, c_line]
                        if producer_instr == "jal": 
                            test_lines = [p_line, f"addi {JUMP_TEST_REG}, x0, 1  # Should be Flushed", "jal_jump:", n_line, c_line]
                        
                        filename_suffix = "rs1.s"
                        if consumer_instr in ["add", "sw"]:
                             filename_suffix = f"rs1_other_{c_rs_other}.s"
                        
                        filename = f"{producer_instr}_{p_rd}__{consumer_instr}_{filename_suffix}"
                        filepath = os.path.join(pair_dir, filename)
                        with open(filepath, "w") as f: f.write(format_test_file(preamble, test_lines))
                        pair_count += 1
                    
                    # Case 2: rs2 Dependency
                    c_line = None
                    if consumer_instr == "add":
                        c_line = f"{consumer_instr} {OTHER_REG_RD}, {c_rs_other}, {p_rd}"
                    elif consumer_instr == "sw":
                        c_line = f"sw {p_rd}, 0({OTHER_REG_BASE})"
                    
                    if c_line:
                        test_lines = [p_line, n_line, c_line]
                        if producer_instr == "jal": 
                            test_lines = [p_line, f"addi {JUMP_TEST_REG}, x0, 1  # Should be Flushed", "jal_jump:", n_line, c_line]
                        
                        filename_suffix = "rs2.s"
                        if consumer_instr == "add":
                             filename_suffix = f"rs2_other_{c_rs_other}.s"

                        filename = f"{producer_instr}_{p_rd}__{consumer_instr}_{filename_suffix}"
                        filepath = os.path.join(pair_dir, filename)
                        with open(filepath, "w") as f: f.write(format_test_file(preamble, test_lines))
                        pair_count += 1

                    # Case 3: rs1 and rs2 Dependency (Same Register)
                    c_line = None
                    if consumer_instr == "add":
                        c_line = f"{consumer_instr} {OTHER_REG_RD}, {p_rd}, {p_rd}"
                    
                    if c_line:
                        test_lines = [p_line, n_line, c_line]
                        if producer_instr == "jal": 
                            test_lines = [p_line, f"addi {JUMP_TEST_REG}, x0, 1  # Should be Flushed", "jal_jump:", n_line, c_line]
                        
                        filename = f"{producer_instr}_{p_rd}__{consumer_instr}_rs1_rs2.s"
                        filepath = os.path.join(pair_dir, filename)
                        with open(filepath, "w") as f: f.write(format_test_file(preamble, test_lines))
                        pair_count += 1
                        
            print(f"Generated {pair_count} {pair_name} tests.")
            count += pair_count
    print(f"Generated {count} total 2-instruction hazard tests.")

def main():
    if not os.path.exists(BASE_DIR):
        os.makedirs(BASE_DIR, exist_ok=True)
    
    if os.path.exists(HAZARD_DIR):
        print(f"Cleaning up old tests in {HAZARD_DIR}...")
        shutil.rmtree(HAZARD_DIR)

    generate_gapped_hazards()
    print("\nAll 3-instruction gapped hazard tests generated.")

    # Update the directory's modification time
    os.utime(HAZARD_DIR, None)

if __name__ == "__main__":
    main()