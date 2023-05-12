#include "torrent_tracker.h"
#include "bencode.h"
#include "byte_tools.h"

#include <cpr/cpr.h>
#include <memory>

const std::vector<Peer> &TorrentTracker::GetPeers() const {
    return peers_;
}

TorrentTracker::TorrentTracker(const std::string& url) : url_(url) { };


void TorrentTracker::UpdatePeers(const TorrentFile& tf, std::string peerId, int port) {
    cpr::Response res = cpr::Get(
        cpr::Url{url_},
        cpr::Parameters {
                {"info_hash", tf.infoHash},
                {"peer_id", peerId},
                {"port", std::to_string(port)},
                {"uploaded", std::to_string(0)},
                {"downloaded", std::to_string(0)},
                {"left", std::to_string(tf.length)},
                {"compact", std::to_string(1)}
        },
        cpr::Timeout{20000}
    );
    std::string tdata(std::move(res.text));


    std::stringstream tstream(tdata);
    tstream.unsetf(std::ios_base::skipws);

    Bencode::TorElemPtr root = Bencode::getRoot(tstream);

    Bencode::Dict d = std::get<1>(root->value);
    std::stringstream peerstream(std::get<0>(d["peers"]->value));
    peerstream.unsetf(std::ios_base::skipws);

    char cur_byte;
    while (peerstream >> cur_byte) {
        Peer peer;
    
        std::string ip[4];
        ip[0] = std::to_string((int)(unsigned char)cur_byte);
        for (int chunk_i = 1; chunk_i < 4; ++chunk_i) {
            peerstream >> cur_byte;
            ip[chunk_i] = std::to_string((int)(unsigned char)cur_byte);
        }
        peer.ip = ip[0] + "." + ip[1] + "." + ip[2] + "." + ip[3];

        int port;
        peerstream >> cur_byte;
        port = (int)(unsigned char)cur_byte * 256;
        peerstream >> cur_byte;
        port += (int)(unsigned char)cur_byte;
        peer.port = port;

        peers_.emplace_back(std::move(peer));
    }
}
