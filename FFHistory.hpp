#include <vector>
#include "Structs.hpp"

#ifndef FFHISTORY_HPP_INCLUDED
#define FFHISTORY_HPP_INCLUDED

int grabDataCallback(void*, int, char**, char**);
//std::string ntTimeToPrettyDate(int64_t);

class FFHistory {
    public:
        void readFile(bool, char*);
    private:
        void readProfiles(std::string, std::vector<FirefoxProfiles>*);
};

#endif
