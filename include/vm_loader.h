/**
 * @file vm_loader.h
 * @brief Provides helper function for all VM modes to decode the memory log file and write to the VM's memory
 */
 
#ifndef VM_LOADER_H
#define VM_LOADER_H

#include "vm/vm_base.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

void LoadMemoryImage(VmBase* vm, const std::string& image_path) {
    std::ifstream in_file(image_path);
    if (!in_file) {
        throw std::runtime_error("Could not open memory image file: " + image_path);
    }

    std::string line;
    while (std::getline(in_file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        char type;
        uint64_t address;
        uint64_t value;

        ss >> type;
        ss >> std::hex >> address;
        ss >> std::hex >> value;

        switch (type) {
            case 'B':
                vm->memory_controller_.WriteByte(address, static_cast<uint8_t>(value));
                break;
            case 'H':
                vm->memory_controller_.WriteHalfWord(address, static_cast<uint16_t>(value));
                break;
            case 'W':
                vm->memory_controller_.WriteWord(address, static_cast<uint32_t>(value));
                break;
            case 'D':
                vm->memory_controller_.WriteDoubleWord(address, value);
                break;
            case 'P':           // log for program size
                vm->SetProgramSize(address); // address now holds the program size
                break;
            default:
                std::cerr << "Warning: Unknown memory image type: " << type << std::endl;
        }
    }
}

#endif