import os
import shutil

# Configuration
BASE_DIR = "assembly_multi_hazard"
HAZARD_DIR = os.path.join(BASE_DIR, "double_dep")

# Dependency registers
REG_1 = "x1"
REG_2 = "x2" 

# Other registers
OTHER_REG_RD = "x3"
OTHER_REG_BASE = "x4"

# Register to test for correct jal jump
JUMP_TEST_REG = "x29" 

# VmConfig constants
DATA_SECTION_LUI_VAL = "0x10000"
DATA_SECTION_OFFSET = 128 

PRODUCERS = ["add", "addi", "lw", "lui", "jal"]
CONSUMERS = ["add", "sw"]       # only instructions that have two source registers

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

def generate_double_dep_hazards():
    count = 0
    os.makedirs(HAZARD_DIR, exist_ok=True)
    
    # Preamble to initialize registers
    preamble = [
        "addi x1, x0, 10",
        "addi x2, x0, 20",
        f"lui {OTHER_REG_BASE}, {DATA_SECTION_LUI_VAL}",
        f"addi {OTHER_REG_BASE}, {OTHER_REG_BASE}, {DATA_SECTION_OFFSET}",
        f"sw x1, 0({OTHER_REG_BASE})", 
        f"addi {JUMP_TEST_REG}, x0, 0"
    ]

    for producer_instr_1 in PRODUCERS:
        for producer_instr_2 in PRODUCERS:
            for consumer_instr in CONSUMERS:
            
                pair_name = f"{producer_instr_1}_{producer_instr_2}_on_{consumer_instr}"
                pair_dir = os.path.join(HAZARD_DIR, pair_name)
                os.makedirs(pair_dir, exist_ok=True)
                
                # First Producer Instruction
                p1_lines = []
                if producer_instr_1 == "add":
                    p1_lines = [f"add {REG_1}, {OTHER_REG_BASE}, {OTHER_REG_BASE}"]
                elif producer_instr_1 == "addi":
                    p1_lines = [f"addi {REG_1}, x0, 15"]
                elif producer_instr_1 == "lw":
                    p1_lines = [f"lw {REG_1}, 0({OTHER_REG_BASE})"]
                elif producer_instr_1 == "lui":
                    p1_lines = [f"lui {REG_1}, 0xAAA"]
                elif producer_instr_1 == "jal":
                    p1_lines = [f"jal {REG_1}, label_p1", f"addi {JUMP_TEST_REG}, x0, 1 # Flushed", "label_p1:"]

                # Second Producer Instruction
                p2_lines = []
                if producer_instr_2 == "add":
                    p2_lines = [f"add {REG_2}, {OTHER_REG_BASE}, {OTHER_REG_BASE}"]
                elif producer_instr_2 == "addi":
                    p2_lines = [f"addi {REG_2}, x0, 25"]
                elif producer_instr_2 == "lw":
                    p2_lines = [f"lw {REG_2}, 0({OTHER_REG_BASE})"]
                elif producer_instr_2 == "lui":
                    p2_lines = [f"lui {REG_2}, 0xBBB"]
                elif producer_instr_2 == "jal":
                    p2_lines = [f"jal {REG_2}, label_p2", f"addi {JUMP_TEST_REG}, x0, 2 # Flushed", "label_p2:"]

                # Consumer Instruction
                # Case 1: rs1=x1, rs2=x2
                c_line_a = ""
                if consumer_instr == "add":
                    c_line_a = f"add {OTHER_REG_RD}, {REG_1}, {REG_2}"
                elif consumer_instr == "sw":
                    c_line_a = f"sw {REG_2}, 0({REG_1})"

                test_lines = p1_lines + p2_lines + [c_line_a]
                
                filename = f"p1_{producer_instr_1}_p2_{producer_instr_2}__{consumer_instr}_x1_x2.s"
                filepath = os.path.join(pair_dir, filename)
                with open(filepath, "w") as f: f.write(format_test_file(preamble, test_lines))
                count += 1

                # Case 2: rs1=x2, rs2=x1
                c_line_b = ""
                if consumer_instr == "add":
                    c_line_b = f"add {OTHER_REG_RD}, {REG_2}, {REG_1}"
                elif consumer_instr == "sw":
                    c_line_b = f"sw {REG_1}, 0({REG_2})"
                
                test_lines = p1_lines + p2_lines + [c_line_b]

                filename = f"p1_{producer_instr_1}_p2_{producer_instr_2}__{consumer_instr}_x2_x1.s"
                filepath = os.path.join(pair_dir, filename)
                with open(filepath, "w") as f: f.write(format_test_file(preamble, test_lines))
                count += 1
                
    print(f"Generated {count} total double dependency tests.")


def main():
    if not os.path.exists(BASE_DIR):
        os.makedirs(BASE_DIR, exist_ok=True)
    
    if os.path.exists(HAZARD_DIR):
        print(f"Cleaning up old tests in {HAZARD_DIR}...")
        shutil.rmtree(HAZARD_DIR)

    generate_double_dep_hazards()
    print("\nAll 3-instruction tests with double dependency hazards generated.")

    # Update the directory's modification time
    os.utime(HAZARD_DIR, None)

if __name__ == "__main__":
    main()