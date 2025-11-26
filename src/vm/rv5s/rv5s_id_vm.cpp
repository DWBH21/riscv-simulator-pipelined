/**
 * @file rv5s_id_vm.cpp
 * @brief Implementation of the 5-stage pipelined RISC-V VM where Branch Comparison happens in the ID stage
 * Branch Prediction happens in the Fetch Stage with the Branch Target Buffer to determine whether the current instruction is a branch and what is its target address
 */

#include "vm/rv5s/rv5s_id_vm.h"
#include "vm/rv5s/btb.h"
#include "vm/rv5s/rv5s_control_unit.h"
#include "vm/rv5s/pipeline_registers.h"
#include "vm/rv5s/rv5s_hazard_unit.h"

#include "vm/rv5s/rv5s_forwarding_unit.h" 
#include "vm/rv5s/branch_prediction/static_predictors.h"
#include "vm/rv5s/branch_prediction/dynamic_1bit_predictor.h"
#include "vm/rv5s/branch_prediction/dynamic_2bit_predictor.h"

#include "utils.h"
#include "globals.h"
#include "common/instructions.h"
#include "config.h"

#include <cctype>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <stack>  
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

using instruction_set::Instruction;
using instruction_set::get_instr_encoding;
using instruction_type::MemReadOp;
using instruction_type::MemWriteOp;
using instruction_type::WriteBackSrc;
using instruction_type::AluSrcA;
using instruction_type::BranchOp;
using vm_config::DataHazardMode;
using vm_config::BranchPredictorType;

RV5SIDVM::RV5SIDVM(bool silent) : VmBase(silent) {
    Reset();
}

RV5SIDVM::~RV5SIDVM() = default;

void RV5SIDVM::enableForwarding(bool enable) {
    forwarding_enabled_ = enable;
}

void RV5SIDVM::setBranchPredictorType(BranchPredictorType type) {
    switch(type) 
    {
        case BranchPredictorType::STATIC_TAKEN:
            branch_predictor_ = std::make_unique<StaticTakenPredictor>();
            break;
        case BranchPredictorType::DYNAMIC_1BIT:
            branch_predictor_ = std::make_unique<Dynamic1BitPredictor>();
            break;
        case BranchPredictorType::DYNAMIC_2BIT:
            branch_predictor_ = std::make_unique<Dynamic2BitPredictor>();
            break;
        default:
            branch_predictor_ = std::make_unique<StaticNotTakenPredictor>();
            break;
    }
}

void RV5SIDVM::Reset() {
    program_counter_ = 0;
    instructions_retired_ = 0;
    cycle_s_ = 0;
    cpi_ = 0.0;
    ipc_ = 0.0;
    stall_cycles_ = 0;
    branch_mispredictions_ = 0;

    stall_request_= false;
    flush_pipeline_ = false;
    forwarding_enabled_ = vm_config::config.getDataHazardMode() == DataHazardMode::FORWARDING;
    setBranchPredictorType(vm_config::config.getBranchPredictorType());

    registers_.Reset();
    memory_controller_.Reset();
    program_size_ = 0;          // this should also be made zero as memory controller is reset (including the text and data segment)
    
    if_id_reg_ = CreateBubble<IF_ID_Reg>();             
    id_ex_reg_ = CreateBubble<ID_EX_Reg>();
    ex_mem_reg_ = CreateBubble<EX_MEM_Reg>();
    mem_wb_reg_ = CreateBubble<MEM_WB_Reg>();
    
    next_if_id_reg_ = CreateBubble<IF_ID_Reg>();
    next_id_ex_reg_ = CreateBubble<ID_EX_Reg>();
    next_ex_mem_reg_ = CreateBubble<EX_MEM_Reg>();
    next_mem_wb_reg_ = CreateBubble<MEM_WB_Reg>();

    if(!silent_mode_) {
        DumpRegisters(globals::registers_dump_file_path, registers_);
        DumpState(globals::vm_state_dump_file_path);
    }
}

void RV5SIDVM::Step() {
    // extra additional check for stop based on output status reqd ? 
    // Execute individual stages and write data to the next pipeline registers 

    if(output_status_ == "VM_PROGRAM_END") {
        std::cout << "VM_PROGRAM_END" << std::endl;
        return;
    }

    stall_request_ = false;
    flush_pipeline_ = false; 

    WriteBack_Stage();
    Memory_Stage();
    Execute_Stage();
    Decode_Stage();

    if(!stall_request_) {    // no need to stall
        Fetch_Stage();
    }

    cycle_s_++; // increment the no of cycles
    if(stall_request_)                  // stall condition -> IF stage paused
    {
        id_ex_reg_ = next_id_ex_reg_;
        ex_mem_reg_ = next_ex_mem_reg_;
        mem_wb_reg_ = next_mem_wb_reg_;
        stall_cycles_++;
    }
    else 
    {
        if_id_reg_ = next_if_id_reg_;       // update pipeline registers -> normal condition (no stall) 
        id_ex_reg_ = next_id_ex_reg_;
        ex_mem_reg_ = next_ex_mem_reg_;
        mem_wb_reg_ = next_mem_wb_reg_;
    }

    // update cpi and ipc
    if(instructions_retired_ > 0) {
        cpi_ = static_cast<double>(cycle_s_) / instructions_retired_;
        ipc_ = static_cast<double>(instructions_retired_) / cycle_s_;
    }
    else
    {
        cpi_ = 0.0;
        ipc_ = 0.0;
    }

    if(!silent_mode_) {
        DumpRegisters(globals::registers_dump_file_path, registers_);
        DumpState(globals::vm_state_dump_file_path);
    }

    bool all_instructions_fetched = (program_counter_ >= program_size_);
    bool is_pipeline_empty = !if_id_reg_.is_valid && !id_ex_reg_.is_valid && !ex_mem_reg_.is_valid && !mem_wb_reg_.is_valid;

    if(all_instructions_fetched && is_pipeline_empty) {
        RequestStop();
        std::cout << "VM_PROGRAM_END" << std::endl;
        output_status_ = "VM_PROGRAM_END";
        
        if(!silent_mode_) {
            DumpState(globals::vm_state_dump_file_path);        // final state with updated output status
        }
    } else {
        std::cout << "VM_STEP_COMPLETED" << std::endl;
        output_status_ = "VM_STEP_COMPLETED";
    }
}

void RV5SIDVM::Run() {
    ClearStop();

    output_status_ = "VM_RUNNING";
    while (true) {
        if(stop_requested_) {
            stop_requested_ = false;
            break;
        }
        Step();
        std::cout << "Program Counter: " << program_counter_ << std::endl;
    }
}

void RV5SIDVM::DebugRun() {
    ClearStop();
    output_status_ = "VM_RUNNING";
    while (true) {
        if(stop_requested_) {
            stop_requested_ = false;
            break;
        }
        if(CheckBreakpoint(program_counter_)) {
            std::cout << "VM_BREAKPOINT_HIT " << program_counter_ << std::endl;
            output_status_ = "VM_BREAKPOINT_HIT";
            if(!silent_mode_) {
                DumpState(globals::vm_state_dump_file_path);
            }
            break;      // stop execution
        }
        Step();
        std::cout << "Program Counter: " << program_counter_ << std::endl;
        unsigned int delay_ms = vm_config::config.getRunStepDelay();
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
}

void RV5SIDVM::Undo() {
    std::cerr << "Undo/Redo Feature is not available in multi-stage pipelining mode." << std::endl;    
}

void RV5SIDVM::Redo() {
    std::cerr << "Undo/Redo Feature is not available in multi-stage pipelining mode." << std::endl;
}

void RV5SIDVM::Fetch_Stage() {
    if (flush_pipeline_) { 
        stall_cycles_++;
        next_if_id_reg_ = CreateBubble<IF_ID_Reg>();
        return;
    }

    if(program_counter_ >= program_size_) {
        next_if_id_reg_ = CreateBubble<IF_ID_Reg>();
        return;
    }

    try {
        uint32_t instruction = memory_controller_.ReadWord(program_counter_);
        
        // Checking for current PC address in the branch target buffer to determine whether it is a branch or not
        auto [btb_hit, btb_target] = btb_.lookup(program_counter_);

        // Getting Branch Prediction result from the branch predictor
        bool predict_taken = branch_predictor_->getPrediction(program_counter_);

        uint64_t next_pc_val = 0;
        if (btb_hit && predict_taken) {
            next_pc_val = btb_target;

            next_if_id_reg_.predicted_outcome = true; 
            next_if_id_reg_.predicted_target = btb_target;
        } 
        else {
            next_pc_val = program_counter_ + 4;
            next_if_id_reg_.predicted_outcome = false;
            next_if_id_reg_.predicted_target = 0;
        }

        // update pipeline register
        next_if_id_reg_.instruction = instruction;
        next_if_id_reg_.pc = program_counter_;
        next_if_id_reg_.pc_inc = program_counter_ + 4; 
        next_if_id_reg_.is_valid = true;

        UpdateProgramCounter(next_pc_val - program_counter_);

    } catch(const std::exception& e) {
        std::cerr << "Fetch Stage Error: " << e.what() << std::endl;
        next_if_id_reg_ = CreateBubble<IF_ID_Reg>();
        UpdateProgramCounter(4);
    }
}

void RV5SIDVM::Decode_Stage() {

    if (flush_pipeline_) {          // set to true in case of a branch misprediction
        if(if_id_reg_.is_valid) {
            stall_cycles_++; 
        }
        next_id_ex_reg_ = CreateBubble<ID_EX_Reg>();
        return;
    }

    if(!if_id_reg_.is_valid)
    {
        next_id_ex_reg_ = CreateBubble<ID_EX_Reg>();
        return;
    }

    // read instruction from the if_id_register
    uint32_t instruction = if_id_reg_.instruction;
    ControlSignals control = control_unit_.getControlSignals(instruction);

    next_id_ex_reg_.pc = if_id_reg_.pc;
    next_id_ex_reg_.pc_inc = if_id_reg_.pc_inc;
    next_id_ex_reg_.is_valid = if_id_reg_.is_valid;
    if(control.is_nop)
    {
        next_id_ex_reg_.control = control;
        return;
    }

    uint8_t opcode = instruction & 0b1111111;
    uint8_t funct3 = (instruction >> 12) & 0b111;

    if(opcode == get_instr_encoding(Instruction::kecall).opcode && 
      funct3 == get_instr_encoding(Instruction::kecall).funct3) {
        control.is_syscall = true;
        next_id_ex_reg_.control = control;
        return;
    }
    if(opcode==0b1110011) {
        control.is_csr = true;
        next_id_ex_reg_.control = control;
        return;
    }

    next_id_ex_reg_.rd_index = (instruction >> 7) & 0b11111;
    next_id_ex_reg_.immediate = ImmGenerator(instruction);  // inherited from vm_base
    next_id_ex_reg_.control = control;

    // Handling rs1 for alu
    if(opcode == 0b0110111 || opcode == 0b0010111) { // lui, auipc
        next_id_ex_reg_.rs1_index = 0;
        next_id_ex_reg_.rs1_data = 0;
    } else {
        next_id_ex_reg_.rs1_index = (instruction >> 15) & 0b11111;
        next_id_ex_reg_.rs1_data = registers_.ReadGpr(next_id_ex_reg_.rs1_index);
    }

    // Handling rs2 for alu
    if(opcode == 0b0110011 || opcode == 0b0100011 || opcode == 0b1100011) { // R-type, S-type, B-type
        next_id_ex_reg_.rs2_index = (instruction >> 20) & 0b11111;
        next_id_ex_reg_.rs2_data = registers_.ReadGpr(next_id_ex_reg_.rs2_index);
    } else {
        next_id_ex_reg_.rs2_index = 0;
        next_id_ex_reg_.rs2_data = 0;
    }

    // resolution for data hazards
    bool data_stall = false;
    if(forwarding_enabled_) {
        data_stall = hazard_unit_.detectLoadUseHazard(control, next_id_ex_reg_.rs1_index, next_id_ex_reg_.rs2_index, id_ex_reg_);
    } else {
        data_stall = hazard_unit_.detectDataHazard(control, next_id_ex_reg_.rs1_index, next_id_ex_reg_.rs2_index, id_ex_reg_, ex_mem_reg_);
    }

    // ALU-Use Hazard (Specific to ID Stage)
    // If we need a register for a branch comparison, but that register is currently being computed in the EX stage, we must stall 1 cycle.
    if (!data_stall && control.branch && forwarding_enabled_) {
        if (id_ex_reg_.is_valid && id_ex_reg_.control.reg_write && id_ex_reg_.rd_index != 0) {
            if (id_ex_reg_.rd_index == next_id_ex_reg_.rs1_index || id_ex_reg_.rd_index == next_id_ex_reg_.rs2_index) {
                data_stall = true;
            }
        }
    }

    // Load-Use Hazard (Specific to ID Stage) 
    // 2-cycle stall needed
    if (!data_stall && (control.branch || control.branch_op == BranchOp::JALR) && forwarding_enabled_) {
        if (ex_mem_reg_.is_valid && ex_mem_reg_.control.mem_read && ex_mem_reg_.rd_index != 0) {
            if (ex_mem_reg_.rd_index == next_id_ex_reg_.rs1_index || ex_mem_reg_.rd_index == next_id_ex_reg_.rs2_index) {
                data_stall = true;
            }
        }
    }

    if(data_stall) {
        stall_request_ = true;
        next_id_ex_reg_ = CreateBubble<ID_EX_Reg>();
        return; 
    }

    // Branch Resolution 
    if (control.branch) {
        
        uint64_t val1 = forwarding_enabled_ ? getForwardedIdReg(next_id_ex_reg_.rs1_index) : next_id_ex_reg_.rs1_data;
        uint64_t val2 = forwarding_enabled_ ? getForwardedIdReg(next_id_ex_reg_.rs2_index) : next_id_ex_reg_.rs2_data;

        // Calculate actual Outcome
        bool actual_taken = false;
        uint64_t actual_target = 0;

        if (control.branch_op == BranchOp::JAL) {
            actual_taken = true;
            actual_target = if_id_reg_.pc + next_id_ex_reg_.immediate;
        } 
        else if (control.branch_op == BranchOp::JALR) {
            actual_taken = true;
            actual_target = val1 + next_id_ex_reg_.immediate;
        } 
        else { // Conditional Branches
            int64_t op1 = static_cast<int64_t>(val1);
            int64_t op2 = static_cast<int64_t>(val2);
            switch (control.branch_op) {
                case BranchOp::BEQ:  actual_taken = (op1 == op2); break;
                case BranchOp::BNE:  actual_taken = (op1 != op2); break;
                case BranchOp::BLT:  actual_taken = (op1 < op2);  break;
                case BranchOp::BGE:  actual_taken = (op1 >= op2); break;
                case BranchOp::BLTU: actual_taken = (val1 < val2); break;
                case BranchOp::BGEU: actual_taken = (val1 >= val2); break;
                default: break;
            }
            if(actual_taken) actual_target = if_id_reg_.pc + next_id_ex_reg_.immediate;
        }

        // Update Branch Predictor and BTB
        if (control.branch) {
            branch_predictor_->updateState(if_id_reg_.pc, if_id_reg_.predicted_outcome, actual_taken);
        }
        
        btb_.update(if_id_reg_.pc, actual_target);      

        // Verify against Fetch Stage Prediction
        bool prediction_correct = false;

        if (if_id_reg_.predicted_outcome == actual_taken) {
            if (actual_taken) {
                // If right target address was predicted
                prediction_correct = (if_id_reg_.predicted_target == actual_target);
            } else {
                prediction_correct = true; 
            }
        }

        // Handling Misprediction
        if (!prediction_correct) {
            branch_mispredictions_++;
            flush_pipeline_ = true; // flush instruction currently in fetch stage
            
            // Update PC to correct value
            if (actual_taken) {
                program_counter_ = actual_target;
            } else {
                program_counter_ = next_id_ex_reg_.pc_inc; // PC + 4
            }
        }
    }
}
void RV5SIDVM::Execute_Stage() {
    if(!id_ex_reg_.is_valid)
    {
        next_ex_mem_reg_ = CreateBubble<EX_MEM_Reg>();
        return;
    }
    ControlSignals control = id_ex_reg_.control;
    next_ex_mem_reg_.control = id_ex_reg_.control;
    next_ex_mem_reg_.is_valid = id_ex_reg_.is_valid;
    if(control.is_nop)
    {
        return;
    }
    else if(control.is_csr)
    {
        // ExecuteCsr();
        return;
    }
    else if(control.is_syscall)
    {
        // HandleSyscall();
        return;
    }

    // Forwarding logic 
    uint64_t data_alu_a = id_ex_reg_.rs1_data;
    uint64_t data_alu_b = id_ex_reg_.rs2_data;

    if(forwarding_enabled_) {

        // find out from where to forward for each of the alu operands 
        ForwardSrc srcA = forwarding_unit_.getAluSrcA(id_ex_reg_.rs1_index, ex_mem_reg_, mem_wb_reg_);
        ForwardSrc srcB = forwarding_unit_.getAluSrcB(id_ex_reg_.rs2_index, ex_mem_reg_, mem_wb_reg_);

        switch(srcA) {
            case ForwardSrc::FROM_EX_MEM:
                data_alu_a = ex_mem_reg_.alu_result;
                break;
            case ForwardSrc::FROM_MEM_WB:
                data_alu_a = getWriteBackData();
                break;
            case ForwardSrc::FROM_REG:
                // no need to forward
            default:
                break;
        }

        switch (srcB) {
            case ForwardSrc::FROM_EX_MEM:
                data_alu_b = ex_mem_reg_.alu_result;
                break;
            case ForwardSrc::FROM_MEM_WB:
                data_alu_b = getWriteBackData();
                break;
            case ForwardSrc::FROM_REG:
                // no need to forward
            default:
                break; 
        }
    } 

    bool overflow = false;
    uint64_t reg1_value, reg2_value;
    switch (control.alu_src_a) {
        case AluSrcA::ALU_SRC_A_PC:         // for auipc
            reg1_value = id_ex_reg_.pc;
            break;
        case AluSrcA::ALU_SRC_A_ZERO:       // for lui
            reg1_value = 0;
            break;
        case AluSrcA::ALU_SRC_A_RS1:        // for remaining r-type, i-type
        default:
            reg1_value = data_alu_a;
            break;
    }

    if(control.alu_src_a == AluSrcA::ALU_SRC_A_ZERO || control.alu_src_a == AluSrcA::ALU_SRC_A_PC)   // lui, auipc
    {
        if(control.branch_op != BranchOp::JAL)
        {
            reg2_value = static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.immediate)) << 12;   // left shift the imm by 12 bits 
            uint64_t and_mask = 0xA0000000;
            uint64_t or_mask = 0xFFFFFFFF00000000;
            if(reg2_value & and_mask)  // sign extend the one
            {
                reg2_value |= or_mask;
            } 
        } else {                                                                                // jal
            reg2_value = static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.immediate));
        }
    }
    else if(control.alu_src_b) {           // normal I type
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.immediate));
    }   // R type
    else {reg2_value = data_alu_b;
    }

    alu::AluOp aluOperation = control.alu_op;
    int64_t execution_result;
    std::tie(execution_result, overflow) = alu_.execute(aluOperation, reg1_value, reg2_value);

    next_ex_mem_reg_.pc_inc = id_ex_reg_.pc_inc;
    next_ex_mem_reg_.alu_result = execution_result;
    next_ex_mem_reg_.store_data = data_alu_b; 
    next_ex_mem_reg_.rd_index = id_ex_reg_.rd_index;
}
void RV5SIDVM::Memory_Stage() {
    
    if(!ex_mem_reg_.is_valid)
    {
        next_mem_wb_reg_ = CreateBubble<MEM_WB_Reg>();
        return;
    }
    ControlSignals control = ex_mem_reg_.control;
    if(control.is_nop || control.is_syscall || control.is_csr)
    {
        next_mem_wb_reg_.is_valid = ex_mem_reg_.is_valid;
        next_mem_wb_reg_.control = ex_mem_reg_.control;
        return;
    }

    uint64_t alu_result = ex_mem_reg_.alu_result;   // the address for load, store
    uint64_t store_data = ex_mem_reg_.store_data;
    int64_t memory_result = 0;

    // Load Instructions
    if(control.mem_read) {
    switch (control.mem_read_op) {
      case MemReadOp::MEM_READ_BYTE: {// LB
        memory_result = static_cast<int8_t>(memory_controller_.ReadByte(alu_result));
        break;
      }
      case MemReadOp::MEM_READ_HALF: {// LH
        memory_result = static_cast<int16_t>(memory_controller_.ReadHalfWord(alu_result));
        break;
      }
      case MemReadOp::MEM_READ_WORD: {// LW
        memory_result = static_cast<int32_t>(memory_controller_.ReadWord(alu_result));
        break;
      }
      case MemReadOp::MEM_READ_DOUBLE: {// LD
        memory_result = memory_controller_.ReadDoubleWord(alu_result);
        break;
      }
      case MemReadOp::MEM_READ_BYTE_UNSIGNED: {// LBU
        memory_result = static_cast<uint8_t>(memory_controller_.ReadByte(alu_result));
        break;
      }
      case MemReadOp::MEM_READ_HALF_UNSIGNED: {// LHU
        memory_result = static_cast<uint16_t>(memory_controller_.ReadHalfWord(alu_result));
        break;
      }
      case MemReadOp::MEM_READ_WORD_UNSIGNED: {// LWU
        memory_result = static_cast<uint32_t>(memory_controller_.ReadWord(alu_result));
        break;
      }
      case MemReadOp::MEM_READ_NONE:
      default: {
      std::cerr << "Default Condition Reached in Read Memory Switch" << std::endl;
      break;
      }
    }
    }
    // Store Instructions
    else if(control.mem_write) {
    switch (control.mem_write_op) {
      case MemWriteOp::MEM_WRITE_BYTE: {// SB
        memory_controller_.WriteByte(alu_result, store_data & 0xFF);
        break;
      }
      case MemWriteOp::MEM_WRITE_HALF: {// SH
        memory_controller_.WriteHalfWord(alu_result, store_data & 0xFFFF);
        break;
      }
      case MemWriteOp::MEM_WRITE_WORD: {// SW
        memory_controller_.WriteWord(alu_result, store_data & 0xFFFFFFFF);
        break;
      }
      case MemWriteOp::MEM_WRITE_DOUBLE: {// SD
        memory_controller_.WriteDoubleWord(alu_result, store_data & 0xFFFFFFFFFFFFFFFF);
        break;
      }
      case MemWriteOp::MEM_WRITE_NONE:
      default: {
      std::cerr << "Default Condition Reached in Write Memory Switch" << std::endl;
      break;
      }
    }
    }
    // transfer data 
    next_mem_wb_reg_.is_valid = ex_mem_reg_.is_valid;
    next_mem_wb_reg_.control = ex_mem_reg_.control;
    next_mem_wb_reg_.pc_inc = ex_mem_reg_.pc_inc;

    if(control.mem_read) {
        next_mem_wb_reg_.memory_data = memory_result;
    } else {
        next_mem_wb_reg_.alu_result = alu_result;       // in case of other R-type instructions
    }
    next_mem_wb_reg_.rd_index = ex_mem_reg_.rd_index;
}
void RV5SIDVM::WriteBack_Stage() {
    if(!mem_wb_reg_.is_valid)
    {
        return;
    }
    instructions_retired_++;
    ControlSignals control = mem_wb_reg_.control;
    if(control.is_syscall)
    {
        return;
    }
    if(control.is_csr) 
    {
        // do something maybe
        return;
    }
    uint8_t rd_index = mem_wb_reg_.rd_index;
    if(control.reg_write && rd_index != 0) {
        
        uint64_t write_data = 0;

        switch (control.wb_src) {
            case WriteBackSrc::WB_FROM_ALU:
                write_data = mem_wb_reg_.alu_result;
                break;
            
            case WriteBackSrc::WB_FROM_MEM:
                write_data = mem_wb_reg_.memory_data;
                break;
            
            case WriteBackSrc::WB_FROM_PC_INC:
                write_data = mem_wb_reg_.pc_inc;
                break;
            
            case WriteBackSrc::WB_NONE:
            default:
                std::cerr << "Default Write Back Stage Switch" << std::endl;    
            return; 
        }
        registers_.WriteGpr(rd_index, write_data);
    }
}
/**
 * @brief Dumps the current VM state to a JSON file.
 * This is the 5-stage pipeline specific implementation.
 */
void RV5SIDVM::DumpState(const std::filesystem::path &filename) {
    std::ofstream file(filename);
    if(!file.is_open()) {
        std::cerr << "Unable to open vm_state_dump file: " << filename << std::endl;
        return;
    }

    file << "{\n";
    
    // Dump General VM State
    file << "  \"vm_state\": {\n";
    file << "    \"program_counter\": " << program_counter_ << ",\n";
    file << "    \"output_status\": \"" << output_status_ << "\",\n";
    file << "    \"flush_pipeline\": \"" << flush_pipeline_ << "\",\n";
    file << "    \"stall_request\": \"" << stall_request_ << "\",\n"; 
    file << "    \"cycles\": " << cycle_s_ << ",\n";
    file << "    \"instructions_retired\": " << instructions_retired_ << ",\n";
    file << "    \"cpi\": " << cpi_ << ",\n";
    file << "    \"ipc\": " << ipc_ << ",\n";
    file << "    \"stall_cycles\": " << stall_cycles_ << ",\n";
    file << "    \"branch_mispredictions\": " << branch_mispredictions_ << "\n";
    file << "  },\n";

    // Dump the pipeline registers
    DumpPipelineRegisters(file, if_id_reg_, id_ex_reg_, ex_mem_reg_, mem_wb_reg_);

    file << "}\n";
    file.close();
}

uint64_t RV5SIDVM::getWriteBackData() {
    ControlSignals control = mem_wb_reg_.control;
    uint64_t write_data = 0;
    switch (control.wb_src) {
        case WriteBackSrc::WB_FROM_ALU:
            write_data = mem_wb_reg_.alu_result;
            break;
        
        case WriteBackSrc::WB_FROM_MEM:
            write_data = mem_wb_reg_.memory_data;
            break;
        
        case WriteBackSrc::WB_FROM_PC_INC:
            write_data = mem_wb_reg_.pc_inc;
            break;
        
        case WriteBackSrc::WB_NONE:
        default:
            std::cerr << "Default Write Back Stage Switch" << std::endl; 
        return 0;
    }
    return write_data;
}

uint64_t RV5SIDVM::getForwardedIdReg(uint8_t reg_index) {
    if(reg_index == 0) return 0;

    if(ex_mem_reg_.is_valid && ex_mem_reg_.control.reg_write && ex_mem_reg_.rd_index == reg_index) {
        return ex_mem_reg_.alu_result; 
    }

    if(mem_wb_reg_.is_valid && mem_wb_reg_.control.reg_write && mem_wb_reg_.rd_index == reg_index) {
        return getWriteBackData(); 
    }

    return registers_.ReadGpr(reg_index);
}