/**
 * @file main_rvss.cpp
 * @brief Isolated Entry Point for the single stage processor mode for Testing
 */

#include "vm/rvss/rvss_vm.h"
#include "vm_loader.h"
#include "utils.h"
#include "config.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc != 4 || std::string(argv[2]) != "-o") {
        std::cerr << "Usage: " << argv[0] << " <input.memimg> -o <output.json>\n";
        return 1;
    }
    std::string input_file = argv[1];
    std::string output_file = argv[3];

    try {
        std::unique_ptr<VmBase> vm = std::make_unique<RVSSVM>(true);
        LoadMemoryImage(vm.get(), input_file);
        vm->Run();

        uint64_t data_addr = vm_config::config.getDataSectionStart();
        vm->DumpFinalState(output_file, data_addr);
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "RVSSVM Error: " << e.what() << '\n';
        return 1;
    }
}