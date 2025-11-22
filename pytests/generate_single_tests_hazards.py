import os
import shutil

# --- Configuration ---
BASE_DIR = "./assembly_single_hazards"
REGS_R = ["x0", "x1", "x2"]
REGS_I = ["x0", "x1"]
REGS_L_RD = ["x0", "x2"]
REGS_S_RS2 = ["x0", "x2"]

# --- MODIFIED LISTS ---
# Kept zero, smallest positive, smallest negative, and boundaries
IMMEDIATES = ["0", "1", "-1", "2047", "-2048"]
# Kept zero, smallest, and max
SHIFTS = ["0", "1", "31"]
# Kept zero, smallest, and max
U_IMMEDIATES = ["0", "1", "1048575"]
# --- END MODIFICATIONS ---

# VmConfig constants2
DATA_SECTION_LUI_VAL = "0x10000"
DATA_SECTION_OFFSET = 128 

# NOPs for Hazard-Free Pipeline
NOPS_STR = """
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
"""

# Instruction Groups (Unchanged)
R_TYPE_ARITH = ["add", "sub", "xor", "or", "and", "sll", "srl", "sra", "slt", "sltu"]
I_TYPE_ARITH = ["addi", "xori", "ori", "andi", "slti", "sltiu"]
I_TYPE_SHIFT = ["slli", "srli", "srai"]
LOAD_TYPE = ["lb", "lh", "lw", "ld", "lbu", "lhu", "lwu"]
STORE_TYPE = ["sb", "sh", "sw", "sd"]
BRANCH_TYPE = ["beq", "bne", "blt", "bge", "bltu", "bgeu"]
U_TYPE = ["lui", "auipc"]
J_TYPE = ["jal", "jalr"]
# --- End Configuration ---

def format_test_file(preamble_lines, test_lines):
    output_asm = f"""
.text
"""
    
    all_lines = preamble_lines + test_lines
    for line in all_lines:
        output_asm += f"    {line}\n"
    output_asm += "\nend_program:\n"
    
    return output_asm

def generate_r_type():
    """Generates: add rd, rs1, rs2
    Preamble: Sets x1 = 10, x2 = 20
    Permutes rd, rs1, rs2 with [x0, x1, x2]
    """
    path = os.path.join(BASE_DIR, "01_r_type_arith")
    os.makedirs(path, exist_ok=True)
    count = 0
    
    preamble = [
        "addi x1, x0, 10",
        "addi x2, x0, 20"
    ]
    
    for instr in R_TYPE_ARITH:
        for rd in REGS_R:
            for rs1 in REGS_R:
                for rs2 in REGS_R:
                    test = [f"{instr} {rd}, {rs1}, {rs2}"]
                    filename = f"{instr}_{rd}_{rs1}_{rs2}.s"
                    filepath = os.path.join(path, filename)
                    with open(filepath, "w") as f:
                        f.write(format_test_file(preamble, test))
                    count += 1
    print(f"Generated {count} R-Type Arith tests.")

def generate_i_type_arith():
    """Generates: addi rd, rs1, imm
    Preamble: Sets x1 = 10
    Permutes rd, rs1 with [x0, x1]
    """
    path = os.path.join(BASE_DIR, "02_i_type_arith")
    os.makedirs(path, exist_ok=True)
    count = 0
    
    preamble = ["addi x1, x0, 10"]
    
    for instr in I_TYPE_ARITH:
        for rd in REGS_I:
            for rs1 in REGS_I:
                for imm in IMMEDIATES:
                    test = [f"{instr} {rd}, {rs1}, {imm}"]
                    filename = f"{instr}_{rd}_{rs1}_{imm}.s"
                    filepath = os.path.join(path, filename)
                    with open(filepath, "w") as f:
                        f.write(format_test_file(preamble, test))
                    count += 1
    print(f"Generated {count} I-Type (Arith) tests.")

def generate_i_type_shift():
    """Generates: slli rd, rs1, shift
    Preamble: Sets x1 = 10
    Permutes rd, rs1 with [x0, x1]
    """
    path = os.path.join(BASE_DIR, "02_i_type_shift")
    os.makedirs(path, exist_ok=True)
    count = 0
    
    preamble = ["addi x1, x0, -1"] # Use -1 for sra testing
    
    for instr in I_TYPE_SHIFT:
        for rd in REGS_I:
            for rs1 in REGS_I:
                for shift in SHIFTS:
                    test = [f"{instr} {rd}, {rs1}, {shift}"]
                    filename = f"{instr}_{rd}_{rs1}_{shift}.s"
                    filepath = os.path.join(path, filename)
                    with open(filepath, "w") as f:
                        f.write(format_test_file(preamble, test))
                    count += 1
    print(f"Generated {count} I-Type (Shift) tests.")

def generate_load_type():
    """Generates: lb rd, imm(x1)
    Preamble: Sets x1 to data section + 128
    Permutes rd with [x0, x2] (avoids x1)
    """
    path = os.path.join(BASE_DIR, "03_load_type")
    os.makedirs(path, exist_ok=True)
    count = 0
    
    preamble = [
        f"lui x1, {DATA_SECTION_LUI_VAL}",
        f"addi x1, x1, {DATA_SECTION_OFFSET}"
    ]
    rs1 = "x1"
    
    for instr in LOAD_TYPE:
        for rd in REGS_L_RD:
            for imm in IMMEDIATES:
                test = [f"{instr} {rd}, {imm}({rs1})"]
                filename = f"{instr}_{rd}_{imm}_{rs1}.s"
                filepath = os.path.join(path, filename)
                with open(filepath, "w") as f:
                    f.write(format_test_file(preamble, test))
                count += 1
    print(f"Generated {count} Load-Type tests.")

def generate_store_type():
    """Generates: sb rs2, imm(x1)
    Preamble: Sets x1 to data section + 128, sets x2 = 99
    Permutes rs2 with [x0, x2] (avoids x1)
    """
    path = os.path.join(BASE_DIR, "04_store_type")
    os.makedirs(path, exist_ok=True)
    count = 0
    
    preamble = [
        f"lui x1, {DATA_SECTION_LUI_VAL}",
        f"addi x1, x1, {DATA_SECTION_OFFSET}",
        "addi x2, x0, 99" # Value to store
    ]
    rs1 = "x1"
    
    for instr in STORE_TYPE:
        for rs2 in REGS_S_RS2:
            for imm in IMMEDIATES:
                test = [f"{instr} {rs2}, {imm}({rs1})"]
                filename = f"{instr}_{rs2}_{imm}_{rs1}.s"
                filepath = os.path.join(path, filename)
                with open(filepath, "w") as f:
                    f.write(format_test_file(preamble, test))
                count += 1
    print(f"Generated {count} Store-Type tests.")

def generate_branch_type():
    """Generates 3 cases for each branch: <, ==, >
    Preamble: Sets x1 and x2 to create the condition
    Test: Branch, sets x6=1 if not taken, x5=1 if taken
    """
    path = os.path.join(BASE_DIR, "05_branch_type")
    os.makedirs(path, exist_ok=True)
    count = 0
    
    cases = {
        "less_than": ["addi x1, x0, 5", "addi x2, x0, 10"], # x1 < x2
        "equal":     ["addi x1, x0, 10", "addi x2, x0, 10"], # x1 == x2
        "greater_than": ["addi x1, x0, 10", "addi x2, x0, 5"]  # x1 > x2
    }
    
    for instr in BRANCH_TYPE:
        for case_name, preamble in cases.items():
            
            test_lines = [
                f"{instr} x1, x2, label_taken",
                "addi x6, x0, 1",        # Not-taken path: set x6
                "jal x0, end_program",   # Jump to end
                "label_taken:",
                "addi x5, x0, 1",        # Taken path: set x5
            ]
            
            filename = f"{instr}_{case_name}.s"
            filepath = os.path.join(path, filename)
            with open(filepath, "w") as f:
                f.write(format_test_file(preamble, test_lines))
            count += 1
    print(f"Generated {count} Branch-Type tests.")

def generate_u_type():
    """Generates: lui rd, imm
    Preamble: None
    Permutes rd with [x0, x1]
    """
    path = os.path.join(BASE_DIR, "06_u_type")
    os.makedirs(path, exist_ok=True)
    count = 0
    
    preamble = []
    
    for instr in U_TYPE:
        for rd in REGS_I:
            for imm in U_IMMEDIATES:
                test = [f"{instr} {rd}, {imm}"]
                filename = f"{instr}_{rd}_{imm}.s"
                filepath = os.path.join(path, filename)
                with open(filepath, "w") as f:
                    f.write(format_test_file(preamble, test))
                count += 1
    print(f"Generated {count} U-Type tests.")

def generate_j_type():
    """Generates: jal, jalr
    jalr: Preamble to set x1
    """
    path = os.path.join(BASE_DIR, "07_j_type")
    os.makedirs(path, exist_ok=True)
    count = 0
    
    # --- JAL ---
    preamble_jal = []
    for rd in REGS_I:
        test_jal = [
            f"jal {rd}, label_jump",
            "addi x5, x0, 1",
            "jal x0, end_program",
            "label_jump:",
            "addi x6, x0, 1"
        ]
        filename = f"jal_{rd}.s"
        filepath = os.path.join(path, filename)
        with open(filepath, "w") as f:
            f.write(format_test_file(preamble_jal, test_jal))
        count += 1

    # --- JALR ---
    preamble_jalr = [
        f"lui x1, {DATA_SECTION_LUI_VAL}",
        f"addi x1, x1, {DATA_SECTION_OFFSET}"
    ]
    rs1 = "x1"
    
    for rd in REGS_I:
        for imm in IMMEDIATES:
            test_jalr = [f"jalr {rd}, {imm}({rs1})"]
            filename = f"jalr_{rd}_{imm}_{rs1}.s"
            filepath = os.path.join(path, filename)
            with open(filepath, "w") as f:
                f.write(format_test_file(preamble_jalr, test_jalr))
            count += 1
    print(f"Generated {count} J-Type tests.")

def main():
    if os.path.exists(BASE_DIR):
        os.makedirs(BASE_DIR, exist_ok=True)

    if os.path.exists(BASE_DIR):
        print(f"Cleaning up old tests in {BASE_DIR}...")
        shutil.rmtree(BASE_DIR)

    generate_r_type()
    generate_i_type_arith()
    generate_i_type_shift()
    generate_load_type()
    generate_store_type()
    generate_branch_type()
    generate_u_type()
    generate_j_type()
    print("\nAll hazard-free test assembly files generated.")

    # Update the directory's modification time
    os.utime(BASE_DIR, None)

if __name__ == "__main__":
    main()