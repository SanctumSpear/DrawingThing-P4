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
    uint32_t payloadSize;
    uint32_t CRC;
    PacketType type;
    GameState state;
    uint8_t sessionID;
    uint8_t srcAddress;  
    uint8_t dstAddress;  
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
        } else {
            data = nullptr;
        }
    }

    ~Packet() {
        delete[] data;
    }

    static Packet MakeConnectPacket(uint8_t sessionID,
                                    uint8_t src = 0, uint8_t dst = 0) {
        Packet p;
        p.header.type = PacketType::CONNECT;
        p.header.state = GameState::WAITING;
        p.header.sessionID = sessionID;
        p.header.srcAddress = src;
        p.header.dstAddress = dst;
        p.header.payloadSize= 0;
        p.header.CRC = 0;
        return p;
    }

    static Packet MakePromptPacket(uint8_t sessionID, const std::string& prompt,
                                   uint8_t src = 0, uint8_t dst = 0) {
        Packet p;
        p.header.type = PacketType::PROMPT;
        p.header.state = GameState::SENDING;
        p.header.sessionID = sessionID;
        p.header.srcAddress = src;
        p.header.dstAddress = dst;
        p.header.payloadSize = (uint32_t)(prompt.size() + 1);
        p.data = new char[p.header.payloadSize];
        memcpy(p.data, prompt.c_str(), p.header.payloadSize);
        p.header.CRC = ComputeCRC(p.data, p.header.payloadSize);
        return p;
    }

    static Packet MakeGameStartPacket(uint8_t sessionID,
                                      uint8_t src = 0, uint8_t dst = 0) {
        Packet p;
        p.header.type = PacketType::GAME_START;
        p.header.state = GameState::SENDING;
        p.header.sessionID = sessionID;
        p.header.srcAddress = src;
        p.header.dstAddress = dst;
        p.header.payloadSize = 0;
        p.header.CRC = 0;
        return p;
    }

    static Packet MakeGameEndPacket(uint8_t sessionID,
                                    uint8_t src = 0, uint8_t dst = 0) {
        Packet p;
        p.header.type = PacketType::GAME_END;
        p.header.state = GameState::ENDING;
        p.header.sessionID = sessionID;
        p.header.srcAddress = src;
        p.header.dstAddress = dst;
        p.header.payloadSize = 0;
        p.header.CRC = 0;
        return p;
    }

    static Packet MakeAckPacket(uint8_t sessionID,
                                uint8_t src = 0, uint8_t dst = 0) {
        Packet p;
        p.header.type = PacketType::ACK;
        p.header.state = GameState::WAITING;
        p.header.sessionID = sessionID;
        p.header.srcAddress = src;
        p.header.dstAddress = dst;
        p.header.payloadSize = 0;
        p.header.CRC = 0;
        return p;
    }

    static Packet MakeErrorPacket(uint8_t sessionID, const std::string& message,
                                  uint8_t src = 0, uint8_t dst = 0) {
        Packet p;
        p.header.type = PacketType::GAME_ERROR;
        p.header.state = GameState::WAITING;
        p.header.sessionID = sessionID;
        p.header.srcAddress = src;
        p.header.dstAddress = dst;
        p.header.payloadSize = (uint32_t)(message.size() + 1);
        p.data = new char[p.header.payloadSize];
        memcpy(p.data, message.c_str(), p.header.payloadSize);
        p.header.CRC = ComputeCRC(p.data, p.header.payloadSize);
        return p;
    }

    // IMAGE packet carries raw binary data (e.g. JPEG bytes)
    static Packet MakeImagePacket(uint8_t sessionID, const char* imageData,
                                  uint32_t imageSize,
                                  uint8_t src = 0, uint8_t dst = 0) {
        Packet p;
        p.header.type = PacketType::IMAGE;
        p.header.state = GameState::SENDING;
        p.header.sessionID = sessionID;
        p.header.srcAddress = src;
        p.header.dstAddress = dst;
        p.header.payloadSize = imageSize;
        p.data = new char[imageSize];
        memcpy(p.data, imageData, imageSize);
        p.header.CRC = ComputeCRC(p.data, p.header.payloadSize);
        return p;
    }

    char* Serialize(uint32_t& outTotalSize) const {
        outTotalSize = sizeof(PacketHeader) + header.payloadSize;
        char* buffer = new char[outTotalSize];
        memcpy(buffer, &header, sizeof(PacketHeader));
        if (data && header.payloadSize > 0)
            memcpy(buffer + sizeof(PacketHeader), data, header.payloadSize);
        return buffer;
    }

    static Packet Deserialize(const char* buffer, uint32_t totalSize) {
        if (totalSize < sizeof(PacketHeader))
            throw std::runtime_error("Buffer too small to contain a header.");
        Packet p;
        memcpy(&p.header, buffer, sizeof(PacketHeader));
        if (p.header.payloadSize > 0) {
            p.data = new char[p.header.payloadSize];
            memcpy(p.data, buffer + sizeof(PacketHeader), p.header.payloadSize);
        }
        return p;
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
    static uint32_t ComputeCRC(const char* buf, uint32_t length) {
        uint32_t counter = 0;
        for (uint32_t byte = 0; byte < length - sizeof(uint32_t); byte++)
            for (int bit = 0; bit < 8; bit++)
                if (buf[byte] & (1 << bit))
                    counter++;
        return counter;
    }
};
