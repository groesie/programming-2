#include "bencode.h"

namespace Bencode {
    size_t info_start = -1, info_end = -1;
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

    std::vector<std::shared_ptr<TorElement>> readList(std::stringstream &tstream) {

        std::vector<std::shared_ptr<TorElement>> res;
        char cur;
        tstream >> cur;
        while (cur != 'e') {

            std::shared_ptr<TorElement> el = std::shared_ptr<TorElement>(new TorElement);
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

    std::unordered_map<std::string, std::shared_ptr<TorElement>> readDict(std::stringstream &tstream) {
        std::unordered_map<std::string, std::shared_ptr<TorElement>> res;
        
        bool procKey = true;
        char cur;
        std::string curKey;
        while (cur != 'e') {
            tstream >> cur;

            if (procKey && cur >= '0' && cur <= '9') {
                curKey = readArr(tstream);
                procKey = false;
            } else {
                std::shared_ptr<TorElement> el = std::shared_ptr<TorElement>(new TorElement);
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

    std::shared_ptr<TorElement> getRoot(std::stringstream &tstream) {
        char cur;
        std::shared_ptr<TorElement> root = std::shared_ptr<TorElement>(new TorElement);

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
        return root;
    }
}
