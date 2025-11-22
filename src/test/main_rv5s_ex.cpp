/**
 * @file main_rv5s_ex.cpp
 * @brief Isolated Entry Point for the 5-stage processor mode (with Branch Comparison in the EX stage) for Testing
 */

#include "vm/rv5s/rv5s_ex_vm.h"
#include "vm_loader.h"
#include "utils.h"
#include "config.h"
#include <iostream>
#include <string>

using vm_config::BranchPredictorType;

int main(int argc, char *argv[]) {
    if (argc != 4 || std::string(argv[2]) != "-o") {
        std::cerr << "Usage: " << argv[0] << " <input.memimg> -o <output.json>\n";
        return 1;
    }
    std::string input_file = argv[1];
    std::string output_file = argv[3];

    try {
        std::unique_ptr<VmBase> vm = std::make_unique<RV5SEXVM>(true);
        LoadMemoryImage(vm.get(), input_file);
        RV5SEXVM* ptr = static_cast<RV5SEXVM*>(vm.get());
        ptr->setBranchPredictorType(BranchPredictorType::STATIC_NOT_TAKEN);
        ptr->enableForwarding(false);
        ptr->Run();

        uint64_t data_addr = vm_config::config.getDataSectionStart();
        ptr->DumpFinalState(output_file, data_addr);
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "RV5SEXVM Error: " << e.what() << '\n';
        return 1;
    }
}