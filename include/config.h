/**
 * @file config.h
 * @brief Contains configuration options for the assembler.
 * @author Vishank Singh, https://github.com/VishankSingh
 */
#ifndef CONFIG_H
#define CONFIG_H

#include "globals.h"
#include <string>
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <fstream>
#include <sstream>

/**
 * @namespace vm_config
 * @brief Namespace for VM configuration management.
 */
namespace vm_config {

// Top level choice of VM
enum class VmTypes {
  SINGLE_STAGE,
  MULTI_STAGE
};

// Defines the Data Hazard Resolution Mode
enum class DataHazardMode {
    IDEAL,                          // No hazard detection (uses RV5SVM)
    STALL_ONLY,                     // Hazard Detection + Stalling (uses RV5SEXVM, no forwarding)
    FORWARDING                      // Forwarding + Stalls (uses RV5SEXVM, forwarding)
};

// Defines the Control Hazard (Branch Prediction) Resolution Mode 
enum class BranchPredictorType {
    STATIC_NOT_TAKEN,             // assume branch not taken
    STATIC_TAKEN,                 // assume branch taken
    DYNAMIC_1BIT,
    DYNAMIC_2BIT,
    TOURNAMENT                 // will add later
};

// Defines in which stage the branch comparison will occur
enum class BranchStage {
    BRANCH_IN_EX,               // Branch result decided in EX stage (using ALU)
    BRANCH_IN_ID                // Branch result decided in ID stage (using early comparator)
};


struct VmConfig {
  VmTypes vm_type = VmTypes::SINGLE_STAGE;
  
  DataHazardMode data_hazard_mode = DataHazardMode::IDEAL;
  BranchPredictorType branch_predictor_type = BranchPredictorType::STATIC_NOT_TAKEN;
  BranchStage branch_stage = BranchStage::BRANCH_IN_EX;

  uint64_t run_step_delay = 300;
  uint64_t memory_size = 0xffffffffffffffff; // 64-bit address space
  uint64_t memory_block_size = 1024; // 1 KB blocks
  uint64_t data_section_start = 0x10000000; // Default start address for data section
  uint64_t text_section_start = 0x0; // Default start address for text section
  uint64_t bss_section_start = 0x11000000; // Default start address for BSS section

  uint64_t instruction_execution_limit = 100;

  bool m_extension_enabled = true;
  bool f_extension_enabled = true;
  bool d_extension_enabled = true;

  void setVmType(const VmTypes &type) {
    if (type != vm_type) {
        vm_type = type;
        std::cout << "Processor type set to: " << (type == VmTypes::SINGLE_STAGE ? "single_stage" : "multi_stage") << std::endl;
        
        // resetting to default settings
        if (type == VmTypes::SINGLE_STAGE) {
            data_hazard_mode = DataHazardMode::IDEAL;
            branch_predictor_type = BranchPredictorType::STATIC_NOT_TAKEN;
            branch_stage = BranchStage::BRANCH_IN_EX;
        }
    }
  }

  void setDataHazardMode(const DataHazardMode &mode) {
    data_hazard_mode = mode;
    std::cout << "Data Hazard Mode set to: ";
    switch(mode) {
      case DataHazardMode::IDEAL: std::cout << "Ideal (No Hazard Detection)"; break;
      case DataHazardMode::STALL_ONLY: std::cout << "Stall Only"; break;
      case DataHazardMode::FORWARDING: std::cout << "Forwarding"; break;
    }
    std::cout << std::endl;
  }

  void setBranchPredictorType(const BranchPredictorType &type) {
    branch_predictor_type = type;
    std::cout << "Branch Predictor set to: ";
    switch(type) {
      case BranchPredictorType::STATIC_NOT_TAKEN: std::cout << "Static (Assume Not Taken)"; break;
      case BranchPredictorType::STATIC_TAKEN: std::cout << "Static (Assume Taken)"; break;
      case BranchPredictorType::DYNAMIC_1BIT: std::cout << "Dynamic (1-bit)"; break;
      case BranchPredictorType::DYNAMIC_2BIT: std::cout << "Dynamic (2-bit)"; break;
      case BranchPredictorType::TOURNAMENT: std::cout << "Tournament"; break;
    }
    std::cout << std::endl;
  }

  void setBranchStage(const BranchStage &stage) {
      branch_stage = stage;
      std::cout << "Branch Stage set to: ";
      switch(stage) {
          case BranchStage::BRANCH_IN_EX: std::cout << "Branch Compare in EX Stage"; break;
          case BranchStage::BRANCH_IN_ID: std::cout << "Branch Compare in ID Stage"; break;
      }
      std::cout << std::endl;
  }

  VmTypes getVmType() const {
    return vm_type;
  }
  DataHazardMode getDataHazardMode() const {
    return data_hazard_mode;
  }
  BranchPredictorType getBranchPredictorType() const {
    return branch_predictor_type; 
  }
  BranchStage getBranchStage() const {
    return branch_stage;
  }

  void setRunStepDelay(uint64_t delay) {
    run_step_delay = delay;
    std::cout << "Run step delay set to: " << run_step_delay << " ms" << std::endl;
  }
  uint64_t getRunStepDelay() const {
    return run_step_delay;
  }
  void setMemorySize(uint64_t size) {
    memory_size = size;
  }
  uint64_t getMemorySize() const {
    return memory_size;
  }
  void setMemoryBlockSize(uint64_t size) {
    memory_block_size = size;
  }
  uint64_t getMemoryBlockSize() const {
    return memory_block_size;
  }
  void setDataSectionStart(uint64_t start) {
    data_section_start = start;
  }
  uint64_t getDataSectionStart() const {
    return data_section_start;
  }

  void setTextSectionStart(uint64_t start) {
    text_section_start = start;
  }

  uint64_t getTextSectionStart() const {
    return text_section_start;
  }

  void setBssSectionStart(uint64_t start) {
    bss_section_start = start;
  }

  uint64_t getBssSectionStart() const {
    return bss_section_start;
  }

  void setInstructionExecutionLimit(uint64_t limit) {
    instruction_execution_limit = limit;
  }

  uint64_t getInstructionExecutionLimit() const {
    return instruction_execution_limit;
  }

  void setMExtensionEnabled(bool enabled) {
    m_extension_enabled = enabled;
  }

  bool getMExtensionEnabled() const {
    return m_extension_enabled;
  }

  void setFExtensionEnabled(bool enabled) {
    f_extension_enabled = enabled;
  }

  bool getFExtensionEnabled() const {
    return f_extension_enabled;
  }

  void setDExtensionEnabled(bool enabled) {
    d_extension_enabled = enabled;
  }

  bool getDExtensionEnabled() const {
    return d_extension_enabled;
  }

  void modifyConfig(const std::string &section, const std::string &key, const std::string &value) {
    if (section == "Execution") {
      if (key == "processor_type") {
        if (value == "single_stage") {
          setVmType(VmTypes::SINGLE_STAGE);
        } else if (value == "multi_stage") {
          setVmType(VmTypes::MULTI_STAGE);
        } else {
          throw std::invalid_argument("Unknown VM type: " + value);
        }
      } else if (key == "run_step_delay") {
        setRunStepDelay(std::stoull(value));
      } else if (key == "instruction_execution_limit") {
        setInstructionExecutionLimit(std::stoull(value));
      }

      // Config Options for RV5S:
      
      else if (key == "data_hazard_mode") {
        if (vm_type == VmTypes::SINGLE_STAGE) {
            throw std::invalid_argument("Cannot set data_hazard_mode when processor_type is single_stage.");
        }

        if (value == "ideal") {
            setDataHazardMode(DataHazardMode::IDEAL);
            // branch predictor and branch_stage set to default
            setBranchPredictorType(BranchPredictorType::STATIC_NOT_TAKEN);
            setBranchStage(BranchStage::BRANCH_IN_EX);
        }
        else if (value == "stall") {
            setDataHazardMode(DataHazardMode::STALL_ONLY);
            // branch predictor set to default
            setBranchPredictorType(BranchPredictorType::STATIC_NOT_TAKEN);
        }
        else if (value == "forwarding") {
            setDataHazardMode(DataHazardMode::FORWARDING);
        }
        else throw std::invalid_argument("Unknown data_hazard_mode: " + value);
      }
      else if (key == "branch_predictor") {
        if (vm_type == VmTypes::SINGLE_STAGE) {
            throw std::invalid_argument("Cannot set branch_predictor when processor_type is single_stage.");
        }
        if (data_hazard_mode == DataHazardMode::IDEAL) {
            throw std::invalid_argument("Cannot set branch_predictor when data_hazard_mode is 'ideal'.");
        }
        // if (data_hazard_mode == DataHazardMode::STALL_ONLY) {
        //     throw std::invalid_argument("Cannot set branch_predictor when data_hazard_mode is 'stall'. Locked to 'static_not_taken'.");
        // }

        // In Forwarding Mode: 
        if (value == "static_not_taken") setBranchPredictorType(BranchPredictorType::STATIC_NOT_TAKEN);
        else if (value == "static_taken") setBranchPredictorType(BranchPredictorType::STATIC_TAKEN);
        else if (value == "dynamic_1bit") setBranchPredictorType(BranchPredictorType::DYNAMIC_1BIT);
        else if (value == "dynamic_2bit") setBranchPredictorType(BranchPredictorType::DYNAMIC_2BIT);
        else if (value == "tournament") setBranchPredictorType(BranchPredictorType::TOURNAMENT);
        else throw std::invalid_argument("Unknown branch_predictor: " + value);
      }
      else if (key == "branch_stage") {
        if (vm_type == VmTypes::SINGLE_STAGE) {
            throw std::invalid_argument("Cannot set branch_stage when processor_type is single_stage.");
        }
        if (data_hazard_mode == DataHazardMode::IDEAL) {
            throw std::invalid_argument("Cannot set branch_stage when data_hazard_mode is 'ideal'.");
        }

        if (value == "ex") {
            setBranchStage(BranchStage::BRANCH_IN_EX);
        }
        else if (value == "id") {
            throw std::invalid_argument("BranchStage 'id' is not yet supported.");
        }
        else throw std::invalid_argument("Unknown branch_stage: " + value);
      }
      else {
        throw std::invalid_argument("Unknown key in Execution section: " + key);
      }
    } else if (section == "Memory") {
      if (key == "memory_size") {
        setMemorySize(std::stoull(value));
      } else if (key == "memory_block_size") {
        setMemoryBlockSize(std::stoull(value));
      } else if (key == "data_section_start") {
        setDataSectionStart(std::stoull(value, nullptr, 16));
      } else if (key == "text_section_start") {
        setTextSectionStart(std::stoull(value, nullptr, 16));
      } else if (key == "bss_section_start") {
        setBssSectionStart(std::stoull(value, nullptr, 16));
      }
      else {
        throw std::invalid_argument("Unknown key in Memory section: " + key);
      }
    } 
    else if (section == "Assembler") {
      if (key == "m_extension_enabled") {
        setMExtensionEnabled(value == "true");
      } else if (key == "f_extension_enabled") {
        setFExtensionEnabled(value == "true");
      } else if (key == "d_extension_enabled") {
        setDExtensionEnabled(value == "true");
      }
      else {
        throw std::invalid_argument("Unknown key in Assembler section: " + key);
      }
    }
    else {
      throw std::invalid_argument("Unknown section: " + section);
    }
  }


};

extern VmConfig config;


} // namespace vm_config


#endif // CONFIG_H