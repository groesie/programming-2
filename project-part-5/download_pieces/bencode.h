#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <vector>
#include <sstream>
#include <cassert>
#include <unordered_map>
#include <charconv>
#include <iostream>
#include <memory>


namespace Bencode {
    struct TorElement {
        std::variant<
            std::string,
            std::unordered_map<std::string, std::shared_ptr<TorElement>>,
            std::vector<std::shared_ptr<TorElement>>,
            uint64_t
        > value;
    };
    using TorElemPtr = std::shared_ptr<Bencode::TorElement>;
    using Dict = std::unordered_map<std::string, std::shared_ptr<TorElement>>;
    using Array = std::string;
    using Int = uint64_t;
    using List = std::vector<std::shared_ptr<TorElement>>;

    extern size_t info_start, info_end;

    Dict readDict(std::stringstream &tstream);

    Array readArr(std::stringstream &tstream);

    Int readInt(std::stringstream &tstream);

    List readList(std::stringstream &tstream);

    TorElemPtr getRoot(std::stringstream &tstream);
}

