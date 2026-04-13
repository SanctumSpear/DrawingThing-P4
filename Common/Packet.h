/**
 * @file Packet.h
 * @brief Defines packet types, packet headers, and packet serialization/deserialization helpers
 *        for communication between the client and server.
 */

#pragma once
#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <stdexcept>

/**
 * @enum PacketType
 * @brief Identifies the type of packet being sent between client and server.
 */
enum class PacketType : uint8_t {
    CONNECT = 0x01, /**< Client connection packet */
    DISCONNECT = 0x02, /**< Client disconnection packet */
    PROMPT = 0x03, /**< Prompt text sent to a client */
    IMAGE = 0x04, /**< Image or drawing data */
    GAME_START = 0x05, /**< Indicates the game has started */
    GAME_END = 0x06, /**< Indicates the game has ended */
    ACK = 0x07, /**< Acknowledgement packet */
    GAME_ERROR = 0x08, /**< Error message packet */
    VOTE = 0x09, /**< Vote packet containing selected player ID */
    VOTE_REQUEST = 0x0A, /**< Request asking clients to vote */
    RESULTS = 0x0B  /**< Final results summary */
};

/**
 * @enum GameState
 * @brief Represents the current game phase tied to a packet.
 */
enum class GameState : uint8_t {
    STARTUP = 0x01, /**< Game is starting up */
    WAITING = 0x02, /**< Waiting for players or input */
    SENDING = 0x03, /**< Sending data */
    RECEIVING = 0x04, /**< Receiving data */
    ENDING = 0x05, /**< Game is ending */
    VOTING = 0x06  /**< Voting phase */
};

/**
 * @struct PacketHeader
 * @brief Header information stored at the front of every packet.
 */
struct PacketHeader {
    uint32_t payloadSize; /**< Size of the packet payload in bytes */
    uint32_t CRC;         /**< CRC or checksum value for payload validation */
    PacketType type;      /**< Type of packet */
    GameState state;      /**< Game state associated with the packet */
    uint8_t sessionID;    /**< Session the packet belongs to */
    uint8_t srcAddress;   /**< Source player/server address */
    uint8_t dstAddress;   /**< Destination player/server address */
};

/**
 * @class Packet
 * @brief Represents a packet of data exchanged between client and server.
 *
 * A packet contains a header and an optional payload. This class also provides
 * helper functions for building, serializing, and reading packets.
 */
class Packet {
public:
    PacketHeader header; /**< Packet header */
    char* data;          /**< Raw payload data */

    /**
     * @brief Default constructor.
     *
     * Initializes the packet header to zero and sets the payload pointer to null.
     */
    Packet() : data(nullptr) {
        memset(&header, 0, sizeof(PacketHeader));
    }

    /**
     * @brief Copy constructor.
     * @param packet Packet to copy
     */
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

    /**
     * @brief Destructor.
     *
     * Frees any dynamically allocated payload memory.
     */
    ~Packet() {
        delete[] data;
    }

    /**
     * @brief Creates a CONNECT packet.
     * @param sessionID Session ID
     * @param src Source address
     * @param dst Destination address
     * @return Constructed connect packet
     */
    static Packet MakeConnectPacket(uint8_t sessionID,
        uint8_t src = 0, uint8_t dst = 0) {
        Packet p;
        p.header.type = PacketType::CONNECT;
        p.header.state = GameState::WAITING;
        p.header.sessionID = sessionID;
        p.header.srcAddress = src;
        p.header.dstAddress = dst;
        p.header.payloadSize = 0;
        p.header.CRC = 0;
        return p;
    }

    /**
     * @brief Creates a PROMPT packet.
     * @param sessionID Session ID
     * @param prompt Prompt text
     * @param src Source address
     * @param dst Destination address
     * @return Constructed prompt packet
     */
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

    /**
     * @brief Creates a GAME_START packet.
     * @param sessionID Session ID
     * @param src Source address
     * @param dst Destination address
     * @return Constructed game start packet
     */
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

    /**
     * @brief Creates a GAME_END packet.
     * @param sessionID Session ID
     * @param src Source address
     * @param dst Destination address
     * @return Constructed game end packet
     */
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

    /**
     * @brief Creates an ACK packet.
     * @param sessionID Session ID
     * @param src Source address
     * @param dst Destination address
     * @return Constructed acknowledgement packet
     */
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

    /**
     * @brief Creates an error packet.
     * @param sessionID Session ID
     * @param message Error message text
     * @param src Source address
     * @param dst Destination address
     * @return Constructed error packet
     */
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

    /**
     * @brief Creates an IMAGE packet.
     * @param sessionID Session ID
     * @param imageData Pointer to image bytes
     * @param imageSize Size of image data in bytes
     * @param src Source address
     * @param dst Destination address
     * @return Constructed image packet
     */
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

    /**
     * @brief Serializes the packet into a byte buffer.
     * @param outTotalSize Receives the total serialized size
     * @return Pointer to newly allocated serialized buffer
     */
    char* Serialize(uint32_t& outTotalSize) const {
        outTotalSize = sizeof(PacketHeader) + header.payloadSize;
        char* buffer = new char[outTotalSize];
        memcpy(buffer, &header, sizeof(PacketHeader));
        if (data && header.payloadSize > 0)
            memcpy(buffer + sizeof(PacketHeader), data, header.payloadSize);
        return buffer;
    }

    /**
     * @brief Deserializes a raw byte buffer into a Packet object.
     * @param buffer Raw packet buffer
     * @param totalSize Total size of the buffer
     * @return Deserialized packet
     * @throws std::runtime_error if the buffer is too small
     */
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

    /**
     * @brief Checks whether the payload CRC matches the stored CRC.
     * @return true if valid or payload is empty, otherwise false
     */
    bool ValidateCRC() const {
        if (header.payloadSize == 0) return true;
        return ComputeCRC(data, header.payloadSize) == header.CRC;
    }

    /**
     * @brief Returns the payload as a prompt string.
     * @return Prompt string
     */
    std::string GetPromptString() const {
        if (!data) return "";
        return std::string(data);
    }

    /**
     * @brief Returns the payload as an error string.
     * @return Error message string
     */
    std::string GetErrorString() const {
        if (!data) return "";
        return std::string(data);
    }

    /**
     * @brief Creates a VOTE packet.
     * @param sessionID Session ID
     * @param votedPlayerId ID of the player being voted for
     * @param src Source address
     * @param dst Destination address
     * @return Constructed vote packet
     */
    static Packet MakeVotePacket(uint8_t sessionID, uint8_t votedPlayerId,
        uint8_t src = 0, uint8_t dst = 0) {
        Packet p;
        p.header.type = PacketType::VOTE;
        p.header.state = GameState::VOTING;
        p.header.sessionID = sessionID;
        p.header.srcAddress = src;
        p.header.dstAddress = dst;
        p.header.payloadSize = 1;
        p.data = new char[1];
        p.data[0] = (char)votedPlayerId;
        p.header.CRC = 0;
        return p;
    }

    /**
     * @brief Creates a VOTE_REQUEST packet.
     * @param sessionID Session ID
     * @param playerList Newline-separated player list
     * @param src Source address
     * @param dst Destination address
     * @return Constructed vote request packet
     */
    static Packet MakeVoteRequestPacket(uint8_t sessionID,
        const std::string& playerList,
        uint8_t src = 0, uint8_t dst = 0) {
        Packet p;
        p.header.type = PacketType::VOTE_REQUEST;
        p.header.state = GameState::VOTING;
        p.header.sessionID = sessionID;
        p.header.srcAddress = src;
        p.header.dstAddress = dst;
        p.header.payloadSize = (uint32_t)(playerList.size() + 1);
        p.data = new char[p.header.payloadSize];
        memcpy(p.data, playerList.c_str(), p.header.payloadSize);
        p.header.CRC = ComputeCRC(p.data, p.header.payloadSize);
        return p;
    }

    /**
     * @brief Creates a RESULTS packet.
     * @param sessionID Session ID
     * @param results Human-readable results string
     * @param src Source address
     * @param dst Destination address
     * @return Constructed results packet
     */
    static Packet MakeResultsPacket(uint8_t sessionID,
        const std::string& results,
        uint8_t src = 0, uint8_t dst = 0) {
        Packet p;
        p.header.type = PacketType::RESULTS;
        p.header.state = GameState::VOTING;
        p.header.sessionID = sessionID;
        p.header.srcAddress = src;
        p.header.dstAddress = dst;
        p.header.payloadSize = (uint32_t)(results.size() + 1);
        p.data = new char[p.header.payloadSize];
        memcpy(p.data, results.c_str(), p.header.payloadSize);
        p.header.CRC = ComputeCRC(p.data, p.header.payloadSize);
        return p;
    }

    /**
     * @brief Reads the voted player ID from a VOTE packet.
     * @return Voted player ID, or 0 if unavailable
     */
    uint8_t GetVotedPlayerId() const {
        if (!data || header.payloadSize == 0) return 0;
        return (uint8_t)data[0];
    }

    /**
     * @brief Returns the payload as a generic string.
     * @return Payload string
     */
    std::string GetPayloadString() const {
        if (!data || header.payloadSize == 0) return "";
        return std::string(data);
    }

private:
    /**
     * @brief Computes a simple CRC/count value for the payload.
     * @param buf Payload buffer
     * @param length Payload length
     * @return Computed CRC value
     */
    static uint32_t ComputeCRC(const char* buf, uint32_t length) {
        if (length <= sizeof(uint32_t)) return 0;
        uint32_t counter = 0;
        for (uint32_t byte = 0; byte < length - sizeof(uint32_t); byte++)
            for (int bit = 0; bit < 8; bit++)
                if (buf[byte] & (1 << bit))
                    counter++;
        return counter;
    }
};