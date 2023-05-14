#include "byte_tools.h"
#include "piece.h"
#include <iostream>
#include <algorithm>
#include "piece.h"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>


namespace {
    constexpr size_t BLOCK_SIZE = 1 << 14;
}


Piece::Piece(size_t index, size_t length, std::string hash)
    : index_(index), length_(length), hash_(hash) {
    size_t num_blocks = (length_ + BLOCK_SIZE - 1) / BLOCK_SIZE;
    blocks_.reserve(num_blocks);

    for (size_t i = 0; i < num_blocks; ++i) {
        size_t block_length = (i == num_blocks - 1) ? length_ - BLOCK_SIZE * i : BLOCK_SIZE;
        blocks_.emplace_back(Block{static_cast<uint32_t>(index_), static_cast<uint32_t>(i * BLOCK_SIZE), static_cast<uint32_t>(block_length), Block::Status::Missing, ""});
    }
}

bool Piece::HashMatches() const {
    return GetDataHash() == hash_;
}

Block* Piece::FirstMissingBlock() {
    for (auto &block : blocks_) {
        if (block.status == Block::Status::Missing) {
            return &block;
        }
    }
    return nullptr;
}

size_t Piece::GetIndex() const {
    return index_;
}

void Piece::SaveBlock(size_t blockOffset, std::string data) {
    size_t blockIndex = blockOffset / BLOCK_SIZE;
    if (blockIndex < blocks_.size()) {
        blocks_[blockIndex].data = std::move(data);
        blocks_[blockIndex].status = Block::Status::Retrieved;
    }
}

bool Piece::AllBlocksRetrieved() const {
    for (const auto &block : blocks_) {
        if (block.status != Block::Status::Retrieved) {
            return false;
        }
    }
    return true;
}

std::string Piece::GetData() const {
    std::string data;
    for (const auto &block : blocks_) {
        if (block.status == Block::Status::Retrieved) {
            data += block.data;
        } else {
            break;
        }
    }
    return data;
}

std::string Piece::GetDataHash() const {
    std::string data = GetData();
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(data.data()), data.size(), digest);

    std::ostringstream oss;
    for (unsigned char c : digest) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return oss.str();
}

const std::string& Piece::GetHash() const {
    return hash_;
}

void Piece::Reset() {
    for (auto &block : blocks_) {
        block.status = Block::Status::Missing;
        block.data.clear();
    }
}

size_t Piece::GetDownloadedSize() const {
    size_t downloadedSize = 0;
    for (const auto& block : blocks_) {
        if (block.Retrieved) {
            downloadedSize += block.length;
        }
    }
    return downloadedSize;
}

size_t Piece::GetRemainingSize() const {
    return length_ - GetDownloadedSize();
}








