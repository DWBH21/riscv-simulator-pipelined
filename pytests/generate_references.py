import subprocess
import os
from pathlib import Path
from colorama import Fore, Style, init

# Initialize colorama
init(autoreset=True)

BUILD_DIR = Path("../build")
ASSEMBLER_BIN = BUILD_DIR / "assembler_binary"
SIM1_BIN = BUILD_DIR / "rvss_binary"  # Base reference: single stage (mode-0)
TEST_S_DIR = Path("./assembly")
REF_DIR = Path("../reference_artifacts")

def main():
    print(f"{Style.BRIGHT}Generating reference artifacts...{Style.RESET_ALL}")
    
    if not ASSEMBLER_BIN.exists() or not SIM1_BIN.exists():
        print(f"{Fore.RED}Error: Executables not found in {BUILD_DIR}.")
        print(f"Please build your C++ project first.")
        return

    # Use rglob to find all .s files recursively in subdirectories
    asm_files = list(TEST_S_DIR.rglob("*.s"))
    if not asm_files:
        print(f"{Fore.YELLOW}No .s files found in {TEST_S_DIR}.")
        return

    success_count = 0
    fail_count = 0

    for asm_file_path in asm_files:
        # Create a relative path to mirror the directory structure
        relative_path = asm_file_path.relative_to(TEST_S_DIR)
        test_ref_dir = REF_DIR / relative_path.parent / asm_file_path.name
        
        print(f"  Processing: {relative_path}", end="... ")
        os.makedirs(test_ref_dir, exist_ok=True)
        
        memimg_out = test_ref_dir / "test.memimg"
        ref_json_out = test_ref_dir / "rvss_output.json"
        
        try:
            # 1. Assemble
            cmd_asm = [str(ASSEMBLER_BIN), str(asm_file_path), "-o", str(memimg_out)]
            subprocess.run(cmd_asm, check=True, timeout=10, capture_output=True, text=True)
            
            # 2. Run RVSS (Reference)
            cmd_rvss = [str(SIM1_BIN), str(memimg_out), "-o", str(ref_json_out)]
            subprocess.run(cmd_rvss, check=True, timeout=10, capture_output=True, text=True)
            
            print(f"{Fore.GREEN}OK")
            success_count += 1

        except subprocess.CalledProcessError as e:
            print(f"{Fore.RED}FAILED")
            print(f"    - Error: {e.stderr}")
            fail_count += 1
        except subprocess.TimeoutExpired as e:
            print(f"{Fore.RED}TIMED OUT")
            fail_count += 1
            
    print(f"\n{Style.BRIGHT}Done. {success_count} succeeded, {fail_count} failed.{Style.RESET_ALL}")

if __name__ == "__main__":
    main()