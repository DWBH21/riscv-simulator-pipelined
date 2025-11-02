/**
 * @file main_rvss.cpp
 * @brief Isolated Entry Point for the single stage processor mode for Testing
 */

#include "vm/rvss/rvss_vm.h"
#include "vm_loader.h"
#include "utils.h"
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
        std::unique_ptr<VmBase> vm = std::make_unique<RVSSVM>();
        LoadMemoryImage(vm.get(), input_file);
        vm->SetSilentMode(true);            // disable all dumps to the global files
        vm->Run();

        // vm->DumpState(output_file);      // should already be done in the vm->Run() function, but output file path needs to be changed in globals ?
        DumpRegisters(output_file, vm->registers_);     // not actually required
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "RVSSVM Error: " << e.what() << '\n';
        return 1;
    }
}