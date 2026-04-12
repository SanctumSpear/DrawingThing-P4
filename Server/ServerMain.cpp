#include <iostream>
#include <string>
#include "Communication.h"
#include "../Common/Game.h"
#include "../Common/Packet.h"

int main(void) {

    Server server(54000);
    std::cout << "Server listening on port 54000...\n";

    // Accept one client connection (the stub / real client)
    server.AcceptClient();
    std::cout << "Client connected.\n\n";

    // Session ID 1 -- matches what StubMain uses
    Game game(1);

    // --------------------------------------------------
    // Phase 1: Wait for players to join
    // Players send a raw string in the format "<id><name>"
    // e.g. "1Alice", "2Bob", "3Joe"
    // The server adds them to the game until 3 have joined.
    // --------------------------------------------------
    std::cout << "--- Phase: Players Joining ---\n";
    game.ChangeState(GameState::WAITING);

    const int MAX_PLAYERS = 3;

    while (game.GetChangeState() == GameState::WAITING) {
        std::string playerJoin = server.ReceiveString();

        if (playerJoin.empty()) continue;

        int         playerId   = playerJoin[0] - '0';
        std::string playerName = playerJoin.substr(1);

        game.AddPlayer(playerName, playerId);

        // Send an ACK so the client knows the join was accepted
        Packet ack = Packet::MakeAckPacket((uint8_t)game.GetSessionID());
        server.SendPacket(ack);

        if (game.GetPlayerCount() == MAX_PLAYERS) {
            game.ChangeState(GameState::SENDING);
        }
    }

    game.PrintPlayers();
    std::cout << "\n";

    // --------------------------------------------------
    // Phase 2: Start the game -- broadcast GAME_START
    // --------------------------------------------------
    std::cout << "--- Phase: Game Start ---\n";

    Packet startPkt = Packet::MakeGameStartPacket((uint8_t)game.GetSessionID());
    server.SendPacket(startPkt);
    std::cout << "GAME_START sent.\n\n";

    // --------------------------------------------------
    // Phase 3: Send a drawing prompt to the current drawer
    // --------------------------------------------------
    std::cout << "--- Phase: Sending Prompt ---\n";

    game.SetPrompt("A cat riding a bicycle");
    std::cout << "Drawer: " << game.GetCurrentDrawer().GetName() << "\n";

    Packet promptPkt = Packet::MakePromptPacket(
        (uint8_t)game.GetSessionID(), game.GetPrompt());
    server.SendPacket(promptPkt);
    std::cout << "PROMPT sent: \"" << game.GetPrompt() << "\"\n\n";

    // --------------------------------------------------
    // Phase 4: Wait for ACK from the drawer
    // --------------------------------------------------
    std::cout << "--- Phase: Waiting for ACK ---\n";

    Packet ackIn = server.ReceivePacket();
    if (ackIn.header.type == PacketType::ACK) {
        std::cout << "ACK received from drawer.\n\n";
    }

    // --------------------------------------------------
    // Phase 5: Simulate guessing round -- award points
    // (In the real game, IMAGE packets would come in here
    // and players would submit guesses.)
    // --------------------------------------------------
    std::cout << "--- Phase: Awarding Points ---\n";

    game.ChangeState(GameState::RECEIVING);
    game.AwardPoints(2, 100); // Bob guessed correctly first
    game.AwardPoints(3, 50);  // Joe guessed second

    game.PrintPlayers();
    std::cout << "\n";

    // --------------------------------------------------
    // Phase 6: End the game -- send GAME_END
    // --------------------------------------------------
    std::cout << "--- Phase: Game End ---\n";

    game.ChangeState(GameState::ENDING);
    Packet endPkt = Packet::MakeGameEndPacket((uint8_t)game.GetSessionID());
    server.SendPacket(endPkt);
    std::cout << "GAME_END sent.\n";

    game.ChangeProgramRunning(false);

    std::cout << "\nGame session ended.\n";
    server.Cleanup();

    return 0;
}
