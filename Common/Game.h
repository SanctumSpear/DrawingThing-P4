#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include "../Common/Packet.h"


class Player {

    std::string name;
    int id;          
    int score;
    int socketIndex; 

public:
    Player() : name(""), id(0), score(0), socketIndex(-1) {}

    Player(const std::string& playerName, int playerId, int sockIdx)
        : name(playerName), id(playerId), score(0), socketIndex(sockIdx) {}

    std::string GetName() const { return name; }
    int GetId() const { return id; }
    int GetScore() const { return score; }
    int GetSocketIndex() const { return socketIndex; }

    void AddScore(int points) { score += points; }

    void Print() const {
        std::cout << "  Player [" << id << "] " << name
                  << "  |  Score: " << score
                  << "  |  Socket: " << socketIndex << "\n";
    }
};

class Game {

    uint8_t             sessionID;
    GameState           currentState;
    std::vector<Player> players;
    bool                programRunning;
    int                 currentDrawerIndex;
    std::string         currentPrompt;

    // Voting support
    std::map<int, std::vector<char>> playerImages; // playerId -> raw pixel bytes
    std::map<int, int>               playerVotes;  // voterId  -> votedForId

public:
    Game(uint8_t sid)
        : sessionID(sid),
          currentState(GameState::STARTUP),
          programRunning(true),
          currentDrawerIndex(0),
          currentPrompt("") {}


    uint8_t GetSessionID() const { return sessionID; }

    bool ProgramRunning()               const { return programRunning; }
    void ChangeProgramRunning(bool val)       { programRunning = val; }

    void      ChangeState(GameState s) { currentState = s; }
    GameState GetChangeState()   const { return currentState; }

    void AddPlayer(const std::string& name, int id, int socketIndex) {
        players.emplace_back(name, id, socketIndex);
        std::cout << "Player joined: " << name
                  << " (ID:" << id << ", socket:" << socketIndex << ")\n";
    }

    int           GetPlayerCount()     const { return (int)players.size(); }
    const Player& GetPlayer(int index) const { return players.at(index); }

    const Player* FindPlayerById(int id) const {
        for (const auto& p : players)
            if (p.GetId() == id) return &p;
        return nullptr;
    }

    void AwardPoints(int playerId, int points) {
        for (auto& p : players) {
            if (p.GetId() == playerId) {
                p.AddScore(points);
                std::cout << p.GetName() << " awarded " << points << " points.\n";
                return;
            }
        }
    }

    void PrintPlayers() const {
        std::cout << "=== Session " << (int)sessionID << " Players ===\n";
        for (const auto& p : players)
            p.Print();
    }

    void SetPrompt(const std::string& prompt) { currentPrompt = prompt; }
    const std::string& GetPrompt() const { return currentPrompt; }

    const Player& GetCurrentDrawer() const {
        return players.at(currentDrawerIndex);
    }

    void NextDrawer() {
        currentDrawerIndex = (currentDrawerIndex + 1) % (int)players.size();
    }

    void StorePlayerImage(int playerId, const char* data, uint32_t size) {
        playerImages[playerId].assign(data, data + size);
        std::cout << "Stored image for Player " << playerId
                  << " (" << size << " bytes).\n";
    }

    bool HasPlayerImage(int playerId) const {
        return playerImages.count(playerId) > 0;
    }

    const std::vector<char>& GetPlayerImage(int playerId) const {
        return playerImages.at(playerId);
    }


    // Record a vote. Returns false if votedForId is invalid.
    bool RecordVote(int voterId, int votedForId) {
        if (!FindPlayerById(votedForId)) {
            std::cout << "  [WARN] Player " << voterId
                      << " voted for non-existent player " << votedForId << " — ignored.\n";
            return false;
        }
        playerVotes[voterId] = votedForId;
        const Player* voter    = FindPlayerById(voterId);
        const Player* votedFor = FindPlayerById(votedForId);
        std::cout << "  Vote: "
                  << (voter    ? voter->GetName()    : "?") << " -> "
                  << (votedFor ? votedFor->GetName() : "?") << "\n";
        return true;
    }

    // Returns the player ID with the most votes (-1 if no votes cast).
    // Ties are broken by lowest player ID.
    int GetWinnerId() const {
        std::map<int, int> tally; // playerId -> vote count
        for (const auto& kv : playerVotes)
            tally[kv.second]++;

        int winnerId  = -1;
        int maxVotes  = 0;
        for (const auto& kv : tally) {
            if (kv.second > maxVotes) {
                maxVotes = kv.second;
                winnerId = kv.first;
            }
        }
        return winnerId;
    }

    // Returns "ID:Name\nID:Name\n..." for the vote request payload.
    std::string GetVoteRequestString() const {
        std::string result;
        for (const auto& p : players) {
            if (!result.empty()) result += "\n";
            result += std::to_string(p.GetId()) + ":" + p.GetName();
        }
        return result;
    }

    // Builds the human-readable results string that is broadcast to clients.
    // Call AwardPoints() before this so scores are already updated.
    std::string GetResultsString() const {
        std::map<int, int> tally;
        for (const auto& kv : playerVotes)
            tally[kv.second]++;

        int winnerId = GetWinnerId();
        const Player* winner = FindPlayerById(winnerId);

        std::ostringstream oss;
        oss << "=== VOTING RESULTS ===\n";
        if (winner)
            oss << "Winner: " << winner->GetName() << "!\n\n";
        else
            oss << "No votes were cast.\n\n";

        oss << "Vote tally:\n";
        for (const auto& p : players) {
            int votes = tally.count(p.GetId()) ? tally.at(p.GetId()) : 0;
            oss << "  " << p.GetName() << ": " << votes << " vote(s)\n";
        }

        oss << "\nFinal Scores:\n";
        for (const auto& p : players)
            oss << "  " << p.GetName() << ": " << p.GetScore() << " point(s)\n";

        return oss.str();
    }
};
