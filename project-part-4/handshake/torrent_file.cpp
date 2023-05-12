#include "torrent_file.h"
#include "bencode.h"
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <sstream>

#include <string>



TorrentFile LoadTorrentFile(const std::string& filename) {
    std::ifstream tifstream(filename);
    std::stringstream tstream;
  
    tstream << tifstream.rdbuf();    
    tifstream.close();
   
    tstream.unsetf(std::ios_base::skipws);
    tstream.seekg(std::ios::beg);
    Bencode::TorElemPtr root = Bencode::getRoot(tstream);

    TorrentFile tfile;

    Bencode::Dict d = std::get<1>(root->value);
    tfile.announce = std::get<0>(d["announce"]->value);
    tfile.comment = std::get<0>(d["comment"]->value);

    Bencode::Dict d_info = std::get<1>(d["info"]->value);

    tfile.name = std::get<0>(d_info["name"]->value);
    tfile.length = std::get<3>(d_info["length"]->value);
    std::vector<std::string> pieceHashes;
    std::string pieces = std::get<0>(d_info["pieces"]->value);
    tfile.pieceLength = std::get<3>(d_info["piece length"]->value);

    for (int i = 0; i < pieces.size(); i += tfile.pieceLength) {
        pieceHashes.push_back(pieces.substr(i, tfile.pieceLength));
    }

    tfile.pieceHashes = pieceHashes;
    size_t info_len = Bencode::info_end - Bencode::info_start + 1;
    char info[info_len];
    unsigned char hbuf[SHA_DIGEST_LENGTH];

    tifstream.open(filename);
   
    tifstream.unsetf(std::ios_base::skipws);
    tifstream.seekg(Bencode::info_start - 1);
    tifstream.read(info, info_len);

    tifstream.close();

    SHA1(reinterpret_cast<unsigned char const* >(info), info_len, hbuf);

    std::string info_hash(reinterpret_cast< char const* >(hbuf), SHA_DIGEST_LENGTH);
    tfile.infoHash = info_hash;

    // delete root;

    return tfile;
}