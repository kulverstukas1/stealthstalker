#include <vector>
#include "Structs.hpp"

#ifndef CHROMEHISTORY_HPP_INCLUDED
#define CHROMEHISTORY_HPP_INCLUDED

int grabChromeDataCallback(void*, int, char**, char**);
//std::string chromeTimeToPrettyDate(int64_t);

class ChromeHistory {
    public:
        void readFile(bool, char*);
    private:
        std::string constructFullPath();
        void probeFolders(std::string, std::vector<ChromeProfiles>*);
};

#endif
