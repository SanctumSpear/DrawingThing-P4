#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "../Common/Packet.h"


// -------------------------------------------------------
// Player: stores a single player's name, id, and score
// -------------------------------------------------------
class Player {

    std::string name;
    int id;
    int score;

public:
    Player() : name(""), id(0), score(0) {}

    Player(const std::string& playerName, int playerId)
        : name(playerName), id(playerId), score(0) {}

    std::string GetName()  const { return name; }
    int         GetId()    const { return id; }
    int         GetScore() const { return score; }

    void AddScore(int points) { score += points; }

    void Print() const {
        std::cout << "  Player [" << id << "] " << name
                  << "  |  Score: " << score << "\n";
    }
};


// -------------------------------------------------------
// Game: manages state, players, session, and round info
// -------------------------------------------------------
class Game {

    uint8_t            sessionID;
    GameState          currentState;
    std::vector<Player> players;
    bool               programRunning;
    int                currentDrawerIndex;
    std::string        currentPrompt;

public:
    // sessionID matches the uint8_t used in PacketHeader
    Game(uint8_t sid)
        : sessionID(sid),
          currentState(GameState::STARTUP),
          programRunning(true),
          currentDrawerIndex(0),
          currentPrompt("") {}

    // --- Session info ---
    uint8_t GetSessionID() const { return sessionID; }

    // --- Program loop control ---
    bool ProgramRunning()              const { return programRunning; }
    void ChangeProgramRunning(bool val)      { programRunning = val; }

    // --- State machine ---
    void      ChangeState(GameState newState) { currentState = newState; }
    GameState GetChangeState()          const { return currentState; }

    // --- Player management ---
    void AddPlayer(const std::string& name, int id) {
        players.emplace_back(name, id);
        std::cout << "Player joined: " << name << " (ID: " << id << ")\n";
    }

    int           GetPlayerCount()       const { return (int)players.size(); }
    const Player& GetPlayer(int index)   const { return players.at(index); }

    // Award points to a player by their id
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
        for (const auto& p : players) {
            p.Print();
        }
    }

    // --- Round / drawing logic ---
    void SetPrompt(const std::string& prompt) { currentPrompt = prompt; }
    const std::string& GetPrompt() const { return currentPrompt; }

    // The player whose turn it is to draw
    const Player& GetCurrentDrawer() const {
        return players.at(currentDrawerIndex);
    }

    // Rotate to the next drawer for the next round
    void NextDrawer() {
        currentDrawerIndex = (currentDrawerIndex + 1) % (int)players.size();
    }
};
