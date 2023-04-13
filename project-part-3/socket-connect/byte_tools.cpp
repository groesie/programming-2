#include "byte_tools.h"

int BytesToInt(std::string_view bytes) {
    int res = 0;
    for (int i = 0; i < bytes.size(); ++i) {
        res <<= 8;
        res += (int)(unsigned char)bytes[i];
    }
    return res;
}
