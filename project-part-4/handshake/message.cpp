#include "message.h"
#include <arpa/inet.h>
#include <sstream>


std::string Message::ToString() const {
    // std::string messageString;
    // uint32_t length = htonl(static_cast<uint32_t>(messageLength));
    // messageString += reinterpret_cast<const char*>(&length);
    // messageString += reinterpret_cast<const char*>(&id);
    // messageString += payload;
    // return messageString;

    std::stringstream buf;
    char* msgLen = (char*) &messageLength;
    for (int i = 0; i < 4; ++i)
        buf << (char) msgLen[3-i];
    buf << (char) id;
    buf << payload;
    return buf.str();

}
