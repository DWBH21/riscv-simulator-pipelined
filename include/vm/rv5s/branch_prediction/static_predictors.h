/**
* @file static_predictors.h
* @brief Definition and Implementation of the Static Branch Predictors: Taken and Not Taken cases
*/

#ifndef STATIC_PREDICTORS_H
#define STATIC_PREDICTORS_H

    #include "config.h"
    #include "vm/rv5s/branch_prediction/i_branch_predictor.h"
    #include <cstdint>
    #include <iostream>

    class StaticTakenPredictor : public IBranchPredictor {
    private: 
            unsigned int no_mispredictions_ = 0;
            vm_config::BranchPredictorType type_ = vm_config::BranchPredictorType::STATIC_TAKEN;
    public: 
        bool getPrediction(uint64_t /*pc*/) override {
            return false;  
        }
    
        void updateState(uint64_t /*pc*/, bool predicted_outcome, bool actual_outcome) override {
            if(predicted_outcome != actual_outcome) 
                no_mispredictions_++;
        }
        unsigned int getMispredictions() const override {
            return no_mispredictions_;
        }

        vm_config::BranchPredictorType getPredictorType() const override {
            return type_;
        }
    };

    class StaticNotTakenPredictor : public IBranchPredictor {
    private: 
            unsigned int no_mispredictions_ = 0;
            vm_config::BranchPredictorType type_ = vm_config::BranchPredictorType::STATIC_NOT_TAKEN;
    public: 
        bool getPrediction(uint64_t /*pc*/) override {
            return true;
        }
    
        void updateState(uint64_t /*pc*/, bool predicted_outcome, bool actual_outcome) override {
            if(predicted_outcome != actual_outcome)
                no_mispredictions_++;
        }
        unsigned int getMispredictions() const override {
            return no_mispredictions_;
        }

        vm_config::BranchPredictorType getPredictorType() const override {
            return type_;
        }
    };
#endif