/**
 * @file dynamic_2bit_predictor.h
 * @brief Defines the 2-bit dynamic branch predictor.
 */

#ifndef DYNAMIC_2BIT_PREDICTOR_H
#define DYNAMIC_2BIT_PREDICTOR_H

#include "vm/rv5s/branch_prediction/i_branch_predictor.h"
#include <map>

class Dynamic2BitPredictor : public IBranchPredictor {
private:
    enum class State : uint8_t {
        TWO_NOT_TAKEN = 0, // 00
        ONE_NOT_TAKEN = 1, // 01
        ONE_TAKEN = 2, // 10
        TWO_TAKEN = 3  // 11
    };

    unsigned int no_mispredictions_ = 0;
    vm_config::BranchPredictorType type_ = vm_config::BranchPredictorType::DYNAMIC_2BIT;

    std::map<uint64_t, State> bht_; // Branch History Table with a state in every PC entry

public:
    Dynamic2BitPredictor() = default;

    bool getPrediction(uint64_t pc) override;    
    void updateState(uint64_t pc, bool predicted, bool actual_outcome) override;
    
    unsigned int getMispredictions() const override {
        return no_mispredictions_;
    }
    vm_config::BranchPredictorType getPredictorType() const override {
        return type_;
    }
};

#endif // DYNAMIC_2BIT_PREDICTOR_H