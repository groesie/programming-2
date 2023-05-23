#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <cassert>
#include <stdexcept>
#include <arpa/inet.h>
#include <cstring>

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

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId, PieceStorage& pieceStorage)
    : tf_(tf), socket_(peer.ip, peer.port, 1s, 1s), selfPeerId_(std::move(selfPeerId)), terminated_(false), choked_(true), pieceStorage_(pieceStorage) {}

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

    ms = ms.Init(MessageId::Interested, "");
    std::string data = ms.ToString();

    socket_.SendData(data);
}

void PeerConnect::MainLoop() {
    while (!terminated_) {
        try {
            std::string response = socket_.ReceiveData();
            Message message = Message::Parse(response);

            switch (message.id) {
                case MessageId::Have:
                {
                    std::cout << "Have" << std::endl;
                    size_t pieceIndex = BytesToInt(message.payload);
                    piecesAvailability_.SetPieceAvailability(pieceIndex);
                    break;
                }
                case MessageId::Piece:
                {
                    std::cout << "Piece" << std::endl;
                    size_t index = BytesToInt(message.payload.substr(0, 4));
                    size_t offset = BytesToInt(message.payload.substr(4, 4));

                    std::string pieceData = message.payload.substr(8);

                    pieceInProgress_ = std::make_shared<Piece>(index, tf_.pieceLength, tf_.pieceHashes[index]);
                    pieceInProgress_->SaveBlock(offset, pieceData);

                    // Проверка наличия всех блоков в части
                    if (pieceInProgress_->AllBlocksRetrieved()) {
                        pieceStorage_.PieceProcessed(pieceInProgress_);
                    }
                    break;
                }
                case MessageId::Choke:
                {
                    std::cout << "Choke" << std::endl;
                    choked_ = true;
                    break;
                }
                case MessageId::Unchoke:
                {
                    std::cout << "Unchoke" << std::endl;
                    choked_ = false;
                    break;
                }
                default:
                    break;
            }

            if (!pendingBlock_ && !choked_) {
                std::cout << "debug peer_connect" << std::endl;
                RequestPiece();
            }

        } catch (const std::exception& e) {
            std::cerr << "Error in main loop: " << e.what() << std::endl;
            Terminate();
        }
    }
}


void PeerConnect::RequestPiece() {
    
}