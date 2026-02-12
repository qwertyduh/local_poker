#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

std::mutex pot_mutex;
int total_pot = 0;
std::vector<int> client_sockets;

void broadcast(std::string message) {
    std::lock_guard<std::mutex> lock(pot_mutex);
    message += '\0'; // Crucial for Raylib player to read correctly
    for (int client : client_sockets) {
        send(client, message.c_str(), message.length(), 0);
    }
}

void handle_player(int client_socket) {
    char buffer[1024];
    // First message: Name
    int valread = recv(client_socket, buffer, 1024, 0);
    std::string name = (valread > 0) ? std::string(buffer) : "Unknown";
    
    std::cout << "[JOINED] " << name << " is at the table." << std::endl;
    broadcast("Pot: $" + std::to_string(total_pot) + " (" + name + " joined)");

    while (true) {
        valread = recv(client_socket, buffer, 1024, 0);
        if (valread <= 0) break;

        buffer[valread] = '\0';
        std::string data(buffer);

        if (isdigit(data[0])) {
            int bet = std::stoi(data);
            {
                std::lock_guard<std::mutex> lock(pot_mutex);
                total_pot += bet;
            }
            std::cout << "[BET] " << name << " bet $" << bet << ". Total: $" << total_pot << std::endl;
            broadcast("Pot: $" + std::to_string(total_pot) + " | Last: " + name);
        }
    }
    close(client_socket);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listens on all Wi-Fi interfaces
    address.sin_port = htons(65432);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return -1;
    }

    listen(server_fd, 5);
    std::cout << "BANKER ACTIVE ON PORT 65432. Waiting for players..." << std::endl;

    while (true) {
        int addrlen = sizeof(address);
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        client_sockets.push_back(new_socket);
        std::thread(handle_player, new_socket).detach();
    }
    return 0;
}
