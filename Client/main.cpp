#include <iostream>
#include "client.h"

int main() {
    try {
        Client client("127.0.0.1", 27500);
        std::cout << "Connected to server!\n";

        // Send a message to the server
        client.SendString("Hello from client!");
        std::cout << "Client: Hello from client!\n";

        // Receive the server's response
        std::string received = client.ReceiveString();
        std::cout << "Server: " << received << "\n";

        client.Cleanup();
    }
    catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}