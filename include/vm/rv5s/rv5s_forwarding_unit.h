/**
* @file rv5s_forwarding_unit.h
* @brief Definition of the 5-stage pipelined RISC-V Forwarding Unit
*/
    #ifndef RV5S_FORWARDING_UNIT_H
    #define RV5S_FORWARDING_UNIT_H

    #include "vm/vm_base.h"
    #include "vm/rv5s/pipeline_registers.h"
    
    // Determines where does the ALU operands come from
    enum ForwardSrc {
        FROM_REG,           // Default -> From register file (no data hazard case) -> no forwarding required
        FROM_EX_MEM,        // in case of r-type/i-type instructions
        FROM_MEM_WB         // In case of load use hazards
    };

    class RV5SForwardingUnit {
    public:
        RV5SForwardingUnit() = default;
        ~RV5SForwardingUnit() = default;

        // Returns appropriate enum type to decide where the first and second operand from the ALU should come from
        ForwardSrc getAluSrcA(uint8_t id_rs1_index, const EX_MEM_Reg& ex_mem_reg, const MEM_WB_Reg& mem_wb_reg);
        ForwardSrc getAluSrcB(uint8_t id_rs2_index, const EX_MEM_Reg& ex_mem_reg, const MEM_WB_Reg& mem_wb_reg);
    };

    #endif