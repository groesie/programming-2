#include "piece_storage.h"

#include <iostream>
#include <stdexcept>

PieceStorage::PieceStorage(const TorrentFile& tf) {
    size_t num_pieces = tf.length / tf.pieceLength;
    size_t last_piece_length = tf.length % tf.pieceLength;

    for (size_t i = 0; i < num_pieces; ++i) {
        remainPieces_.push(std::make_shared<Piece>(i, tf.pieceLength, tf.pieceHashes[i]));
    }
    if (last_piece_length > 0) {
        remainPieces_.push(std::make_shared<Piece>(num_pieces, last_piece_length, tf.pieceHashes[num_pieces]));
    }
}

PiecePtr PieceStorage::GetNextPieceToDownload() {
    if (remainPieces_.empty()) {
        throw std::out_of_range("No more pieces to download");
    }
    auto piece = remainPieces_.front();
    remainPieces_.pop();
    return piece;
}

void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    while (!remainPieces_.empty()) {
        remainPieces_.pop();
    }
}

bool PieceStorage::QueueIsEmpty() const {
    return remainPieces_.empty();
}

size_t PieceStorage::TotalPiecesCount() const {
    return remainPieces_.size();
}

void PieceStorage::SavePieceToDisk(PiecePtr piece) {
    // Эта функция будет переопределена при запуске вашего решения в проверяющей системе
    // Вместо сохранения на диск там пока что будет проверка, что часть файла скачалась правильно
    std::cout << "Downloaded piece " << piece->GetIndex() << std::endl;
    std::cerr << "Clear pieces list, don't want to download all of them" << std::endl;
    while (!remainPieces_.empty()) {
        remainPieces_.pop();
    }
}
