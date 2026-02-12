#include "raylib.h"
#include <iostream>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int sock = 0;
std::string myName, potStr = "0", walletStr = "0", currentTurnName = "", lastBetStr = "0", myContribStr = "0";
bool isMyTurn = false;
char raiseInput[10] = "50";
int letterCount = 2;
bool inputActive = false;

void listenToServer() {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, 1024);
        int valread = recv(sock, buffer, 1024, 0);
        if (valread > 0) {
            std::string msg(buffer);
            size_t pPos = msg.find("P:"), wPos = msg.find("|W:"), tPos = msg.find("|T:"), lPos = msg.find("|L:"), cPos = msg.find("|C:");
            if (pPos != std::string::npos) {
                potStr = msg.substr(pPos + 2, wPos - (pPos + 2));
                walletStr = msg.substr(wPos + 3, tPos - (wPos + 3));
                currentTurnName = msg.substr(tPos + 3, lPos - (tPos + 3));
                lastBetStr = msg.substr(lPos + 3, cPos - (lPos + 3));
                myContribStr = msg.substr(cPos + 3);
                isMyTurn = (currentTurnName == myName);
            }
        }
    }
}

bool DrawButton(Rectangle rect, const char* text, Color color, bool active) {
    if (!active) {
        DrawRectangleRec(rect, GRAY);
        DrawText(text, rect.x + (rect.width/2 - MeasureText(text, 16)/2), rect.y + 15, 16, DARKGRAY);
        return false;
    }
    Vector2 mouse = GetMousePosition();
    bool hovering = CheckCollisionPointRec(mouse, rect);
    DrawRectangleRec(rect, hovering ? Fade(color, 0.7f) : color);
    DrawText(text, rect.x + (rect.width/2 - MeasureText(text, 16)/2), rect.y + 15, 16, WHITE);
    return (hovering && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

int main() {
    std::cout << "Name: "; std::cin >> myName;
    std::cout << "Banker IP: "; std::string ip; std::cin >> ip;

    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(65432);
    inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return -1;
    inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    send(sock, myName.c_str(), myName.length(), 0);
    std::thread(listenToServer).detach();

    InitWindow(500, 650, "Poker Pro");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (inputActive) {
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= '0' && key <= '9') && (letterCount < 9)) {
                    raiseInput[letterCount] = (char)key;
                    raiseInput[letterCount+1] = '\0';
                    letterCount++;
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && letterCount > 0) {
                letterCount--;
                raiseInput[letterCount] = '\0';
            }
        }

        BeginDrawing();
            ClearBackground(GetColor(0x1a1a1aff));
            
            int callAmount = std::stoi(lastBetStr) - std::stoi(myContribStr);
            
            DrawText(TextFormat("POT: $%s", potStr.c_str()), 180, 40, 30, GOLD);
            DrawText(TextFormat("WALLET: $%s", walletStr.c_str()), 40, 100, 20, GREEN);
            DrawText(TextFormat("Total In Round: $%s", myContribStr.c_str()), 40, 130, 20, SKYBLUE);
            DrawText(TextFormat("To Call: $%d", callAmount), 40, 160, 20, MAROON);

            DrawRectangle(20, 200, 460, 60, isMyTurn ? DARKGREEN : DARKGRAY);
            DrawText(isMyTurn ? "YOUR TURN" : TextFormat("WAITING FOR: %s", currentTurnName.c_str()), 40, 220, 20, WHITE);

            if (isMyTurn) {
                if (DrawButton({50, 300, 180, 50}, "FOLD", RED, true)) send(sock, "FOLD", 4, 0);
                
                std::string callText = (callAmount == 0) ? "CHECK" : "CALL $" + std::to_string(callAmount);
                if (DrawButton({270, 300, 180, 50}, callText.c_str(), BLUE, true)) send(sock, "CHECK", 5, 0);
                
                if (DrawButton({270, 410, 180, 50}, "RAISE", DARKGREEN, true)) {
                    std::string action = "BET:" + std::string(raiseInput);
                    send(sock, action.c_str(), action.length(), 0);
                }
            }
            
            DrawText("CUSTOM RAISE:", 50, 380, 18, LIGHTGRAY);
            Rectangle textBox = {50, 410, 180, 50};
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) inputActive = CheckCollisionPointRec(GetMousePosition(), textBox);
            DrawRectangleRec(textBox, inputActive ? WHITE : GRAY);
            DrawText(raiseInput, textBox.x + 10, textBox.y + 15, 20, BLACK);

            if (DrawButton({270, 410, 180, 50}, "CONFIRM RAISE", DARKGREEN, isMyTurn)) {
                std::string action = "BET:" + std::string(raiseInput);
                send(sock, action.c_str(), action.length(), 0);
            }

            DrawRectangle(0, 550, 500, 50, BLACK);
            DrawText(TextFormat("Local: [%s] vs Server: [%s]", myName.c_str(), currentTurnName.c_str()), 10, 565, 12, GRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}