/**
 * @file rv5s_control_unit.cpp
 * @brief RV5S Control Unit implementation
 */
 
#include "vm/rv5s/rv5s_control_unit.h"
#include "vm/alu.h"
#include "common/instructions.h"
#include "vm/rv5s/pipeline_registers.h"

#include <cstdint>
#include <iostream> 

using instruction_set::Instruction;
using instruction_set::get_instr_encoding;
using instruction_type::MemReadOp;
using instruction_type::MemWriteOp;
using instruction_type::WriteBackSrc;
using instruction_type::AluSrcA;
using instruction_type::BranchOp;

// To check if an instruction is a floating point instruction
bool isFloatingPointOpcode(uint8_t opcode) {
    switch (opcode) {
        case 0b0000111: case 0b0100111: case 0b1010011:
        case 0b1000011: case 0b1000111: case 0b1001011: case 0b1001111:
            return true;
        default:
            return false;
    }
}

ControlSignals RV5SControlUnit::CreateNOP() {
    ControlSignals nop;      // all signals are disabled by default
    nop.is_nop = true;
    return nop;
}

ControlSignals RV5SControlUnit::getControlSignals(uint32_t instruction) {
    ControlSignals signals;
    signals.is_nop = false;

    // Handling NOP
    // bubble or addi x0, x0, 0 or add x0, x0, x0
    if(instruction == 0 || instruction == 0x00000013 || instruction == 0x00000033) {
        signals.is_nop = true;
        if (instruction == 0x00000013) {            // addi x0, x0, 0
            signals.alu_op = alu::AluOp::kAdd;
            signals.alu_src_b = true;
        }
        else if (instruction == 0x00000033) {           // add x0, x0, x0
            signals.alu_op = alu::AluOp::kAdd;
        }
        return signals;
    }
    uint8_t opcode = instruction & 0b1111111;

    // Check for floating point and m instructions in assembled program but disabled in config file 
    bool is_fp_instr = isFloatingPointOpcode(opcode);
    if (is_fp_instr) {
        std::cerr << "RV5SControlUnit Error: Floating-point instruction (opcode 0x"
                  << std::hex << (int)opcode << std::dec << ") encountered but F/D extensions is disabled." << std::endl;
        return CreateNOP(); // Return bubble
    }
    
    signals.alu_src_a = AluSrcA::ALU_SRC_A_RS1;  // default

    // same as in rvss control unit
    switch (opcode) {
    case 0b0110011: /* R-type (kAdd, kSub, kAnd, kOr, kXor, kSll, kSrl, etc.) */ {
      signals.reg_write = true;
      signals.wb_src = WriteBackSrc::WB_FROM_ALU;
      break;
    }
    case 0b0000011:
    {// Load instructions (LB, LH, LW, LD)
      signals.alu_src_b = true;
      signals.wb_src = WriteBackSrc::WB_FROM_MEM;
      signals.reg_write = true;
      signals.mem_read = true;
      break;
    }
    case 0b0100011: {// Store instructions (SB, SH, SW, SD)
      signals.alu_src_b = true;
      signals.mem_write = true;
      break;
    }
    case 0b1100011: {// signals.branch instructions (BEQ, BNE, BLT, BGE)
      signals.branch = true;
      break;
    }
    case 0b0010011: {// I-type alu instructions (ADDI, ANDI, ORI, XORI, SLTI, SLLI, SRLI)
      signals.alu_src_b = true;
      signals.reg_write = true;
      signals.wb_src = WriteBackSrc::WB_FROM_ALU;
      break;
    }
    case 0b0110111: {// LUI (Load Upper Immediate)
      signals.alu_src_b = true;
      signals.reg_write = true; // alu will add immediate to zero
      signals.wb_src = WriteBackSrc::WB_FROM_ALU;
      signals.alu_src_a = AluSrcA::ALU_SRC_A_ZERO;
      break;
    }
    case 0b0010111: {// AUIPC (Add Upper Immediate to PC)
      signals.alu_src_b = true;
      signals.reg_write = true; // alu will add immediate to PC
      signals.wb_src = WriteBackSrc::WB_FROM_ALU;
      signals.alu_src_a = AluSrcA::ALU_SRC_A_PC;
      break;
    }
    case 0b1101111: {// JAL (Jump and Link)
      signals.reg_write = true;
      signals.branch = true;
      signals.wb_src = WriteBackSrc::WB_FROM_PC_INC;

      signals.alu_src_a = AluSrcA::ALU_SRC_A_PC;   // Use PC as the first operand
      signals.alu_src_b = true;
      break;
    }
    case 0b1100111: {// JALR (Jump and Link Register)
      signals.alu_src_b = true;
      signals.reg_write = true;
      signals.branch = true;
      signals.wb_src = WriteBackSrc::WB_FROM_PC_INC;

      signals.alu_src_a = AluSrcA::ALU_SRC_A_RS1;   // specifying again
      break;
    }
    case 0b0000001: {// kMul
      signals.reg_write = true;
      signals.wb_src = WriteBackSrc::WB_FROM_ALU;
      break;
    }

    default:
      std::cerr << "RVS5ControlUnit: Unknown opcode: 0x" << std::hex << (int)opcode << std::dec << std::endl;
      return CreateNOP();
  }

  // Determining the correct alu op
    uint8_t funct3 = (instruction >> 12) & 0b111;
    uint8_t funct7 = (instruction >> 25) & 0b1111111;
    // uint8_t funct5 = (instruction >> 20) & 0b11111;     // not used till now
    // uint8_t funct2 = (instruction >> 25) & 0b11;        // not used till now

    switch (opcode)
    {
    case 0b0110011: {// R-Type
        switch (funct3)
        {
        case 0b000:{ // kAdd, kSub, kMul
            switch (funct7)
            {
            case 0x0000000: {// kAdd
                signals.alu_op = alu::AluOp::kAdd;
                break;
            }
            case 0b0100000: {// kSub
                signals.alu_op = alu::AluOp::kSub;
                break;
            }
            case 0b0000001: {// kMul
                signals.alu_op = alu::AluOp::kMul;
                break;
            }
            }
            break;
        }
        case 0b001: {// kSll, kMulh
            switch (funct7)
            {
            case 0b0000000: {// kSll
                signals.alu_op = alu::AluOp::kSll;
                break;
            }
            case 0b0000001: {// kMulh
                signals.alu_op = alu::AluOp::kMulh;
                break;
            }
            }
            break;
        }
        case 0b010: {// kSlt, kMulhsu
            switch (funct7)
            {
            case 0b0000000: {// kSlt
                signals.alu_op = alu::AluOp::kSlt;
                break;
            }
            case 0b0000001: {// kMulhsu
                signals.alu_op = alu::AluOp::kMulhsu;
                break;
            }
            }
            break;
        }
        case 0b011: {// kSltu, kMulhu
            switch (funct7)
            {
            case 0b0000000: {// kSltu
                signals.alu_op = alu::AluOp::kSltu;
                break;
            }
            case 0b0000001: {// kMulhu
                signals.alu_op = alu::AluOp::kMulhu;
                break;
            }
            }
            break;
        }
        case 0b100: {// kXor, kDiv
            switch (funct7)
            {
            case 0b0000000: {// kXor
                signals.alu_op = alu::AluOp::kXor;
                break;
            }
            case 0b0000001: {// kDiv
                signals.alu_op = alu::AluOp::kDiv;
                break;
            }
            }
            break;
        }
        case 0b101: {// kSrl, kSra, kDivu
            switch (funct7)
            {
            case 0b0000000: {// kSrl
                signals.alu_op = alu::AluOp::kSrl;
                break;
            }
            case 0b0100000: {// kSra
                signals.alu_op = alu::AluOp::kSra;
                break;
            }
            case 0b0000001: {// kDivu
                signals.alu_op = alu::AluOp::kDivu;
                break;
            }
            }
            break;
        }
        case 0b110: {// kOr, kRem
            switch (funct7)
            {
            case 0b0000000: {// kOr
                signals.alu_op = alu::AluOp::kOr;
                break;
            }
            case 0b0000001: {// kRem
                signals.alu_op = alu::AluOp::kRem;
                break;
            }
            }
            break;
        }
        case 0b111: {// kAnd, kRemu
            switch (funct7)
            {
            case 0b0000000: {// kAnd
                signals.alu_op = alu::AluOp::kAnd;
                break;
            }
            case 0b0000001: {// kRemu
                signals.alu_op = alu::AluOp::kRemu;
                break;
            }
            }
            break;
        }
        }
        break;
    }
    case 0b0010011: {// I-Type
        switch (funct3)
        {
        case 0b000: {// ADDI
            signals.alu_op = alu::AluOp::kAdd;
            break;
        }
        case 0b001: {// SLLI
            signals.alu_op = alu::AluOp::kSll;
            break;
        }
        case 0b010: {// SLTI
            signals.alu_op = alu::AluOp::kSlt;
            break;
        }
        case 0b011: {// SLTIU
            signals.alu_op = alu::AluOp::kSltu;
            break;
        }
        case 0b100: {// XORI
            signals.alu_op = alu::AluOp::kXor;
            break;
        }
        case 0b101: {// SRLI & SRAI
            switch (funct7)
            {
            case 0b0000000: {// SRLI
                signals.alu_op = alu::AluOp::kSrl;
                break;
            }
            case 0b0100000: {// SRAI
                signals.alu_op = alu::AluOp::kSra;
                break;
            }
            }
            break;
        }
        case 0b110: {// ORI
            signals.alu_op = alu::AluOp::kOr;
            break;
        }
        case 0b111: {// ANDI
            signals.alu_op = alu::AluOp::kAnd;
            break;
        }
        }
        break;
    }
    case 0b1100011: {// B-Type
        switch (funct3)
        {
        case 0b000: {// BEQ
            signals.alu_op = alu::AluOp::kSub;
            signals.branch_op = BranchOp::BEQ;
            break;
        }
        case 0b001: {// BNE
            signals.alu_op = alu::AluOp::kSub;
            signals.branch_op = BranchOp::BNE;
            break;
        }
        case 0b100: {// BLT
            signals.alu_op = alu::AluOp::kSlt;
            signals.branch_op = BranchOp::BLT;
            break;
        }
        case 0b101: {// BGE
            signals.alu_op = alu::AluOp::kSlt;
            signals.branch_op = BranchOp::BGE;
            break;
        }
        case 0b110: {// BLTU
            signals.alu_op = alu::AluOp::kSltu;
            signals.branch_op = BranchOp::BLTU;
            break;
        }
        case 0b111: {// BGEU
            signals.alu_op = alu::AluOp::kSltu;
            signals.branch_op = BranchOp::BGEU;
            break;
        }
        }
        break;
    }
    case 0b0000011: {// Load
        signals.alu_op = alu::AluOp::kAdd;
        switch (funct3) {
            case 0b000: signals.mem_read_op = MemReadOp::MEM_READ_BYTE; break;
            case 0b001: signals.mem_read_op = MemReadOp::MEM_READ_HALF; break;
            case 0b010: signals.mem_read_op = MemReadOp::MEM_READ_WORD; break;
            case 0b011: signals.mem_read_op = MemReadOp::MEM_READ_DOUBLE; break;
            case 0b100: signals.mem_read_op = MemReadOp::MEM_READ_BYTE_UNSIGNED; break;
            case 0b101: signals.mem_read_op = MemReadOp::MEM_READ_HALF_UNSIGNED; break;
            case 0b110: signals.mem_read_op = MemReadOp::MEM_READ_WORD_UNSIGNED; break;
            default:    signals.mem_read_op = MemReadOp::MEM_READ_NONE; break;
        }
        break;
    }
    case 0b0100011: {// Store
        signals.alu_op = alu::AluOp::kAdd;
        switch (funct3) {
            case 0b000: signals.mem_write_op = MemWriteOp::MEM_WRITE_BYTE; break;
            case 0b001: signals.mem_write_op = MemWriteOp::MEM_WRITE_HALF; break;
            case 0b010: signals.mem_write_op = MemWriteOp::MEM_WRITE_WORD; break;
            case 0b011: signals.mem_write_op = MemWriteOp::MEM_WRITE_DOUBLE; break;
            default:    signals.mem_write_op = MemWriteOp::MEM_WRITE_NONE; break;
        }
        break;
    }
    case 0b1100111: {// JALR
        signals.alu_op = alu::AluOp::kAdd;
        signals.branch_op = BranchOp::JALR;
        break;
    }
    case 0b1101111: {// JAL
        signals.alu_op = alu::AluOp::kAdd;
        signals.branch_op = BranchOp::JAL;
        break;
    }
    case 0b0110111: {// LUI
        signals.alu_op = alu::AluOp::kAdd;
        break;
    }
    case 0b0010111: {// AUIPC
        signals.alu_op = alu::AluOp::kAdd;
        break;
    }
    case 0b0000000: {// FENCE
        signals.alu_op = alu::AluOp::kNone;
        break;
    }
    case 0b1110011: {// SYSTEM
        switch (funct3) 
        {
        case 0b000: // ECALL
            signals.alu_op = alu::AluOp::kNone;
            break;
        case 0b001: // CSRRW
            signals.alu_op = alu::AluOp::kNone;
            break;
        default:
            break;
        }
        break;
    }
    case 0b0011011: {// R4-Type
        switch (funct3) 
        {
            case 0b000: {// ADDIW
                signals.alu_op = alu::AluOp::kAddw;
                break;
            }
            case 0b001: {// SLLIW
                signals.alu_op = alu::AluOp::kSllw;
                break;
            }
            case 0b101: {// SRLIW & SRAIW
                switch (funct7) 
                {
                case 0b0000000: {// SRLIW
                    signals.alu_op = alu::AluOp::kSrlw;
                    break;
                }
                case 0b0100000: {// SRAIW
                        signals.alu_op = alu::AluOp::kSraw;
                        break;
                    }
                }
                break;
            }
        }
        break;
    }
    case 0b0111011: {// R4-Type
        switch (funct3) {
        case 0b000: {// kAddw, kSubw, kMulw
            switch (funct7) 
            {
            case 0b0000000: {// kAddw
                signals.alu_op = alu::AluOp::kAddw;
                break;
            }
            case 0b0100000: {// kSubw
                signals.alu_op = alu::AluOp::kSubw;
                break;
            }
            case 0b0000001: {// kMulw
                signals.alu_op = alu::AluOp::kMulw;
                break;
            }
            }
            break;
        }
        case 0b001: {// kSllw
            signals.alu_op = alu::AluOp::kSllw;
            break;
        }
        case 0b100: {// kDivw
            switch (funct7) {// kDivw
                case 0b0000001: {// kDivw
                    signals.alu_op = alu::AluOp::kDivw;
                    break;
                }
            }
            break;
        }
        case 0b101: {// kSrlw, kSraw, kDivuw
            switch (funct7) {
                case 0b0000000: {// kSrlw
                    signals.alu_op = alu::AluOp::kSrlw;
                    break;
                }
                case 0b0100000: {// kSraw
                    signals.alu_op = alu::AluOp::kSraw;
                    break;
                }
                case 0b0000001: {// kDivuw
                    signals.alu_op = alu::AluOp::kDivuw;
                    break;
                }
            }
            break;
        }
        case 0b110: {// kRemw
            switch (funct7) 
            {
            case 0b0000001: {// kRemw
                signals.alu_op = alu::AluOp::kRemw;
                break;
            }
            }
            break;
        }
        case 0b111: {// kRemuw
                switch (funct7) {
                    case 0b0000001: {// kRemuw
                        signals.alu_op = alu::AluOp::kRemuw;
                        break;
                    }
                }
                break;
            }
        }
        break;
    }
        default:
            signals.alu_op = alu::AluOp::kNone;
    }
    return signals;
}

