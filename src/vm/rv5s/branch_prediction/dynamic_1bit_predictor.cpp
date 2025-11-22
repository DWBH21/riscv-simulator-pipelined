/**
 * @file dynamic_1bit_predictor.cpp
 * @brief Implementation for the Dynamic 1-Bit Predictor
 */

#include "vm/rv5s/branch_prediction/dynamic_1bit_predictor.h"

bool Dynamic1BitPredictor::getPrediction(uint64_t pc) {
    if(bht_.find(pc) == bht_.end())         // pc not found in bht
        return false;                       // default -> branch not taken

    return bht_[pc];        // return the prediction
}
void Dynamic1BitPredictor::updateState(uint64_t pc, bool predicted_outcome, bool actual_outcome) {
    if (predicted_outcome != actual_outcome) {
        no_mispredictions_++;
    }

    bht_[pc] = actual_outcome;
}
