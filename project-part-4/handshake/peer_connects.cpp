#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <cassert>

//debug
#include <fstream>
#include <cstddef>
#include <algorithm>
#include <cstring>


PeerPiecesAvailability::PeerPiecesAvailability() {}

PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield) : bitfield_(bitfield) {}

bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const{
    size_t pos = pieceIndex / 8;
    size_t bit_pos = pieceIndex % 8;
    if(bitfield_.size() <= pos){
        throw std::runtime_error("Invalid bitfield size");
    }
    if(int(bitfield_[pos]) & (1 << (7 - bit_pos))){
        return true;
    }else{
        return false;
    }
}

void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex){
    size_t pos = pieceIndex / 8;
    size_t bit_pos = pieceIndex % 8;
    if(bitfield_.size() <= pos){
        throw std::runtime_error("Invalid bitfield size");
    }
    bitfield_[pos]  |= (1 << (7 - bit_pos));
}

size_t PeerPiecesAvailability::Size() const{
    return bitfield_.size() * 8;

}

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile &tf, std::string selfPeerId) :
 tf_(tf), selfPeerId_(selfPeerId), terminated_(false), choked_(true),
 socket_(TcpConnect (peer.ip, peer.port, std::chrono::milliseconds(1000), std::chrono::milliseconds(1000))) {
    std::cout << "RUN PEER WITH IP " << peer.ip << std::endl;
    if(selfPeerId_.size() != 20){
        throw std::runtime_error("Self id is not 20 bytes long");
    }
 }

void PeerConnect::Run() {
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer" <<  std::endl;
            MainLoop();
        } else {
            std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
    }
}

void PeerConnect::PerformHandshake() {
    socket_.EstablishConnection();
    std::string s(1, char(19));
    s += "BitTorrent protocol";
    for(int i = 0; i < 8; ++i){
        s += char(0);
    }
    s += tf_.infoHash;
    s += selfPeerId_; 
    socket_.SendData(s);
    std::string handshake_recieved = socket_.ReceiveData(68);
    
    if((unsigned char)handshake_recieved[0] != 19){
        throw std::runtime_error("1 Not a BIttorrent");
    }
    if(handshake_recieved.substr(1, 19) != "BitTorrent protocol"){
        throw std::runtime_error("2 Not a BIttorrent");
    }
    if(handshake_recieved.substr(28, 20) != tf_.infoHash){
        throw std::runtime_error("3 Not a BIttorrent");
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

void PeerConnect::ReceiveBitfield() {
    std::string bitfield = socket_.ReceiveData();
    Message ms = Message::Parse(bitfield);
    if(bitfield.size() == 0){
        throw std::runtime_error("bitfield size < 0");
    }
    if(ms.id == MessageId::BitField){
        bitfield = bitfield.substr(1);
        piecesAvailability_ = PeerPiecesAvailability(bitfield);
    }else if(ms.id == MessageId::Unchoke){
        choked_ = false;
    }else{
    
        throw std::runtime_error("Message type Neither 1 nor 5");
    }
}

void PeerConnect::SendInterested() {
    Message ms;
    ms = ms.Init(MessageId::Interested, std::string());
    std::string send = ms.ToString();
    
    // std::cout << "Send Interested" << std::endl;

    socket_.SendData(send);
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
