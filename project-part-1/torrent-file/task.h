#pragma once

#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <unordered_map>
#include <cassert>
#include <iostream>

size_t info_start = -1, info_end = -1;

struct TorrentFile {
    std::string announce;
    std::string comment;
    std::vector<std::string> pieceHashes;
    size_t pieceLength;
    size_t length;
    std::string name;
    std::string infoHash;
};

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

std::unordered_map<std::string, TorElement*> readDict(std::ifstream &tstream);

std::string readArr(std::ifstream &tstream) {
    std::string res;
    tstream.seekg(-1, std::ios::cur);
    char cur;
    tstream >> cur;
    int len = 0;
    while(cur != ':') {
        len = len * 10 + cur - '0';
        tstream >> cur;
    }
    for (int i = 0; i < len; ++i) {
        tstream >> cur;
        res.push_back(cur);
    }
    // if (len == 94) exit(0);
    return std::move(res);
}

uint64_t readInt(std::ifstream &tstream) {
    char cur;
    tstream >> cur;
    uint64_t res = 0;
    while(cur != 'e') {
        res = res * 10 + cur - '0';
        tstream >> cur;
    }
    return res;
}

std::vector<TorElement*> readList(std::ifstream &tstream) {

    std::vector<TorElement*> res;
    char cur;
    tstream >> cur;
    while (cur != 'e') {
        // std::cout << cur << ' ' << "List" << std::endl;
        // break;
        TorElement *el = new TorElement;
        if (cur >= '0' && cur <= '9') {
            el->value = readArr(tstream);
        } else if (cur == 'd') {
            el->value = readDict(tstream);
        } else if (cur == 'i') {
            el->value = readInt(tstream);
        } else if (cur == 'l') {
            el->value = readList(tstream);
        }
        res.push_back(el);
        tstream >> cur;
    }
    return std::move(res);
}

std::unordered_map<std::string, TorElement*> readDict(std::ifstream &tstream) {
    std::unordered_map<std::string, TorElement*> res;
    
    bool procKey = true;
    char cur;
    std::string curKey;
    while (cur != 'e') {
        tstream >> cur;
        // std::cout << cur << std::endl;
        if (procKey && cur >= '0' && cur <= '9') {
            curKey = readArr(tstream);
            procKey = false;
        } else {
            TorElement *el = new TorElement;
            if (cur >= '0' && cur <= '9') {
                el->value = readArr(tstream);
            } else if (cur == 'd') {
                if (curKey == "info")
                    info_start = tstream.tellg();
                el->value = readDict(tstream);
                if (curKey == "info")
                    info_end = tstream.tellg();
            } else if (cur == 'i') {
                el->value = readInt(tstream);
            } else if (cur == 'l') {
                el->value = readList(tstream);
            }
            res[curKey] = el;
            curKey.clear();
            procKey = true;
        }
    }
    return std::move(res);
}
/*
 * Функция парсит .torrent файл и загружает информацию из него в структуру `TorrentFile`. Как устроен .torrent файл, можно
 * почитать в открытых источниках (например http://www.bittorrent.org/beps/bep_0003.html).
 * После парсинга файла нужно также заполнить поле `infoHash`, которое не хранится в файле в явном виде и должно быть
 * вычислено. Алгоритм вычисления этого поля можно найти в открытых источника, как правило, там же,
 * где описание формата .torrent файлов.
 * Данные из файла и infoHash будут использованы для запроса пиров у торрент-трекера. Если структура `TorrentFile`
 * была заполнена правильно, то трекер найдет нужную раздачу в своей базе и ответит списком пиров. Если данные неверны,
 * то сервер ответит ошибкой.
 */

 TorrentFile LoadTorrentFile(const std::string& filename) {
    std::ifstream tstream(filename);
    tstream.unsetf(std::ios_base::skipws);
    tstream.seekg(0, std::ios::end);
    size_t info_endd = tstream.tellg();
    tstream.seekg(0, std::ios::beg);
    char cur;
    // std::cout << cur << std::endl;
    TorElement *root = new TorElement;
    while (tstream >> cur) {
        if (cur >= '0' && cur <= '9') {
            root->value = readArr(tstream);
        } else if (cur == 'd') {
            root->value = readDict(tstream);
        } else if (cur == 'i') {
            root->value = readInt(tstream);
        } else if (cur == 'l') {
            root->value = readList(tstream);
        }
    }

    tstream.close();
    TorrentFile tfile;
    std::unordered_map<std::string, TorElement*> d = std::get<1>(root->value);
    tfile.announce = std::get<0>(d["announce"]->value);
    
    tfile.comment = std::get<0>(d["comment"]->value);

    std::unordered_map<std::string, TorElement*> d_info = std::get<1>(d["info"]->value);

    tfile.name = std::get<0>(d_info["name"]->value);
    tfile.length = std::get<3>(d_info["length"]->value);
    std::vector<std::string> pieceHashes;
    std::string pieces = std::get<0>(d_info["pieces"]->value);
    tfile.pieceLength = std::get<3>(d_info["piece length"]->value);
    for (int i = 0; i < pieces.size(); i += tfile.pieceLength) {
        pieceHashes.push_back(pieces.substr(i, tfile.pieceLength));
    }
    tfile.pieceHashes = pieceHashes;
    size_t info_len = info_end - info_start + 1;
    char info[info_len];
    unsigned char hbuf[SHA_DIGEST_LENGTH];
    tstream.open(filename);
    tstream.unsetf(std::ios_base::skipws);
    tstream.seekg(info_start - 1);

    tstream.read(info, info_len);

    SHA1(reinterpret_cast<unsigned char const* >(info), info_len, hbuf);
    std::string info_hash(reinterpret_cast< char const* >(hbuf), SHA_DIGEST_LENGTH);
    tfile.infoHash = info_hash;

    tstream.close();
    return tfile;
}
