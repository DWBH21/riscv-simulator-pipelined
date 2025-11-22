/**
* @file rv5s_hazard_unit.h
* @brief Definition of the 5-stage pipelined RISC-V Hazard Unit
*/
    #ifndef RV5S_HAZARD_UNIT_H
    #define RV5S_HAZARD_UNIT_H

    #include "vm/vm_base.h"
    #include "vm/rv5s/pipeline_registers.h"
    
    class RV5SHazardUnit {
    public:
        RV5SHazardUnit() = default;
        ~RV5SHazardUnit() = default;

        // checks for any hazard in general 
        bool detectHazard(ControlSignals signals, uint8_t rs1_index, uint8_t rs2_index, ID_EX_Reg id_ex_reg, EX_MEM_Reg ex_mem_reg);
        
        // checks for data hazards
        bool detectDataHazard(ControlSignals signals, uint8_t rs1_index, uint8_t rs2_index, ID_EX_Reg id_ex_reg, EX_MEM_Reg ex_mem_reg);

        // check for load use data hazard specifically 
        bool detectLoadUseHazard(ControlSignals signals, uint8_t rs1_index, uint8_t rs2_index, ID_EX_Reg id_ex_reg);

        // checks for control hazards
        bool detectControlHazard(ControlSignals signals);

    };

    #endif