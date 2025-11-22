import subprocess
import os
import sys
from pathlib import Path
from colorama import Fore, Style, init

# Initialize colorama
init(autoreset=True)

BUILD_DIR = Path("../build")
ASSEMBLER_BIN = BUILD_DIR / "assembler_binary"
SIM1_BIN = BUILD_DIR / "rvss_binary"

TEST_SUITES = {
    "single_without_hazards": {
        "asm_dir": Path("./assembly_single_without_hazards"),
        "ref_dir": Path("./reference_artifacts_without_hazards"),
        "gen_file": Path("./generate_single_tests_without_hazards.py")
    },
    "single_hazards": {
        "asm_dir": Path("./assembly_single_hazards"),
        "ref_dir": Path("./reference_artifacts_hazards"),
        "gen_file": Path("./generate_single_tests_hazards.py")
    },
    "multi_2_instr": {
        "asm_dir": Path("./assembly_multi_hazard/2_instr"),
        "ref_dir": Path("./reference_artifacts_multi/2_instr"),
        "gen_file": Path("./generate_2_instr_tests.py")
    },
    "multi_gapped": {
        "asm_dir": Path("./assembly_multi_hazard/gapped"),
        "ref_dir": Path("./reference_artifacts_multi/gapped"),
        "gen_file": Path("./generate_gapped_tests.py")
    },
    "double_dep": {
        "asm_dir": Path("./assembly_multi_hazard/double_dep"),
        "ref_dir": Path("./reference_artifacts_multi/double_dep"),
        "gen_file": Path("./generate_double_dep_tests.py")
    },
    "control_data": {
        "asm_dir": Path("./assembly_multi_hazard/control_data"),
        "ref_dir": Path("./reference_artifacts_multi/control_data"),
        "gen_file": Path("./generate_control_data_tests.py")
    },
}
def process_suite(suite_name, suite_config, force=False):
    """
    Processes a single test suite, generating all its reference artifacts.
    """
    
    asm_dir = suite_config["asm_dir"]
    ref_dir = suite_config["ref_dir"]
    gen_file = suite_config["gen_file"]
    timestamp_file = ref_dir / ".gen_timestamp"
    
    # Check if test files are stale (corresponding generate file has been changed later)
    if not force and gen_file.exists() and asm_dir.exists():
        try:
            gen_mtime = gen_file.stat().st_mtime
            asm_mtime = asm_dir.stat().st_mtime
            
            if asm_mtime < gen_mtime:
                print(f"{Fore.RED}Error: Assembly files for '{suite_name}' are stale.")
                print(f"  Generator '{gen_file.name}' is newer than '{asm_dir}'.")
                print(f"{Fore.YELLOW}  Please re-run 'python3 {gen_file.name}' to update assembly files.")
                return 0, 1 # 0 success, 1 fail
        except FileNotFoundError:
            pass # A file is missing, so we must regenerate anyway

    # 1. Check if we need to regenerate
    if not force:
        try:
            # Check if source generator, this script, and asm dir are all older than the timestamp
            if timestamp_file.exists() and gen_file.exists() and asm_dir.exists():
                ref_mtime = timestamp_file.stat().st_mtime
                gen_mtime = gen_file.stat().st_mtime
                asm_mtime = asm_dir.stat().st_mtime
                self_mtime = Path(__file__).stat().st_mtime
                
                if (ref_mtime > gen_mtime and 
                    ref_mtime > asm_mtime and 
                    ref_mtime > self_mtime):
                    print(f"{Style.BRIGHT}Suite '{suite_name}' is up-to-date. Skipping.{Style.RESET_ALL}")
                    return 0, 0 # 0 success, 0 fail
        except FileNotFoundError:
            pass # A file is missing, so we must regenerate

    print(f"{Style.BRIGHT}Generating references for '{suite_name}'...{Style.RESET_ALL}")

    # Use rglob to find all .s files recursively
    asm_files = list(asm_dir.rglob("*.s"))
    if not asm_files:
        print(f"{Fore.YELLOW}No .s files found in {asm_dir}.")
        return 0, 0

    success_count = 0
    fail_count = 0

    for asm_file_path in asm_files:
        # Create a relative path to mirror the directory structure
        relative_path = asm_file_path.relative_to(asm_dir)
        test_ref_dir = ref_dir / relative_path.parent / asm_file_path.name
        
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
            
    # 3. Write timestamp file on success
    if fail_count == 0 and success_count > 0:
        os.makedirs(ref_dir, exist_ok=True)
        timestamp_file.touch()
        print(f"{Fore.GREEN}Successfully generated {success_count} artifacts for '{suite_name}'.")

    return success_count, fail_count

def main():
    print(f"{Style.BRIGHT}Starting reference artifact generation...{Style.RESET_ALL}")
    
    # Check for --force flag
    force_regenerate = "--force" in sys.argv or "-f" in sys.argv
    if force_regenerate:
        print(f"{Fore.YELLOW}Force flag detected. Regenerating all artifacts.{Style.RESET_ALL}")
    
    if not ASSEMBLER_BIN.exists() or not SIM1_BIN.exists():
        print(f"{Fore.RED}Error: Executables not found in {BUILD_DIR}.")
        print(f"Please build your C++ project first.")
        return

    total_success = 0
    total_fail = 0
    
    # Loop over all defined test suites
    for suite_name, suite_config in TEST_SUITES.items():
        if not suite_config["asm_dir"].exists():
            print(f"{Fore.YELLOW}Skipping '{suite_name}': Directory {suite_config['asm_dir']} not found.")
            print(f"{Fore.YELLOW}  Please re-run 'python3 {suite_config['gen_file'].name}' to create assembly files.")
            continue
            
        s, f = process_suite(suite_name, suite_config, force_regenerate)
        total_success += s
        total_fail += f
        print("-" * 30) # Separator
        
    print(f"\n{Style.BRIGHT}All suites processed. {total_success} total succeeded, {total_fail} total failed.{Style.RESET_ALL}")

if __name__ == "__main__":
    main()