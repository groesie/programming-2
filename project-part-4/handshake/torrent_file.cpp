#include "torrent_file.h"
#include "bencode.h"
#include "byte_tools.h"
#include <variant>

TorrentFile LoadTorrentFile(const std::string& filename) {
    TorrentFile torrentFile;

    std::ifstream file(filename);
    std::stringstream fileStringStream;
    fileStringStream << file.rdbuf();

    std::string torrentString = fileStringStream.str();

    Bencode2::Decoder decoder(torrentString);

    auto dict = std::get<Bencode2::dictionary>(decoder.DecodeElement().value);
    torrentFile.announce = std::get<Bencode2::string>(dict["announce"].value);
    torrentFile.comment = std::get<Bencode2::string>(dict["comment"].value);

    auto info = dict["info"];
    auto infoDict = std::get<Bencode2::dictionary>(info.value);
    torrentFile.length = std::get<Bencode2::number>(infoDict["length"].value);
    torrentFile.name = std::get<Bencode2::string>(infoDict["name"].value);
    torrentFile.pieceLength = std::get<Bencode2::number>(infoDict["piece length"].value);

    std::string pieces = std::get<Bencode2::string>(infoDict["pieces"].value);
    size_t propertyStart = 0;
    size_t i = pieces.size();
    while(i > 0) {
        if (i < 20) {
            torrentFile.pieceHashes.push_back(pieces.substr(propertyStart, i));
            propertyStart += i;
        }
        else {
            torrentFile.pieceHashes.push_back(pieces.substr(propertyStart, 20));
            propertyStart += 20;
        }
        i -= 20;
    }

    Bencode2::Encoder encoder;

    std::string infoString = encoder.EncodeElement(info);

    torrentFile.infoHash = CalculateSHA1(infoString);

    return torrentFile;
}