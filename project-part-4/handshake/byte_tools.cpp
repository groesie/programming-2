#include "byte_tools.h"

#include <openssl/sha.h>

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
    // unsigned char buffer[20];
    // SHA1((unsigned char*)msg.c_str(), msg.length(), buffer);

    // std::string hash;

    // for (auto &c: buffer)
    //     hash += char(c);

    // return hash;
}
