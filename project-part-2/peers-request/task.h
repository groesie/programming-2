#pragma once

#include "peer.h"
#include "torrent_file.h"
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <cpr/cpr.h>
#include <iostream>

size_t info_start = -1, info_end = -1;

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

std::unordered_map<std::string, TorElement*> readDict(std::stringstream &tstream);

std::string readArr(std::stringstream &tstream) {
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

uint64_t readInt(std::stringstream &tstream) {
    char cur;
    tstream >> cur;
    uint64_t res = 0;
    while(cur != 'e') {
        res = res * 10 + cur - '0';
        tstream >> cur;
    }
    return res;
}

std::vector<TorElement*> readList(std::stringstream &tstream) {

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

std::unordered_map<std::string, TorElement*> readDict(std::stringstream &tstream) {
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

TorElement *getRoot(std::stringstream &tstream) {
    char cur;
    TorElement *root = new TorElement;
    // std::cout << "cur" << std::endl;
    while (tstream >> cur) {
        // std::cout << cur << std::endl;
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
    return root;
}

class TorrentTracker {
public:
    /*
     * url - адрес трекера, берется из поля announce в .torrent-файле
     */
    TorrentTracker(const std::string& url) : url_(url) {
        // std::cout << url_ << std::endl;
    };

    /*
     * Получить список пиров у трекера и сохранить его для дальнейшей работы.
     * Запрос пиров происходит посредством HTTP GET запроса, данные передаются в формате bencode.
     * Такой же формат использовался в .torrent файле.
     * Подсказка: посмотрите, что было написано в main.cpp в домашнем задании torrent-file
     *
     * tf: структура с разобранными данными из .torrent файла из предыдущего домашнего задания.
     * peerId: id, под которым представляется наш клиент.
     * port: порт, на котором наш клиент будет слушать входящие соединения (пока что мы не слушаем и на этот порт никто
     *  не сможет подключиться).
     */
    void UpdatePeers(const TorrentFile& tf, std::string peerId, int port) {
        cpr::Response res = cpr::Get(
            cpr::Url{url_},
            cpr::Parameters {
                    {"info_hash", tf.infoHash},
                    {"peer_id", peerId},
                    {"port", std::to_string(port)},
                    {"uploaded", std::to_string(0)},
                    {"downloaded", std::to_string(0)},
                    {"left", std::to_string(tf.length)},
                    {"compact", std::to_string(1)}
            },
            cpr::Timeout{20000}
        );
        std::string tdata(std::move(res.text));

        // std::cout << tdata << std::endl;

        std::stringstream tstream(tdata);
        tstream.unsetf(std::ios_base::skipws);

        // std::cout <<  "-----" << std::endl;
        TorElement *root = getRoot(tstream);

        // std::cout <<  "-----" << std::endl;
        std::unordered_map<std::string, TorElement*> d = std::get<1>(root->value);
        // std::cout << d.size() << "-----" << std::endl;
        std::stringstream peerstream(std::move(std::get<0>(d["peers"]->value)));

        unsigned char cur_byte;
        while (peerstream >> cur_byte) {
            Peer peer;
            std::string ip[4];

            uint8_t byte;
            std::stringstream ss;
            ss << cur_byte;
            ss >> std::hex >> byte;
            ip[0] = std::to_string(byte);

            for (int chunk_i = 1; chunk_i < 4; ++chunk_i) {
                peerstream >> cur_byte;
                ss << cur_byte;
                ss >> std::hex >> byte;
                ip[chunk_i] = std::to_string(byte);
            }
            peer.ip = ip[0] + "." + ip[1] + "." + ip[2] + "." + ip[3];
            // std::cout << "<--- ";
            int port;
            peerstream >> cur_byte;
            // std::cout << cur_byte << ' ';
            ss << cur_byte;
            ss >> std::hex >> byte;
            port = byte * 256;
            peerstream >> cur_byte;
            // std::cout << cur_byte << ' ';
            ss << cur_byte;
            ss >> std::hex >> byte;
            port += byte;
            
            peer.port = port;


            // std::cout << peer.port << "--->" << std::endl;

            peers_.emplace_back(std::move(peer));
        }
    }

    /*
     * Отдает полученный ранее список пиров
     */
    const std::vector<Peer>& GetPeers() const {
        return peers_;
    }

private:
    std::string url_;
    std::vector<Peer> peers_;
};

TorrentFile LoadTorrentFile(const std::string& filename) {
    std::ifstream tifstream(filename);
    std::stringstream tstream;
    if (tifstream) {
        tstream << tifstream.rdbuf();    
        tifstream.close();
    } else {
        //TODO
    }
    tstream.unsetf(std::ios_base::skipws);
    tstream.seekg(std::ios::beg);
    TorElement *root = getRoot(tstream);

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
    // tstream.open(filename);
    tifstream.open(filename);
    // std::stringstream tstream;
    // if (tifstream) {
    //     tstream << tifstream.rdbuf();    
    //     tifstream.close();
    // } else {
    //     //TODO
    // }
    // tstream.seekg(std::ios::beg);
    tifstream.unsetf(std::ios_base::skipws);
    tifstream.seekg(info_start - 1);

    tifstream.read(info, info_len);
    // std::cout << info[0] << "---" << std::endl;
    SHA1(reinterpret_cast<unsigned char const* >(info), info_len, hbuf);

    std::string info_hash(reinterpret_cast< char const* >(hbuf), SHA_DIGEST_LENGTH);
    tfile.infoHash = info_hash;

    tifstream.close();
    return tfile;
}