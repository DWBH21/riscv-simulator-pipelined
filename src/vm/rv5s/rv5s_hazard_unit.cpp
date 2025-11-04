/**
* @file rv5s_hazard_unit.h
* @brief Implementation of the 5-stage pipelined RISC-V Hazard Unit
*/

#include "vm/rv5s/rv5s_hazard_unit.h"
#include "vm/rv5s/pipeline_registers.h"
#include <iostream>

using instruction_type::BranchOp;
using instruction_type::AluSrcA;
using instruction_type::WriteBackSrc;

bool RV5SHazardUnit::detectHazard(ControlSignals signals, uint8_t rs1_index, uint8_t rs2_index, ID_EX_Reg id_ex_reg, EX_MEM_Reg ex_mem_reg) {
    return detectControlHazard(signals) || detectDataHazard(signals, rs1_index, rs2_index, id_ex_reg, ex_mem_reg);
}
    
bool RV5SHazardUnit::detectDataHazard(ControlSignals signals, uint8_t rs1_index, uint8_t rs2_index, ID_EX_Reg id_ex_reg, EX_MEM_Reg ex_mem_reg) {
    
    bool rs1_reqd = false, rs2_reqd = false;
    // checking if rs1 is required by the instruction or not
    if(signals.alu_src_a == AluSrcA::ALU_SRC_A_RS1) {
        rs1_reqd = true;
    }
    else if(signals.branch_op == BranchOp::JALR) {
        rs1_reqd = true;
    }

    // checking if rs2 is required by the instruction or not
    if(signals.alu_src_b != 0 && signals.reg_write && signals.wb_src == WriteBackSrc::WB_FROM_ALU) {     // r type arithmetic instruction 
        rs2_reqd = true;
    }
    else if(signals.mem_write) {                            // s type instruction
        rs2_reqd = true;    
    } 
    else if(signals.branch && signals.branch_op!=BranchOp::JAL && signals.branch_op!=BranchOp::JALR) {  // normal branch instructions
        rs2_reqd = true;
    }  

    // check for hazard with instruction at EX stage
    if(id_ex_reg.control.reg_write && id_ex_reg.rd_index != 0) {
        if((rs1_reqd && id_ex_reg.rd_index == rs1_index)  || (rs2_reqd && id_ex_reg.rd_index == rs2_index)) 
            return true;
    }

    // check for hazard with instruction at MEM stage
    if(ex_mem_reg.control.reg_write && ex_mem_reg.rd_index != 0) {
        if((rs1_reqd && ex_mem_reg.rd_index == rs1_index)  || (rs2_reqd && ex_mem_reg.rd_index == rs2_index)) 
            return true;
    }

    return false;
}

bool RV5SHazardUnit::detectControlHazard(ControlSignals signals) {
    if(signals.branch)
        return true;

    if(signals.branch_op == BranchOp::JAL || signals.branch_op == BranchOp::JALR)   // hv to check if reqd
        return true;

    return false;
}