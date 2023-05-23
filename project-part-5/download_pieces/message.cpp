#include "message.h"
#include "byte_tools.h"

#include <arpa/inet.h>
#include <sstream>


std::string Message::ToString() const {
    // std::string messageString;

    // messageString += IntToBytesBigEndian(messageLength);
    // messageString += (char) id;
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

