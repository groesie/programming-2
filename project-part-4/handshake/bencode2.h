#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <memory>

namespace Bencode {
    struct TorElement {
        std::variant<
            std::string,
            std::unordered_map<std::string, std::shared_ptr<TorElement>>,
            std::vector<std::shared_ptr<TorElement>>,
            uint64_t
        > value;
        // ~TorElement() {
        //     if (std::holds_alternative<std::vector<TorElement*>>(value)) {
        //         for (auto i: std::get<std::vector<TorElement*>>(value)) {
        //             delete i;
        //         }
        //     } else if (std::holds_alternative<std::unordered_map<std::string, TorElement*>>(value)) {
        //         std::unordered_map<std::string, TorElement*> my_map = std::get<std::unordered_map<std::string, TorElement*>>(value);
        //         std::for_each (my_map.begin(), my_map.end(), [](auto item) -> void
        //         {
        //         delete item.second;
        //         });
        //     }
        // }
    };
    extern size_t info_start, info_end;

    std::unordered_map<std::string, std::shared_ptr<TorElement>> readDict(std::stringstream &tstream);

    std::string readArr(std::stringstream &tstream);

    uint64_t readInt(std::stringstream &tstream);

    std::vector<std::shared_ptr<TorElement>> readList(std::stringstream &tstream);

    std::shared_ptr<TorElement> getRoot(std::stringstream &tstream);
}