import pytest
import subprocess
import os
import sys
from pathlib import Path

TIME_OUT_WAIT = 3
try:
    from compare_json import compare_states
except ImportError:
    try:
        from tests.compare_json import compare_states
    except ImportError:
        sys.exit("Error: Could not import 'compare_states' from 'compare_json.py'.")

BUILD_DIR = Path("../build") 
SIM2_BIN = BUILD_DIR / "rv5s_binary" # The 5-stage Simulator under test
RUN_DIR_BASE = Path("./test_run_outputs")

SIMULATOR_CONFIGS = [
    {
        "id": "5Stage_Ideal",
        "flags": [
            "--config", "Execution", "processor_type", "multi_stage", 
            "--config", "Execution", "data_hazard_mode", "ideal"
        ]
    },
]

HAZARD_MODES = [
    ("Stall", "stall"), 
    ("Forwarding", "forwarding")
]

BRANCH_PREDICTORS = [
    "static_not_taken", 
    "static_taken", 
    "dynamic_1bit", 
    "dynamic_2bit"
]

for mode_name, mode_val in HAZARD_MODES:
    for pred in BRANCH_PREDICTORS:
        config_id = f"5Stage_{mode_name}_{pred}"
        
        SIMULATOR_CONFIGS.append({
            "id": config_id,
            "flags": [
                "--config", "Execution", "processor_type", "multi_stage", 
                "--config", "Execution", "data_hazard_mode", mode_val,
                "--config", "Execution", "branch_stage", "ex",
                "--config", "Execution", "branch_predictor", pred
            ]
        })

# Usage: 
#   Run all: pytest test_runner.py
#   Run specific: SIM_MODE=5Stage_Stall_static_taken pytest test_runner.py
target_mode = os.environ.get("SIM_MODE")

if target_mode:
    filtered_configs = [c for c in SIMULATOR_CONFIGS if c["id"] == target_mode]
    
    if not filtered_configs:
        print(f"\n[Error] SIM_MODE='{target_mode}' not found.")
        print("Available modes:")
        for c in SIMULATOR_CONFIGS:
            print(f"  {c['id']}")
        sys.exit(1)
        
    print(f"\n[Info] Running Single Mode: {target_mode}")
    SIMULATOR_CONFIGS = filtered_configs
else:
    print(f"\n[Info] Running {len(SIMULATOR_CONFIGS)} Configurations")

# The testing stages
TEST_SUITES = {
    "single_without_hazards": {
        "asm_dir": Path("./assembly_single_without_hazards"),
        "ref_dir": Path("./reference_artifacts_without_hazards"),
    },
    "single_hazards": {
        "asm_dir": Path("./assembly_single_hazards"),
        "ref_dir": Path("./reference_artifacts_hazards"),
    },
    "multi_2_instr": {
        "asm_dir": Path("./assembly_multi_hazard/2_instr"),
        "ref_dir": Path("./reference_artifacts_multi/2_instr"),
    },
    "multi_gapped": {
        "asm_dir": Path("./assembly_multi_hazard/gapped"),
        "ref_dir": Path("./reference_artifacts_multi/gapped"),
    },
    "double_dep": {
        "asm_dir": Path("./assembly_multi_hazard/double_dep"),
        "ref_dir": Path("./reference_artifacts_multi/double_dep"),
    },
    "control_data": {
        "asm_dir": Path("./assembly_multi_hazard/control_data"),
        "ref_dir": Path("./reference_artifacts_multi/control_data"),
    },
}

def collect_test_cases():
    """
    Iterates through TEST_SUITES and finds all .s files.
    Returns a list of test case dictionaries.
    """
    test_cases = []
    
    for suite_name, config in TEST_SUITES.items():
        asm_dir = config["asm_dir"]
        ref_dir = config["ref_dir"]
        
        if not asm_dir.exists():
            print(f"Warning: Suite '{suite_name}' directory not found: {asm_dir}")
            continue

        # recursively find all .s files
        for asm_file in asm_dir.rglob("*.s"):
            test_cases.append({
                "suite": suite_name,
                "asm_file": asm_file,
                "asm_root": asm_dir,
                "ref_root": ref_dir
            })
            
    return test_cases

def generate_test_id(test_case):
    suite = test_case["suite"]
    rel_path = test_case["asm_file"].relative_to(test_case["asm_root"])
    return f"{suite}::{rel_path}"

# --- Double Parametrization ---
# 1. Iterate over configurations (Filtered by SIM_MODE if set)
@pytest.mark.parametrize("sim_config", SIMULATOR_CONFIGS, ids=lambda c: c["id"])
@pytest.mark.parametrize("case_data", collect_test_cases(), ids=generate_test_id)
def test_simulator_pipeline(sim_config, case_data):
    asm_file = case_data["asm_file"]
    suite_name = case_data["suite"]
    asm_root = case_data["asm_root"]
    ref_root = case_data["ref_root"]

    config_id = sim_config["id"] 

    relative_path = asm_file.relative_to(asm_root)
    test_ref_dir = ref_root / relative_path.parent / asm_file.name
    test_run_dir = RUN_DIR_BASE / config_id / suite_name / relative_path.parent / asm_file.name
    os.makedirs(test_run_dir, exist_ok=True)

    memimg_in = test_ref_dir / "test.memimg"
    ref_json_in = test_ref_dir / "rvss_output.json"
    dut_json_out = test_run_dir / "rv5s_output.json"

    # 1. Prerequisite Checks
    if not SIM2_BIN.exists():
        pytest.fail(f"Simulator binary not found at: {SIM2_BIN}")

    if not memimg_in.exists():
        pytest.skip(f"Reference memimg not found: {memimg_in}")
        
    if not ref_json_in.exists():
        pytest.skip(f"Reference JSON not found: {ref_json_in}")

    # 2. Run the DUT (5-Stage Simulator)
    try:
        # Syntax: ./rv5s_cli <input.memimg> -o <output.json> [FLAGS...]
        cmd = [str(SIM2_BIN), str(memimg_in), "-o", str(dut_json_out)] + sim_config["flags"]
        cmd_str = " ".join(cmd)
        # Run with timeout to catch infinite loops in the pipeline
        subprocess.run(
            cmd, 
            check=True, 
            timeout=TIME_OUT_WAIT, 
            capture_output=True, 
            text=True
        )
    except subprocess.CalledProcessError as e:
        error_msg = (
            f"\n{'='*40}\n"
            f"SIMULATOR CRASHED ({config_id})\n"
            f"Exit Code: {e.returncode}\n"
            f"Command: {cmd_str}\n"
            f"Stderr:\n{e.stderr}\n"
            f"{'='*40}"
        )
        pytest.fail(error_msg)
    except subprocess.TimeoutExpired:
        pytest.fail("Simulator timed out ({config_id}).")

    # 3. Compare Results
    if not dut_json_out.exists():
        pytest.fail("Simulator did not produce an output JSON file.")

    is_match, diff_msg = compare_states(ref_json_in, dut_json_out)

    # 4. Final Assertion
    assert is_match, f"State Mismatch in {config_id} / {suite_name}:\n{diff_msg}"