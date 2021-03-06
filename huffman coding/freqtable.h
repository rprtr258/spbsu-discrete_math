#pragma once
#include <utility>
#include <vector>

typedef std::pair<char, int unsigned> CharOccur;

class FrequencyTable {
    private:
        CharOccur *data = nullptr;
        int unsigned size = 0;
    public:
        FrequencyTable(char const *filename);
        ~FrequencyTable();
        
        CharOccur operator[](int unsigned const index);
        
        int unsigned getSize();
        void erase();
};
