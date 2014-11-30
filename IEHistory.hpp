#include <vector>
#include <fstream>
#include "Structs.hpp"

#ifndef IEHISTORY_HPP_INCLUDED
#define IEHISTORY_HPP_INCLUDED
class IEHistory {
    public:
        void readFile(bool, char*);
    private:
        inline int32_t readInt32(char*);
        std::vector<int32_t> findTableRecords(std::vector<int32_t>, std::ifstream&);
        std::vector<int32_t> findTableOffsets(int32_t, std::ifstream&);
        void extractRecords(std::vector<int32_t>, std::ifstream &f, int64_t, bool, char*);
        std::string ntTimeToPrettyDate(int64_t);
        inline int64_t readInt64(char*);
        std::string formatLine(std::string);
        void writeIEData(IEHistoryData, std::ofstream&, bool, std::string&);
};
#endif
