#include "byte_tools.h"

#include <openssl/sha.h>
#include <algorithm>

int BytesToInt(std::string_view bytes) {
    int res = 0;
    for (int i = 0; i < bytes.size(); ++i) {
        res <<= 8;
        res += (int)(unsigned char)bytes[i];
    }
    return res;
}

std::string CalculateSHA1(const std::string& msg) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)msg.c_str(), msg.length(), hash);

    return std::string((char*)hash, SHA_DIGEST_LENGTH);
}

std::string IntToBytesBigEndian(uint32_t value) {
    std::string result(4, '\0');
    result[0] = (value >> 24) & 0xFF;
    result[1] = (value >> 16) & 0xFF;
    result[2] = (value >> 8) & 0xFF;
    result[3] = value & 0xFF;
    return result;
}

std::string IntToBytesLittleEndian(uint32_t value) {
    std::string result(4, '\0');
    for (int i = 0; i < 4; ++i) {
        result[i] = static_cast<char>(value & 0xFF);
        value >>= 8;
    }
    return result;
}
