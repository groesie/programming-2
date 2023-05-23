#include "message.h"
#include "byte_tools.h"

#include <arpa/inet.h>
#include <sstream>


std::string Message::ToString() const {
    std::string messageString;

    messageString += IntToBytesBigEndian(messageLength);
    messageString += (char) id;
    messageString += payload;

    return messageString;
}
