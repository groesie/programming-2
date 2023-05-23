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

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId, PieceStorage& pieceStorage)
    : tf_(tf), socket_(peer.ip, peer.port, 1s, 1s), selfPeerId_(std::move(selfPeerId)), terminated_(false), choked_(true), pieceStorage_(pieceStorage) { }

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

void PeerConnect::ProcessMessage(const Message& message) {
    switch (message.id) {
        case MessageId::Choke:
            choked_ = true;
            break;
        case MessageId::Unchoke:
            choked_ = false;
            break;
        case MessageId::Have:
            piecesAvailability_.SetPieceAvailability(ByteTools::ParseBigEndian(message.payload));
            break;
        case MessageId::Piece:
            if (requestedPieceIndex_ != PieceStorage::InvalidPieceIndex) {
                assert(requestedPieceIndex_ == ByteTools::ParseBigEndian(message.payload.substr(0, 4)));
                assert(requestedPieceOffset_ == ByteTools::ParseBigEndian(message.payload.substr(4, 4)));
                pieceStorage_.WritePiece(requestedPieceIndex_, requestedPieceOffset_, message.payload.substr(8));
                requestedPieceIndex_ = PieceStorage::InvalidPieceIndex;
            }
            break;
        default:
            break;
    }
}

void PeerConnect::SendRequest(size_t pieceIndex, size_t offset, size_t length) {
    Message message = Message::Init(MessageId::Request, "");
    message.payload = ByteTools::ToBigEndian(pieceIndex) + ByteTools::ToBigEndian(offset) + ByteTools::ToBigEndian(length);
    socket_.SendData(message.ToString());
}

void PeerConnect::MainLoop() {
    while (!terminated_) {
        if (!choked_ && requestedPieceIndex_ == PieceStorage::InvalidPieceIndex) {
            size_t pieceIndex = piecesAvailability_.Size();
            for (size_t i = 0; i < pieceIndex; ++i) {
                if (piecesAvailability_.IsPieceAvailable(i) && !pieceStorage_.HasPiece(i)) {
                    requestedPieceIndex_ = i;
                    requestedPieceOffset_ = 0;
                    requestedPieceLength_ = tf_.pieceLength;
                    if (i == pieceIndex - 1) {
                        requestedPieceLength_ = tf_.totalSize % tf_.pieceLength;
                    }
                    SendRequest(requestedPieceIndex_, requestedPieceOffset_, requestedPieceLength_);
                    break;
                }
            }
        }

        std::string response = socket_.ReceiveData();
        Message message = Message::Parse(response);
        ProcessMessage(message);

        if (requestedPieceIndex_ != PieceStorage::InvalidPieceIndex) {
            size_t pieceSize = tf_.pieceLength;
            if (requestedPieceIndex_ == piecesAvailability_.Size() - 1) {
                pieceSize = tf_.length % tf_.pieceLength;
            }
            if (pieceStorage_.HasPiece(requestedPieceIndex_)) {
                requestedPieceIndex_ = PieceStorage::InvalidPieceIndex;
                continue;
            }
            if (requestedPieceOffset_ + requestedPieceLength_ >= pieceSize) {
                requestedPieceLength_ = pieceSize - requestedPieceOffset_;
            }
            SendRequest(requestedPieceIndex_, requestedPieceOffset_, requestedPieceLength_);
            requestedPieceOffset_ += requestedPieceLength_;
            if (requestedPieceOffset_ >= pieceSize) {
                requestedPieceIndex_ = PieceStorage::InvalidPieceIndex;
            }
        }
    }
}