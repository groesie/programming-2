#include <iostream>
#include "piece_storage.h"

PieceStorage::PieceStorage(const TorrentFile& tf) {
    size_t num_pieces = tf.pieceHashes.size();
    for (size_t i = 0; i < num_pieces; ++i) {
        size_t piece_length = (i == num_pieces - 1) ? tf.length - i * tf.pieceLength : tf.pieceLength;
        PiecePtr piece = std::make_shared<Piece>(i, piece_length, tf.pieceHashes[i]);
        remainPieces_.push(piece);
    }
}

PiecePtr PieceStorage::GetNextPieceToDownload() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (remainPieces_.empty()) {
        return nullptr;
    }
    PiecePtr piece = remainPieces_.front();
    remainPieces_.pop();
    return piece;
}

void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (piece->HashMatches()) {
        SavePieceToDisk(piece);
        while (!remainPieces_.empty()) {
            remainPieces_.pop();
        }
    } else {
        piece->Reset();
        remainPieces_.push(piece);
    }
}

bool PieceStorage::QueueIsEmpty() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return remainPieces_.empty();
}

size_t PieceStorage::TotalPiecesCount() const {
    std::unique_lock<std::mutex> lock(mutex_);
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
