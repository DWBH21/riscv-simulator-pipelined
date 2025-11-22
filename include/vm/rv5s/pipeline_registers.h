/**
 * @file pipeline_registers.h
 * @brief Defines the data structures for pipeline registers and control signals.
 */

#ifndef PIPELINE_REGISTERS_H
#define PIPELINE_REGISTERS_H

#include "vm/alu.h"
#include <cstdint>

namespace instruction_type {
    // enums to distinguish between different load and store instructions respectively
    enum MemReadOp {
        MEM_READ_NONE,
        MEM_READ_BYTE,
        MEM_READ_HALF,
        MEM_READ_WORD,
        MEM_READ_DOUBLE,
        MEM_READ_BYTE_UNSIGNED,
        MEM_READ_HALF_UNSIGNED,
        MEM_READ_WORD_UNSIGNED
    };

    enum MemWriteOp {
        MEM_WRITE_NONE,
        MEM_WRITE_BYTE,
        MEM_WRITE_HALF,
        MEM_WRITE_WORD,
        MEM_WRITE_DOUBLE
    };

    enum WriteBackSrc {
        WB_NONE,
        WB_FROM_ALU,
        WB_FROM_MEM,
        WB_FROM_PC_INC
    };

    enum AluSrcA {
        ALU_SRC_A_RS1,
        ALU_SRC_A_ZERO,     // for lui
        ALU_SRC_A_PC       // for auipc
    };

    enum BranchOp {
        B_NONE,
        BEQ,
        BNE,
        BLT,
        BGE,
        BLTU,
        BGEU,
        JAL,
        JALR
    };
}
// Control Signals 
struct ControlSignals {
    // EX Stage Controls
    alu::AluOp alu_op = alu::AluOp::kNone;
    bool alu_src_b = false;     // ALU src2: true=Imm, false=rs2
    instruction_type::AluSrcA alu_src_a;
    
    // MEM Stage Controls
    bool mem_read = false;    
    bool mem_write = false;   
    instruction_type::MemReadOp mem_read_op = instruction_type::MemReadOp::MEM_READ_NONE;
    instruction_type::MemWriteOp mem_write_op = instruction_type::MemWriteOp::MEM_WRITE_NONE;

    bool branch = false;
    instruction_type::BranchOp branch_op = instruction_type::BranchOp::B_NONE;

    // WB Stage Controls
    bool reg_write = false;   
    bool mem_to_reg = false;
    instruction_type::WriteBackSrc wb_src = instruction_type::WB_NONE;

    bool is_csr = false;
    bool is_syscall = false;
    // // Additional control to detect nops/fp instrs -> is this really required ? 
    bool is_nop = true;
};


// The pipeline registers: 
struct IF_ID_Reg {
    bool is_valid = false;
    uint32_t instruction = 0;           // the original instruction fetched from memory 
    uint64_t pc = 0;            // pc of the original instruction -> will be needed in pc relative addressing later.
    uint64_t pc_inc = 0;                    // the incremented program counter
};

struct ID_EX_Reg {
    bool is_valid = false;
    ControlSignals control;   // All control signals
    uint64_t pc = 0;           // the original pc
    uint64_t pc_inc = 0;          // the incremented pc

    // Data read from register file
    uint64_t rs1_data = 0;
    uint64_t rs2_data = 0;

    // Immediate value
    int32_t immediate = 0;

    // Register indices
    uint8_t rs1_index = 0;
    uint8_t rs2_index = 0;
    uint8_t rd_index = 0;

    bool predicted_outcome = false;         // the branch predicted in case of branch instruction in the earlier stage, required to determine if it was a misprediction or not
};

struct EX_MEM_Reg {
    bool is_valid = false;
    ControlSignals control; // Passed from ID/EX. All control signals included for simplicity (even those not reqd by this stage) 

    uint64_t pc_inc = 0;    // for jal, jalr
    uint64_t alu_result = 0;    // If data from alu is to be written back
    uint64_t store_data = 0; // Data from rs2, for store instructions
    uint8_t rd_index = 0;    // register index for write back 
};

struct MEM_WB_Reg {
    bool is_valid = false;
    ControlSignals control; // Passed from EX/MEM. All control signals included for simplicity (even those not reqd by this stage)

    uint64_t pc_inc = 0;      // for jal, jalr
    uint64_t memory_data = 0; // Data read from memory
    uint64_t alu_result = 0;  // IF data from alu is to be written back
    uint8_t rd_index = 0;       // register index for write back
};

#endif