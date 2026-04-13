#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include "../Common/Packet.h"

/**
 * @class Player
 * @brief Stores basic information about a player in the game session.
 */
class Player {

    std::string name;   ///< Player username
    int id;             ///< Unique player ID
    int score;          ///< Current score
    int socketIndex;    ///< Socket index used by the server

public:
    /**
     * @brief Default constructor.
     */
    Player() : name(""), id(0), score(0), socketIndex(-1) {}

    /**
     * @brief Creates a player with a name, ID, and socket index.
     * @param playerName The player's username
     * @param playerId The player's ID
     * @param sockIdx The socket index associated with the player
     */
    Player(const std::string& playerName, int playerId, int sockIdx)
        : name(playerName), id(playerId), score(0), socketIndex(sockIdx) {}

    /**
     * @brief Gets the player's name.
     * @return Player name
     */
    std::string GetName() const { return name; }

    /**
     * @brief Gets the player's ID.
     * @return Player ID
     */
    int GetId() const { return id; }

    /**
     * @brief Gets the player's score.
     * @return Current score
     */
    int GetScore() const { return score; }

    /**
     * @brief Gets the player's socket index.
     * @return Socket index
     */
    int GetSocketIndex() const { return socketIndex; }

    /**
     * @brief Adds points to the player's score.
     * @param points Number of points to add
     */
    void AddScore(int points) { score += points; }

    /**
     * @brief Prints player information to the console.
     */
    void Print() const {
        std::cout << "  Player [" << id << "] " << name
            << "  |  Score: " << score
            << "  |  Socket: " << socketIndex << "\n";
    }
};

/**
 * @class Game
 * @brief Manages a single drawing game session.
 *
 * Stores players, game state, prompts, votes, and submitted images.
 */
class Game {

    uint8_t             sessionID;          ///< Session ID for this game
    GameState           currentState;       ///< Current state of the game
    std::vector<Player> players;            ///< Players in the session
    bool                programRunning;     ///< Indicates whether the game loop should continue
    int                 currentDrawerIndex; ///< Index of the current drawing player
    std::string         currentPrompt;      ///< Prompt currently assigned for drawing

    /// Maps player ID to raw image bytes
    std::map<int, std::vector<char>> playerImages;

    /// Maps voter ID to the player they voted for
    std::map<int, int> playerVotes;

public:
    /**
     * @brief Creates a new game session.
     * @param sid Session ID
     */
    Game(uint8_t sid)
        : sessionID(sid),
        currentState(GameState::STARTUP),
        programRunning(true),
        currentDrawerIndex(0),
        currentPrompt("") {}

    /**
     * @brief Gets the session ID.
     * @return Session ID
     */
    uint8_t GetSessionID() const { return sessionID; }

    /**
     * @brief Checks whether the game is still running.
     * @return true if running
     */
    bool ProgramRunning() const { return programRunning; }

    /**
     * @brief Updates the running flag.
     * @param val New running state
     */
    void ChangeProgramRunning(bool val) { programRunning = val; }

    /**
     * @brief Changes the current game state.
     * @param s New game state
     */
    void ChangeState(GameState s) { currentState = s; }

    /**
     * @brief Gets the current game state.
     * @return Current state
     */
    GameState GetChangeState() const { return currentState; }

    /**
     * @brief Adds a player to the session.
     * @param name Player name
     * @param id Player ID
     * @param socketIndex Socket index used by the player
     */
    void AddPlayer(const std::string& name, int id, int socketIndex) {
        players.emplace_back(name, id, socketIndex);
        std::cout << "Player joined: " << name
            << " (ID:" << id << ", socket:" << socketIndex << ")\n";
    }

    /**
     * @brief Gets the total number of players.
     * @return Number of players
     */
    int GetPlayerCount() const { return (int)players.size(); }

    /**
     * @brief Gets a player by index.
     * @param index Index in the player list
     * @return Reference to the player
     */
    const Player& GetPlayer(int index) const { return players.at(index); }

    /**
     * @brief Finds a player by ID.
     * @param id Player ID to search for
     * @return Pointer to the player if found, otherwise nullptr
     */
    const Player* FindPlayerById(int id) const {
        for (const auto& p : players)
            if (p.GetId() == id) return &p;
        return nullptr;
    }

    /**
     * @brief Awards points to a player.
     * @param playerId ID of the player receiving points
     * @param points Number of points to award
     */
    void AwardPoints(int playerId, int points) {
        for (auto& p : players) {
            if (p.GetId() == playerId) {
                p.AddScore(points);
                std::cout << p.GetName() << " awarded " << points << " points.\n";
                return;
            }
        }
    }

    /**
     * @brief Awards points based on votes received.
     * @param pointsPerVote Number of points per vote
     * @return ID of the winning player, or -1 if no votes were cast
     */
    int AwardVotePoints(int pointsPerVote) {
        std::map<int, int> tally;
        for (const auto& kv : playerVotes)
            tally[kv.second]++;

        for (const auto& kv : tally)
            AwardPoints(kv.first, kv.second * pointsPerVote);

        return GetWinnerId();
    }

    /**
     * @brief Prints all players in the current session.
     */
    void PrintPlayers() const {
        std::cout << "=== Session " << (int)sessionID << " Players ===\n";
        for (const auto& p : players)
            p.Print();
    }

    /**
     * @brief Sets the current drawing prompt.
     * @param prompt Prompt text
     */
    void SetPrompt(const std::string& prompt) { currentPrompt = prompt; }

    /**
     * @brief Gets the current drawing prompt.
     * @return Prompt string
     */
    const std::string& GetPrompt() const { return currentPrompt; }

    /**
     * @brief Gets the player currently assigned to draw.
     * @return Current drawer
     */
    const Player& GetCurrentDrawer() const {
        return players.at(currentDrawerIndex);
    }

    /**
     * @brief Advances to the next drawing player.
     */
    void NextDrawer() {
        currentDrawerIndex = (currentDrawerIndex + 1) % (int)players.size();
    }

    /**
     * @brief Stores an image submitted by a player.
     * @param playerId Player ID
     * @param data Raw image bytes
     * @param size Number of bytes
     */
    void StorePlayerImage(int playerId, const char* data, uint32_t size) {
        playerImages[playerId].assign(data, data + size);
        std::cout << "Stored image for Player " << playerId
            << " (" << size << " bytes).\n";
    }

    /**
     * @brief Checks whether a player has submitted an image.
     * @param playerId Player ID
     * @return true if image exists
     */
    bool HasPlayerImage(int playerId) const {
        return playerImages.count(playerId) > 0;
    }

    /**
     * @brief Gets a stored image for a player.
     * @param playerId Player ID
     * @return Raw image byte vector
     */
    const std::vector<char>& GetPlayerImage(int playerId) const {
        return playerImages.at(playerId);
    }

    /**
     * @brief Records a player's vote.
     * @param voterId Player casting the vote
     * @param votedForId Player receiving the vote
     * @return true if the vote was valid
     */
    bool RecordVote(int voterId, int votedForId) {
        if (!FindPlayerById(votedForId)) {
            std::cout << "  [WARN] Player " << voterId
                << " voted for non-existent player " << votedForId << " — ignored.\n";
            return false;
        }
        playerVotes[voterId] = votedForId;
        const Player* voter = FindPlayerById(voterId);
        const Player* votedFor = FindPlayerById(votedForId);
        std::cout << "  Vote: "
            << (voter ? voter->GetName() : "?") << " -> "
            << (votedFor ? votedFor->GetName() : "?") << "\n";
        return true;
    }

    /**
     * @brief Determines which player has the most votes.
     * @return Winner's player ID, or -1 if no votes were cast
     */
    int GetWinnerId() const {
        std::map<int, int> tally;
        for (const auto& kv : playerVotes)
            tally[kv.second]++;

        int winnerId = -1;
        int maxVotes = 0;
        for (const auto& kv : tally) {
            if (kv.second > maxVotes) {
                maxVotes = kv.second;
                winnerId = kv.first;
            }
        }
        return winnerId;
    }

    /**
     * @brief Builds the vote request payload string.
     * @return String in the format "ID:Name\\nID:Name..."
     */
    std::string GetVoteRequestString() const {
        std::string result;
        for (const auto& p : players) {
            if (!result.empty()) result += "\n";
            result += std::to_string(p.GetId()) + ":" + p.GetName();
        }
        return result;
    }

    /**
     * @brief Builds the final human-readable results string.
     * @return Results summary string
     */
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