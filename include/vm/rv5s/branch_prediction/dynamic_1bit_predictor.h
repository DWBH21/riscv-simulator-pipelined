/**
 * @file dynamic_1bit_predictor.h
 * @brief Defines the 1-bit dynamic branch predictor.
 */

#ifndef DYNAMIC_1BIT_PREDICTOR_H
#define DYNAMIC_1BIT_PREDICTOR_H

#include "vm/rv5s/branch_prediction/i_branch_predictor.h"
#include <map>

class Dynamic1BitPredictor : public IBranchPredictor {
private:
    unsigned int no_mispredictions_ = 0;
    vm_config::BranchPredictorType type_ = vm_config::BranchPredictorType::DYNAMIC_1BIT;
    
    // stores 1 bit for every PC entry -> true for branch taken and false for branch not taken
    std::map<uint64_t, bool> bht_; 

public:
    Dynamic1BitPredictor() = default;

    bool getPrediction(uint64_t pc) override;
    void updateState(uint64_t pc, bool predicted_outcome, bool actual_outcome) override;
    
    unsigned int getMispredictions() const override {
        return no_mispredictions_;
    }
    vm_config::BranchPredictorType getPredictorType() const override {
        return type_;
    }
};

#endif // DYNAMIC_1BIT_PREDICTOR_H