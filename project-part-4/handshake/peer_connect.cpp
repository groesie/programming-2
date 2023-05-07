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
    return (bitfield_[pieceIndex / 8] >> (7 - pieceIndex % 8)) & 1;
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

    std::string handshake = "\x13" "BitTorrent protocol" "\x00\x00\x00\x00\x00\x00\x00\x00" + tf_.infoHash + selfPeerId_;

    socket_.SendData(handshake);
    std::string response = socket_.ReceiveData(68);  // размер handshake сообщения
    std::cerr << response << std::endl; // debug
    std::cerr << "cerr " + handshake << std::endl;
    std::cout << "cout " + handshake << std::endl;
    if (response.substr(0, 28) != "\x13" "BitTorrent protocol" "\x00\x00\x00\x00\x00\x00\x00\x00" ||
        response.substr(28, 20) != tf_.infoHash) {
        throw std::runtime_error("Invalid handshake response");
    }
    peerId_ = response.substr(48, 20);
}


void PeerConnect::ReceiveBitfield() {
    std::string response = socket_.ReceiveData();
    int messageLength = BytesToInt(response.substr(0, 4));
    uint8_t messageId = response[4];
    if (messageId == 5) {
        piecesAvailability_ = PeerPiecesAvailability(response.substr(5, messageLength - 1));
    } else if (messageId == 1) {
        choked_ = false;
    } else {
        throw std::runtime_error("Expected Bitfield or Unchoke message");
    }
}

void PeerConnect::SendInterested() {
    std::string message = "\x00\x00\x00\x01\x02";
    socket_.SendData(message);
}
