/**
* @file rv5s_ex_vm.h
* @brief Definition of the 5-stage pipelined RISC-V VM where Branch Comparison happens in the EX stage
*/

#ifndef RV5S_EX_VM_H
#define RV5S_EX_VM_H

    #include "vm/vm_base.h"
    #include "vm/rv5s/pipeline_registers.h"
    #include "vm/rv5s/rv5s_control_unit.h"
    #include "vm/rv5s/rv5s_hazard_unit.h"
    #include "vm/rv5s/rv5s_forwarding_unit.h"
    #include "vm/rv5s/branch_prediction/i_branch_predictor.h"
    #include "config.h"                  // see if reqd later 

    #include <cstdint>
    #include <iostream> 
    #include <string>  

    class RV5SEXVM : public VmBase {
        public: 
            explicit RV5SEXVM(bool silent = false);
            ~RV5SEXVM();

            void Run() override;
            void DebugRun() override;
            void Step() override;
            void Undo() override;
            void Redo() override;
            void Reset() override;

            void DumpState(const std::filesystem::path &filename);
            void enableForwarding(bool enable);                     // to change config during testing
            void setBranchPredictorType(vm_config::BranchPredictorType type);

            void PrintType() {
                std::cout << "rv5s_ex_vm" << std::endl;
            }

        private: 
            RV5SControlUnit control_unit_;
            RV5SHazardUnit hazard_unit_;
            RV5SForwardingUnit forwarding_unit_;
            std::unique_ptr<IBranchPredictor> branch_predictor_;        // pointer to the base class IBranchPredictor

            bool stall_request_ = false;            // to indicate a stall signal from the decode stage
            bool flush_pipeline_ = false;           // to flush pipeline in case of branch taken in branch instructions
            bool forwarding_enabled_ = false;

            IF_ID_Reg if_id_reg_{};             // pipeline registers to hold state at beginning of a clock cycle
            ID_EX_Reg id_ex_reg_{};
            EX_MEM_Reg ex_mem_reg_{};
            MEM_WB_Reg mem_wb_reg_{};
            
            IF_ID_Reg next_if_id_reg_{};             // pipeline registers to hold state during a clock cycle and will be stored at the end
            ID_EX_Reg next_id_ex_reg_{};
            EX_MEM_Reg next_ex_mem_reg_{};
            MEM_WB_Reg next_mem_wb_reg_{};
            
            void Fetch_Stage();                     //  pipeline stage functions 
            void Decode_Stage();
            void Execute_Stage();
            void Memory_Stage();
            void WriteBack_Stage();

            template <typename RegType> 
            RegType CreateBubble() {                // function to create a bubble of an register type
                RegType bubble;
                bubble.is_valid = false;
                return bubble;
            }

            uint64_t getWriteBackData();            // to forward the correct data that wlil be written back to register file
    };
    #endif