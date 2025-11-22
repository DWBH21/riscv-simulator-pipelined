/**
* @file rv5s_forwarding_unit.h
* @brief Implementation of the 5-stage pipelined RISC-V Forwarding Unit
*/

#include "vm/rv5s/rv5s_forwarding_unit.h"
#include "vm/rv5s/pipeline_registers.h"

ForwardSrc RV5SForwardingUnit::getAluSrcA(uint8_t id_rs1_index, const EX_MEM_Reg& ex_mem_reg, const MEM_WB_Reg& mem_wb_reg) {
    if (id_rs1_index == 0) {
        return ForwardSrc::FROM_REG;
    }
    if (ex_mem_reg.control.reg_write && ex_mem_reg.rd_index == id_rs1_index) {
        return ForwardSrc::FROM_EX_MEM;
    }   
    if (mem_wb_reg.control.reg_write && mem_wb_reg.rd_index == id_rs1_index) {
        return ForwardSrc::FROM_MEM_WB;
    }
    return ForwardSrc::FROM_REG;
}

ForwardSrc RV5SForwardingUnit::getAluSrcB(uint8_t id_rs2_index, const EX_MEM_Reg& ex_mem_reg, const MEM_WB_Reg& mem_wb_reg) {
    if (id_rs2_index == 0) {
        return ForwardSrc::FROM_REG;
    }
    if (ex_mem_reg.control.reg_write && ex_mem_reg.rd_index == id_rs2_index) {
        return ForwardSrc::FROM_EX_MEM;
    }   
    if (mem_wb_reg.control.reg_write && mem_wb_reg.rd_index == id_rs2_index) {
        return ForwardSrc::FROM_MEM_WB;
    }
    return ForwardSrc::FROM_REG;
}