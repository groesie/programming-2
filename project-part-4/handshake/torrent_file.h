#pragma once

#include "bencode.h"

#include <string>
#include <vector>
#include <memory>

struct TorrentFile {
    std::string announce;
    std::string comment;
    std::vector<std::string> pieceHashes;
    size_t pieceLength;
    size_t length;
    std::string name;
    std::string infoHash;
};

TorrentFile LoadTorrentFile(const std::string& filename);

using TorElemPtr = std::shared_ptr<Bencode::TorElement>;