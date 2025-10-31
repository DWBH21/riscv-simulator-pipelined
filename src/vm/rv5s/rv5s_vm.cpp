/**
 * @file rvss_vm.cpp
 * @brief RV5S VM implementation
 */

#include "vm/rv5s/rv5s_vm.h"
#include "vm/rv5s/rv5s_control_unit.h"
#include "vm/rv5s/pipeline_registers.h"
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

RV5SVM::RV5SVM() : VmBase() {
  Reset();
  DumpRegisters(globals::registers_dump_file_path, registers_);
  DumpState(globals::vm_state_dump_file_path);
}

RV5SVM::~RV5SVM() = default;

void RV5SVM::Reset() {
    program_counter_ = 0;
    instructions_retired_ = 0;
    cycle_s_ = 0;
    cpi_ = 0.0;
    ipc_ = 0.0;
    stall_cycles_ = 0;
    branch_mispredictions_ = 0;
    pipeline_drain_counter_ = 0;
    registers_.Reset();
    memory_controller_.Reset();
    
    if_id_reg_ = CreateBubble<IF_ID_Reg>();             
    id_ex_reg_ = CreateBubble<ID_EX_Reg>();
    ex_mem_reg_ = CreateBubble<EX_MEM_Reg>();
    mem_wb_reg_ = CreateBubble<MEM_WB_Reg>();
    
    next_if_id_reg_ = CreateBubble<IF_ID_Reg>();
    next_id_ex_reg_ = CreateBubble<ID_EX_Reg>();
    next_ex_mem_reg_ = CreateBubble<EX_MEM_Reg>();
    next_mem_wb_reg_ = CreateBubble<MEM_WB_Reg>();

    // to add reset for control, status
    DumpRegisters(globals::registers_dump_file_path, registers_);
    DumpState(globals::vm_state_dump_file_path);
}

void RV5SVM::Step() {
    // extra additional check for stop based on output status reqd ? 
    // Execute individual stages and write data to the next pipeline registers 

    WriteBack_Stage();
    Memory_Stage();
    Execute_Stage();
    Decode_Stage();
    Fetch_Stage();

    if(program_counter_ <= program_size_ || pipeline_drain_counter_ > 0) {
        std::cout << "VM_STEP_COMPLETED" << std::endl;
        output_status_ = "VM_STEP_COMPLETED";         
    }                                                                           

    cycle_s_++; // increment the no of cycles

    if_id_reg_ = next_if_id_reg_;       // update pipeline registers
    id_ex_reg_ = next_id_ex_reg_;
    ex_mem_reg_ = next_ex_mem_reg_;
    mem_wb_reg_ = next_mem_wb_reg_;

    // update cpi and ipc
    if (instructions_retired_ > 0) {
        cpi_ = static_cast<double>(cycle_s_) / instructions_retired_;
        ipc_ = static_cast<double>(instructions_retired_) / cycle_s_;
    }
    else
    {
        cpi_ = 0.0;
        ipc_ = 0.0;
    }

    DumpRegisters(globals::registers_dump_file_path, registers_);
    DumpState(globals::vm_state_dump_file_path);

    if (pipeline_drain_counter_ > 0) {
        pipeline_drain_counter_--;
        if (pipeline_drain_counter_ == 0) {     // all instructions cleared from pipeline
            RequestStop(); 
            std::cout << "VM_PROGRAM_END" << std::endl;                         
            output_status_ = "VM_PROGRAM_END";      
        }
    }
    // if the next instruction is a breakpoint, indicate a stop to the run or debugRun functions
    // if (CheckBreakpoint(program_counter_)) {
    //    RequestStop(); 
    //    output_status_ = "VM_BREAKPOINT";
    //    DumpState(globals::vm_state_dump_file_path);         // dump state again to show breakpoint
    //    std::cout << "VM_BREAKPOINT" << std::endl;
    // }
}

void RV5SVM::Run() {
    ClearStop();

    output_status_ = "VM_RUNNING";
    while (true) {
        if (stop_requested_) {
            stop_requested_ = false;
            break;
        }
        Step();
    }
}

void RV5SVM::DebugRun() {
    ClearStop();
    output_status_ = "VM_RUNNING";
    while (true) {
        if (stop_requested_) {
            stop_requested_ = false;
            break;
        }
        if (CheckBreakpoint(program_counter_)) {
            std::cout << "VM_BREAKPOINT_HIT " << program_counter_ << std::endl;
            output_status_ = "VM_BREAKPOINT_HIT";
            DumpState(globals::vm_state_dump_file_path);
            break;      // stop execution
        }
        Step();
        unsigned int delay_ms = vm_config::config.getRunStepDelay();
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
}

void RV5SVM::Undo() {
    std::cerr << "Undo/Redo Feature is not available in multi-stage pipelining mode." << std::endl;    
}

void RV5SVM::Redo() {
    std::cerr << "Undo/Redo Feature is not available in multi-stage pipelining mode." << std::endl;
}

void RV5SVM::Fetch_Stage() {
    // If last instruction has been fetched already, but there are instructions remaining in the pipeline.
    if (pipeline_drain_counter_ > 0) {
        next_if_id_reg_ = CreateBubble<IF_ID_Reg>();
        return;
    }

    if(program_counter_ >= program_size_) {
        std::cout << "All instructions have been fetched. Subsequent steps will drain the pipeline." << std::endl;
        pipeline_drain_counter_ = 4;

        next_if_id_reg_ = CreateBubble<IF_ID_Reg>();
    }
    else  
    {
        // the Memory::ReadWord function throws a std::out_of_range exception if an invalid memory address is read.
        try {
            uint32_t instruction = memory_controller_.ReadWord(program_counter_);
            next_if_id_reg_.instruction = instruction;
            next_if_id_reg_.pc = program_counter_;       // original pc stored 
            UpdateProgramCounter(4);
            next_if_id_reg_.pc_inc = program_counter_; // the incremented pc to be stored in the pipeline register
            next_if_id_reg_.is_valid = true;
        } catch(const std::exception& e) {
            std::cerr << "Fetch Stage Error: " << e.what() << std::endl;
            next_if_id_reg_.instruction = 0;    // insert bubble 
            next_if_id_reg_.pc = program_counter_;       // original pc stored 
            UpdateProgramCounter(4);
            next_if_id_reg_.pc_inc = program_counter_; // the incremented pc to be stored in the pipeline register
            next_if_id_reg_.is_valid = true;
        }
    }
}

void RV5SVM::Decode_Stage() {

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

    if (opcode == get_instr_encoding(Instruction::kecall).opcode && 
      funct3 == get_instr_encoding(Instruction::kecall).funct3) {
        control.is_syscall = true;
        next_id_ex_reg_.control = control;
        return;
    }
    if (opcode==0b1110011) {
        control.is_csr = true;
        next_id_ex_reg_.control = control;
        return;
    }

    next_id_ex_reg_.rd_index = (instruction >> 7) & 0b11111;
    next_id_ex_reg_.immediate = ImmGenerator(instruction);  // inherited from vm_base
    next_id_ex_reg_.control = control;

    // Handling rs1 for alu
    if (opcode == 0b0110111 || opcode == 0b0010111) { // lui, auipc
        next_id_ex_reg_.rs1_index = 0;
        next_id_ex_reg_.rs1_data = 0;
    } else {
        next_id_ex_reg_.rs1_index = (instruction >> 15) & 0b11111;
        next_id_ex_reg_.rs1_data = registers_.ReadGpr(next_id_ex_reg_.rs1_index);
    }

    // Handling rs2 for alu
    if (opcode == 0b0110011 || opcode == 0b0100011 || opcode == 0b1100011) { // R-type, S-type, B-type
        next_id_ex_reg_.rs2_index = (instruction >> 20) & 0b11111;
        next_id_ex_reg_.rs2_data = registers_.ReadGpr(next_id_ex_reg_.rs2_index);
    } else {
        next_id_ex_reg_.rs2_index = 0;
        next_id_ex_reg_.rs2_data = 0;
    }
}
void RV5SVM::Execute_Stage() {
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
    // else if(control.is_csr)
    // {
    //     ExecuteCsr();
    //     return;
    // }
    // else if(control.is_syscall)
    // {
    //     HandleSyscall();
    //     return;
    // }
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
            reg1_value = id_ex_reg_.rs1_data;
            break;
    }

    if(control.alu_src_b) {       // I type instruction
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.immediate));
    } else {                    // R type instruction
        reg2_value = id_ex_reg_.rs2_data;
    }

    alu::AluOp aluOperation = control.alu_op;
    int64_t execution_result;
    std::tie(execution_result, overflow) = alu_.execute(aluOperation, reg1_value, reg2_value);

    next_ex_mem_reg_.pc_inc = id_ex_reg_.pc_inc;
    next_ex_mem_reg_.alu_result = execution_result;
    next_ex_mem_reg_.store_data = id_ex_reg_.rs2_data; 
    next_ex_mem_reg_.rd_index = id_ex_reg_.rd_index;
}
void RV5SVM::Memory_Stage() {
    
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
    if (control.mem_read) {
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
    else if (control.mem_write) {
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
void RV5SVM::WriteBack_Stage() {
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
    if (control.reg_write && rd_index != 0) {
        
        uint64_t data_to_write = 0;

        switch (control.wb_src) {
            case WriteBackSrc::WB_FROM_ALU:
                data_to_write = mem_wb_reg_.alu_result;
                break;
            
            case WriteBackSrc::WB_FROM_MEM:
                data_to_write = mem_wb_reg_.memory_data;
                break;
            
            case WriteBackSrc::WB_FROM_PC_INC:
                data_to_write = mem_wb_reg_.pc_inc;
                break;
            
            case WriteBackSrc::WB_NONE:
            default:
                std::cerr << "Default Write Back Stage Switch" << std::endl;    
            return; 
        }
        registers_.WriteGpr(rd_index, data_to_write);
    }
}
/**
 * @brief Dumps the current VM state to a JSON file.
 * This is the 5-stage pipeline specific implementation.
 */
void RV5SVM::DumpState(const std::filesystem::path &filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Unable to open vm_state_dump file: " << filename << std::endl;
        return;
    }

    file << "{\n";
    
    // Dump General VM State
    file << "  \"vm_state\": {\n";
    file << "    \"program_counter\": " << program_counter_ << ",\n";
    file << "    \"output_status\": \"" << output_status_ << "\",\n";
    file << "    \"cycles\": " << cycle_s_ << ",\n";
    file << "    \"instructions_retired\": " << instructions_retired_ << ",\n";
    file << "    \"cpi\": " << cpi_ << ",\n";
    file << "    \"ipc\": " << ipc_ << ",\n";
    file << "    \"pipeline_drain_counter\": " << pipeline_drain_counter_ << "\n";
    file << "  },\n"; // <-- Add comma

    // Dump the pipeline registers
    DumpPipelineRegisters(file, if_id_reg_, id_ex_reg_, ex_mem_reg_, mem_wb_reg_);

    file << "}\n";
    file.close();
}
