import os
import shutil

# Configuration
BASE_DIR = "assembly_multi_hazard"
HAZARD_DIR = os.path.join(BASE_DIR, "control_data")

# Dependent registers 
DEP_REGS = ["x0", "x1"]

# Test Registers
TEST_REG = "x30"   # 1 = Success, 2 = Failure
FLUSH_REG = "x29"  # Should remain 0. If 1, flush failed.

REG_VAL_10 = "x1"
REG_VAL_20 = "x2"
REG_COPY_10 = "x3"
REG_ADDR_LABEL = "x5"

OTHER_REG_BASE = "x4"
DATA_SECTION_LUI_VAL = "0x10000"
DATA_SECTION_OFFSET = 128

PRODUCERS = ["add", "addi", "lw", "lui", "jal"]

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

def get_producer_line(instr, rd):
    if instr == "add": return f"add {rd}, {REG_COPY_10}, x0" 
    if instr == "addi": return f"addi {rd}, x0, 10"          
    if instr == "lw": return f"lw {rd}, 0({OTHER_REG_BASE})" 
    if instr == "lui": return f"lui {rd}, 0x1"          
    if instr == "jal": return f"jal {rd}, jal_jump"
    return ""

def generate_branch_taken_hazards():
    """
    Tests: Producer -> BEQ (Taken).
    Verifies forwarding to ID stage for comparison.
    """
    pair_dir = os.path.join(HAZARD_DIR, "branch_taken")
    os.makedirs(pair_dir, exist_ok=True)

    # Preamble
    preamble = [
        f"addi {REG_VAL_10}, x0, 10",
        f"addi {REG_VAL_20}, x0, 20",
        f"addi {REG_COPY_10}, x0, 10",
        f"lui {OTHER_REG_BASE}, {DATA_SECTION_LUI_VAL}",
        f"addi {OTHER_REG_BASE}, {OTHER_REG_BASE}, {DATA_SECTION_OFFSET}",
        f"sw {REG_VAL_10}, 0({OTHER_REG_BASE})",
        f"addi {TEST_REG}, x0, 0",
        f"addi {FLUSH_REG}, x0, 0"
    ]
    count = 0
    for p_instr in PRODUCERS:
        for p_rd in DEP_REGS:
            p_line = get_producer_line(p_instr, p_rd)
            
            # For taken tests, we need equality
            cmp_reg = "x0" if p_rd == "x0" else REG_COPY_10
            
            # Special case for lui -> just test self-equality
            if p_instr == "lui": cmp_reg = p_rd 
            # Special case for jal -> just test self-equality
            if p_instr == "jal": cmp_reg = p_rd

            # Case 1: rs1 dependency
            test_lines = []
            if p_instr == "jal":
                test_lines.append(p_line)
                test_lines.append(f"addi {FLUSH_REG}, x0, 1 # Should be flushed")
                test_lines.append("jal_jump:")
            else:
                test_lines.append(p_line)

            test_lines.extend([
                f"beq {p_rd}, {cmp_reg}, label_pass_rs1",
                f"addi {TEST_REG}, x0, 2", # Fail
                "jal x0, end_program",
                "label_pass_rs1:",
                f"addi {TEST_REG}, x0, 1"  # Pass
            ])
            count += 1
            with open(os.path.join(pair_dir, f"{p_instr}_{p_rd}__beq_taken_rs1.s"), "w") as f:
                f.write(format_test_file(preamble, test_lines))

            # Case 2: rs2 dependency
            test_lines = []
            if p_instr == "jal":
                test_lines.append(p_line)
                test_lines.append(f"addi {FLUSH_REG}, x0, 1 # Should be flushed")
                test_lines.append("jal_jump:")
            else:
                test_lines.append(p_line)

            test_lines.extend([
                f"beq {cmp_reg}, {p_rd}, label_pass_rs2",
                f"addi {TEST_REG}, x0, 2", # Fail
                "jal x0, end_program",
                "label_pass_rs2:",
                f"addi {TEST_REG}, x0, 1"  # Pass
            ])
            count += 1
            with open(os.path.join(pair_dir, f"{p_instr}_{p_rd}__beq_taken_rs2.s"), "w") as f:
                f.write(format_test_file(preamble, test_lines))

    return count

def generate_branch_not_taken_hazards():
    """
    Tests: Producer -> BEQ (Not Taken).
    Verifies forwarding allows branch to correctly fail.
    """
    pair_dir = os.path.join(HAZARD_DIR, "branch_not_taken")
    os.makedirs(pair_dir, exist_ok=True)

    preamble = [
        f"addi {REG_VAL_10}, x0, 10",
        f"addi {REG_VAL_20}, x0, 20",
        f"lui {OTHER_REG_BASE}, {DATA_SECTION_LUI_VAL}",
        f"addi {OTHER_REG_BASE}, {OTHER_REG_BASE}, {DATA_SECTION_OFFSET}",
        f"sw {REG_VAL_10}, 0({OTHER_REG_BASE})",
        f"addi {TEST_REG}, x0, 0",
        f"addi {FLUSH_REG}, x0, 0"
    ]
    count = 0
    for p_instr in PRODUCERS:
        for p_rd in DEP_REGS:
            if p_instr == "jal": continue

            p_line = get_producer_line(p_instr, p_rd)
            
            # Making sure not taken case occurs (inequality)
            cmp_reg = REG_VAL_20
            
            test_lines = [
                p_line,
                f"beq {p_rd}, {cmp_reg}, label_fail",
                f"addi {TEST_REG}, x0, 1", # Pass
                "jal x0, end_program",
                "label_fail:",
                f"addi {TEST_REG}, x0, 2"  # Fail
            ]
            count += 1
            with open(os.path.join(pair_dir, f"{p_instr}_{p_rd}__beq_not_taken_rs1.s"), "w") as f:
                f.write(format_test_file(preamble, test_lines))

    return count

def generate_jalr_hazards():
    """
    Tests: Producer -> JALR.
    Verifies if data hazards for jalr are correctly handled
    """
    pair_dir = os.path.join(HAZARD_DIR, "jalr_addr")
    os.makedirs(pair_dir, exist_ok=True)

    preamble = [
        f"auipc {REG_ADDR_LABEL}, 0",           # x5 = PC of this instruction
        f"addi {REG_ADDR_LABEL}, {REG_ADDR_LABEL}, 32", # x5 = PC + 32 (Address of target)
        f"addi {TEST_REG}, x0, 0",
        f"addi {FLUSH_REG}, x0, 0"
    ]
    
    count = 0
    p_rd = "x1"
    p_line = f"add {p_rd}, {REG_ADDR_LABEL}, x0"
    
    test_lines = [
        p_line,
        f"jalr x0, 0({p_rd})",      # Jumps to address in p_rd (which is label_jalr_target)
        f"addi {TEST_REG}, x0, 2",  # Fail
        "jal x0, end_program",
        "label_jalr_target:",
        f"addi {TEST_REG}, x0, 1"   # Pass
    ]
    count += 1
    with open(os.path.join(pair_dir, f"add_{p_rd}__jalr.s"), "w") as f:
        f.write(format_test_file(preamble, test_lines))

    return count

def generate_hazard_across_branch():
    """
    Tests: Producer -> Branch(Taken) -> ... -> Consumer.
    Verifies forwarding works across a pipeline flush.
    """
    pair_dir = os.path.join(HAZARD_DIR, "across_branch")
    os.makedirs(pair_dir, exist_ok=True)

    preamble = [
        f"addi {TEST_REG}, x0, 0",
        f"addi {FLUSH_REG}, x0, 0"
    ]
    count = 0
    for p_instr in PRODUCERS:

        p_line = ""
        if p_instr == "add": p_line = "addi x1, x0, 10"
        elif p_instr == "addi": p_line = "addi x1, x0, 10"
        elif p_instr == "lw": continue
        elif p_instr == "lui": p_line = "lui x1, 1" # 0x1000
        elif p_instr == "jal": continue

        expected_val = "10"
        if p_instr == "lui": expected_val = "0x1000"

        test_lines = [
            p_line,
            "beq x0, x0, label_target",      # Always taken
            f"addi {FLUSH_REG}, x0, 1",      # Should be flushed
            "label_target:",
            "add x3, x1, x0"
        ]
        count += 1
        with open(os.path.join(pair_dir, f"{p_instr}_across_branch.s"), "w") as f:
            f.write(format_test_file(preamble, test_lines))

    return count

def main():
    if not os.path.exists(BASE_DIR):
        os.makedirs(BASE_DIR, exist_ok=True)
    
    if os.path.exists(HAZARD_DIR):
        print(f"Cleaning up old tests in {HAZARD_DIR}...")
        shutil.rmtree(HAZARD_DIR)
        
    tot_count = 0
    tot_count += generate_branch_taken_hazards()
    tot_count += generate_branch_not_taken_hazards()
    tot_count += generate_jalr_hazards()
    tot_count += generate_hazard_across_branch()
    
    print(f"Generated {tot_count} total control+data hazard tests.")

    # Update the directory's modification time
    os.utime(HAZARD_DIR, None)
    
    print("\nAll control-data hazard tests generated.")

if __name__ == "__main__":
    main()