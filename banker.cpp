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
#include <algorithm>

struct Player {
    int socket;
    int wallet;
    int round_contribution; // How much they've put in THIS round
    bool folded;
};

std::mutex game_mutex;
std::map<std::string, Player> players;
std::vector<std::string> turn_order;
int turn_index = 0;
int total_pot = 0;
int starting_budget = 500;
int last_bet = 0; // The current high bet to match

void broadcast_game_state() {
    std::lock_guard<std::mutex> lock(game_mutex);
    if (turn_order.empty()) return;

    std::string current_turn = turn_order[turn_index];
    for (auto const& [name, player] : players) {
        // P:Pot | W:Wallet | T:Turn | L:HighBet | C:YourContribution
        std::string msg = "P:" + std::to_string(total_pot) + 
                          "|W:" + std::to_string(player.wallet) + 
                          "|T:" + current_turn + 
                          "|L:" + std::to_string(last_bet) + 
                          "|C:" + std::to_string(player.round_contribution) + "\0";
        send(player.socket, msg.c_str(), msg.length() + 1, 0);
    }
}

void next_turn() {
    if (turn_order.size() < 2) return;
    int initial_index = turn_index;
    do {
        turn_index = (turn_index + 1) % turn_order.size();
        if (turn_index == initial_index) break;
    } while (players[turn_order[turn_index]].folded);
}

void handle_player(int client_socket) {
    char buffer[1024];
    memset(buffer, 0, 1024);
    int valread = recv(client_socket, buffer, 1024, 0);
    if (valread <= 0) return;
    
    std::string name(buffer);
    {
        std::lock_guard<std::mutex> lock(game_mutex);
        players[name] = {client_socket, starting_budget, 0, false};
        turn_order.push_back(name);
    }
    broadcast_game_state();

    while (true) {
        memset(buffer, 0, 1024);
        valread = recv(client_socket, buffer, 1024, 0);
        if (valread <= 0) break;

        std::string action(buffer);
        action.erase(std::remove_if(action.begin(), action.end(), [](char c) { return !isprint(c); }), action.end());

        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(game_mutex);
            if (name == turn_order[turn_index]) {
                Player &p = players[name];

                if (action == "FOLD") {
                    p.folded = true;
                    next_turn();
                    changed = true;
                } else if (action == "CHECK") {
                    // CALL Logic: Pay the difference to match last_bet
                    int diff = last_bet - p.round_contribution;
                    if (p.wallet >= diff) {
                        p.wallet -= diff;
                        p.round_contribution += diff;
                        total_pot += diff;
                        next_turn();
                        changed = true;
                    }
                } else if (action.find("BET:") == 0) {
                    int amount = std::stoi(action.substr(4));
                    // RAISE Logic: Total contribution must be >= last_bet
                    if (p.wallet >= amount && (p.round_contribution + amount) >= last_bet) {
                        p.wallet -= amount;
                        p.round_contribution += amount;
                        total_pot += amount;
                        last_bet = p.round_contribution; // New High Bet
                        next_turn();
                        changed = true;
                    }
                }
            }
        }
        if (changed) broadcast_game_state();
    }
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
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);
    std::cout << "BANKER ACTIVE." << std::endl;
    while (true) {
        int new_socket = accept(server_fd, NULL, NULL);
        std::thread(handle_player, new_socket).detach();
    }
    return 0;
}