#include "torrent_file.h"
#include "bencode.h"
#include "byte_tools.h"

#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <sstream>
#include <algorithm>
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
    // tfile.pieceLength = std::get<3>(d_info["piece length"]->value);
    tfile.pieceLength = 20;
    size_t pieceI;
    for (pieceI = 0; pieceI < pieces.size(); pieceI += tfile.pieceLength) {
        tfile.pieceHashes.emplace_back(pieces.substr(pieceI, std::min(tfile.pieceLength, pieces.size() - pieceI)));
    }
    // if (pieces.size() % tfile.pieceLength != 0)
    //     tfile.pieceHashes.emplace_back(pieces.substr(pieceI - tfile.pieceLength));
        
    size_t info_len = Bencode::info_end - Bencode::info_start + 1;
    std::string info;
    info.resize(info_len);

    unsigned char hbuf[SHA_DIGEST_LENGTH];

    tifstream.open(filename);
   
    tifstream.unsetf(std::ios_base::skipws);
    tifstream.seekg(Bencode::info_start - 1);
    tifstream.read(&info[0], info_len);

    tifstream.close();

    tfile.infoHash = CalculateSHA1(info);

    return tfile;
}
