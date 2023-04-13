#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <unordered_map>

namespace Bencode {
    struct TorElement {
        TorElement *parent;
        std::vector<TorElement*> childs;
        std::variant<
            std::string,
            std::unordered_map<std::string, TorElement*>,
            std::vector<TorElement*>,
            uint64_t
        > value;
    };
    extern size_t info_start, info_end;

    std::unordered_map<std::string, TorElement*> readDict(std::stringstream &tstream);

    std::string readArr(std::stringstream &tstream);

    uint64_t readInt(std::stringstream &tstream);

    std::vector<TorElement*> readList(std::stringstream &tstream);

    TorElement *getRoot(std::stringstream &tstream);
}
