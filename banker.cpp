#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

struct Player {
    int socket;
    int wallet;
};

std::mutex game_mutex;
std::map<std::string, Player> players;
std::vector<std::string> turn_order;
int turn_index = 0;
int total_pot = 0;
int starting_budget = 500;

void broadcast_game_state() {
    // This function now handles its own locking
    std::lock_guard<std::mutex> lock(game_mutex);
    if (turn_order.empty()) return;

    std::string current_turn = turn_order[turn_index];
    for (auto const& [name, player] : players) {
        std::string msg = "POT:" + std::to_string(total_pot) + 
                          "|WALLET:" + std::to_string(player.wallet) + 
                          "|TURN:" + current_turn + "\0";
        send(player.socket, msg.c_str(), msg.length(), 0);
    }
}

void handle_player(int client_socket) {
    char buffer[1024];
    memset(buffer, 0, 1024);
    int valread = recv(client_socket, buffer, 1024, 0);
    if (valread <= 0) return;
    std::string name(buffer);

    {
        std::lock_guard<std::mutex> lock(game_mutex);
        players[name] = {client_socket, starting_budget};
        turn_order.push_back(name);
    }
    
    std::cout << "[JOIN] " << name << " connected." << std::endl;
    broadcast_game_state();

    while (true) {
        memset(buffer, 0, 1024);
        valread = recv(client_socket, buffer, 1024, 0);
        if (valread <= 0) break;
        std::string data(buffer);

        bool state_changed = false;
        {
            std::lock_guard<std::mutex> lock(game_mutex);
            if (name == turn_order[turn_index] && isdigit(data[0])) {
                int bet = std::stoi(data);
                if (players[name].wallet >= bet) {
                    players[name].wallet -= bet;
                    total_pot += bet;
                    turn_index = (turn_index + 1) % turn_order.size();
                    state_changed = true;
                    std::cout << "[BET] " << name << " bet $" << bet << std::endl;
                }
            }
        }
        
        if (state_changed) {
            broadcast_game_state();
        }
    }
    
    // Cleanup on disconnect
    std::cout << "[LEAVE] " << name << " disconnected." << std::endl;
    close(client_socket);
}

int main() {
    std::cout << "Starting Budget: "; std::cin >> starting_budget;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(65432);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return -1;
    }

    listen(server_fd, 5);
    std::cout << "BANKER LIVE on port 65432" << std::endl;

    while (true) {
        int new_socket = accept(server_fd, NULL, NULL);
        std::thread(handle_player, new_socket).detach();
    }
    return 0;
}