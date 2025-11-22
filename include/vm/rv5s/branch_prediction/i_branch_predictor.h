/**
* @file i_branch_predictor.h
* @brief Definition of the Interface for all Branch Predictors
*/

#ifndef I_BRANCH_PREDICTOR_H
#define I_BRANCH_PREDICTOR_H

    #include "config.h"
    #include <cstdint>
    #include <iostream>

    class IBranchPredictor {
        public: 
            virtual ~IBranchPredictor() = default;

            // Returns to the VM true if branch is to be taken and false if not to be taken
            virtual bool getPrediction(uint64_t pc) = 0;

            // Updates the state of the predictor, after the actual branch outcome is determined. Also increments the no of mispredictions
            virtual void updateState(uint64_t pc, bool predicted, bool actual_outcome) = 0;

            // Getter for no of mispredictions of the branch
            virtual unsigned int getMispredictions() const = 0;

            // Getter for the branch predictor type
            virtual vm_config::BranchPredictorType getPredictorType() const = 0;
    };

#endif