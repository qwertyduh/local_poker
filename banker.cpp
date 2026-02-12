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

        // --- FIXED REMOVAL LOGIC ---
        for (auto it = turn_order.begin(); it != turn_order.end(); ) {
            std::string pName = *it;
            if (players[pName].wallet <= 0) {
                std::cout << "[REMOVAL] " << pName << " is bankrupt and removed." << std::endl;
                std::string exitMsg = "EXIT:Bankrupt";
                send(players[pName].socket, exitMsg.c_str(), exitMsg.length() + 1, 0);
                
                // Remove from both containers
                it = turn_order.erase(it);
                players.erase(pName); 
            } else {
                ++it;
            }
        }
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

bool can_all_match(int target_bet) {
    for (auto const& [name, p] : players) {
        if (!p.folded) {
            int needed = target_bet - p.round_contribution;
            if (p.wallet < needed) return false;
        }
    }
    return true;
}

void handle_banker_input() {
    while (true) {
        if (banker_deciding) {
            std::cout << "\n--- BANKER CONTROL ---\n1. NEXT PHASE\n2. AWARD WINNER\nChoice: ";
            int choice; std::cin >> choice;
            std::lock_guard<std::mutex> lock(game_mutex);
            if (choice == 1) {
                last_bet = 0; turn_index = 0;
                for (auto &pair : players) { pair.second.round_contribution = 0; pair.second.acted = false; }
            } else {
                std::vector<std::string> active;
                for (auto const& n : turn_order) if (!players[n].folded) active.push_back(n);
                for (int i=0; i<active.size(); ++i) std::cout << i+1 << ". " << active[i] << std::endl;
                int idx; std::cin >> idx;
                if (idx > 0 && idx <= (int)active.size()) award_winner(active[idx-1]);
            }
            banker_deciding = false;
            broadcast_game_state();
        }
        usleep(100000);
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
            // Check if player still exists before processing turn
            if (players.count(name) && !banker_deciding && name == turn_order[turn_index]) {
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
                    try {
                        int amount = std::stoi(action.substr(4));
                        int target_total = p.round_contribution + amount;
                        if (p.wallet >= amount && target_total >= last_bet && can_all_match(target_total)) {
                            p.wallet -= amount;
                            p.round_contribution += amount;
                            total_pot += amount;
                            last_bet = p.round_contribution;
                            for (auto &pair : players) pair.second.acted = false;
                            p.acted = true;
                            valid = true;
                        }
                    } catch (...) {}
                }

                if (valid) {
                    int active_count = 0; std::string last;
                    for (auto const& n : turn_order) {
                        if (players.count(n) && !players[n].folded) { active_count++; last = n; }
                    }
                    if (active_count == 1) award_winner(last);
                    else {
                        bool done = true;
                        for (auto const& n : turn_order) {
                            if (players.count(n) && !players[n].folded && (players[n].round_contribution < last_bet || !players[n].acted)) done = false;
                        }
                        if (done) banker_deciding = true;
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
    int opt = 1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr; addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY; addr.sin_port = htons(65432);
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);
    std::thread(handle_banker_input).detach();
    std::cout << "BANKER LIVE." << std::endl;
    while (true) {
        int ns = accept(server_fd, NULL, NULL);
        std::thread(handle_player, ns).detach();
    }
    return 0;
}