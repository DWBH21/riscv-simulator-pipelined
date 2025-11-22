/**
 * @file dynamic_2bit_predictor.cpp
 * @brief Implementation for the Dynamic 2-Bit Predictor
 */

#include "vm/rv5s/branch_prediction/dynamic_2bit_predictor.h"

bool Dynamic2BitPredictor::getPrediction(uint64_t pc) {
    if(bht_.find(pc) == bht_.end())         // pc not found in bht
        return false;                       // default -> branch not taken

    State state = bht_[pc];
    return (state == State::TWO_TAKEN) || (state == State::ONE_TAKEN);      
}
void Dynamic2BitPredictor::updateState(uint64_t pc, bool predicted_outcome, bool actual_outcome) {
    if (predicted_outcome != actual_outcome) {
        no_mispredictions_++;
    }

    State state = bht_[pc];
    if(bht_.find(pc) == bht_.end()) {        // pc not found in bht
        bht_[pc] = State::TWO_NOT_TAKEN;
    }

    // updating state
    if(actual_outcome) {            // branch was taken
        switch (state) {
            case State::TWO_NOT_TAKEN: state = State::ONE_NOT_TAKEN; break;
            case State::ONE_NOT_TAKEN:   state = State::ONE_TAKEN; break;
            case State::ONE_TAKEN:       state = State::TWO_TAKEN; break;
            case State::TWO_TAKEN:     break;
        }  
    }  
    else {                          // branch was not taken
        switch (state) {
            case State::TWO_TAKEN:     state = State::ONE_TAKEN; break;
            case State::ONE_TAKEN:       state = State::ONE_NOT_TAKEN; break;
            case State::ONE_NOT_TAKEN:   state = State::TWO_NOT_TAKEN; break;
            case State::TWO_NOT_TAKEN:   break;
        }
    }
    bht_[pc] = state;
}
