#include <iostream>
#include <string>
#include "../Client/client.h"
#include "../Common/Packet.h"

// StubMain acts as the CLIENT side of the game.
// Run ServerMain first, then run this -- it connects to
// the server and drives the full packet exchange.

int main(void) {

    std::cout << "========================================\n";
    std::cout << "  StubMain -- Client-Side Game Test\n";
    std::cout << "========================================\n\n";

    // Connect to the local server
    Client client("127.0.0.1", 54000);
    std::cout << "Connected to server.\n\n";

    // --------------------------------------------------
    // Phase 1: Send player join strings
    // Format: "<id><name>" e.g. "1Alice"
    // Server expects 3 players before starting the game.
    // --------------------------------------------------
    std::cout << "--- Phase: Players Joining ---\n";

    std::string joinRequests[] = { "1Alice", "2Bob", "3Joe" };

    for (const auto& entry : joinRequests) {
        std::cout << "Sending join: " << entry << "\n";
        client.SendString(entry);

        // Wait for the server's ACK confirming the player joined
        Packet ack = client.ReceivePacket();
        if (ack.header.type == PacketType::ACK) {
            std::cout << "  ACK received for " << entry.substr(1) << "\n";
        }
    }

    std::cout << "\n";

    // --------------------------------------------------
    // Phase 2: Receive GAME_START from server
    // --------------------------------------------------
    std::cout << "--- Phase: Game Start ---\n";

    Packet startPkt = client.ReceivePacket();
    if (startPkt.header.type == PacketType::GAME_START) {
        std::cout << "GAME_START received -- game is beginning!\n";
    }

    std::cout << "\n";

    // --------------------------------------------------
    // Phase 3: Receive the drawing prompt
    // --------------------------------------------------
    std::cout << "--- Phase: Receiving Prompt ---\n";

    Packet promptPkt = client.ReceivePacket();
    if (promptPkt.header.type == PacketType::PROMPT) {
        bool crcOk = promptPkt.ValidateCRC();
        std::cout << "PROMPT received: \"" << promptPkt.GetPromptString() << "\"\n";
        std::cout << "CRC valid: " << (crcOk ? "YES" : "NO") << "\n";
    }

    std::cout << "\n";

    // --------------------------------------------------
    // Phase 4: Send ACK back to confirm we got the prompt
    // --------------------------------------------------
    std::cout << "--- Phase: Sending ACK ---\n";

    Packet ackOut = Packet::MakeAckPacket(promptPkt.header.sessionID);
    client.SendPacket(ackOut);
    std::cout << "ACK sent to server.\n\n";

    // --------------------------------------------------
    // Phase 5: Wait for GAME_END
    // (In a real client, the IMAGE packet and guesses
    //  would happen here before the server ends the round.)
    // --------------------------------------------------
    std::cout << "--- Phase: Waiting for Game End ---\n";

    Packet endPkt = client.ReceivePacket();
    if (endPkt.header.type == PacketType::GAME_END) {
        std::cout << "GAME_END received -- session "
                  << (int)endPkt.header.sessionID << " is over.\n";
    }

    std::cout << "\n========================================\n";
    std::cout << "  Stub complete.\n";
    std::cout << "========================================\n";

    client.Cleanup();
    return 0;
}
