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

namespace Bencode2 {

/*
 * В это пространство имен рекомендуется вынести функции для работы с данными в формате bencode.
 * Этот формат используется в .torrent файлах и в протоколе общения с трекером
 */
    struct element;

    using number = long long;
    using string = std::string;
    using list = std::list<element>;
    using dictionary = std::map<string, element>;

    struct element {
        std::variant<number, string, list, dictionary> value;
    };

    class Decoder {
    public:
        explicit Decoder(const std::string& torrent_string);

        element DecodeElement();
    private:
        number DecodeNumber();

        string DecodeString();

        list DecodeList();

        dictionary DecodeDictionary();

        std::string_view torrent_string;
    };

    class Encoder {
    public:
        explicit Encoder();

        std::string EncodeElement(element& element_);
    private:
        static std::string EncodeNumber(number& number_);

        static std::string EncodeString(const string& string_);

        std::string EncodeList(list& list_);

        std::string EncodeDictionary(dictionary& dictionary_);
    };

}
