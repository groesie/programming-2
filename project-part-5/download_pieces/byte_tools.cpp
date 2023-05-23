#include "byte_tools.h"

#include <openssl/sha.h>
#include <algorithm>

int BytesToInt(std::string_view bytes) {
    // int res = 0;
    // for (int i = 0; i < bytes.size(); ++i) {
    //     res <<= 8;
    //     res += (int)(unsigned char)bytes[i];
    // }
    // return res;
    return static_cast<int>(static_cast<unsigned char>(bytes[0]) << 24 |
                            static_cast<unsigned char>(bytes[1]) << 16 |
                            static_cast<unsigned char>(bytes[2]) << 8 |
                            static_cast<unsigned char>(bytes[3]));
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


std::string IntToBytes(uint32_t num, bool isHost) {
    std::string res;
    uint8_t out[4];
    *(uint32_t*)&out = num;
    if (!isHost)
        std::reverse(out, out + 4);
    for (auto &i: out)
        res += (char) i;
    return res;
}