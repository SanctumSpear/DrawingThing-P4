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
        case GameState::ENDING:    name = "ENDING";    break;
    }
    std::cout << "[STATE] -> " << name << "\n";
    log.Log(SC_XSTATE, game.GetSessionID(), true, name);
}

int main(void) {

    const int MAX_PLAYERS = 1;
    const int SESSION_ID  = 1;

    Logger         log("server_log.txt");
    AccountManager accounts("accounts.txt");
    Server         server(54000);
    Game           game((uint8_t)SESSION_ID);

    std::cout << "Server listening on port 54000...\n";

    EnterState(game, GameState::STARTUP, log);

    EnterState(game, GameState::WAITING, log);
    std::cout << "Waiting for " << MAX_PLAYERS << " players...\n\n";

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
                game.GetSessionID(), "Bad credentials format", 0, (uint8_t)(sockIdx + 1));
            server.SendPacket(sockIdx, err);
            log.Log(SL_LOGIN, game.GetSessionID(), false, "malformed credentials");
            server.CloseClient(sockIdx);
            continue;
        }

        std::string username = credentials.substr(0, colonPos);
        std::string password = credentials.substr(colonPos + 1);

        if (!accounts.Authenticate(username, password)) {
            Packet err = Packet::MakeErrorPacket(
                game.GetSessionID(), "Authentication failed", 0, (uint8_t)(sockIdx + 1));
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
    const Player& drawer = game.GetCurrentDrawer();

    Packet promptPkt = Packet::MakePromptPacket(
        game.GetSessionID(), game.GetPrompt(),
        0, (uint8_t)drawer.GetId());

    server.SendPacket(drawer.GetSocketIndex(), promptPkt);
    std::cout << "PROMPT sent to " << drawer.GetName()
              << ": \"" << game.GetPrompt() << "\"\n";
    log.Log(SS_PRMT, game.GetSessionID(), true,
            "prompt to " + drawer.GetName());

    EnterState(game, GameState::WAITING, log);

    Packet drawerAck = server.ReceivePacket(drawer.GetSocketIndex());
    if (drawerAck.header.type == PacketType::ACK) {
        std::cout << "Drawer ACK received from " << drawer.GetName() << ".\n";
        log.Log(SR_RECV, game.GetSessionID(), true,
                "ACK from " + drawer.GetName());
    }

    EnterState(game, GameState::RECEIVING, log);
    std::cout << "Waiting for image from " << drawer.GetName() << "...\n";

    Packet imgPkt = server.ReceivePacket(drawer.GetSocketIndex());
    if (imgPkt.header.type == PacketType::IMAGE) {
        bool crcOk = imgPkt.ValidateCRC();
        std::cout << "IMAGE received (" << imgPkt.header.payloadSize
                  << " bytes, CRC " << (crcOk ? "OK" : "FAIL") << ").\n";
        log.Log(SR_JPEG, game.GetSessionID(), crcOk,
                std::to_string(imgPkt.header.payloadSize) + " bytes");
    }

    game.AwardPoints(2, 100);
    game.AwardPoints(3, 50);
    game.PrintPlayers();
    std::cout << "\n";

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
