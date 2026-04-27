// ChatNet Client — client.cpp
// Usage: ./client <server_ip> <port>

#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <atomic>

#define BUFFER_SIZE 1024

std::atomic<bool> running(true);

// ── Receive thread — prints incoming messages ───────────
void receiveMessages(int sock) {
    char buffer[BUFFER_SIZE];
    while (running) {
        memset(buffer, 0, BUFFER_SIZE);
        int n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (n <= 0) {
            if (running) {
                std::cout << "\n[Disconnected from server]\n";
                running = false;
            }
            break;
        }
        // Print above current input line
        std::cout << "\r" << std::string(buffer, n) << "> " << std::flush;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>\n";
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(std::stoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "[ERROR] Connection failed.\n";
        return 1;
    }

    // Send username first
    std::cout << "Enter username: ";
    std::string username;
    std::getline(std::cin, username);
    send(sock, username.c_str(), username.size(), 0);

    std::cout << "\n--- ChatNet connected. Type /help for commands ---\n\n";

    // Start receive thread
    std::thread receiver(receiveMessages, sock);
    receiver.detach();

    // Main send loop
    while (running) {
        std::cout << "> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        if (line == "/help") {
            std::cout << "  /nick <name>          — change username\n"
                      << "  /msg <user> <text>   — private message\n"
                      << "  /quit                 — disconnect\n";
            continue;
        }

        send(sock, line.c_str(), line.size(), 0);
        if (line == "/quit") break;
    }

    running = false;
    close(sock);
    std::cout << "[ChatNet] Goodbye.\n";
    return 0;
}
