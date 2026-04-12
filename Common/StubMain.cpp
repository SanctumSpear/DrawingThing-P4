#include <iostream>
#include <string>
#include "../Client/client.h"
#include "../Common/Packet.h"

static const std::string SERVER_IP   = "127.0.0.1";
static const int         SERVER_PORT = 54000;
static const std::string USERNAME    = "Alice";      
static const std::string PASSWORD    = "password123";

int main(void) {
    try {
    std::cout << "  StubMain -- Player: " << USERNAME << "\n";
   

    Client client(SERVER_IP, SERVER_PORT);
    std::cout << "Connected to server at "
              << SERVER_IP << ":" << SERVER_PORT << "\n\n";


    std::cout << "--- Phase: Authentication ---\n";
    bool authed = client.Authenticate(USERNAME, PASSWORD);

    if (!authed) {
        std::cerr << "Authentication failed for \"" << USERNAME << "\".\n";
        client.Cleanup();
        return 1;
    }
    std::cout << "Authenticated as " << USERNAME << ".\n\n";

  
    std::cout << "--- Phase: Waiting for Game Start ---\n";
    Packet startPkt = client.ReceivePacket();
    if (startPkt.header.type == PacketType::GAME_START) {
        std::cout << "GAME_START received -- game is beginning!\n\n";
    }


    std::cout << "--- Phase: Receiving Prompt (if drawer) ---\n";
    Packet next = client.ReceivePacket();

    if (next.header.type == PacketType::PROMPT) {
        bool crcOk = next.ValidateCRC();
        std::cout << "PROMPT received: \"" << next.GetPromptString() << "\"\n";
        std::cout << "CRC valid: " << (crcOk ? "YES" : "NO") << "\n\n";

        std::cout << "--- Phase: ACK Prompt ---\n";
        Packet ack = Packet::MakeAckPacket(next.header.sessionID,
                                           next.header.dstAddress, 0);
        client.SendPacket(ack);
        std::cout << "ACK sent.\n\n";

        std::cout << "--- Phase: Sending Image ---\n";
        const char dummyImage[] = "DUMMY_JPEG_DATA";
        Packet imgPkt = Packet::MakeImagePacket(
            next.header.sessionID,
            dummyImage, sizeof(dummyImage),
            next.header.dstAddress, 0);
        client.SendPacket(imgPkt);
        std::cout << "IMAGE sent (" << sizeof(dummyImage) << " bytes).\n\n";

        next = client.ReceivePacket();
    }


    std::cout << "--- Phase: Game End ---\n";
    if (next.header.type == PacketType::GAME_END) {
        std::cout << "GAME_END received -- session "
                  << (int)next.header.sessionID << " is over.\n";
    }

    std::cout << "  Stub complete for " << USERNAME << ".\n";

    client.Cleanup();

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << "\n";
        std::cerr << "Make sure ServerMain is running before starting StubMain.\n";
        return 1;
    }

    return 0;
}
