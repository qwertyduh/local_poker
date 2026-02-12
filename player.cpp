#include "raylib.h"
#include <iostream>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h> // Added for memset

int sock = 0;
std::string myName;
std::string potStr = "0", walletStr = "0", currentTurnName = "";
bool isMyTurn = false;

void listenToServer() {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, 1024); // Clear buffer every time
        int valread = recv(sock, buffer, 1024, 0);
        if (valread > 0) {
            std::string msg(buffer);
            size_t pPos = msg.find("POT:"), wPos = msg.find("|WALLET:"), tPos = msg.find("|TURN:");
            if (pPos != std::string::npos && wPos != std::string::npos && tPos != std::string::npos) {
                potStr = msg.substr(pPos + 4, wPos - (pPos + 4));
                walletStr = msg.substr(wPos + 8, tPos - (wPos + 8));
                currentTurnName = msg.substr(tPos + 6);
                isMyTurn = (currentTurnName == myName);
            }
        } else {
            break; 
        }
    }
}

int main() {
    std::string ip;
    std::cout << "Enter Your Name: "; std::cin >> myName;
    std::cout << "Enter Banker IP: "; std::cin >> ip;

    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(65432);
    inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection Failed!" << std::endl;
        return -1;
    }

    // FIXED: Now sending 'myName' (the one with data) instead of 'name'
    send(sock, myName.c_str(), myName.length(), 0);
    
    std::thread(listenToServer).detach();

    InitWindow(400, 450, "Poker - Player");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (isMyTurn && IsKeyPressed(KEY_SPACE)) {
            send(sock, "10", 2, 0);
        }

        BeginDrawing();
            ClearBackground(GetColor(0x1a1a1aff));
            
            DrawText(TextFormat("POT: $%s", potStr.c_str()), 120, 40, 30, GOLD);
            DrawText(TextFormat("WALLET: $%s", walletStr.c_str()), 40, 100, 20, GREEN);

            DrawRectangle(20, 160, 360, 80, isMyTurn ? DARKGREEN : DARKGRAY);
            std::string turnText = isMyTurn ? "YOUR TURN!" : "WAITING FOR: " + currentTurnName;
            DrawText(turnText.c_str(), 40, 190, 20, WHITE);

            if(isMyTurn) DrawText("Press [SPACE] to bet $10", 80, 380, 18, YELLOW);
            else DrawText("Wait for your turn...", 110, 380, 18, GRAY);
            
            DrawText(TextFormat("Playing as: %s", myName.c_str()), 10, 430, 10, GRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}