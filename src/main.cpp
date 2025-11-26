#include "main.h"
#include "assembler/assembler.h"
#include "utils.h"
#include "globals.h"
#include "vm/rvss/rvss_vm.h"
#include "vm/rv5s/rv5s_vm.h"
#include "vm/rv5s/rv5s_ex_vm.h"
#include "vm/rv5s/rv5s_id_vm.h"
#include "vm_runner.h"
#include "command_handler.h"
#include "config.h"

#include <iostream>
#include <thread>
#include <bitset>
#include <regex>

// Helper to initialize a new VM object from the changed config modes
std::unique_ptr<VmBase> initializeVm() {
    std::unique_ptr<VmBase> vm;
    vm_config::VmTypes vmType = vm_config::config.getVmType();

    if (vmType == vm_config::VmTypes::SINGLE_STAGE) {
        std::cout << "Initializing Single-Stage VM..." << std::endl;
        vm = std::make_unique<RVSSVM>();
    } else {
        vm_config::DataHazardMode hazardMode = vm_config::config.getDataHazardMode();
        vm_config::BranchStage branch_stage = vm_config::config.getBranchStage();

        if (hazardMode == vm_config::DataHazardMode::IDEAL) {
            std::cout << "Initializing 5-Stage Pipeline VM (Ideal Mode)..." << std::endl;
            vm = std::make_unique<RV5SVM>(); 
        } else {
            if (branch_stage == vm_config::BranchStage::BRANCH_IN_EX) {
                
                std::cout << "Initializing 5-Stage Pipeline VM (Branch in EX)..." << std::endl;
                auto rv5s_vm = std::make_unique<RV5SEXVM>();

                rv5s_vm->setBranchPredictorType(vm_config::config.getBranchPredictorType());
                
                bool forwarding_enabled = (hazardMode == vm_config::DataHazardMode::FORWARDING);
                rv5s_vm->enableForwarding(forwarding_enabled);
                vm = std::move(rv5s_vm);
            }
            else if (branch_stage == vm_config::BranchStage::BRANCH_IN_ID) {
                
                std::cout << "Initializing 5-Stage Pipeline VM (Branch in ID)..." << std::endl;
                auto rv5s_vm = std::make_unique<RV5SIDVM>();

                rv5s_vm->setBranchPredictorType(vm_config::config.getBranchPredictorType());
                
                bool forwarding_enabled = (hazardMode == vm_config::DataHazardMode::FORWARDING);
                rv5s_vm->enableForwarding(forwarding_enabled);
                vm = std::move(rv5s_vm);
            } else {
                std::cerr << "Error main.cpp: Combination of requested Pipeline Modes not supported." << std::endl;
                vm = std::make_unique<RVSSVM>();
            }
        }
    }
    return vm;
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    std::cerr << "No arguments provided. Use --help for usage information.\n";
    return 1;
  }

  std::unique_ptr<VmBase> vm_ptr; // Pointer to the vm_base class
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--help" || arg == "-h") {
        std::cout << "Usage: " << argv[0] << " [options]\n"
                  << "Options:\n"
                  << "  --help, -h                             Show this help message\n"
                  << "  --assemble <file>                      Assemble the specified file\n"
                  << "  --config <section> <key> <value>       Modify vm config\n"
                  << "  --run <file>                           Run the specified file\n"
                  << "  --verbose-errors                       Enable verbose error printing\n"
                  << "  --start-vm                             Start the VM with the default program\n"
                  << "  --start-vm --vm-as-backend             Start the VM with the default program in backend mode\n";
        return 0;

    } else if (arg == "--config") {
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
    } else if (arg == "--assemble") {
        if (++i >= argc) {
            std::cerr << "Error: No file specified for assembly.\n";
            return 1;
        }
        try {
            AssembledProgram program = assemble(argv[i]);
            std::cout << "Assembled program: " << program.filename << '\n';
            return 0;
        } catch (const std::runtime_error& e) {
            std::cerr << e.what() << '\n';
            return 1;
        }

    } else if (arg == "--run") {
        if (++i >= argc) {
            std::cerr << "Error: No file specified to run.\n";
            return 1;
        }
        try {
            vm_ptr = initializeVm();
            AssembledProgram program = assemble(argv[i]);
            RVSSVM vm;
            vm_ptr->LoadProgram(program);
            vm_ptr->Run();
            std::cout << "Program running: " << program.filename << '\n';
            return 0;
        } catch (const std::runtime_error& e) {
            std::cerr << e.what() << '\n';
            return 1;
        }

    } else if (arg == "--verbose-errors") {
        globals::verbose_errors_print = true;
        std::cout << "Verbose error printing enabled.\n";

    } else if (arg == "--vm-as-backend") {
        globals::vm_as_backend = true;
        std::cout << "VM backend mode enabled.\n";
    } else if (arg == "--start-vm") {
        break;      // enters interactive mode

    } else {
        std::cerr << "Unknown option: " << arg << '\n';
        return 1;
    }
  }
  


  setupVmStateDirectory();

  // initializing the vm_ptr on the basis of the config file
  vm_ptr = initializeVm();
  AssembledProgram program;
  RVSSVM vm;
  // try {
  //   program = assemble("/home/vis/Desk/codes/assembler/examples/ntest1.s");
  // } catch (const std::runtime_error &e) {
  //   std::cerr << e.what() << '\n';
  //   return 0;
  // }

  // std::cout << "Program: " << program.filename << std::endl;

  // unsigned int count = 0;
  // for (const uint32_t &instruction : program.text_buffer) {
  //     std::cout << std::bitset<32>(instruction)
  //               << " | "
  //               << std::setw(8) << std::setfill('0') << std::hex << instruction
  //               << " | "
  //               << std::setw(0) << count
  //               << std::dec << "\n";
  //     count += 4;
  // }

  // vm_ptr->LoadProgram(program);
  

  std::cout << "VM_STARTED" << std::endl;
  // std::cout << globals::invokation_path << std::endl;

  std::thread vm_thread;
  bool vm_running = false;

  auto launch_vm_thread = [&](auto fn) {
    if (vm_thread.joinable()) {       // clean up the previous run
      vm_ptr->RequestStop();   
      vm_thread.join();
    }
    vm_running = true;
    vm_thread = std::thread([&]() {     // creates a new thread and passes the fn to execute provided as the argument
      fn();               
      vm_running = false;               // after thread function is completed.
    });
  };



  std::string command_buffer;
  while (true) {
    // std::cout << "=> ";
    std::getline(std::cin, command_buffer);
    command_handler::Command command = command_handler::ParseCommand(command_buffer);

    if (command.type==command_handler::CommandType::MODIFY_CONFIG) {
      if (command.args.size() != 3) {
        std::cout << "VM_MODIFY_CONFIG_ERROR" << std::endl;
        continue;
      }
      try {
        vm_config::config.modifyConfig(command.args[0], command.args[1], command.args[2]);
        std::cout << "VM_MODIFY_CONFIG_SUCCESS" << std::endl;
        
        vm_ptr = initializeVm();

        // Checking if the program from the previous load exists
        if (!program.filename.empty()) {
            std::cout << "Re-loading existing program: " << program.filename << std::endl;
            vm_ptr->LoadProgram(program);
        }

      } catch (const std::exception &e) {
        std::cout << "VM_MODIFY_CONFIG_ERROR" << std::endl;
        std::cerr << e.what() << '\n';
        continue;
      }
      continue;
    }



    if (command.type==command_handler::CommandType::LOAD) {
      try {
        program = assemble(command.args[0]);
        std::cout << "VM_PARSE_SUCCESS" << std::endl;
        vm_ptr->output_status_ = "VM_PARSE_SUCCESS";
        vm_ptr->DumpState(globals::vm_state_dump_file_path);
      } catch (const std::runtime_error &e) {
        std::cout << "VM_PARSE_ERROR" << std::endl;
        vm_ptr->output_status_ = "VM_PARSE_ERROR";
        vm_ptr->DumpState(globals::vm_state_dump_file_path);
        std::cerr << e.what() << '\n';
        continue;
      }
      vm_ptr->LoadProgram(program);
      std::cout << "Program loaded: " << command.args[0] << std::endl;
    } else if (command.type==command_handler::CommandType::RUN) {
      launch_vm_thread([&]() { vm_ptr->Run(); });
    } else if (command.type==command_handler::CommandType::DEBUG_RUN) {
      launch_vm_thread([&]() { vm_ptr->DebugRun(); });
    } else if (command.type==command_handler::CommandType::STOP) {
      vm_ptr->RequestStop();
      std::cout << "VM_STOPPED" << std::endl;
      vm_ptr->output_status_ = "VM_STOPPED";
      vm_ptr->DumpState(globals::vm_state_dump_file_path);
    } else if (command.type==command_handler::CommandType::STEP) {
      if (vm_running) continue;               // if vm is running an instruction, dont step
      launch_vm_thread([&]() { vm_ptr->Step(); });

    } else if (command.type==command_handler::CommandType::UNDO) {
      if (vm_running) continue;
      vm_ptr->Undo();
    } else if (command.type==command_handler::CommandType::REDO) {
      if (vm_running) continue;
      vm_ptr->Redo();
    } else if (command.type==command_handler::CommandType::RESET) {
      vm_ptr->Reset();
      program = AssembledProgram();
      vm_ptr->output_status_ = "VM_RESET";
      vm_ptr->DumpState(globals::vm_state_dump_file_path);
    } else if (command.type==command_handler::CommandType::EXIT) {
      vm_ptr->RequestStop();
      if (vm_thread.joinable()) vm_thread.join(); // ensure clean exit      // if thread is running, join it with main such that main finishes only after thread finishes
      vm_ptr->output_status_ = "VM_EXITED";
      vm_ptr->DumpState(globals::vm_state_dump_file_path);
      break;
    } else if (command.type==command_handler::CommandType::ADD_BREAKPOINT) {
      vm_ptr->AddBreakpoint(std::stoul(command.args[0], nullptr, 10));
    } else if (command.type==command_handler::CommandType::REMOVE_BREAKPOINT) {
      vm_ptr->RemoveBreakpoint(std::stoul(command.args[0], nullptr, 10));
    } else if (command.type==command_handler::CommandType::MODIFY_REGISTER) {
      try {
        if (command.args.size() != 2) {
          std::cout << "VM_MODIFY_REGISTER_ERROR" << std::endl;
          continue;
        }
        std::string reg_name = command.args[0];
        uint64_t value = std::stoull(command.args[1], nullptr, 16);
        vm_ptr->ModifyRegister(reg_name, value);
        DumpRegisters(globals::registers_dump_file_path, vm_ptr->registers_);
        std::cout << "VM_MODIFY_REGISTER_SUCCESS" << std::endl;
      } catch (const std::out_of_range &e) {
        std::cout << "VM_MODIFY_REGISTER_ERROR" << std::endl;
        continue;
      } catch (const std::exception& e) {
        std::cout << "VM_MODIFY_REGISTER_ERROR" << std::endl;
        continue;
      }
    } else if (command.type==command_handler::CommandType::GET_REGISTER) {
      std::string reg_str = command.args[0];
      if (reg_str[0] == 'x') {
        std::cout << "VM_REGISTER_VAL_START";
        std::cout << "0x"
                  << std::hex
                  << vm_ptr->registers_.ReadGpr(std::stoi(reg_str.substr(1))) 
                  << std::dec;
        std::cout << "VM_REGISTER_VAL_END"<< std::endl;
      } 
    }

  
    else if (command.type==command_handler::CommandType::MODIFY_MEMORY) {
      if (command.args.size() != 3) {
        std::cout << "VM_MODIFY_MEMORY_ERROR" << std::endl;
        continue;
      }
      try {
        uint64_t address = std::stoull(command.args[0], nullptr, 16);
        std::string type = command.args[1];
        uint64_t value = std::stoull(command.args[2], nullptr, 16);

        if (type == "byte") {
          vm_ptr->memory_controller_.WriteByte(address, static_cast<uint8_t>(value));
        } else if (type == "half") {
          vm_ptr->memory_controller_.WriteHalfWord(address, static_cast<uint16_t>(value));
        } else if (type == "word") {
          vm_ptr->memory_controller_.WriteWord(address, static_cast<uint32_t>(value));
        } else if (type == "double") {
          vm_ptr->memory_controller_.WriteDoubleWord(address, value);
        } else {
          std::cout << "VM_MODIFY_MEMORY_ERROR" << std::endl;
          continue;
        }
        std::cout << "VM_MODIFY_MEMORY_SUCCESS" << std::endl;
      } catch (const std::out_of_range &e) {
        std::cout << "VM_MODIFY_MEMORY_ERROR" << std::endl;
        continue;
      } catch (const std::exception& e) {
        std::cout << "VM_MODIFY_MEMORY_ERROR" << std::endl;
        continue;
      }
    }
    
    
    
    else if (command.type==command_handler::CommandType::DUMP_MEMORY) {
      try {
        vm_ptr->memory_controller_.DumpMemory(command.args);
      } catch (const std::out_of_range &e) {
        std::cout << "VM_MEMORY_DUMP_ERROR" << std::endl;
        continue;
      } catch (const std::exception& e) {
        std::cout << "VM_MEMORY_DUMP_ERROR" << std::endl;
        continue;
      }
    } else if (command.type==command_handler::CommandType::PRINT_MEMORY) {
      for (size_t i = 0; i < command.args.size(); i+=2) {
        uint64_t address = std::stoull(command.args[i], nullptr, 16);
        uint64_t rows = std::stoull(command.args[i+1]);
        vm_ptr->memory_controller_.PrintMemory(address, rows);
      }
      std::cout << std::endl;
    } else if (command.type==command_handler::CommandType::GET_MEMORY_POINT) {
      if (command.args.size() != 1) {
        std::cout << "VM_GET_MEMORY_POINT_ERROR" << std::endl;
        continue;
      }
      // uint64_t address = std::stoull(command.args[0], nullptr, 16);
      vm_ptr->memory_controller_.GetMemoryPoint(command.args[0]);
    } 


    else if (command.type==command_handler::CommandType::VM_STDIN) {
      vm_ptr->PushInput(command.args[0]);
    }
    
    
    else if (command.type==command_handler::CommandType::DUMP_CACHE) {
      std::cout << "Cache dumped." << std::endl;
    } else {
      std::cout << "Invalid command.";
      std::cout << command_buffer << std::endl;
    }

  }






  return 0;
}