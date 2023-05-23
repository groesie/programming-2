#include "byte_tools.h"
#include "piece.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>

namespace {
    constexpr size_t BLOCK_SIZE = 1 << 14;
}


Piece::Piece(size_t index, size_t length, std::string hash)
    : index_(index), length_(length), hash_(std::move(hash)) {
        
    uint32_t num_blocks = length_ / BLOCK_SIZE;
    uint32_t last_block_length = length_ % BLOCK_SIZE;

    blocks_.reserve(num_blocks + (last_block_length > 0 ? 1 : 0));
    for (uint32_t i = 0; i < num_blocks; ++i) {
        blocks_.emplace_back(Block{(uint32_t)index_, i * (uint32_t)BLOCK_SIZE, BLOCK_SIZE, Block::Status::Missing, ""});
    }
    if (last_block_length > 0) {
        blocks_.emplace_back(Block{(uint32_t)index_, (uint32_t)(num_blocks * BLOCK_SIZE), last_block_length, Block::Status::Missing, ""});
    }
}

bool Piece::HashMatches() const {
    return GetDataHash() == hash_;
}

Block* Piece::FirstMissingBlock() {
    for (auto& block : blocks_) {
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
    if (blockIndex >= blocks_.size()) {
        throw std::out_of_range("Block index out of range");
    }
    Block& block = blocks_[blockIndex];
    block.data = std::move(data);
    block.status = Block::Status::Retrieved;
}

bool Piece::AllBlocksRetrieved() const {
    for (const auto& block : blocks_) {
        if (block.status != Block::Status::Retrieved) {
            return false;
        }
    }
    return true;
}

std::string Piece::GetData() const {
    std::string data;
    for (const auto& block : blocks_) {
        data += block.data;
    }
    return data;
}

std::string Piece::GetDataHash() const {
    // unsigned char hash[SHA_DIGEST_LENGTH];
    // SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    // return std::string(reinterpret_cast<const char*>(hash), SHA_DIGEST_LENGTH);
    std::string data = GetData();
    std::string hash = CalculateSHA1(data);
    return hash;
}

const std::string& Piece::GetHash() const {
    return hash_;
}

void Piece::Reset() {
    for (auto& block : blocks_) {
        block.status = Block::Status::Missing;
        block.data.clear();
    }
}
