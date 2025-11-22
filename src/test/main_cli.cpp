/**
 * @file main_rv5s_cli.cpp
 * @brief Isolated Entry Point for testing all the 5-stage processor modes
 */

#include "vm/rvss/rvss_vm.h"
#include "vm/rv5s/rv5s_vm.h"
#include "vm/rv5s/rv5s_ex_vm.h"
#include "vm_loader.h"
#include "utils.h"
#include "config.h"
#include <iostream>
#include <string>

// Helper to initialize a new VM object from the changed config modes
std::unique_ptr<VmBase> initializeVm() {
    std::unique_ptr<VmBase> vm;
    vm_config::VmTypes vmType = vm_config::config.getVmType();

    if (vmType == vm_config::VmTypes::SINGLE_STAGE) {
        vm = std::make_unique<RVSSVM>(true);
    } else {
        vm_config::DataHazardMode hazardMode = vm_config::config.getDataHazardMode();
        vm_config::BranchStage branch_stage = vm_config::config.getBranchStage();

        if (hazardMode == vm_config::DataHazardMode::IDEAL) {
            vm = std::make_unique<RV5SVM>(true); 
        } else {
            if (branch_stage == vm_config::BranchStage::BRANCH_IN_EX) {                
                auto rv5s_vm = std::make_unique<RV5SEXVM>(true);
                rv5s_vm->setBranchPredictorType(vm_config::config.getBranchPredictorType());
                
                bool forwarding_enabled = (hazardMode == vm_config::DataHazardMode::FORWARDING);
                rv5s_vm->enableForwarding(forwarding_enabled);
                vm = std::move(rv5s_vm);
            } else {
                std::cerr << "Error main_cli.cpp: Combination of requested Pipeline Modes not supported." << std::endl;
                return nullptr;
            }
        }
    }
    return vm;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input.memimg> -o <output.json> [--config Sec Key Val]...\n";
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Error: -o requires a filename\n";
                return 1;
            }
            output_file = argv[++i];
        } 
        else if (arg == "--config") {
            if (i + 3 >= argc) {
                std::cerr << "Error: --config requires 3 arguments: <SECTION> <KEY> <VALUE>\n";
                return 1;
            }
            std::string section = argv[++i];
            std::string key = argv[++i];
            std::string value = argv[++i];

            try {
                vm_config::config.modifyConfig(section, key, value);
            } catch (const std::exception &e) {
                std::cerr << "Configuration Error: " << e.what() << '\n';
                return 1;
            }
        }
    }
    try {
        std::unique_ptr<VmBase> vm = initializeVm();
        if (!vm) {
            std::cerr << "Error: Failed to initialize VM (Invalid Config combination?)\n";
            return 1;
        }
        
        LoadMemoryImage(vm.get(), input_file);
        vm->Run();

        uint64_t data_addr = vm_config::config.getDataSectionStart();
        vm->DumpFinalState(output_file, data_addr);
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "MAIN CLI Error: " << e.what() << '\n';
        return 1;
    }
}