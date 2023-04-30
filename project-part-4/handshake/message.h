#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

enum class MessageId : uint8_t {
    Choke = 0,
    Unchoke,
    Interested,
    NotInterested,
    Have,
    BitField,
    Request,
    Piece,
    Cancel,
    Port,
    KeepAlive,
};

struct Message {
    MessageId id;
    size_t messageLength;
    std::string payload;

    /*
     * Выделяем тип сообщения и длину и создаем объект типа Message.
     * Подразумевается, что здесь в качестве `messageString` будет приниматься строка, прочитанная из TCP-сокета
     */
    static Message Parse(const std::string& messageString) {
        if (messageString.size() < 5) {
            throw std::invalid_argument("Invalid message string");
        }

        uint32_t messageLength = 
            (unsigned char)messageString[0] << 24 |
            (unsigned char)messageString[1] << 16 |
            (unsigned char)messageString[2] << 8 |
            (unsigned char)messageString[3];

        MessageId id = static_cast<MessageId>(messageString[4]);

        std::string payload = messageString.substr(5);

        return {id, messageLength, payload};
    }

    /*
     * Создаем сообщение с заданным типом и содержимым. Длина вычисляется автоматически
     */
    static Message Init(MessageId id, const std::string& payload) {
        uint32_t messageLength = 1 + payload.size(); // 1 for id
        return {id, messageLength, payload};
    }

    /*
     * Формируем строку с сообщением, которую можно будет послать пиру в соответствии с протоколом.
     * Получается строка вида "<1 + payload length><message id><payload>"
     * Секция с длиной сообщения занимает 4 байта и представляет собой целое число в формате big-endian
     * id сообщения занимает 1 байт и может принимать значения от 0 до 9 включительно
     */
    std::string ToString() const;
};

