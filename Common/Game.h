#pragma once

#include <iostream>
#include <vector>
#include <string>
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
};
