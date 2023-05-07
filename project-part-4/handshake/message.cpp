#include "message.h"
#include <arpa/inet.h>


std::string Message::ToString() const {
    std::string messageString;
    uint32_t length = htonl(static_cast<uint32_t>(messageLength));
    messageString += reinterpret_cast<const char*>(&length);
    messageString += reinterpret_cast<const char*>(&id);
    messageString += payload;
    return messageString;
}