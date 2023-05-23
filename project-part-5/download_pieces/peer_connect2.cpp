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

    ms = ms.Init(MessageId::Interested, std::string());
    std::string data = ms.ToString();

    socket_.SendData(data);
}

// void PeerConnect::MainLoop() {
//     while (!terminated_) {
//         std::string response = socket_.ReceiveData();
//         Message message = Message::Parse(response);
//         if (message.id == MessageId::Unchoke) {
//             choked_ = false;
//         } else if (message.id == MessageId::Have) {
//             HandleHaveMessage(message);
//         } else if (message.id == MessageId::Piece) {
//             HandlePieceMessage(message);
//         }
//     }
// }

// void PeerConnect::HandleHaveMessage(const Message& message) {
//     size_t pieceIndex = BytesToInt(message.payload);
//     piecesAvailability_.SetPieceAvailability(pieceIndex);
// }

// void PeerConnect::HandlePieceMessage(const Message& message) {
//     // Получение индекса и смещения части
//     size_t pieceIndex = BytesToInt(message.payload.substr(0, 4));
//     size_t offset = BytesToInt(message.payload.substr(4, 4));

//     // Получение данных части
//     std::string pieceData = message.payload.substr(8);

//     // Сохранение полученной части
//     PiecePtr piece = std::make_shared<Piece>(pieceIndex, tf_.pieceLength);
//     piece->SaveBlock(offset, pieceData);

//     // Проверка наличия всех блоков в части
//     if (piece->AllBlocksRetrieved()) {
//         pieceStorage_.PieceProcessed(piece);
//     }
// }


void PeerConnect::MainLoop() {
    while (!terminated_) {
        try {
            std::string response = socket_.ReceiveData();
            Message message = Message::Parse(response);

            switch (message.id) {
                case MessageId::Have:
                    HandleHaveMessage(message);
                    break;
                case MessageId::Piece:
                    HandlePieceMessage(message);
                    break;
                case MessageId::Choke:
                    HandleChokeMessage();
                    break;
                case MessageId::Unchoke:
                    HandleUnchokeMessage();
                    break;
                default:
                    break;
            }

            if (!pendingBlock_ && !choked_) {
                RequestPiece();
            }

        } catch (const std::exception& e) {
            std::cerr << "Error in main loop: " << e.what() << std::endl;
            Terminate();
        }
    }
}
void PeerConnect::HandleHaveMessage(const Message &message) {
    size_t pieceIndex = ByteTools::ToInt(message.payload);
    piecesAvailability_.SetPieceAvailability(pieceIndex);
}

// void PeerConnect::HandlePieceMessage(const Message &message) {
//     pieceInProgress_->AddBlock(message.payload);
//     if (pieceInProgress_->IsComplete()) {
//         pieceStorage_.PieceProcessed(pieceInProgress_);
//         pieceInProgress_.reset();
//     }
//     pendingBlock_ = false;
// }

void PeerConnect::HandleChokeMessage() {
    choked_ = true;
}

void PeerConnect::HandleUnchokeMessage() {
    choked_ = false;
}
void Peer::HandlePieceMessage(const Message& message) {
    auto pieceIndex = ByteTools::NetworkToHost32(
        ByteTools::ToUInt32(message.payload.substr(0, 4)));

    auto blockOffset = ByteTools::NetworkToHost32(
        ByteTools::ToUInt32(message.payload.substr(4, 4)));

    auto blockData = message.payload.substr(8);

    auto piecePtr = fileManager.GetPiece(pieceIndex);

    if (piecePtr) {
        piecePtr->SaveBlock(blockOffset, blockData);

        if (piecePtr->AllBlocksRetrieved()) {
            if (piecePtr->HashMatches()) {
                fileManager.WritePieceToFile(piecePtr->GetIndex(),
                                              piecePtr->GetData());
                torrentManager.UpdateStatus(pieceIndex, TorrentStatus::Completed);
            } else {
                piecePtr->Reset();
                torrentManager.UpdateStatus(pieceIndex, TorrentStatus::Incomplete);
            }
        }

        // Request next block if there are still blocks to download
        if (!piecePtr->AllBlocksRetrieved()) {
            RequestPiece(pieceIndex);
        }
    }
}

void Peer::RequestPiece(size_t pieceIndex) {
    auto piecePtr = fileManager.GetPiece(pieceIndex);

    if (piecePtr) {
        Block* missingBlock = piecePtr->FirstMissingBlock();

        if (missingBlock) {
            missingBlock->status = Block::Status::Pending;

            std::string payload;
            payload.reserve(12);
            payload += ByteTools::FromUInt32(ByteTools::HostToNetwork32(pieceIndex));
            payload += ByteTools::FromUInt32(ByteTools::HostToNetwork32(missingBlock->offset));
            payload += ByteTools::FromUInt32(ByteTools::HostToNetwork32(missingBlock->length));

            Message requestMessage = Message::Init(MessageId::Request, payload);
            SendMessage(requestMessage);
        }
    }
}

// void PeerConnect::RequestPiece() {
//     if (!pieceInProgress_) {
//         pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
//     }

//     if (pieceInProgress_) {
//         size_t offset = pieceInProgress_->GetNextBlockOffset();
//         size_t blockLength = std::min(static_cast<size_t>(1 << 14), pieceInProgress_->GetSize() - offset);
//         Message requestMessage = Message::CreateRequestMessage(pieceInProgress_->GetIndex(), offset, blockLength);
//         socket_.SendData(requestMessage.ToString());
//         pendingBlock_ = true;
//     }
// }


// The HandlePieceMessage function now works with the Piece class and its methods, and the RequestPiece function has been adjusted accordingly. The HandlePieceMessage function saves the received block data to the corresponding piece and checks if all blocks have been retrieved. If so, it verifies the hash and writes the data to the file if the hash matches. The RequestPiece function requests the next missing block of the specified piece from the peer.
// void PeerConnect::HandlePieceMessage(const Message& message) {
//     // Получение индекса и смещения части
//     size_t pieceIndex = BytesToInt(message.payload.substr(0, 4));
//     size_t offset = BytesToInt(message.payload.substr(4, 4));

//     // Получение данных части
//     std::string pieceData = message.payload.substr(8);

//     // Сохранение полученной части
//     PiecePtr piece = std::make_shared<Piece>(pieceIndex, tf_.pieceLength, tf_.pieceHashes[pieceIndex]);
//     piece->SaveBlock(offset, pieceData);

//     // Проверка наличия всех блоков в части
//     if (piece->AllBlocksRetrieved()) {
//         pieceStorage_.PieceProcessed(piece);
//     }
// }

// void PeerConnect::RequestPiece() {
//     if (!choked_) {
//         PiecePtr piece = pieceStorage_.GetNextPieceToDownload();
//         if (piece) {
//             // Формирование запроса
//             std::string payload = IntToBytesBigEndian(piece->GetIndex()) +
//                                   IntToBytesBigEndian(piece->GetDownloadedSize()) +
//                                   IntToBytesBigEndian(piece->GetRemainingSize());
//             Message requestMessage = Message::Init(MessageId::Request, payload);

//             // Отправка запроса
//             socket_.SendData(requestMessage.ToString());
//         }
//     }
// }