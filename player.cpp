#include "raylib.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <iostream>

int sock = 0;
std::string potStatus = "Waiting for Banker...";
std::string lastAction = "Press SPACE to bet $10";

void listenToServer() {
    char buffer[1024];
    while (true) {
        int valread = recv(sock, buffer, 1024, 0);
        if (valread > 0) {
            buffer[valread] = '\0'; // Null-terminate the string
            potStatus = std::string(buffer);
        } else {
            potStatus = "CONNECTION LOST";
            break;
        }
    }
}

int main() {
    std::string ip, name;
    std::cout << "Enter Banker IP: "; std::cin >> ip;
    std::cout << "Enter Your Name: "; std::cin >> name;

    // --- Networking ---
    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(65432);
    inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection Failed!" << std::endl;
        return -1;
    }

    send(sock, name.c_str(), name.length(), 0);
    std::thread(listenToServer).detach();

    // --- Raylib Window ---
    InitWindow(400, 300, "Poker Project - PLAYER");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            std::string bet = "10";
            send(sock, bet.c_str(), bet.length(), 0);
            lastAction = "You bet $10!";
        }

        BeginDrawing();
            ClearBackground(GetColor(0x181818FF)); // Dark grey background
            DrawText("LOCAL POKER", 20, 20, 20, MAROON);
            
            // Draw the Pot Status
            DrawRectangle(20, 80, 360, 60, DARKGRAY);
            DrawText(potStatus.c_str(), 40, 95, 20, GREEN);
            
            DrawText(lastAction.c_str(), 40, 250, 15, LIGHTGRAY);
            DrawText("Press [SPACE] to Bet $10", 40, 220, 18, GOLD);
        EndDrawing();
    }

    close(sock);
    CloseWindow();
    return 0;
}