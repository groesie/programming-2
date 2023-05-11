#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <cassert>
#include <stdexcept>

using namespace std::chrono_literals;


void PeerConnect::Run() {
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer" << std::endl;
            MainLoop();
        } else {
            std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
    }
}


bool PeerConnect::EstablishConnection() {
    try {
        PerformHandshake();
        ReceiveBitfield();
        SendInterested();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to establish connection with peer " << socket_.GetIp() << ":" <<
            socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
}


void PeerConnect::Terminate() {
    std::cerr << "Terminate" << std::endl;
    terminated_ = true;
}

void PeerConnect::MainLoop() {
    /*
     * При проверке вашего решения на сервере этот метод будет переопределен.
     * Если вы провели handshake верно, то в этой функции будет работать обмен данными с пиром
     */
    std::cout << "Dummy main loop" << std::endl;
    Terminate();
}

PeerPiecesAvailability::PeerPiecesAvailability() {
    // по умолчанию у пира нет никаких частей файла
    bitfield_ = "";
}

PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield) 
    : bitfield_(std::move(bitfield)) {}

bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const {
    return (bitfield_[pieceIndex / 8] & (1 << (7 - pieceIndex % 8)) & 1);
}

void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex) {
    bitfield_[pieceIndex / 8] |= 1 << (7 - pieceIndex % 8);
}

size_t PeerPiecesAvailability::Size() const {
    return bitfield_.size() * 8;
}

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId)
    : tf_(tf), socket_(peer.ip, peer.port, 1s, 1s), selfPeerId_(std::move(selfPeerId)), terminated_(false), choked_(true) { }

void PeerConnect::PerformHandshake() {
    socket_.EstablishConnection();
    std::string handshake;
    handshake += (char)19;
    handshake += "BitTorrent protocol";
    for (int i = 0; i < 8; ++i) handshake += '\0';
    handshake += tf_.infoHash;
    handshake += selfPeerId_;
    socket_.SendData(handshake);
    std::string response = socket_.ReceiveData(68);  // размер handshake сообщения
    if (response.substr(28, 20) != tf_.infoHash) {
        throw std::runtime_error("Invalid handshake response");
    }
    peerId_ = response.substr(48, 20);
} 


void PeerConnect::ReceiveBitfield() {
    std::string response = socket_.ReceiveData();
    Message message = Message::Parse(response);
    if (message.id == MessageId::Unchoke)
        choked_ = false;
    else if (message.id != MessageId::BitField)
        throw std::runtime_error("Cannot receive bitfield!");
    piecesAvailability_ = PeerPiecesAvailability(message.payload);
}

void PeerConnect::SendInterested() {
    Message ms;

    ms = ms.Init(MessageId::Interested, std::string());
    std::string data = ms.ToString();

    socket_.SendData(data);
}
// #include "byte_tools.h"
// #include "peer_connect.h"
// #include "message.h"
// #include <iostream>
// #include <utility>

// using namespace std::chrono_literals;

// PeerPiecesAvailability::PeerPiecesAvailability() = default;

// PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield)
//     : bitfield_(std::move(bitfield))
//     {}

// bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const {
//     return ((bitfield_[pieceIndex / 8] >> (7 - pieceIndex % 8) & 1)) != 0;
// }

// void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex) {
//     bitfield_[pieceIndex / 8] = char(bitfield_[pieceIndex / 8] | (1 << (7 - pieceIndex % 8)));
// }

// size_t PeerPiecesAvailability::Size() const {
//     return bitfield_.size() * 8;
// }

// PeerConnect::PeerConnect(const Peer &peer, const TorrentFile &tf, std::string selfPeerId)
//         : tf_(tf),
//           socket_(peer.ip, peer.port, 1000ms, 1000ms),
//           selfPeerId_(std::move(selfPeerId)),
//           choked_(true),
//           terminated_(false) {}

// void PeerConnect::Run() {
//     while (!terminated_) {
//         if (EstablishConnection()) {
//             std::cout << "Connection established to peer" << std::endl;
//             MainLoop();
//         } else {
//             std::cerr << "Cannot establish connection to peer" << std::endl;
//             Terminate();
//         }
//     }
// }

// void PeerConnect::PerformHandshake() {
//     socket_.EstablishConnection();
//     std::string pstr = "BitTorrent protocol";
//     std::string handshake;
//     handshake += (char) pstr.length();
//     handshake += pstr;
//     for (int i = 0; i < 8; ++i)
//         handshake += '\0';
//     handshake += tf_.infoHash;
//     handshake += selfPeerId_;
//     socket_.SendData(handshake);
//     std::string response = socket_.ReceiveData(handshake.length());
//     std::string infoHashFromPeer = response.substr(28, 20);
//     if (infoHashFromPeer != tf_.infoHash)
//         throw std::runtime_error("Info hashes don't same!");
// }

// bool PeerConnect::EstablishConnection() {
//     try {
//         PerformHandshake();
//         ReceiveBitfield();
//         SendInterested();
//         return true;
//     } catch (const std::exception &e) {
//         std::cerr << "Failed to establish connection with peer " << socket_.GetIp() << ":" <<
//                   socket_.GetPort() << " -- " << e.what() << std::endl;
//         return false;
//     }
// }

// void PeerConnect::ReceiveBitfield() {
//     const std::string response = socket_.ReceiveData();
//     Message message = Message::Parse(response);
//     if (message.id == MessageId::Unchoke)
//         choked_ = false;
//     else if (message.id != MessageId::BitField)
//         throw std::runtime_error("Cannot receive bitfield!");
//     piecesAvailability_ = PeerPiecesAvailability(message.payload);
// }

// void PeerConnect::SendInterested() {
//     std::string interested = Message::Init(MessageId::Interested, "").ToString();
//     socket_.SendData(interested);
// }

// void PeerConnect::Terminate() {
//     std::cerr << "Terminate" << std::endl;
//     terminated_ = true;
// }

// void PeerConnect::MainLoop() {
//     /*
//      * При проверке вашего решения на сервере этот метод будет переопределен.
//      * Если вы провели handshake верно, то в этой функции будет работать обмен данными с пиром
//      */
//     std::cout << "Dummy main loop" << std::endl;
//     Terminate();
// }