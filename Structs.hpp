#ifndef STRUCTS_HPP_INCLUDED
#define STRUCTS_HPP_INCLUDED
struct OperaHistoryData {
    std::string title;
    std::string url;
    std::string datetime;
    std::string visits;
};

struct IEHistoryData {
    std::string url;
    std::string datetime;
};

struct SkypeProfiles {
    std::string profileName;
    std::string fullPath;
};

struct FirefoxProfiles {
    std::string profileName;
    std::string profilePath;
};

struct ChromeProfiles {
    std::string profileName;
    std::string fullPath;
};
#endif
