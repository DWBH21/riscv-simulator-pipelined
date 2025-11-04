import pytest
import subprocess
import os
from pathlib import Path

# Import the json comparison logic
try:
    from compare_json import compare_states
except ImportError:
    # This helps if pytest is run from the root directory
    from tests.compare_json import compare_states

BUILD_DIR = Path("../build")
SIM2_BIN = BUILD_DIR / "rv5s_binary" # Simulator under test
TEST_S_DIR = Path("./assembly_single")
REF_DIR = Path("../reference_artifacts")
RUN_DIR = Path("../test_run_outputs") # Separate dir for output

# 1. Discover all .s files and use them as test parameters
def get_asm_tests():
    """Finds all .s files in the assembly directory."""
    return list(TEST_S_DIR.rglob("*.s"))

# 2. Pytest "parametrization"
@pytest.mark.parametrize(
    "asm_file_path", 
    get_asm_tests(), 
    # --- FIX 1: Make test IDs unique and readable ---
    # This creates a name like '01_r_type_arith_add_x0_x0_x0.s'
    ids=lambda p: str(p.relative_to(TEST_S_DIR)).replace(os.sep, '_')
)
def test_simulator(asm_file_path):
    """
    Runs rv5s (rv5s_binary) on a test file and compares its output
    to the golden reference from rvss.
    """
    
    # --- FIX 2: Use the relative path to find artifacts ---
    # This creates a path like '01_r_type_arith/add_x0_x0_x0.s'
    relative_path = asm_file_path.relative_to(TEST_S_DIR)
    
    # This now correctly points to 'reference_artifacts/01_r_type_arith/add_x0_x0_x0.s'
    test_ref_dir = REF_DIR / relative_path.parent / asm_file_path.name
    
    # This now correctly points to 'test_run_outputs/01_r_type_arith/add_x0_x0_x0.s'
    test_run_dir = RUN_DIR / relative_path.parent / asm_file_path.name
    
    os.makedirs(test_run_dir, exist_ok=True)
    
    # Input files (the golden reference files)
    memimg_in = test_ref_dir / "test.memimg"
    
    # --- FIX 3: Using your filenames ---
    ref_json_in = test_ref_dir / "rvss_output.json" # Your reference filename
    
    # Output file (for this specific run)
    dut_json_out = test_run_dir / "rv5s_output.json" # Your DUT filename

    # 2. Check if reference files exist
    if not memimg_in.exists() or not ref_json_in.exists():
        pytest.skip(f"Reference files for {relative_path} not found. "
                    f"Run 'python generate_references.py' first.")
    
    if not SIM2_BIN.exists():
        pytest.fail(f"rv5s executable not found: {SIM2_BIN}. Build first.")

    # 3. Run Your Simulator (Sim2)
    try:
        cmd_rv5s = [str(SIM2_BIN), str(memimg_in), "-o", str(dut_json_out)]
        subprocess.run(
            cmd_rv5s,
            check=True, timeout=10, capture_output=True, text=True
        )
    except subprocess.CalledProcessError as e:
        pytest.fail(f"Your simulator (rv5s) CRASHED:\n{e.stderr}")
    except subprocess.TimeoutExpired:
        pytest.fail("Your simulator (rv5s) timed out.")

    # 4. Compare Outputs
    is_match, diff_message = compare_states(ref_json_in, dut_json_out)

    # 5. Assert difference
    assert is_match, diff_message