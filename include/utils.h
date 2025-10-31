/**
 * @file utils.h
 * @brief Contains utility functions for file I/O and string manipulation.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef UTILS_H
#define UTILS_H

#include "assembler/parser.h"
#include "vm/registers.h"
#include "vm/vm_base.h"
#include "vm_asm_mw.h"
#include "vm/rv5s/pipeline_registers.h" 

#include <string>
#include <filesystem>

void setupVmStateDirectory();

/**
 * @brief Counts the number of lines in a given file.
 * 
 * @param filename The name of the file to count lines in.
 * @return The number of lines in the file.
 * @throws std::ios_base::failure If the file cannot be opened.
 */
int64_t CountLines(const std::string &filename);

/**
 * @brief Retrieves a specific line from a file.
 * 
 * @param fileName The name of the file to read from.
 * @param lineNumber The 1-based index of the line to retrieve.
 * @return The content of the specified line as a string.
 * @throws std::out_of_range If the lineNumber is invalid.
 * @throws std::ios_base::failure If the file cannot be opened.
 */
std::string GetLineFromFile(const std::string &fileName, unsigned int lineNumber);

/**
 * @brief Parses a string containing escaped characters into its unescaped form.
 * 
 * @param input The input string with escaped characters.
 * @return The unescaped string.
 */
std::string ParseEscapedString(const std::string &input);

void DumpErrors(const std::filesystem::path &filename, const std::vector<ParseError> &errors);

void DumpNoErrors(const std::filesystem::path &filename);

void DumpRegisters(const std::filesystem::path &filename, RegisterFile &register_file);

void DumpDisasssembly(const std::filesystem::path &filename, AssembledProgram &program);

void SetupConfigFile();

/**
 * @brief Dumps the state of all 5-stage pipeline registers to a JSON stream.
 * @param file The output file stream to write to.
 * @param if_id The IF/ID pipeline register.
 * @param id_ex The ID/EX pipeline register.
 * @param ex_mem The EX/MEM pipeline register.
 * @param mem_wb The MEM/WB pipeline register.
 */
void DumpPipelineRegisters(
    std::ofstream &file,
    const IF_ID_Reg &if_id,
    const ID_EX_Reg &id_ex,
    const EX_MEM_Reg &ex_mem,
    const MEM_WB_Reg &mem_wb
);

#endif // UTILS_H
