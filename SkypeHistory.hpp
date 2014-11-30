#include <vector>
#include "Structs.hpp"

#ifndef SKYPEHISTORY_HPP_INCLUDED
#define SKYPEHISTORY_HPP_INCLUDED

int grabSkypeDataCallback(void*, int, char**, char**);

class SkypeHistory {
    public:
        void readFile(bool, char*);
        std::string convertMillisToDatetime(long int);
    private:
        void probeFolders(std::string, std::vector<SkypeProfiles>*);
};

#endif
