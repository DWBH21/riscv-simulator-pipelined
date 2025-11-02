import os

BASE_DIR = "assembly"
REGS = ["x0", "x1"]         # Special Tests for x0, x1 represents any gpr register: x1-x31
IMMEDIATES = ["0", "1", "10", "-1", "2047", "-2048"] # Test zero, positive, negative, max/min
SHIFTS = ["0", "1", "5", "31"] # Test zero, small, large shifts

DATA_SECTION_BASE_HEX = "0x10000000"
DATA_SECTION_LUI_VAL = "0x10000"
DATA_SECTION_OFFSET = 128 


# Instruction Groups
R_TYPE_ARITH = ["add", "sub", "xor", "or", "and", "sll", "srl", "sra", "slt", "sltu"]
I_TYPE_ARITH = ["addi", "xori", "ori", "andi", "slti", "sltiu"]
I_TYPE_SHIFT = ["slli", "srli", "srai"]
LOAD_TYPE = ["lb", "lh", "lw", "ld", "lbu", "lhu", "lwu"]
STORE_TYPE = ["sb", "sh", "sw", "sd"]
BRANCH_TYPE = ["beq", "bne", "blt", "bge", "bltu", "bgeu"]
U_TYPE = ["lui", "auipc"]
J_TYPE = ["jal", "jalr"]


def format_test_file(instruction_line, instr_type="default"):
    """
    Wraps a single instruction in a valid .s file format.
    If type is 'load', 'store', or 'jalr', adds a preamble to setup x1.
    """
    setup = ""
    if instr_type in ["load", "store", "jalr"]:
        # Setup x1 to point to the middle of our testable data section
        # so that 'imm(x1)' accesses valid, predictable memory.
        setup = f"""
    # --- Test Preamble ---
    lui x1, {DATA_SECTION_LUI_VAL}  # Load 0x10000000 into x1
    addi x1, x1, {DATA_SECTION_OFFSET} # x1 = 0x10000080
    # --- End Preamble ---
"""

    return f"""
.text
    {setup}

    # Your NOPs (these are fine)
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0

    # Test instruction
    {instruction_line}
    
    # End of program
"""

def generate_r_type():
    """Generates: add rd, rs1, rs2"""
    path = os.path.join(BASE_DIR, "01_r_type_arith")
    os.makedirs(path, exist_ok=True)
    count = 0
    for instr in R_TYPE_ARITH:
        for rd in REGS:
            for rs1 in REGS:
                for rs2 in REGS:
                    line = f"{instr} {rd}, {rs1}, {rs2}"
                    filename = f"{instr}_{rd}_{rs1}_{rs2}.s"
                    filepath = os.path.join(path, filename)
                    with open(filepath, "w") as f:
                        f.write(format_test_file(line))
                    count += 1
    print(f"Generated {count} R-Type Arith tests.")

def generate_i_type_arith():
    """Generates: addi rd, rs1, imm"""
    path = os.path.join(BASE_DIR, "02_i_type_arith")
    os.makedirs(path, exist_ok=True)
    count = 0
    for instr in I_TYPE_ARITH:
        for rd in REGS:
            for rs1 in REGS:
                for imm in IMMEDIATES:
                    line = f"{instr} {rd}, {rs1}, {imm}"
                    filename = f"{instr}_{rd}_{rs1}_{imm}.s"
                    filepath = os.path.join(path, filename)
                    with open(filepath, "w") as f:
                        f.write(format_test_file(line))
                    count += 1
    print(f"Generated {count} I-Type (Arith) tests.")

def generate_i_type_shift():
    """Generates: slli rd, rs1, shift"""
    path = os.path.join(BASE_DIR, "02_i_type_shift")
    os.makedirs(path, exist_ok=True)
    count = 0
    for instr in I_TYPE_SHIFT:
        for rd in REGS:
            for rs1 in REGS:
                for shift in SHIFTS:
                    line = f"{instr} {rd}, {rs1}, {shift}"
                    filename = f"{instr}_{rd}_{rs1}_{shift}.s"
                    filepath = os.path.join(path, filename)
                    with open(filepath, "w") as f:
                        f.write(format_test_file(line))
                    count += 1
    print(f"Generated {count} I-Type (Shift) tests.")

def generate_load_type():
    """Generates: lb rd, imm(x1)"""
    path = os.path.join(BASE_DIR, "03_load_type")
    os.makedirs(path, exist_ok=True)
    count = 0
    rs1 = "x1"  # x1 is set as base register
    for instr in LOAD_TYPE:
        for rd in REGS:
            for imm in IMMEDIATES:
                line = f"{instr} {rd}, {imm}({rs1})"
                filename = f"{instr}_{rd}_{imm}_{rs1}.s"
                filepath = os.path.join(path, filename)
                with open(filepath, "w") as f:
                    f.write(format_test_file(line, instr_type="load"))
                count += 1
    print(f"Generated {count} Load-Type tests.")

def generate_store_type():
    """Generates: sb rs2, imm(x1)"""
    path = os.path.join(BASE_DIR, "04_store_type")
    os.makedirs(path, exist_ok=True)
    count = 0
    rs1 = "x1"  # x1 is set as base register
    for instr in STORE_TYPE:
        for rs2 in REGS:
            for imm in IMMEDIATES:
                line = f"{instr} {rs2}, {imm}({rs1})"
                filename = f"{instr}_{rs2}_{imm}_{rs1}.s"
                filepath = os.path.join(path, filename)
                with open(filepath, "w") as f:
                    f.write(format_test_file(line, instr_type="store"))
                count += 1
    print(f"Generated {count} Store-Type tests.")

def generate_branch_type():
    """Generates: beq rs1, rs2, label"""
    path = os.path.join(BASE_DIR, "05_branch_type")
    os.makedirs(path, exist_ok=True)
    count = 0
    for instr in BRANCH_TYPE:
        for rs1 in REGS:
            for rs2 in REGS:
                line = f"{instr} {rs1}, {rs2}, end_label\nend_label:"
                filename = f"{instr}_{rs1}_{rs2}.s"
                filepath = os.path.join(path, filename)
                with open(filepath, "w") as f:
                    f.write(format_test_file(line))
                count += 1
    print(f"Generated {count} Branch-Type tests.")

def generate_u_type():
    """Generates: lui rd, imm"""
    path = os.path.join(BASE_DIR, "06_u_type")
    os.makedirs(path, exist_ok=True)
    count = 0
    for instr in U_TYPE:
        for rd in REGS:
            for imm in ["0", "1", "1024", "1048575"]: # Max 20-bit
                line = f"{instr} {rd}, {imm}"
                filename = f"{instr}_{rd}_{imm}.s"
                filepath = os.path.join(path, filename)
                with open(filepath, "w") as f:
                    f.write(format_test_file(line))
                count += 1
    print(f"Generated {count} U-Type tests.")

def generate_j_type():
    """Generates: jal rd, label
       Generates: jalr rd, imm(x1)"""
    path = os.path.join(BASE_DIR, "07_j_type")
    os.makedirs(path, exist_ok=True)
    count = 0
    
    # JAL
    for rd in REGS:
        line = f"jal {rd}, end_label\nend_label:"
        filename = f"jal_{rd}.s"
        filepath = os.path.join(path, filename)
        with open(filepath, "w") as f:
            f.write(format_test_file(line))
        count += 1

    # JALR
    rs1 = "x1"  # x1 is set as base register
    for rd in REGS:
        for imm in IMMEDIATES:
            line = f"jalr {rd}, {imm}({rs1})"
            filename = f"jalr_{rd}_{imm}_{rs1}.s"
            filepath = os.path.join(path, filename)
            with open(filepath, "w") as f:
                f.write(format_test_file(line, instr_type="jalr"))
            count += 1
    print(f"Generated {count} J-Type tests.")

def main():
    if os.path.exists(BASE_DIR):
        print(f"Warning: Directory '{BASE_DIR}' already exists.")
        print("For a clean test, please delete 'tests/assembly' and 'reference_artifacts' first.")
    
    generate_r_type()
    generate_i_type_arith()
    generate_i_type_shift()
    generate_load_type()
    generate_store_type()
    generate_branch_type()
    generate_u_type()
    generate_j_type()
    print("\nAll test assembly files generated.")

if __name__ == "__main__":
    main()