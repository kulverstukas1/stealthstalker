#ifndef SENDDATA_HPP_INCLUDED
#define SENDDATA_HPP_INCLUDED

class SendData {
    public:
        void transmit(std::string, std::string, std::string, std::string);
        std::string constructBody(std::string, std::string, std::string, std::string);
};

#endif
