#include "../include/common.h"
#include "../include/core.h"
#include "../include/network.h"

// Global game instance
Game game;

int main(void)
{
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
    
    return 0;
}