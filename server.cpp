// ChatNet Server — server.cpp
// Compile: g++ -std=c++17 -Wall -pthread -o server server.cpp

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// ── Global State ────────────────────────────────────────
std::vector<int>            clients;      // active socket FDs
std::map<int, std::string>  usernames;    // socket → username
std::mutex                  state_mutex;  // protects both above

// ── Broadcast to all except sender ──────────────────────
void broadcast(const std::string& msg, int sender_fd = -1) {
    std::lock_guard<std::mutex> lock(state_mutex);
    for (int fd : clients) {
        if (fd != sender_fd)
            send(fd, msg.c_str(), msg.size(), 0);
    }
}

// ── Send to one specific client ─────────────────────────
bool sendPrivate(const std::string& target,
                 const std::string& msg, int sender_fd) {
    std::lock_guard<std::mutex> lock(state_mutex);
    for (auto& [fd, name] : usernames) {
        if (name == target) {
            send(fd, msg.c_str(), msg.size(), 0);
            return true;
        }
    }
    return false;
}

// ── Per-client thread ───────────────────────────────────
void handleClient(int client_fd) {
    char buffer[BUFFER_SIZE];
    std::string username;

    // Step 1: receive username
    memset(buffer, 0, BUFFER_SIZE);
    int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) { close(client_fd); return; }
    username = std::string(buffer, bytes);
    username.erase(username.find_last_not_of("\r\n") + 1);

    {
        std::lock_guard<std::mutex> lock(state_mutex);
        usernames[client_fd] = username;
    }

    std::string joinMsg = "[SERVER] " + username + " has joined the chat.\n";
    broadcast(joinMsg, client_fd);
    std::cout << joinMsg;

    // Step 2: message loop
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int n = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (n <= 0) break;

        std::string raw(buffer, n);
        raw.erase(raw.find_last_not_of("\r\n") + 1);

        // /nick <new_name> — change username
        if (raw.substr(0, 5) == "/nick") {
            std::string newName = raw.substr(6);
            std::string notice = "[SERVER] " + username +
                                 " is now known as " + newName + ".\n";
            {
                std::lock_guard<std::mutex> lock(state_mutex);
                usernames[client_fd] = newName;
            }
            username = newName;
            broadcast(notice, -1);
            std::cout << notice;

        // /msg <target> <message> — private message
        } else if (raw.substr(0, 4) == "/msg") {
            auto space = raw.find(' ', 5);
            if (space != std::string::npos) {
                std::string target = raw.substr(5, space - 5);
                std::string pmText = raw.substr(space + 1);
                std::string toRec  = "[PM from " + username + "]: " + pmText + "\n";
                std::string toSend = "[PM to " + target + "]: " + pmText + "\n";
                bool ok = sendPrivate(target, toRec, client_fd);
                send(client_fd, ok ? toSend.c_str() :
                    "[SERVER] User not found.\n", ok ? toSend.size() : 24, 0);
            }

        // /quit — graceful disconnect
        } else if (raw == "/quit") {
            break;

        // Regular broadcast message
        } else {
            std::string out = "[" + username + "]: " + raw + "\n";
            broadcast(out, client_fd);
            std::cout << out;
        }
    }

    // Cleanup on disconnect
    std::string leaveMsg = "[SERVER] " + username + " has left the chat.\n";
    broadcast(leaveMsg, client_fd);
    std::cout << leaveMsg;

    {
        std::lock_guard<std::mutex> lock(state_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
        usernames.erase(client_fd);
    }
    close(client_fd);
}

// ── Main ────────────────────────────────────────────────
int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, SOMAXCONN);

    std::cout << "[SERVER] Listening on port " << PORT << "...\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int client_fd = accept(server_fd, (sockaddr*)&clientAddr, &clientLen);
        if (client_fd < 0) continue;

        {
            std::lock_guard<std::mutex> lock(state_mutex);
            clients.push_back(client_fd);
        }

        std::thread(handleClient, client_fd).detach();
    }

    close(server_fd);
    return 0;
}
