# tests/compare_json.py
# Compares the Final Dump States of the two simulator modes

import json

def compare_states(ref_json_path, dut_json_path):
    """
    Compares the state (vm_state, registers, memory) of two simulator JSON dumps.
    """
    try:
        with open(ref_json_path) as f:
            ref_data = json.load(f)
        with open(dut_json_path) as f:
            dut_data = json.load(f)
    except Exception as e:
        return False, f"Failed to load or parse JSON files: {e}"

    diffs = []
    
    # Compare VM State
    ref_state = ref_data.get("vm_state", {})
    dut_state = dut_data.get("vm_state", {})
    
    # These must match
    state_keys_to_match = ["program_counter", "instructions_retired"]
    
    for key in state_keys_to_match:
        ref_val = ref_state.get(key)
        dut_val = dut_state.get(key)
        if ref_val != dut_val:
            diffs.append(f"  - '{key}' mismatch: Expected {ref_val}, Got {dut_val}")
    
    # Note: We intentionally ignore cycle_s, cpi, and ipc as they are performance metrics, not correctness metrics.

    # Compare Registers
    ref_regs = ref_data.get("registers", {})
    dut_regs = dut_data.get("registers", {})

    if ref_regs != dut_regs:
        diffs.append("  - Register state mismatch:")
        for i in range(32):
            reg_name = f"x{i}"
            ref_val = ref_regs.get(reg_name)
            dut_val = dut_regs.get(reg_name)
            if ref_val != dut_val:
                diffs.append(f"    - '{reg_name}': Expected {ref_val}, Got {dut_val}")

    # Compare Memory Dump
    ref_mem = ref_data.get("memory_dump", {})
    dut_mem = dut_data.get("memory_dump", {})

    if ref_mem != dut_mem:
        diffs.append("  - Memory state mismatch:")
        all_addrs = set(ref_mem.keys()) | set(dut_mem.keys())       # find which addresses failed
        for addr in sorted(all_addrs):
            ref_val = ref_mem.get(addr)
            dut_val = dut_mem.get(addr)
            if ref_val != dut_val:
                diffs.append(f"    - '{addr}': Expected {ref_val}, Got {dut_val}")
                if len(diffs) > 20: 
                    diffs.append("    - ... (and many more)")
                    break

    # Return Final Result
    if diffs:
        return False, "State mismatch:\n" + "\n".join(diffs)
    else:
        return True, "States match."