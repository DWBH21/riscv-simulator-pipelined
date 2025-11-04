    /**
    * @file rv5s_vm.h
    * @brief Definition of the 5-stage pipelined RISC-V VM with Hazard Detection and Resolution Using Stalls.
    */
    #ifndef RV5S_VM_H
    #define RV5S_VM_H

    #include "vm/vm_base.h"
    #include "vm/rv5s/pipeline_registers.h"
    #include "vm/rv5s/rv5s_control_unit.h"
    #include "vm/rv5s/hazard_unit.h"     // added
    // #include "config.h"                  // see if reqd later 

    #include <cstdint>
    #include <iostream> 
    #include <string>  

    class RV5SVM : public VmBase {
        public: 
            explicit RV5SVM(bool silent = false);
            ~RV5SVM();

            void Run() override;
            void DebugRun() override;
            void Step() override;
            void Undo() override;
            void Redo() override;
            void Reset() override;

            void DumpState(const std::filesystem::path &filename);

            void PrintType() {
                std::cout << "rv5svm" << std::endl;
            }

        private: 
            RV5SControlUnit control_unit_;
            HazardUnit hazard_unit_;           // added hazard unit
            unsigned int no_data_hazards_{};
            unsigned int no_control_hazards_{};

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
    };
    #endif