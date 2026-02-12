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
    int round_contribution;
    bool folded;
    bool acted; 
};

std::mutex game_mutex;
std::map<std::string, Player> players;
std::vector<std::string> turn_order;
int turn_index = 0;
int total_pot = 0;
int starting_budget = 500;
int last_bet = 0;
bool banker_deciding = false;

void broadcast_game_state() {
    if (turn_order.empty()) return;
    std::string current_turn = banker_deciding ? "BANKER_DECIDING" : turn_order[turn_index];
    
    for (auto const& [name, player] : players) {
        std::string msg = "P:" + std::to_string(total_pot) + 
                          "|W:" + std::to_string(player.wallet) + 
                          "|T:" + current_turn + 
                          "|L:" + std::to_string(last_bet) + 
                          "|C:" + std::to_string(player.round_contribution) + "\0";
        send(player.socket, msg.c_str(), msg.length() + 1, 0);
    }
}

void award_winner(std::string winner_name) {
    if (players.count(winner_name)) {
        players[winner_name].wallet += total_pot;
        std::cout << "\n>>> " << winner_name << " WON $" << total_pot << " <<<\n" << std::endl;
        total_pot = 0;
        last_bet = 0;
        turn_index = 0;
        for (auto &pair : players) {
            pair.second.folded = false;
            pair.second.round_contribution = 0;
            pair.second.acted = false;
        }
    }
}

void handle_banker_input() {
    while (true) {
        if (banker_deciding) {
            std::cout << "\n======================================" << std::endl;
            std::cout << "         BANKER CONTROL PANEL         " << std::endl;
            std::cout << "======================================" << std::endl;
            std::cout << "1. NEXT ROUND (Flop/Turn/River)" << std::endl;
            std::cout << "2. END HAND (Select Winner)" << std::endl;
            std::cout << "Choice: ";
            int choice; std::cin >> choice;

            std::lock_guard<std::mutex> lock(game_mutex);
            if (choice == 1) {
                last_bet = 0;
                turn_index = 0;
                for (auto &pair : players) {
                    pair.second.round_contribution = 0;
                    pair.second.acted = false;
                }
                std::cout << "[SYSTEM] Starting next betting phase..." << std::endl;
            } else {
                std::vector<std::string> active_players;
                for (auto const& name : turn_order) {
                    if (!players[name].folded) active_players.push_back(name);
                }

                std::cout << "\n--- SELECT WINNER ---" << std::endl;
                for (int i = 0; i < active_players.size(); ++i) {
                    std::cout << i + 1 << ". " << active_players[i] << std::endl;
                }
                std::cout << "Enter Number: ";
                int winner_idx; std::cin >> winner_idx;

                if (winner_idx > 0 && winner_idx <= active_players.size()) {
                    award_winner(active_players[winner_idx - 1]);
                } else {
                    std::cout << "[ERR] Invalid selection. Try again." << std::endl;
                    continue; // Re-prompt
                }
            }
            banker_deciding = false;
            broadcast_game_state();
        }
        usleep(100000);
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
        players[name] = {client_socket, starting_budget, 0, false, false};
        turn_order.push_back(name);
    }
    broadcast_game_state();

    while (true) {
        memset(buffer, 0, 1024);
        valread = recv(client_socket, buffer, 1024, 0);
        if (valread <= 0) break;
        std::string action(buffer);
        action.erase(std::remove_if(action.begin(), action.end(), [](char c) { return !isprint(c); }), action.end());

        {
            std::lock_guard<std::mutex> lock(game_mutex);
            if (!banker_deciding && name == turn_order[turn_index]) {
                Player &p = players[name];
                bool valid = false;

                if (action == "FOLD") {
                    p.folded = true;
                    valid = true;
                } else if (action == "CHECK") {
                    int diff = last_bet - p.round_contribution;
                    if (p.wallet >= diff) {
                        p.wallet -= diff;
                        p.round_contribution += diff;
                        total_pot += diff;
                        p.acted = true;
                        valid = true;
                    }
                } else if (action.find("BET:") == 0) {
                    int amount = std::stoi(action.substr(4));
                    if (p.wallet >= amount && (p.round_contribution + amount) >= last_bet) {
                        p.wallet -= amount;
                        p.round_contribution += amount;
                        total_pot += amount;
                        last_bet = p.round_contribution;
                        for (auto &pair : players) pair.second.acted = false;
                        p.acted = true;
                        valid = true;
                    }
                }

                if (valid) {
                    int active_count = 0;
                    std::string last_standing;
                    for (auto const& [n, pl] : players) {
                        if (!pl.folded) { active_count++; last_standing = n; }
                    }
                    if (active_count == 1) award_winner(last_standing);
                    else {
                        bool all_matched = true;
                        for (auto const& [n, pl] : players) {
                            if (!pl.folded && (pl.round_contribution < last_bet || !pl.acted)) {
                                all_matched = false;
                                break;
                            }
                        }
                        if (all_matched) banker_deciding = true;
                        else next_turn();
                    }
                }
            }
        }
        broadcast_game_state();
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
    std::thread(handle_banker_input).detach();
    std::cout << "BANKER LIVE." << std::endl;
    while (true) {
        int new_socket = accept(server_fd, NULL, NULL);
        std::thread(handle_player, new_socket).detach();
    }
    return 0;
}