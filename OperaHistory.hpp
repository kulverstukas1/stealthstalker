#include "Structs.hpp"

int grabOperaDataCallback(void *NotUsed, int argc, char **argv, char **azColName);

class OperaHistory {
    public:
        void detectOperaVersion(bool, char*);
    private:
        void readNewFile(bool, char*);
        void readOldFile(bool, char*);
        void writeToControlFile(std::string, std::string);
        std::string getPrevMillis(std::string);
        std::string convertMillisToDatetime(std::string);
        void writeOperaData(OperaHistoryData, std::ofstream&, bool, std::string&);
};
