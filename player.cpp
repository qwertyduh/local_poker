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
                try {
                    potStr = msg.substr(pPos + 2, wPos - (pPos + 2));
                    walletStr = msg.substr(wPos + 3, tPos - (wPos + 3));
                    currentTurnName = msg.substr(tPos + 3, lPos - (tPos + 3));
                    lastBetStr = msg.substr(lPos + 3, cPos - (lPos + 3));
                    myContribStr = msg.substr(cPos + 3);
                    isMyTurn = (currentTurnName == myName);
                } catch (...) {}
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
    std::cout << "Name (No Spaces): "; std::cin >> myName;
    std::cout << "Banker IP: "; std::string ip; std::cin >> ip;

    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(65432);
    inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return -1;
    send(sock, myName.c_str(), myName.length(), 0);
    std::thread(listenToServer).detach();

    InitWindow(500, 650, "Poker Sync - Wi-Fi Table");
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
            int callAmt = std::stoi(lastBetStr) - std::stoi(myContribStr);
            
            // Header Stats
            DrawRectangle(0, 0, 500, 180, GetColor(0x252525FF));
            DrawText(TextFormat("POT: $%s", potStr.c_str()), 180, 40, 30, GOLD);
            DrawText(TextFormat("YOUR WALLET: $%s", walletStr.c_str()), 40, 100, 20, GREEN);
            DrawText(TextFormat("Your Bet This Round: $%s", myContribStr.c_str()), 40, 130, 16, SKYBLUE);
            DrawText(TextFormat("To Call: $%d", callAmt), 40, 155, 18, MAROON);

            // Turn Indicator
            DrawRectangle(20, 200, 460, 60, isMyTurn ? DARKGREEN : DARKGRAY);
            std::string status = (currentTurnName == "BANKER_DECIDING") ? "WAITING FOR BANKER..." : 
                                 (isMyTurn ? "YOUR TURN" : "WAITING FOR: " + currentTurnName);
            DrawText(status.c_str(), 40, 220, 20, WHITE);

            // Action Buttons
            if (isMyTurn && currentTurnName != "BANKER_DECIDING") {
                if (DrawButton({50, 300, 180, 50}, "FOLD", RED, true)) send(sock, "FOLD", 4, 0);
                std::string callTxt = (callAmt == 0) ? "CHECK" : "CALL";
                if (DrawButton({270, 300, 180, 50}, callTxt.c_str(), BLUE, true)) send(sock, "CHECK", 5, 0);
                
                Rectangle rb = {50, 410, 180, 50};
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) inputActive = CheckCollisionPointRec(GetMousePosition(), rb);
                DrawRectangleRec(rb, inputActive ? WHITE : GRAY);
                DrawText(raiseInput, rb.x + 10, rb.y + 15, 20, BLACK);
                
                if (DrawButton({270, 410, 180, 50}, "RAISE", DARKGREEN, true)) {
                    std::string act = "BET:" + std::string(raiseInput);
                    send(sock, act.c_str(), act.length(), 0);
                }
            }
            
            // Debug footer
            DrawRectangle(0, 600, 500, 50, BLACK);
            DrawText(TextFormat("Dev Status: [%s] vs [%s]", myName.c_str(), currentTurnName.c_str()), 10, 615, 12, GRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}