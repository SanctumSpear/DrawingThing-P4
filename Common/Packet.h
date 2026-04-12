#pragma once
#pragma once
#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <stdexcept>

enum class PacketType : uint8_t {
    CONNECT = 0x01,
    DISCONNECT = 0x02,
    PROMPT = 0x03,
    IMAGE = 0x04,
    GAME_START = 0x05,
    GAME_END = 0x06,
    ACK = 0x07,
    GAME_ERROR = 0x08
};

enum class GameState : uint8_t {
    STARTUP = 0x01,
    WAITING = 0x02,
    SENDING = 0x03,
    RECEIVING = 0x04,
    ENDING = 0x05
};

struct PacketHeader {
    uint32_t   payloadSize;
    uint32_t   CRC;
    PacketType type;
    GameState  state;
    uint8_t    sessionID;
};

class Packet {
public:
    PacketHeader header;
    char* data;

    Packet() : data(nullptr) {
        memset(&header, 0, sizeof(PacketHeader));
    }

    Packet(const Packet& packet) {
        header = packet.header;

        if (packet.data && packet.header.payloadSize > 0) {
            data = new char[packet.header.payloadSize];
            memcpy(data, packet.data, packet.header.payloadSize);
        }
        else {
            data = nullptr;
        }
    }

    // Destructor
    ~Packet() {
        delete[] data;
    }

    static Packet MakeConnectPacket(uint8_t sessionID) {
        Packet packet;

        packet.header.type = PacketType::CONNECT;
        packet.header.state = GameState::WAITING;
        packet.header.sessionID = sessionID;
        packet.header.payloadSize = 0;
        packet.header.CRC = 0;

        return packet;
    }

    static Packet MakePromptPacket(uint8_t sessionID, const std::string& prompt) {
        Packet packet;

        packet.header.type = PacketType::PROMPT;
        packet.header.state = GameState::SENDING;
        packet.header.sessionID = sessionID;
        packet.header.payloadSize = prompt.size() + 1;
        packet.data = new char[packet.header.payloadSize];

        memcpy(packet.data, prompt.c_str(), packet.header.payloadSize);

        packet.header.CRC = ComputeCRC(packet.data, packet.header.payloadSize);

        return packet;
    }

    static Packet MakeGameStartPacket(uint8_t sessionID) {
        Packet packet;

        packet.header.type = PacketType::GAME_START;
        packet.header.state = GameState::SENDING;
        packet.header.sessionID = sessionID;
        packet.header.payloadSize = 0;
        packet.header.CRC = 0;

        return packet;
    }

    static Packet MakeGameEndPacket(uint8_t sessionID) {
        Packet packet;

        packet.header.type = PacketType::GAME_END;
        packet.header.state = GameState::ENDING;
        packet.header.sessionID = sessionID;
        packet.header.payloadSize = 0;
        packet.header.CRC = 0;

        return packet;
    }

    static Packet MakeAckPacket(uint8_t sessionID) {
        Packet packet;

        packet.header.type = PacketType::ACK;
        packet.header.state = GameState::WAITING;
        packet.header.sessionID = sessionID;
        packet.header.payloadSize = 0;
        packet.header.CRC = 0;

        return packet;
    }

    static Packet MakeErrorPacket(uint8_t sessionID, const std::string& message) {
        Packet packet;

        packet.header.type = PacketType::GAME_ERROR;
        packet.header.state = GameState::WAITING;
        packet.header.sessionID = sessionID;
        packet.header.payloadSize = message.size() + 1;
        packet.data = new char[packet.header.payloadSize];

        memcpy(packet.data, message.c_str(), packet.header.payloadSize);

        packet.header.CRC = ComputeCRC(packet.data, packet.header.payloadSize);

        return packet;
    }


    char* Serialize(uint32_t& outTotalSize) const {
        outTotalSize = sizeof(PacketHeader) + header.payloadSize;
        char* buffer = new char[outTotalSize];

        memcpy(buffer, &header, sizeof(PacketHeader));

        if (data && header.payloadSize > 0) {
            memcpy(buffer + sizeof(PacketHeader), data, header.payloadSize);
        }

        return buffer;
    }

    static Packet Deserialize(const char* buffer, uint32_t totalSize) {
        if (totalSize < sizeof(PacketHeader))
            throw std::runtime_error("Buffer too small to contain a header.");

        Packet packet;
        memcpy(&packet.header, buffer, sizeof(PacketHeader));

        if (packet.header.payloadSize > 0) {
            packet.data = new char[packet.header.payloadSize];
            memcpy(packet.data, buffer + sizeof(PacketHeader), packet.header.payloadSize);
        }

        return packet;
    }


    bool ValidateCRC() const {
        if (header.payloadSize == 0) return true;
        return ComputeCRC(data, header.payloadSize) == header.CRC;
    }


    std::string GetPromptString() const {
        if (!data) return "";
        return std::string(data);
    }

    std::string GetErrorString() const {
        if (!data) return "";
        return std::string(data);
    }

private:
    static uint32_t ComputeCRC(const char* RawBuffer, uint32_t length) {
        uint32_t counter = 0;

        for (uint32_t byte = 0; byte < length - sizeof(uint32_t); byte++) {
            for (int bit = 0; bit < 8; bit++) {
                if (RawBuffer[byte] & (1 << bit)) {
                    counter++;
                }
            }
        }

        return counter;
    }
};