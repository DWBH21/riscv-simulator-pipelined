/**
 * @file main_assembler.cpp
 * @brief Isolated Assembler for Testing.
 * It reads a .s file, assembles it and converts the AssembledProgram struct into a file that can be decoded by the different VMs.
 * This file logs the memory_controller_.write operations that the VmBase::LoadProgram functions performs to populate the text and data segments in memory
 * The AssembledProgram is too complex to store directly into a file. This approach takes advantage of the fact that the assembly of the program and then loading it into memory is identical for all VM modes
 */

#include "assembler/assembler.h"
#include "config.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring> // For memcpy

int main(int argc, char *argv[]) {
    if (argc != 4 || std::string(argv[2]) != "-o") {
        std::cerr << "Usage: " << argv[0] << " <input.s> -o <output.memimg>\n";
        return 1;
    }
    
    std::string input_file = argv[1];
    std::string output_file = argv[3];

    try {
        AssembledProgram program = assemble(input_file);
        std::ofstream out_file(output_file);
        out_file << std::hex << std::setfill('0');

        // Write .text segment (from VmBase::LoadProgram)
        unsigned int counter = 0;
        for (const auto &instruction : program.text_buffer) {
            out_file << "W 0x" << counter << " 0x" << std::setw(8) << instruction << '\n';
            counter += 4;
        }

        // Write .data segment (from VmBase::LoadProgram)
        unsigned int data_counter = 0;
        uint64_t base_data_address = vm_config::config.getDataSectionStart();
        auto align = [&](unsigned int alignment) {
            if (data_counter % alignment != 0)
                data_counter += alignment - (data_counter % alignment);
        };

        for (const auto& data : program.data_buffer) {
            std::visit([&](auto&& value) {
                using T = std::decay_t<decltype(value)>;
                uint64_t addr = base_data_address + data_counter;

                if constexpr (std::is_same_v<T, uint8_t>) {
                    align(1); addr = base_data_address + data_counter;
                    out_file << "B 0x" << addr << " 0x" << std::setw(2) << (int)value << '\n';
                    data_counter += 1;
                } else if constexpr (std::is_same_v<T, uint16_t>) {
                    align(2); addr = base_data_address + data_counter;
                    out_file << "H 0x" << addr << " 0x" << std::setw(4) << value << '\n';
                    data_counter += 2;
                } else if constexpr (std::is_same_v<T, uint32_t>) {
                    align(4); addr = base_data_address + data_counter;
                    out_file << "W 0x" << addr << " 0x" << std::setw(8) << value << '\n';
                    data_counter += 4;
                } else if constexpr (std::is_same_v<T, uint64_t>) {
                    align(8); addr = base_data_address + data_counter;
                    out_file << "D 0x" << addr << " 0x" << std::setw(16) << value << '\n';
                    data_counter += 8;
                } else if constexpr (std::is_same_v<T, float>) {
                    align(4); addr = base_data_address + data_counter;
                    uint32_t f_as_i; std::memcpy(&f_as_i, &value, sizeof(float));
                    out_file << "W 0x" << addr << " 0x" << std::setw(8) << f_as_i << '\n';
                    data_counter += 4;
                } else if constexpr (std::is_same_v<T, double>) {
                    align(8); addr = base_data_address + data_counter;
                    uint64_t d_as_i; std::memcpy(&d_as_i, &value, sizeof(double));
                    out_file << "D 0x" << addr << " 0x" << std::setw(16) << d_as_i << '\n';
                    data_counter += 8;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    align(1); addr = base_data_address + data_counter;
                    for (size_t i = 0; i < value.size(); i++) {
                        out_file << "B 0x" << (addr + i)
                                 << " 0x" << std::setw(2) << (int)static_cast<uint8_t>(value[i]) << '\n';
                        data_counter += 1;
                    }
                }
            }, data);
        }
        out_file << "P 0x" << counter << " 0x0\n";    // writing the program_size_ as an address at the end of the file
        out_file.close();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Assembler Error: " << e.what() << '\n';
        return 1;
    }
}