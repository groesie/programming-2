#include "message.h"


std::string Message::ToString() const {
    std::string messageString;
    messageString += (messageLength >> 24) & 0xFF;
    messageString += (messageLength >> 16) & 0xFF;
    messageString += (messageLength >> 8) & 0xFF;
    messageString += messageLength & 0xFF;
    messageString += static_cast<uint8_t>(id);
    messageString += payload;
    return messageString;
}