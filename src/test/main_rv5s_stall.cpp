/**
 * @file main_rv5s_stall.cpp
 * @brief Isolated Entry Point for the 5-stage processor mode (stalls only) for Testing
 */

#include "vm/rv5s/rv5s_stall_vm.h"
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
        std::unique_ptr<VmBase> vm = std::make_unique<RV5SStallVM>(true);
        LoadMemoryImage(vm.get(), input_file);
        vm->Run();

        uint64_t data_addr = vm_config::config.getDataSectionStart();
        vm->DumpFinalState(output_file, data_addr);
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "RV5SStallVM Error: " << e.what() << '\n';
        return 1;
    }
}