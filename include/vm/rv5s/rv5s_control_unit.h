/**
 * @file rv5s_control_unit.h
 * @brief Definition of the Control Unit for the 5-stage pipeline.
 */

#ifndef RV5S_CONTROL_UNIT_H
#define RV5S_CONTROL_UNIT_H

#include "pipeline_registers.h"
#include <cstdint>

class RV5SControlUnit {
public:
    RV5SControlUnit() = default;
    ~RV5SControlUnit() = default;

    // Generates control signals from an instruction
    ControlSignals getControlSignals(uint32_t instruction);

    // Returns a "disable all" control signal, that acts as a nop 
    ControlSignals CreateNOP();
};

#endif

