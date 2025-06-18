#include "../include/common.h"
#include "../include/core.h"
#include "../include/network.h"

// Global game instance
Game game;

int main(void)
{
#ifdef _WIN32
    // Initialize Winsock for Windows
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock\n");
        return 1;
    }
#endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Layla - 2D Multiplayer Shooter (C/raylib)");
    SetTargetFPS(0); // Uncapped FPS for maximum performance
    
    InitGame();
    
    while (!WindowShouldClose())
    {
        UpdateGame();
        
        BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawGame();
        EndDrawing();
    }
    
    CloseNetwork();
    CloseWindow();
    
#ifdef _WIN32
    // Cleanup Winsock for Windows
    WSACleanup();
#endif
    
    return 0;
}