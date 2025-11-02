/**
 * @file main_rv5s.cpp
 * @brief Isolated Entry Point for the 5-stage processor mode for Testing
 */

#include "vm/rv5s/rv5s_vm.h"
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
        std::unique_ptr<VmBase> vm = std::make_unique<RV5SVM>();
        LoadMemoryImage(vm.get(), input_file);
        vm->SetSilentMode(true);            // disable all dumps to the global files
        vm->Run();

        // vm->DumpState(output_file);      // not actually required
        DumpRegisters(output_file, vm->registers_);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "RV5SVM Error: " << e.what() << '\n';
        return 1;
    }
}