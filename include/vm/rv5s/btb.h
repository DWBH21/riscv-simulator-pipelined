/**
 * @file bth.h
 * @brief Definition of the Branch Target Buffer for use in the RV5SIDVM class 
 */

#ifndef BTB_H
#define BTB_H
    #include <unordered_map>

    struct BTBEntry {
        bool valid = false;
        uint64_t target_address = 0;
    };

    class BranchTargetBuffer {
    private:
        std::unordered_map<uint64_t, BTBEntry> table; 

    public:
        // Returns {hit, target}
        std::pair<bool, uint64_t> lookup(uint64_t pc) {
            if (table.find(pc) != table.end() && table[pc].valid) {
                return {true, table[pc].target_address};
            }
            return {false, 0};
        }

        // Called by Decode stage to update the BTB
        void update(uint64_t pc, uint64_t target) {
            table[pc] = {true, target};
        }
        
        void reset() {
            table.clear();
        }
    };

#endif