#include <iostream>
#include <string>
#include "Communication.h"
#include "AccountManager.h"
#include "../Common/Game.h"
#include "../Common/Packet.h"
#include "../Common/logger.h"

static void EnterState(Game& game, GameState s, Logger& log) {
    game.ChangeState(s);
    std::string name;
    switch (s) {
        case GameState::STARTUP:   name = "STARTUP";   break;
        case GameState::WAITING:   name = "WAITING";   break;
        case GameState::SENDING:   name = "SENDING";   break;
        case GameState::RECEIVING: name = "RECEIVING"; break;
        case GameState::VOTING:    name = "VOTING";    break;
        case GameState::ENDING:    name = "ENDING";    break;
    }
    std::cout << "[STATE] -> " << name << "\n";
    log.Log(SC_XSTATE, game.GetSessionID(), true, name);
}

int main(void) {

    const int MAX_PLAYERS = 1;   // all connected players draw in round 1
    const int SESSION_ID  = 1;

    Logger         log("server_log.txt");
    AccountManager accounts("accounts.txt");
    Server         server(54000);
    Game           game((uint8_t)SESSION_ID);

    std::cout << "Server listening on port 54000...\n";

    EnterState(game, GameState::STARTUP, log);
    EnterState(game, GameState::WAITING, log);
    std::cout << "Waiting for " << MAX_PLAYERS << " player(s)...\n\n";

    while (game.GetPlayerCount() < MAX_PLAYERS) {

        int sockIdx = server.AcceptClient();
        std::cout << "Connection accepted (socket " << sockIdx << ").\n";
        log.Log(SC_CONNECT, game.GetSessionID(), true,
                "socket " + std::to_string(sockIdx));

        std::string credentials = server.ReceiveString(sockIdx);
        log.Log(SR_RECV, game.GetSessionID(), true,
                "credentials from socket " + std::to_string(sockIdx));

        auto colonPos = credentials.find(':');
        if (colonPos == std::string::npos) {
            Packet err = Packet::MakeErrorPacket(
                game.GetSessionID(), "Bad credentials format",
                0, (uint8_t)(sockIdx + 1));
            server.SendPacket(sockIdx, err);
            log.Log(SL_LOGIN, game.GetSessionID(), false, "malformed credentials");
            server.CloseClient(sockIdx);
            continue;
        }

        std::string username = credentials.substr(0, colonPos);
        std::string password = credentials.substr(colonPos + 1);

        if (!accounts.Authenticate(username, password)) {
            Packet err = Packet::MakeErrorPacket(
                game.GetSessionID(), "Authentication failed",
                0, (uint8_t)(sockIdx + 1));
            server.SendPacket(sockIdx, err);
            std::cout << "  Auth FAILED for \"" << username << "\"\n";
            log.Log(SL_LOGIN, game.GetSessionID(), false, username);
            server.CloseClient(sockIdx);
            continue;
        }

        int playerId = game.GetPlayerCount() + 1;
        game.AddPlayer(username, playerId, sockIdx);

        Packet ack = Packet::MakeAckPacket(
            game.GetSessionID(), 0, (uint8_t)playerId);
        server.SendPacket(sockIdx, ack);

        std::cout << "  Auth OK: " << username
                  << " -> Player " << playerId << "\n\n";
        log.Log(SL_LOGIN, game.GetSessionID(), true, username);
    }

    game.PrintPlayers();
    std::cout << "\n";

    EnterState(game, GameState::SENDING, log);

    Packet startPkt = Packet::MakeGameStartPacket(game.GetSessionID(), 0, 0);
    server.BroadcastPacket(startPkt);
    std::cout << "GAME_START broadcast.\n";
    log.Log(SS_SEND, game.GetSessionID(), true, "GAME_START broadcast");

    game.SetPrompt("A cat riding a bicycle");

    for (int i = 0; i < game.GetPlayerCount(); i++) {
        const Player& p = game.GetPlayer(i);
        Packet promptPkt = Packet::MakePromptPacket(
            game.GetSessionID(), game.GetPrompt(),
            0, (uint8_t)p.GetId());
        server.SendPacket(p.GetSocketIndex(), promptPkt);
        std::cout << "PROMPT sent to " << p.GetName()
                  << ": \"" << game.GetPrompt() << "\"\n";
        log.Log(SS_PRMT, game.GetSessionID(), true,
                "prompt -> " + p.GetName());
    }

    EnterState(game, GameState::WAITING, log);
    std::cout << "Waiting for ACKs from all players...\n";

    for (int i = 0; i < game.GetPlayerCount(); i++) {
        const Player& p = game.GetPlayer(i);
        Packet ack = server.ReceivePacket(p.GetSocketIndex());
        if (ack.header.type == PacketType::ACK) {
            std::cout << "  ACK from " << p.GetName() << ".\n";
            log.Log(SR_RECV, game.GetSessionID(), true,
                    "ACK from " + p.GetName());
        }
    }

    EnterState(game, GameState::RECEIVING, log);
    std::cout << "\nWaiting for drawings from all players...\n";

    for (int i = 0; i < game.GetPlayerCount(); i++) {
        const Player& p = game.GetPlayer(i);
        std::cout << "  Waiting for image from " << p.GetName() << "...\n";

        Packet imgPkt = server.ReceivePacket(p.GetSocketIndex());
        if (imgPkt.header.type == PacketType::IMAGE) {
            bool crcOk = imgPkt.ValidateCRC();
            game.StorePlayerImage(p.GetId(), imgPkt.data,
                                  imgPkt.header.payloadSize);
            std::cout << "  IMAGE from " << p.GetName()
                      << " (" << imgPkt.header.payloadSize
                      << " bytes, CRC " << (crcOk ? "OK" : "FAIL") << ").\n";
            log.Log(SR_JPEG, game.GetSessionID(), crcOk,
                    std::to_string(imgPkt.header.payloadSize)
                    + " bytes from " + p.GetName());
        }
    }

    std::cout << "All drawings received.\n\n";
    EnterState(game, GameState::VOTING, log);

    for (int i = 0; i < game.GetPlayerCount(); i++) {
        const Player& p       = game.GetPlayer(i);
        const auto&   imgData = game.GetPlayerImage(p.GetId());

        Packet imgBcast = Packet::MakeImagePacket(
            game.GetSessionID(),
            imgData.data(),
            (uint32_t)imgData.size(),
            (uint8_t)p.GetId(),   // srcAddress = drawing player's ID
            0);
        server.BroadcastPacket(imgBcast);
        std::cout << "Broadcast " << p.GetName() << "'s drawing ("
                  << imgData.size() << " bytes) to all players.\n";
        log.Log(SS_JPEG, game.GetSessionID(), true,
                "broadcast drawing from " + p.GetName());
    }

    // Send VOTE_REQUEST to all clients with the full player list.
    std::string playerList = game.GetVoteRequestString();
    Packet voteReqPkt = Packet::MakeVoteRequestPacket(
        game.GetSessionID(), playerList, 0, 0);
    server.BroadcastPacket(voteReqPkt);
    std::cout << "VOTE_REQUEST broadcast.\n";
    log.Log(SS_SEND, game.GetSessionID(), true, "VOTE_REQUEST broadcast");

    // Collect one VOTE packet from each player.
    std::cout << "\nCollecting votes...\n";
    for (int i = 0; i < game.GetPlayerCount(); i++) {
        const Player& p = game.GetPlayer(i);
        Packet votePkt = server.ReceivePacket(p.GetSocketIndex());

        if (votePkt.header.type == PacketType::VOTE) {
            uint8_t votedId = votePkt.GetVotedPlayerId();

            // Prevent self-voting (server-side guard)
            if ((int)votedId == p.GetId()) {
                std::cout << "  " << p.GetName()
                          << " tried to vote for themselves — ignored.\n";
                log.Log(SR_RECV, game.GetSessionID(), false,
                        p.GetName() + " self-vote rejected");
            } else {
                game.RecordVote(p.GetId(), (int)votedId);
                log.Log(SR_RECV, game.GetSessionID(), true,
                        p.GetName() + " voted for player "
                        + std::to_string(votedId));
            }
        }
    }

    // Award 100 points to the winner.
    int winnerId = game.GetWinnerId();
    if (winnerId != -1) {
        game.AwardPoints(winnerId, 100);
        std::cout << "\nWinner: Player " << winnerId << " gets 100 points!\n";
        log.Log(SS_SEND, game.GetSessionID(), true,
                "awarded 100 pts to player " + std::to_string(winnerId));
    } else {
        std::cout << "\nNo votes cast — no winner.\n";
    }

    game.PrintPlayers();
    std::cout << "\n";

    EnterState(game, GameState::SENDING, log);

    std::string resultsStr = game.GetResultsString();
    Packet resultsPkt = Packet::MakeResultsPacket(
        game.GetSessionID(), resultsStr, 0, 0);
    server.BroadcastPacket(resultsPkt);
    std::cout << "RESULTS broadcast:\n" << resultsStr;
    log.Log(SS_SEND, game.GetSessionID(), true, "RESULTS broadcast");

    EnterState(game, GameState::ENDING, log);

    Packet endPkt = Packet::MakeGameEndPacket(game.GetSessionID(), 0, 0);
    server.BroadcastPacket(endPkt);
    std::cout << "GAME_END broadcast.\n";
    log.Log(SS_SEND, game.GetSessionID(), true, "GAME_END broadcast");

    game.ChangeProgramRunning(false);

    for (int i = 0; i < game.GetPlayerCount(); i++)
        log.Log(SC_DISCONNECT, game.GetSessionID(), true,
                game.GetPlayer(i).GetName());

    std::cout << "\nGame session ended.\n";
    server.Cleanup();

    return 0;
}
