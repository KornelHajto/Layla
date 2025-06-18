#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

// Game constants
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define MAX_PLAYERS 8
#define MAX_BULLETS 500
#define PLAYER_SIZE 20
#define PLAYER_SPEED 300.0f
#define BULLET_SIZE 3
#define BULLET_SPEED 600.0f
#define BULLET_LIFETIME 3.0f
#define GUN_LENGTH 25
#define SHOOT_COOLDOWN 0.1f
#define MAX_MESSAGE_SIZE 1024
#define DEFAULT_PORT 12345

// Game states
typedef enum {
    GAME_MENU,
    GAME_HOST_SETUP,
    GAME_JOIN_SETUP,
    GAME_PLAYING
} GameState;

// Player structure
typedef struct {
    char id[32];
    Vector2 position;
    Vector2 velocity;
    float gunAngle;
    float health;
    float shootCooldown;
    Color color;
    bool active;
    bool isLocal;
    double lastUpdate;
} Player;

// Bullet structure
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    char playerId[32];
    bool active;
} Bullet;

// Network message types
typedef enum {
    MSG_PLAYER_JOIN = 1,
    MSG_PLAYER_UPDATE = 2,
    MSG_BULLET_FIRE = 3,
    MSG_PLAYER_LEAVE = 4,
    MSG_PING = 5,
    MSG_PONG = 6
} MessageType;

// Network message structure
typedef struct {
    int type;
    char playerId[32];
    Vector2 position;
    Vector2 velocity;
    float gunAngle;
    float health;
    double timestamp;
} NetworkMessage;

// Game state structure
typedef struct {
    GameState state;
    Player players[MAX_PLAYERS];
    Bullet bullets[MAX_BULLETS];
    int playerCount;
    int bulletCount;
    char localPlayerId[32];
    
    // Network
    int socket_fd;
    bool isHost;
    bool isConnected;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddrs[MAX_PLAYERS];
    int clientCount;
    char hostIP[16];
    int hostPort;
    char joinIP[16];
    int joinPort;
    
    // Input fields
    bool editingHostPort;
    bool editingJoinIP;
    bool editingJoinPort;
    char hostPortStr[8];
    char joinIPStr[16];
    char joinPortStr[8];
    
    // Performance metrics
    float ping;
    double lastPingTime;
    double pingStartTime;
    int packetsSent;
    int packetsReceived;
    
    // Debug
    bool debugMode;
    char statusMessage[256];
    float statusTimer;
    
    // Performance settings
    int targetFPS;
    bool vsyncEnabled;
    bool showAdvancedStats;
} Game;

// Global game instance
Game game = {0};

// Function prototypes
void InitGame(void);
void UpdateGame(void);
void DrawGame(void);
void HandleInput(void);
void UpdateNetwork(void);
void SendMessage(NetworkMessage* msg, struct sockaddr_in* addr);
void ProcessMessage(NetworkMessage* msg, struct sockaddr_in* from);
Player* FindPlayer(const char* id);
Player* CreatePlayer(const char* id, Vector2 pos);
void RemovePlayer(const char* id);
void CreateBullet(Vector2 pos, Vector2 vel, const char* playerId);
void UpdatePlayers(float dt);
void UpdateBullets(float dt);
void DrawPlayers(void);
void DrawBullets(void);
void DrawMenu(void);
void DrawHostSetup(void);
void DrawJoinSetup(void);
void DrawUI(void);
int StartHost(int port);
int ConnectToServer(const char* ip, int port);
void CloseNetwork(void);
void SetStatusMessage(const char* message, float duration);
void GeneratePlayerId(char* id);

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

void InitGame(void)
{
    srand(time(NULL));
    
    game.state = GAME_MENU;
    game.playerCount = 0;
    game.bulletCount = 0;
    game.isHost = false;
    game.isConnected = false;
    game.socket_fd = -1;
    game.debugMode = false;
    game.targetFPS = 0; // Uncapped by default
    game.vsyncEnabled = false;
    game.showAdvancedStats = false;
    
    // Initialize input fields
    strcpy(game.hostPortStr, "12345");
    strcpy(game.joinIPStr, "localhost");
    strcpy(game.joinPortStr, "12345");
    
    GeneratePlayerId(game.localPlayerId);
    
    // Initialize all players as inactive
    for (int i = 0; i < MAX_PLAYERS; i++) {
        game.players[i].active = false;
    }
    
    // Initialize all bullets as inactive
    for (int i = 0; i < MAX_BULLETS; i++) {
        game.bullets[i].active = false;
    }
}

void UpdateGame(void)
{
    float dt = GetFrameTime();
    
    HandleInput();
    
    if (game.state == GAME_PLAYING) {
        UpdatePlayers(dt);
        UpdateBullets(dt);
        UpdateNetwork();
    }
    
    // Update status message timer
    if (game.statusTimer > 0) {
        game.statusTimer -= dt;
    }
}

void DrawGame(void)
{
    switch (game.state) {
        case GAME_MENU:
            DrawMenu();
            break;
        case GAME_HOST_SETUP:
            DrawHostSetup();
            break;
        case GAME_JOIN_SETUP:
            DrawJoinSetup();
            break;
        case GAME_PLAYING:
            DrawPlayers();
            DrawBullets();
            DrawUI();
            break;
    }
}

void HandleInput(void)
{
    if (game.state == GAME_MENU) {
        if (IsKeyPressed(KEY_H)) {
            game.state = GAME_HOST_SETUP;
            game.editingHostPort = true;
        } else if (IsKeyPressed(KEY_J)) {
            game.state = GAME_JOIN_SETUP;
            game.editingJoinIP = true;
        } else if (IsKeyPressed(KEY_S)) {
            // Single player mode
            CreatePlayer(game.localPlayerId, (Vector2){100, 100});
            game.state = GAME_PLAYING;
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            // Quit handled by WindowShouldClose()
        }
    }
    else if (game.state == GAME_HOST_SETUP) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            game.state = GAME_MENU;
            game.editingHostPort = false;
        } else if (IsKeyPressed(KEY_ENTER)) {
            int port = atoi(game.hostPortStr);
            if (port > 1024 && port < 65536) {
                if (StartHost(port) == 0) {
                    CreatePlayer(game.localPlayerId, (Vector2){100, 100});
                    game.state = GAME_PLAYING;
                    SetStatusMessage("Server started successfully!", 3.0f);
                } else {
                    SetStatusMessage("Failed to start server!", 3.0f);
                }
            } else {
                SetStatusMessage("Invalid port! Use 1025-65535", 3.0f);
            }
        }
        
        // Handle text input for port
        if (game.editingHostPort) {
            int key = GetCharPressed();
            while (key > 0) {
                if (key >= '0' && key <= '9' && strlen(game.hostPortStr) < 5) {
                    int len = strlen(game.hostPortStr);
                    game.hostPortStr[len] = (char)key;
                    game.hostPortStr[len + 1] = '\0';
                }
                key = GetCharPressed();
            }
            
            if (IsKeyPressed(KEY_BACKSPACE) && strlen(game.hostPortStr) > 0) {
                game.hostPortStr[strlen(game.hostPortStr) - 1] = '\0';
            }
        }
    }
    else if (game.state == GAME_JOIN_SETUP) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            game.state = GAME_MENU;
            game.editingJoinIP = false;
            game.editingJoinPort = false;
        } else if (IsKeyPressed(KEY_ENTER)) {
            int port = atoi(game.joinPortStr);
            if (port > 0 && port < 65536 && strlen(game.joinIPStr) > 0) {
                SetStatusMessage("Connecting...", 5.0f);
                if (ConnectToServer(game.joinIPStr, port) == 0) {
                    CreatePlayer(game.localPlayerId, (Vector2){200, 200});
                    game.state = GAME_PLAYING;
                    SetStatusMessage("Connected successfully!", 3.0f);
                } else {
                    SetStatusMessage("Failed to connect!", 5.0f);
                }
            } else {
                SetStatusMessage("Invalid IP or port!", 3.0f);
            }
        } else if (IsKeyPressed(KEY_TAB)) {
            if (game.editingJoinIP) {
                game.editingJoinIP = false;
                game.editingJoinPort = true;
            } else {
                game.editingJoinIP = true;
                game.editingJoinPort = false;
            }
        }
        
        // Handle text input
        int key = GetCharPressed();
        while (key > 0) {
            if (game.editingJoinIP && strlen(game.joinIPStr) < 15) {
                if ((key >= '0' && key <= '9') || key == '.' || 
                    (key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z')) {
                    int len = strlen(game.joinIPStr);
                    game.joinIPStr[len] = (char)key;
                    game.joinIPStr[len + 1] = '\0';
                }
            } else if (game.editingJoinPort && strlen(game.joinPortStr) < 5) {
                if (key >= '0' && key <= '9') {
                    int len = strlen(game.joinPortStr);
                    game.joinPortStr[len] = (char)key;
                    game.joinPortStr[len + 1] = '\0';
                }
            }
            key = GetCharPressed();
        }
        
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (game.editingJoinIP && strlen(game.joinIPStr) > 0) {
                game.joinIPStr[strlen(game.joinIPStr) - 1] = '\0';
            } else if (game.editingJoinPort && strlen(game.joinPortStr) > 0) {
                game.joinPortStr[strlen(game.joinPortStr) - 1] = '\0';
            }
        }
    }
    else if (game.state == GAME_PLAYING) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            game.state = GAME_MENU;
            CloseNetwork();
            // Reset game
            for (int i = 0; i < MAX_PLAYERS; i++) {
                game.players[i].active = false;
            }
            for (int i = 0; i < MAX_BULLETS; i++) {
                game.bullets[i].active = false;
            }
            game.playerCount = 0;
            game.bulletCount = 0;
        }
        
        if (IsKeyPressed(KEY_F1)) {
            game.debugMode = !game.debugMode;
        } else if (IsKeyPressed(KEY_F2)) {
            game.showAdvancedStats = !game.showAdvancedStats;
        } else if (IsKeyPressed(KEY_F3)) {
            // Cycle through FPS caps: 0 (uncapped), 60, 120, 144, 240
            int fpsCaps[] = {0, 60, 120, 144, 240};
            int currentIndex = 0;
            for (int i = 0; i < 5; i++) {
                if (fpsCaps[i] == game.targetFPS) {
                    currentIndex = i;
                    break;
                }
            }
            currentIndex = (currentIndex + 1) % 5;
            game.targetFPS = fpsCaps[currentIndex];
            SetTargetFPS(game.targetFPS);
            
            if (game.targetFPS == 0) {
                SetStatusMessage("FPS: Uncapped", 2.0f);
            } else {
                char msg[64];
                sprintf(msg, "FPS: %d", game.targetFPS);
                SetStatusMessage(msg, 2.0f);
            }
        } else if (IsKeyPressed(KEY_F4)) {
            game.vsyncEnabled = !game.vsyncEnabled;
            if (game.vsyncEnabled) {
                SetTargetFPS(60);
                SetStatusMessage("VSync: ON", 2.0f);
            } else {
                SetTargetFPS(game.targetFPS);
                SetStatusMessage("VSync: OFF", 2.0f);
            }
        }
    }
}

void UpdatePlayers(float dt)
{
    Player* localPlayer = FindPlayer(game.localPlayerId);
    if (localPlayer && localPlayer->active) {
        // Update gun angle to point at mouse
        Vector2 mousePos = GetMousePosition();
        Vector2 diff = {mousePos.x - localPlayer->position.x, mousePos.y - localPlayer->position.y};
        localPlayer->gunAngle = atan2f(diff.y, diff.x);
        
        // Handle movement
        Vector2 movement = {0};
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) movement.y -= 1;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) movement.y += 1;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) movement.x -= 1;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) movement.x += 1;
        
        // Normalize movement
        float length = sqrtf(movement.x * movement.x + movement.y * movement.y);
        if (length > 0) {
            movement.x /= length;
            movement.y /= length;
        }
        
        // Apply movement (immediate client-side prediction)
        localPlayer->velocity.x = movement.x * PLAYER_SPEED;
        localPlayer->velocity.y = movement.y * PLAYER_SPEED;
        localPlayer->position.x += localPlayer->velocity.x * dt;
        localPlayer->position.y += localPlayer->velocity.y * dt;
        
        // Keep player in bounds
        if (localPlayer->position.x < PLAYER_SIZE/2) localPlayer->position.x = PLAYER_SIZE/2;
        if (localPlayer->position.x > SCREEN_WIDTH - PLAYER_SIZE/2) 
            localPlayer->position.x = SCREEN_WIDTH - PLAYER_SIZE/2;
        if (localPlayer->position.y < PLAYER_SIZE/2) localPlayer->position.y = PLAYER_SIZE/2;
        if (localPlayer->position.y > SCREEN_HEIGHT - PLAYER_SIZE/2) 
            localPlayer->position.y = SCREEN_HEIGHT - PLAYER_SIZE/2;
        
        // Update cooldowns
        if (localPlayer->shootCooldown > 0) {
            localPlayer->shootCooldown -= dt;
        }
        
        // Handle shooting
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && localPlayer->shootCooldown <= 0) {
            Vector2 gunEnd = {
                localPlayer->position.x + cosf(localPlayer->gunAngle) * GUN_LENGTH,
                localPlayer->position.y + sinf(localPlayer->gunAngle) * GUN_LENGTH
            };
            Vector2 bulletVel = {
                cosf(localPlayer->gunAngle) * BULLET_SPEED,
                sinf(localPlayer->gunAngle) * BULLET_SPEED
            };
            CreateBullet(gunEnd, bulletVel, localPlayer->id);
            localPlayer->shootCooldown = SHOOT_COOLDOWN;
        }
    }
    
    // Update all players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active) {
            // Update non-local players with simple prediction
            if (!game.players[i].isLocal) {
                game.players[i].position.x += game.players[i].velocity.x * dt * 0.3f;
                game.players[i].position.y += game.players[i].velocity.y * dt * 0.3f;
            }
            
            // Update cooldowns
            if (game.players[i].shootCooldown > 0) {
                game.players[i].shootCooldown -= dt;
            }
        }
    }
}

void UpdateBullets(float dt)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game.bullets[i].active) {
            // Move bullet
            game.bullets[i].position.x += game.bullets[i].velocity.x * dt;
            game.bullets[i].position.y += game.bullets[i].velocity.y * dt;
            
            // Update lifetime
            game.bullets[i].lifetime -= dt;
            
            // Remove if expired or out of bounds
            if (game.bullets[i].lifetime <= 0 ||
                game.bullets[i].position.x < -50 || game.bullets[i].position.x > SCREEN_WIDTH + 50 ||
                game.bullets[i].position.y < -50 || game.bullets[i].position.y > SCREEN_HEIGHT + 50) {
                game.bullets[i].active = false;
                game.bulletCount--;
                continue;
            }
            
            // Check collision with players
            for (int j = 0; j < MAX_PLAYERS; j++) {
                if (game.players[j].active && strcmp(game.players[j].id, game.bullets[i].playerId) != 0) {
                    float dx = game.bullets[i].position.x - game.players[j].position.x;
                    float dy = game.bullets[i].position.y - game.players[j].position.y;
                    float distance = sqrtf(dx * dx + dy * dy);
                    if (distance < (PLAYER_SIZE/2 + BULLET_SIZE)) {
                        // Hit!
                        game.players[j].health -= 20;
                        game.bullets[i].active = false;
                        game.bulletCount--;
                        
                        if (game.players[j].health <= 0) {
                            // Respawn player
                            game.players[j].position = (Vector2){
                                (float)(rand() % (SCREEN_WIDTH - PLAYER_SIZE)) + PLAYER_SIZE/2,
                                (float)(rand() % (SCREEN_HEIGHT - PLAYER_SIZE)) + PLAYER_SIZE/2
                            };
                            game.players[j].health = 100;
                        }
                        break;
                    }
                }
            }
        }
    }
}

void DrawMenu(void)
{
    DrawText("LAYLA", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 100, 40, WHITE);
    DrawText("Press H to Host Game", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 30, 20, WHITE);
    DrawText("Press J to Join Game", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 10, 20, WHITE);
    DrawText("Press S for Single Player", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 10, 20, WHITE);
    DrawText("Press ESC to Quit", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 30, 20, WHITE);
}

void DrawHostSetup(void)
{
    DrawText("HOST GAME", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 120, 30, WHITE);
    DrawText("Port:", SCREEN_WIDTH/2 - 50, SCREEN_HEIGHT/2 - 50, 20, WHITE);
    
    // Port input field
    Rectangle portRect = {SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 20, 200, 30};
    DrawRectangleRec(portRect, game.editingHostPort ? BLUE : DARKBLUE);
    DrawRectangleLinesEx(portRect, 2, WHITE);
    DrawText(game.hostPortStr, portRect.x + 5, portRect.y + 5, 20, WHITE);
    
    DrawText("Click on field to edit, press ENTER to start", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2 + 30, 16, WHITE);
    DrawText("ESC to go back", SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2 + 50, 16, WHITE);
    
    // Status message
    if (game.statusTimer > 0) {
        DrawText(game.statusMessage, SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 + 80, 16, 
                strstr(game.statusMessage, "success") ? GREEN : RED);
    }
}

void DrawJoinSetup(void)
{
    DrawText("JOIN GAME", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 140, 30, WHITE);
    
    DrawText("IP Address:", SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2 - 80, 20, WHITE);
    Rectangle ipRect = {SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 50, 300, 30};
    DrawRectangleRec(ipRect, game.editingJoinIP ? BLUE : DARKBLUE);
    DrawRectangleLinesEx(ipRect, 2, WHITE);
    DrawText(game.joinIPStr, ipRect.x + 5, ipRect.y + 5, 20, WHITE);
    
    DrawText("Port:", SCREEN_WIDTH/2 - 50, SCREEN_HEIGHT/2 - 10, 20, WHITE);
    Rectangle portRect = {SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 20, 200, 30};
    DrawRectangleRec(portRect, game.editingJoinPort ? BLUE : DARKBLUE);
    DrawRectangleLinesEx(portRect, 2, WHITE);
    DrawText(game.joinPortStr, portRect.x + 5, portRect.y + 5, 20, WHITE);
    
    DrawText("Examples: localhost, 192.168.1.100", SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT/2 + 70, 16, LIGHTGRAY);
    DrawText("TAB to switch fields, ENTER to connect", SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT/2 + 90, 16, WHITE);
    DrawText("ESC to go back", SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2 + 110, 16, WHITE);
    
    // Status message
    if (game.statusTimer > 0) {
        Color color = WHITE;
        if (strstr(game.statusMessage, "success")) color = GREEN;
        else if (strstr(game.statusMessage, "Failed")) color = RED;
        else if (strstr(game.statusMessage, "Connecting")) color = YELLOW;
        DrawText(game.statusMessage, SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 + 140, 16, color);
    }
}

void DrawPlayers(void)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active) {
            Player* p = &game.players[i];
            
            // Draw player body (square)
            Rectangle playerRect = {
                p->position.x - PLAYER_SIZE/2,
                p->position.y - PLAYER_SIZE/2,
                PLAYER_SIZE,
                PLAYER_SIZE
            };
            DrawRectangleRec(playerRect, p->color);
            DrawRectangleLinesEx(playerRect, 2, WHITE);
            
            // Draw gun
            Vector2 gunEnd = {
                p->position.x + cosf(p->gunAngle) * GUN_LENGTH,
                p->position.y + sinf(p->gunAngle) * GUN_LENGTH
            };
            DrawLineEx(p->position, gunEnd, 3, WHITE);
            
            // Draw health bar
            Rectangle healthBg = {p->position.x - PLAYER_SIZE/2, p->position.y - PLAYER_SIZE/2 - 10, PLAYER_SIZE, 4};
            DrawRectangleRec(healthBg, DARKGRAY);
            Rectangle healthBar = {p->position.x - PLAYER_SIZE/2, p->position.y - PLAYER_SIZE/2 - 10, 
                                 PLAYER_SIZE * (p->health / 100.0f), 4};
            Color healthColor = p->health > 50 ? GREEN : (p->health > 25 ? YELLOW : RED);
            DrawRectangleRec(healthBar, healthColor);
            
            // Highlight local player
            if (p->isLocal) {
                DrawCircleLines(p->position.x, p->position.y, PLAYER_SIZE/2 + 5, YELLOW);
            }
            
            // Draw player ID
            DrawText(p->id, p->position.x - 30, p->position.y + PLAYER_SIZE/2 + 5, 10, WHITE);
        }
    }
}

void DrawBullets(void)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game.bullets[i].active) {
            DrawCircle(game.bullets[i].position.x, game.bullets[i].position.y, BULLET_SIZE, YELLOW);
        }
    }
}

void DrawUI(void)
{
    // FPS
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 16, WHITE);
    
    // Player count
    DrawText(TextFormat("Players: %d", game.playerCount), 10, 30, 16, WHITE);
    
    // Network status
    if (game.isHost) {
        DrawText(TextFormat("Hosting on port %d", game.hostPort), 10, 50, 16, WHITE);
    } else if (game.isConnected) {
        DrawText(TextFormat("Connected to %s", game.hostIP), 10, 50, 16, WHITE);
        DrawText(TextFormat("Ping: %.0fms", game.ping), 10, 70, 16, 
                game.ping > 100 ? RED : (game.ping > 50 ? YELLOW : GREEN));
    } else {
        DrawText("Not connected", 10, 50, 16, LIGHTGRAY);
    }
    
    if (game.debugMode) {
        DrawText("DEBUG MODE (F1 to toggle)", 10, 90, 16, YELLOW);
        DrawText(TextFormat("Local Player: %s", game.localPlayerId), 10, 110, 16, WHITE);
        DrawText(TextFormat("Bullets: %d", game.bulletCount), 10, 130, 16, WHITE);
        DrawText(TextFormat("Packets Sent: %d", game.packetsSent), 10, 150, 16, WHITE);
        DrawText(TextFormat("Packets Received: %d", game.packetsReceived), 10, 170, 16, WHITE);
        
        if (game.showAdvancedStats) {
            float frameTime = GetFrameTime() * 1000.0f; // Convert to ms
            DrawText(TextFormat("Frame Time: %.2fms", frameTime), 10, 190, 16, WHITE);
            DrawText(TextFormat("Target FPS: %s", game.targetFPS == 0 ? "Uncapped" : TextFormat("%d", game.targetFPS)), 10, 210, 16, WHITE);
            DrawText(TextFormat("VSync: %s", game.vsyncEnabled ? "ON" : "OFF"), 10, 230, 16, WHITE);
            DrawText(TextFormat("1% Low FPS: %.1f", GetFPS() * 0.99f), 10, 250, 16, WHITE);
        }
    } else {
        DrawText("F1=Debug F2=Stats F3=FPS F4=VSync", 10, 90, 14, LIGHTGRAY);
    }
}

Player* FindPlayer(const char* id)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && strcmp(game.players[i].id, id) == 0) {
            return &game.players[i];
        }
    }
    return NULL;
}

Player* CreatePlayer(const char* id, Vector2 pos)
{
    // Find existing player first
    Player* existing = FindPlayer(id);
    if (existing) return existing;
    
    // Find empty slot
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!game.players[i].active) {
            Player* p = &game.players[i];
            strcpy(p->id, id);
            p->position = pos;
            p->velocity = (Vector2){0, 0};
            p->gunAngle = 0;
            p->health = 100;
            p->shootCooldown = 0;
            p->color = (Color){
                128 + rand() % 128,
                128 + rand() % 128,
                128 + rand() % 128,
                255
            };
            p->active = true;
            p->isLocal = (strcmp(id, game.localPlayerId) == 0);
            p->lastUpdate = GetTime();
            game.playerCount++;
            return p;
        }
    }
    return NULL;
}

void CreateBullet(Vector2 pos, Vector2 vel, const char* playerId)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!game.bullets[i].active) {
            game.bullets[i].position = pos;
            game.bullets[i].velocity = vel;
            game.bullets[i].lifetime = BULLET_LIFETIME;
            strcpy(game.bullets[i].playerId, playerId);
            game.bullets[i].active = true;
            game.bulletCount++;
            
            // Send bullet over network
            if (game.isConnected || game.isHost) {
                NetworkMessage msg = {0};
                msg.type = MSG_BULLET_FIRE;
                strcpy(msg.playerId, playerId);
                msg.position = pos;
                msg.velocity = vel;
                msg.timestamp = GetTime();
                
                if (game.isHost) {
                    // Broadcast to all clients
                    for (int j = 0; j < game.clientCount; j++) {
                        SendMessage(&msg, &game.clientAddrs[j]);
                    }
                } else {
                    SendMessage(&msg, &game.serverAddr);
                }
            }
            break;
        }
    }
}

int StartHost(int port)
{
    game.socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (game.socket_fd < 0) {
        printf("Failed to create socket\n");
        return -1;
    }
    
    // Set non-blocking
    int flags = fcntl(game.socket_fd, F_GETFL, 0);
    fcntl(game.socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(game.socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Failed to bind socket\n");
        close(game.socket_fd);
        game.socket_fd = -1;
        return -1;
    }
    
    game.isHost = true;
    game.hostPort = port;
    game.clientCount = 0;
    
    printf("Server started on port %d\n", port);
    return 0;
}

int ConnectToServer(const char* ip, int port)
{
    game.socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (game.socket_fd < 0) {
        printf("Failed to create socket\n");
        return -1;
    }
    
    // Set non-blocking
    int flags = fcntl(game.socket_fd, F_GETFL, 0);
    fcntl(game.socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Convert localhost to 127.0.0.1
    if (strcmp(ip, "localhost") == 0) {
        ip = "127.0.0.1";
    }
    
    game.serverAddr.sin_family = AF_INET;
    game.serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &game.serverAddr.sin_addr);
    
    strcpy(game.hostIP, ip);
    game.hostPort = port;
    game.isConnected = true;
    
    // Send join message
    NetworkMessage msg = {0};
    msg.type = MSG_PLAYER_JOIN;
    strcpy(msg.playerId, game.localPlayerId);
    msg.position = (Vector2){200, 200};
    msg.timestamp = GetTime();
    
    SendMessage(&msg, &game.serverAddr);
    
    printf("Connected to server at %s:%d\n", ip, port);
    return 0;
}

void CloseNetwork(void)
{
    if (game.socket_fd >= 0) {
        close(game.socket_fd);
        game.socket_fd = -1;
    }
    game.isHost = false;
    game.isConnected = false;
    game.clientCount = 0;
}

void UpdateNetwork(void)
{
    if (game.socket_fd < 0) return;
    
    // Receive messages
    char buffer[MAX_MESSAGE_SIZE];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    
    while (1) {
        ssize_t received = recvfrom(game.socket_fd, buffer, sizeof(buffer), 0, 
                                  (struct sockaddr*)&from, &fromlen);
        if (received <= 0) break;
        
        game.packetsReceived++;
        
        if (received == sizeof(NetworkMessage)) {
            NetworkMessage* msg = (NetworkMessage*)buffer;
            ProcessMessage(msg, &from);
        }
    }
    
    // Send periodic updates for local player
    static double lastUpdate = 0;
    double now = GetTime();
    if (now - lastUpdate > 0.05) { // 20 FPS update rate
        Player* localPlayer = FindPlayer(game.localPlayerId);
        if (localPlayer && (game.isConnected || game.isHost)) {
            NetworkMessage msg = {0};
            msg.type = MSG_PLAYER_UPDATE;
            strcpy(msg.playerId, game.localPlayerId);
            msg.position = localPlayer->position;
            msg.velocity = localPlayer->velocity;
            msg.gunAngle = localPlayer->gunAngle;
            msg.health = localPlayer->health;
            msg.timestamp = now;
            
            if (game.isHost) {
                // Broadcast to all clients
                for (int i = 0; i < game.clientCount; i++) {
                    SendMessage(&msg, &game.clientAddrs[i]);
                }
            } else {
                SendMessage(&msg, &game.serverAddr);
            }
        }
        lastUpdate = now;
    }
    
    // Send ping
    static double lastPing = 0;
    if (now - lastPing > 2.0 && game.isConnected && !game.isHost) {
        NetworkMessage msg = {0};
        msg.type = MSG_PING;
        msg.timestamp = now;
        game.pingStartTime = now;
        SendMessage(&msg, &game.serverAddr);
        lastPing = now;
    }
}

void SendMessage(NetworkMessage* msg, struct sockaddr_in* addr)
{
    if (game.socket_fd < 0) return;
    
    ssize_t sent = sendto(game.socket_fd, msg, sizeof(NetworkMessage), 0,
                         (struct sockaddr*)addr, sizeof(*addr));
    if (sent > 0) {
        game.packetsSent++;
    }
}

void ProcessMessage(NetworkMessage* msg, struct sockaddr_in* from)
{
    switch (msg->type) {
        case MSG_PLAYER_JOIN:
            if (game.isHost) {
                // Add client to list
                bool found = false;
                for (int i = 0; i < game.clientCount; i++) {
                    if (game.clientAddrs[i].sin_addr.s_addr == from->sin_addr.s_addr &&
                        game.clientAddrs[i].sin_port == from->sin_port) {
                        found = true;
                        break;
                    }
                }
                if (!found && game.clientCount < MAX_PLAYERS - 1) {
                    game.clientAddrs[game.clientCount] = *from;
                    game.clientCount++;
                }
                
                // Send existing players to new client
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (game.players[i].active) {
                        NetworkMessage response = {0};
                        response.type = MSG_PLAYER_JOIN;
                        strcpy(response.playerId, game.players[i].id);
                        response.position = game.players[i].position;
                        response.timestamp = GetTime();
                        SendMessage(&response, from);
                    }
                }
            }
            CreatePlayer(msg->playerId, msg->position);
            break;
            
        case MSG_PLAYER_UPDATE: {
            Player* player = FindPlayer(msg->playerId);
            if (player && !player->isLocal) {
                // Smooth interpolation
                float lerpFactor = 0.3f;
                player->position.x += (msg->position.x - player->position.x) * lerpFactor;
                player->position.y += (msg->position.y - player->position.y) * lerpFactor;
                player->velocity = msg->velocity;
                player->gunAngle = msg->gunAngle;
                player->health = msg->health;
                player->lastUpdate = GetTime();
            }
            
            // Broadcast to other clients if host
            if (game.isHost) {
                for (int i = 0; i < game.clientCount; i++) {
                    if (game.clientAddrs[i].sin_addr.s_addr != from->sin_addr.s_addr ||
                        game.clientAddrs[i].sin_port != from->sin_port) {
                        SendMessage(msg, &game.clientAddrs[i]);
                    }
                }
            }
            break;
        }
        
        case MSG_BULLET_FIRE:
            // Only create bullet if not from local player
            if (strcmp(msg->playerId, game.localPlayerId) != 0) {
                CreateBullet(msg->position, msg->velocity, msg->playerId);
            }
            
            // Broadcast to other clients if host
            if (game.isHost) {
                for (int i = 0; i < game.clientCount; i++) {
                    if (game.clientAddrs[i].sin_addr.s_addr != from->sin_addr.s_addr ||
                        game.clientAddrs[i].sin_port != from->sin_port) {
                        SendMessage(msg, &game.clientAddrs[i]);
                    }
                }
            }
            break;
            
        case MSG_PING:
            if (game.isHost) {
                // Send pong back
                NetworkMessage pong = *msg;
                pong.type = MSG_PONG;
                SendMessage(&pong, from);
            }
            break;
            
        case MSG_PONG:
            game.ping = (GetTime() - game.pingStartTime) * 1000.0; // Convert to ms
            break;
    }
}

void SetStatusMessage(const char* message, float duration)
{
    strcpy(game.statusMessage, message);
    game.statusTimer = duration;
}

void GeneratePlayerId(char* id)
{
    sprintf(id, "player_%d", rand() % 10000);
}