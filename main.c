#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Game constants
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define MAX_PLAYERS 8
#define MAX_BULLETS 500
#define PLAYER_SIZE 20
#define PLAYER_SPEED 300.0f
#define PLAYER_ACCELERATION 2000.0f
#define PLAYER_FRICTION 1200.0f
#define BULLET_SIZE 3
#define BULLET_SPEED 600.0f
#define BULLET_LIFETIME 3.0f
#define GUN_LENGTH 25
#define SCREEN_SHAKE_DECAY 15.0f
#define MAX_AMMO_DISPLAY 999
#define MAX_MESSAGE_SIZE 1024
#define DEFAULT_PORT 12345

// Game states
typedef enum {
    GAME_MENU,
    GAME_HOST_SETUP,
    GAME_JOIN_SETUP,
    GAME_PLAYING
} GameState;

// Weapon types
typedef enum {
    WEAPON_PISTOL = 0,
    WEAPON_SHOTGUN = 1,
    WEAPON_RIFLE = 2,
    WEAPON_SMG = 3,
    WEAPON_SNIPER = 4,
    WEAPON_COUNT = 5
} WeaponType;

// Weapon statistics
typedef struct {
    WeaponType type;
    char name[16];
    float damage;
    float fireRate;          // Shots per second
    float recoilStrength;
    float recoilRecovery;
    float maxRecoilOffset;
    float bulletSpeed;
    float bulletLifetime;
    int pelletCount;         // For shotgun spread
    float spreadAngle;       // For shotgun/SMG spread
    int maxAmmo;
    int clipSize;
    float reloadTime;
    float range;
    Color bulletColor;
} WeaponStats;

// Player structure
typedef struct {
    char id[32];
    Vector2 position;
    Vector2 velocity;
    Vector2 targetVelocity;
    Vector2 recoilOffset;
    float gunAngle;
    float health;
    float shootCooldown;
    float recoilAmount;
    Color color;
    bool active;
    bool isLocal;
    double lastUpdate;
    
    // Weapon system
    WeaponType currentWeapon;
    int ammo[WEAPON_COUNT];
    int clipAmmo[WEAPON_COUNT];
    float reloadTimer;
    bool isReloading;
} Player;

// Bullet structure
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    char playerId[32];
    bool active;
    float damage;
    WeaponType weaponType;
    Color color;
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
    
    // Visual effects
    Vector2 screenShake;
    float screenShakeIntensity;
    bool screenShakeEnabled;
    
    // Movement settings
    bool smoothMovement;
} Game;

// Global game instance
Game game = {0};

// Weapon statistics array
WeaponStats weaponStats[WEAPON_COUNT] = {
    // PISTOL
    {
        .type = WEAPON_PISTOL,
        .name = "Pistol",
        .damage = 25.0f,
        .fireRate = 3.0f,
        .recoilStrength = 8.0f,
        .recoilRecovery = 12.0f,
        .maxRecoilOffset = 15.0f,
        .bulletSpeed = 700.0f,
        .bulletLifetime = 3.0f,
        .pelletCount = 1,
        .spreadAngle = 0.0f,
        .maxAmmo = 120,
        .clipSize = 12,
        .reloadTime = 1.5f,
        .range = 600.0f,
        .bulletColor = YELLOW
    },
    // SHOTGUN
    {
        .type = WEAPON_SHOTGUN,
        .name = "Shotgun",
        .damage = 15.0f,
        .fireRate = 1.2f,
        .recoilStrength = 25.0f,
        .recoilRecovery = 8.0f,
        .maxRecoilOffset = 35.0f,
        .bulletSpeed = 500.0f,
        .bulletLifetime = 1.5f,
        .pelletCount = 6,
        .spreadAngle = 0.3f,
        .maxAmmo = 36,
        .clipSize = 6,
        .reloadTime = 3.0f,
        .range = 300.0f,
        .bulletColor = ORANGE
    },
    // RIFLE
    {
        .type = WEAPON_RIFLE,
        .name = "Rifle",
        .damage = 40.0f,
        .fireRate = 2.0f,
        .recoilStrength = 18.0f,
        .recoilRecovery = 10.0f,
        .maxRecoilOffset = 25.0f,
        .bulletSpeed = 900.0f,
        .bulletLifetime = 4.0f,
        .pelletCount = 1,
        .spreadAngle = 0.0f,
        .maxAmmo = 90,
        .clipSize = 30,
        .reloadTime = 2.5f,
        .range = 800.0f,
        .bulletColor = WHITE
    },
    // SMG
    {
        .type = WEAPON_SMG,
        .name = "SMG",
        .damage = 18.0f,
        .fireRate = 8.0f,
        .recoilStrength = 12.0f,
        .recoilRecovery = 15.0f,
        .maxRecoilOffset = 20.0f,
        .bulletSpeed = 600.0f,
        .bulletLifetime = 2.5f,
        .pelletCount = 1,
        .spreadAngle = 0.1f,
        .maxAmmo = 180,
        .clipSize = 30,
        .reloadTime = 2.0f,
        .range = 500.0f,
        .bulletColor = LIME
    },
    // SNIPER
    {
        .type = WEAPON_SNIPER,
        .name = "Sniper",
        .damage = 80.0f,
        .fireRate = 0.8f,
        .recoilStrength = 35.0f,
        .recoilRecovery = 6.0f,
        .maxRecoilOffset = 50.0f,
        .bulletSpeed = 1200.0f,
        .bulletLifetime = 5.0f,
        .pelletCount = 1,
        .spreadAngle = 0.0f,
        .maxAmmo = 20,
        .clipSize = 5,
        .reloadTime = 4.0f,
        .range = 1000.0f,
        .bulletColor = RED
    }
};

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
WeaponStats* GetCurrentWeaponStats(Player* player);
void SwitchWeapon(Player* player, WeaponType weapon);
void ReloadWeapon(Player* player);
bool CanShoot(Player* player);
void FireWeapon(Player* player);
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
    game.screenShake = (Vector2){0, 0};
    game.screenShakeIntensity = 0;
    game.screenShakeEnabled = true;
    game.smoothMovement = true;
    
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
    
    // Update screen shake (much more subtle)
    if (game.screenShakeEnabled && game.screenShakeIntensity > 0) {
        game.screenShake.x = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * game.screenShakeIntensity * 0.2f;
        game.screenShake.y = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * game.screenShakeIntensity * 0.2f;
        game.screenShakeIntensity -= SCREEN_SHAKE_DECAY * dt;
        if (game.screenShakeIntensity < 0) game.screenShakeIntensity = 0;
    } else {
        game.screenShake = (Vector2){0, 0};
        game.screenShakeIntensity = 0;
    }
}

void DrawGame(void)
{
    // Apply screen shake offset
    if (game.screenShakeEnabled && game.screenShakeIntensity > 0) {
        rlPushMatrix();
        rlTranslatef(game.screenShake.x, game.screenShake.y, 0);
    }
    
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
    
    // Remove screen shake offset
    if (game.screenShakeEnabled && game.screenShakeIntensity > 0) {
        rlPopMatrix();
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
        } else if (IsKeyPressed(KEY_F5)) {
            game.screenShakeEnabled = !game.screenShakeEnabled;
            if (game.screenShakeEnabled) {
                SetStatusMessage("Screen Shake: ON", 2.0f);
            } else {
                SetStatusMessage("Screen Shake: OFF", 2.0f);
                game.screenShakeIntensity = 0;
            }
        } else if (IsKeyPressed(KEY_F6)) {
            game.smoothMovement = !game.smoothMovement;
            if (game.smoothMovement) {
                SetStatusMessage("Movement: Smooth", 2.0f);
            } else {
                SetStatusMessage("Movement: Direct", 2.0f);
            }
        }
        
        // Weapon switching and reloading for local player
        Player* localPlayer = FindPlayer(game.localPlayerId);
        if (localPlayer && localPlayer->active) {
            if (IsKeyPressed(KEY_ONE)) SwitchWeapon(localPlayer, WEAPON_PISTOL);
            else if (IsKeyPressed(KEY_TWO)) SwitchWeapon(localPlayer, WEAPON_SHOTGUN);
            else if (IsKeyPressed(KEY_THREE)) SwitchWeapon(localPlayer, WEAPON_RIFLE);
            else if (IsKeyPressed(KEY_FOUR)) SwitchWeapon(localPlayer, WEAPON_SMG);
            else if (IsKeyPressed(KEY_FIVE)) SwitchWeapon(localPlayer, WEAPON_SNIPER);
            else if (IsKeyPressed(KEY_R)) ReloadWeapon(localPlayer);
        }
    }
}

void UpdatePlayers(float dt)
{
    Player* localPlayer = FindPlayer(game.localPlayerId);
    if (localPlayer && localPlayer->active) {
        // Update gun angle to point at mouse (with recoil offset)
        Vector2 mousePos = GetMousePosition();
        Vector2 adjustedMousePos = {
            mousePos.x + localPlayer->recoilOffset.x,
            mousePos.y + localPlayer->recoilOffset.y
        };
        Vector2 diff = {adjustedMousePos.x - localPlayer->position.x, adjustedMousePos.y - localPlayer->position.y};
        localPlayer->gunAngle = atan2f(diff.y, diff.x);
        
        // Handle movement input
        Vector2 inputDirection = {0};
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) inputDirection.y -= 1;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) inputDirection.y += 1;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) inputDirection.x -= 1;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) inputDirection.x += 1;
        
        // Normalize input direction
        float inputLength = sqrtf(inputDirection.x * inputDirection.x + inputDirection.y * inputDirection.y);
        if (inputLength > 0) {
            inputDirection.x /= inputLength;
            inputDirection.y /= inputLength;
        }
        
        // Calculate target velocity
        localPlayer->targetVelocity.x = inputDirection.x * PLAYER_SPEED;
        localPlayer->targetVelocity.y = inputDirection.y * PLAYER_SPEED;
        
        // Movement physics - choose between smooth or direct
        if (game.smoothMovement) {
            // Smooth movement with improved direction changes
            if (inputLength > 0) {
                // Use different acceleration based on whether we're changing direction or not
                float currentSpeed = sqrtf(localPlayer->velocity.x * localPlayer->velocity.x + localPlayer->velocity.y * localPlayer->velocity.y);
                
                // Check if we're changing direction significantly
                float dotProduct = (localPlayer->velocity.x * inputDirection.x + localPlayer->velocity.y * inputDirection.y) / (currentSpeed + 0.001f);
                
                float effectiveAccel = PLAYER_ACCELERATION;
                if (dotProduct < 0.3f && currentSpeed > PLAYER_SPEED * 0.2f) {
                    // Changing direction - use much higher acceleration for responsiveness
                    effectiveAccel = PLAYER_ACCELERATION * 4.0f;
                }
                
                // Smooth acceleration with clamping
                float accelX = (localPlayer->targetVelocity.x - localPlayer->velocity.x) * effectiveAccel * dt;
                float accelY = (localPlayer->targetVelocity.y - localPlayer->velocity.y) * effectiveAccel * dt;
                
                localPlayer->velocity.x += accelX;
                localPlayer->velocity.y += accelY;
                
                // Clamp to maximum speed
                float speed = sqrtf(localPlayer->velocity.x * localPlayer->velocity.x + localPlayer->velocity.y * localPlayer->velocity.y);
                if (speed > PLAYER_SPEED) {
                    localPlayer->velocity.x = (localPlayer->velocity.x / speed) * PLAYER_SPEED;
                    localPlayer->velocity.y = (localPlayer->velocity.y / speed) * PLAYER_SPEED;
                }
            } else {
                // Apply friction when no input
                float frictionForce = PLAYER_FRICTION * dt;
                float currentSpeed = sqrtf(localPlayer->velocity.x * localPlayer->velocity.x + localPlayer->velocity.y * localPlayer->velocity.y);
                if (currentSpeed > 0) {
                    float frictionScale = frictionForce / currentSpeed;
                    if (frictionScale >= 1.0f) {
                        localPlayer->velocity.x = 0;
                        localPlayer->velocity.y = 0;
                    } else {
                        localPlayer->velocity.x *= (1.0f - frictionScale);
                        localPlayer->velocity.y *= (1.0f - frictionScale);
                    }
                }
            }
        } else {
            // Direct movement - instant response like classic shooters
            localPlayer->velocity = localPlayer->targetVelocity;
        }
        
        // Apply velocity to position
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
        
        // Update recoil recovery
        WeaponStats* currentWeapon = GetCurrentWeaponStats(localPlayer);
        if (localPlayer->recoilAmount > 0) {
            localPlayer->recoilAmount -= currentWeapon->recoilRecovery * dt;
            if (localPlayer->recoilAmount < 0) localPlayer->recoilAmount = 0;
        }
        
        // Update recoil offset based on current recoil amount
        float recoilAngle = localPlayer->gunAngle + M_PI; // Opposite direction of gun
        localPlayer->recoilOffset.x = cosf(recoilAngle) * localPlayer->recoilAmount;
        localPlayer->recoilOffset.y = sinf(recoilAngle) * localPlayer->recoilAmount;
        
        // Handle shooting
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CanShoot(localPlayer)) {
            FireWeapon(localPlayer);
        }
        
        // Update reload timer
        if (localPlayer->isReloading && localPlayer->reloadTimer > 0) {
            localPlayer->reloadTimer -= dt;
            if (localPlayer->reloadTimer <= 0) {
                // Reload complete
                WeaponStats* stats = GetCurrentWeaponStats(localPlayer);
                int ammoNeeded = stats->clipSize - localPlayer->clipAmmo[localPlayer->currentWeapon];
                int ammoToReload = (localPlayer->ammo[localPlayer->currentWeapon] < ammoNeeded) ? 
                                  localPlayer->ammo[localPlayer->currentWeapon] : ammoNeeded;
                
                localPlayer->clipAmmo[localPlayer->currentWeapon] += ammoToReload;
                localPlayer->ammo[localPlayer->currentWeapon] -= ammoToReload;
                localPlayer->isReloading = false;
            }
        }
    }
    
    // Update all players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active) {
            // Update non-local players with smooth interpolation
            if (!game.players[i].isLocal) {
                // Reduced prediction to prevent teleporting
                game.players[i].position.x += game.players[i].velocity.x * dt * 0.05f;
                game.players[i].position.y += game.players[i].velocity.y * dt * 0.05f;
                
                // Update recoil recovery for remote players
                WeaponStats* remoteWeapon = GetCurrentWeaponStats(&game.players[i]);
                if (game.players[i].recoilAmount > 0) {
                    game.players[i].recoilAmount -= remoteWeapon->recoilRecovery * dt;
                    if (game.players[i].recoilAmount < 0) game.players[i].recoilAmount = 0;
                }
                
                // Update recoil offset
                float recoilAngle = game.players[i].gunAngle + M_PI;
                game.players[i].recoilOffset.x = cosf(recoilAngle) * game.players[i].recoilAmount;
                game.players[i].recoilOffset.y = sinf(recoilAngle) * game.players[i].recoilAmount;
                
                // Update reload timer for remote players
                if (game.players[i].isReloading && game.players[i].reloadTimer > 0) {
                    game.players[i].reloadTimer -= dt;
                    if (game.players[i].reloadTimer <= 0) {
                        game.players[i].isReloading = false;
                    }
                }
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
                        game.players[j].health -= game.bullets[i].damage;
                        game.bullets[i].active = false;
                        game.bulletCount--;
                        
                        if (game.players[j].health <= 0) {
                            // Respawn player
                            game.players[j].position = (Vector2){
                                (float)(rand() % (SCREEN_WIDTH - PLAYER_SIZE)) + PLAYER_SIZE/2,
                                (float)(rand() % (SCREEN_HEIGHT - PLAYER_SIZE)) + PLAYER_SIZE/2
                            };
                            game.players[j].health = 100;
                            // Reset velocity to prevent teleporting
                            game.players[j].velocity = (Vector2){0, 0};
                            game.players[j].targetVelocity = (Vector2){0, 0};
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
            DrawCircle(game.bullets[i].position.x, game.bullets[i].position.y, BULLET_SIZE, game.bullets[i].color);
        }
    }
}

void DrawUI(void)
{
    Player* localPlayer = FindPlayer(game.localPlayerId);
    
    // Clean top-left HUD
    DrawRectangle(5, 5, 200, 80, (Color){0, 0, 0, 120}); // Semi-transparent background
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 16, 
             GetFPS() > 60 ? GREEN : (GetFPS() > 30 ? YELLOW : RED));
    DrawText(TextFormat("Players: %d", game.playerCount), 10, 30, 16, WHITE);
    
    // Network status with color coding
    if (game.isHost) {
        DrawText("HOST", 10, 50, 16, GREEN);
        DrawText(TextFormat(":%d", game.hostPort), 50, 50, 16, LIGHTGRAY);
    } else if (game.isConnected) {
        Color pingColor = game.ping > 100 ? RED : (game.ping > 50 ? YELLOW : GREEN);
        DrawText(TextFormat("PING: %.0fms", game.ping), 10, 50, 16, pingColor);
    } else {
        DrawText("OFFLINE", 10, 50, 16, RED);
    }
    
    // Weapon/Ammo HUD (bottom-right)
    if (localPlayer && localPlayer->active) {
        WeaponStats* stats = GetCurrentWeaponStats(localPlayer);
        int hudX = SCREEN_WIDTH - 250;
        int hudY = SCREEN_HEIGHT - 120;
        
        // Weapon HUD background
        DrawRectangle(hudX - 10, hudY - 10, 240, 110, (Color){0, 0, 0, 140});
        DrawRectangleLines(hudX - 10, hudY - 10, 240, 110, WHITE);
        
        // Weapon name
        DrawText(stats->name, hudX, hudY, 24, stats->bulletColor);
        
        // Ammo display
        if (localPlayer->isReloading) {
            float reloadProgress = 1.0f - (localPlayer->reloadTimer / stats->reloadTime);
            DrawText("RELOADING", hudX, hudY + 30, 18, YELLOW);
            DrawRectangle(hudX, hudY + 55, 200, 10, DARKGRAY);
            DrawRectangle(hudX, hudY + 55, 200 * reloadProgress, 10, YELLOW);
        } else {
            // Clip ammo (large)
            DrawText(TextFormat("%d", localPlayer->clipAmmo[localPlayer->currentWeapon]), 
                     hudX, hudY + 30, 32, 
                     localPlayer->clipAmmo[localPlayer->currentWeapon] > 0 ? WHITE : RED);
            
            // Reserve ammo (small)
            DrawText(TextFormat("/ %d", localPlayer->ammo[localPlayer->currentWeapon]), 
                     hudX + 50, hudY + 40, 16, LIGHTGRAY);
        }
        
        // Health bar
        int healthBarY = hudY + 70;
        DrawText("HEALTH", hudX, healthBarY, 14, WHITE);
        DrawRectangle(hudX, healthBarY + 20, 200, 12, DARKGRAY);
        Color healthColor = localPlayer->health > 50 ? GREEN : 
                           (localPlayer->health > 25 ? YELLOW : RED);
        DrawRectangle(hudX, healthBarY + 20, 200 * (localPlayer->health / 100.0f), 12, healthColor);
        DrawText(TextFormat("%.0f", localPlayer->health), hudX + 210, healthBarY + 20, 12, WHITE);
    }
    
    // Weapon selection indicator (center-bottom)
    if (localPlayer && localPlayer->active) {
        int weaponHudY = SCREEN_HEIGHT - 60;
        int startX = SCREEN_WIDTH / 2 - 150;
        
        for (int i = 0; i < WEAPON_COUNT; i++) {
            int x = startX + i * 60;
            Rectangle weaponSlot = {x, weaponHudY, 50, 40};
            
            Color slotColor = (i == localPlayer->currentWeapon) ? 
                             weaponStats[i].bulletColor : DARKGRAY;
            
            DrawRectangleRec(weaponSlot, (Color){slotColor.r/3, slotColor.g/3, slotColor.b/3, 180});
            DrawRectangleLinesEx(weaponSlot, 2, slotColor);
            
            // Weapon number
            DrawText(TextFormat("%d", i + 1), x + 20, weaponHudY + 5, 16, slotColor);
            
            // Weapon initial
            DrawText(TextFormat("%c", weaponStats[i].name[0]), x + 20, weaponHudY + 20, 14, slotColor);
        }
    }
    
    // Debug overlay (cleaner)
    if (game.debugMode) {
        DrawRectangle(5, 90, 300, 200, (Color){0, 0, 0, 180});
        DrawText("DEBUG MODE", 10, 95, 16, YELLOW);
        DrawText(TextFormat("Bullets: %d", game.bulletCount), 10, 115, 14, WHITE);
        DrawText(TextFormat("Packets: %d/%d", game.packetsSent, game.packetsReceived), 10, 135, 14, WHITE);
        
        if (game.showAdvancedStats && localPlayer) {
            float frameTime = GetFrameTime() * 1000.0f;
            DrawText(TextFormat("Frame: %.1fms", frameTime), 10, 155, 14, WHITE);
            DrawText(TextFormat("Velocity: %.0f", sqrtf(localPlayer->velocity.x * localPlayer->velocity.x + localPlayer->velocity.y * localPlayer->velocity.y)), 10, 175, 14, WHITE);
            DrawText(TextFormat("Recoil: %.1f", localPlayer->recoilAmount), 10, 195, 14, WHITE);
            DrawText(TextFormat("Movement: %s", game.smoothMovement ? "Smooth" : "Direct"), 10, 215, 14, WHITE);
        }
    }
    
    // Controls hint (bottom-left, smaller)
    if (!game.debugMode) {
        DrawText("F1=Debug | 1-5=Weapons | R=Reload | F6=Movement", 10, SCREEN_HEIGHT - 20, 12, GRAY);
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
            p->targetVelocity = (Vector2){0, 0};
            p->recoilOffset = (Vector2){0, 0};
            p->gunAngle = 0;
            p->health = 100;
            p->shootCooldown = 0;
            p->recoilAmount = 0;
            p->currentWeapon = WEAPON_PISTOL;
            p->isReloading = false;
            p->reloadTimer = 0;
            
            // Initialize ammo
            for (int w = 0; w < WEAPON_COUNT; w++) {
                p->ammo[w] = weaponStats[w].maxAmmo;
                p->clipAmmo[w] = weaponStats[w].clipSize;
            }
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
    Player* player = FindPlayer(playerId);
    WeaponStats* stats = player ? GetCurrentWeaponStats(player) : &weaponStats[WEAPON_PISTOL];
    
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!game.bullets[i].active) {
            game.bullets[i].position = pos;
            game.bullets[i].velocity = vel;
            game.bullets[i].lifetime = stats->bulletLifetime;
            game.bullets[i].damage = stats->damage;
            game.bullets[i].weaponType = stats->type;
            game.bullets[i].color = stats->bulletColor;
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

WeaponStats* GetCurrentWeaponStats(Player* player)
{
    if (!player || player->currentWeapon >= WEAPON_COUNT) {
        return &weaponStats[WEAPON_PISTOL];
    }
    return &weaponStats[player->currentWeapon];
}

void SwitchWeapon(Player* player, WeaponType weapon)
{
    if (!player || player->isReloading || weapon >= WEAPON_COUNT) return;
    
    player->currentWeapon = weapon;
    char msg[64];
    sprintf(msg, "Switched to %s", weaponStats[weapon].name);
    SetStatusMessage(msg, 1.5f);
}

void ReloadWeapon(Player* player)
{
    if (!player || player->isReloading) return;
    
    WeaponStats* stats = GetCurrentWeaponStats(player);
    
    // Check if reload is needed and possible
    if (player->clipAmmo[player->currentWeapon] >= stats->clipSize || 
        player->ammo[player->currentWeapon] <= 0) {
        return;
    }
    
    player->isReloading = true;
    player->reloadTimer = stats->reloadTime;
}

bool CanShoot(Player* player)
{
    if (!player || player->isReloading || player->shootCooldown > 0) return false;
    
    return player->clipAmmo[player->currentWeapon] > 0;
}

void FireWeapon(Player* player)
{
    if (!CanShoot(player)) return;
    
    WeaponStats* stats = GetCurrentWeaponStats(player);
    
    // Consume ammo
    player->clipAmmo[player->currentWeapon]--;
    
    // Set cooldown based on fire rate
    player->shootCooldown = 1.0f / stats->fireRate;
    
    // Fire bullets (multiple for shotgun)
    for (int pellet = 0; pellet < stats->pelletCount; pellet++) {
        float spreadOffset = 0;
        if (stats->pelletCount > 1) {
            // Shotgun spread
            spreadOffset = ((float)pellet / (stats->pelletCount - 1) - 0.5f) * stats->spreadAngle;
        } else if (stats->spreadAngle > 0) {
            // Random spread for SMG
            spreadOffset = ((float)rand() / RAND_MAX - 0.5f) * stats->spreadAngle;
        }
        
        float angle = player->gunAngle + spreadOffset;
        Vector2 gunEnd = {
            player->position.x + cosf(angle) * GUN_LENGTH,
            player->position.y + sinf(angle) * GUN_LENGTH
        };
        Vector2 bulletVel = {
            cosf(angle) * stats->bulletSpeed,
            sinf(angle) * stats->bulletSpeed
        };
        
        CreateBullet(gunEnd, bulletVel, player->id);
    }
    
    // Apply recoil (clamped to prevent excessive buildup)
    player->recoilAmount += stats->recoilStrength * 0.7f; // Reduced multiplier
    if (player->recoilAmount > stats->maxRecoilOffset) {
        player->recoilAmount = stats->maxRecoilOffset;
    }
    
    // Add screen shake for local player (only if enabled)
    if (player->isLocal && game.screenShakeEnabled) {
        game.screenShakeIntensity += stats->recoilStrength * 0.05f;
        if (game.screenShakeIntensity > 3.0f) {
            game.screenShakeIntensity = 3.0f;
        }
    }
    
    // Apply recoil to velocity (kickback)
    float recoilKickback = stats->recoilStrength * 1.5f;
    player->velocity.x -= cosf(player->gunAngle) * recoilKickback;
    player->velocity.y -= sinf(player->gunAngle) * recoilKickback;
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
    if (now - lastUpdate > 0.033) { // 30 FPS update rate
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
                // Check if position difference is too large (potential teleport)
                float dx = msg->position.x - player->position.x;
                float dy = msg->position.y - player->position.y;
                float distance = sqrtf(dx * dx + dy * dy);
                
                if (distance > 100.0f) {
                    // Large difference, snap to server position
                    player->position = msg->position;
                } else {
                    // Small difference, smooth interpolation
                    float lerpFactor = 0.15f; // Reduced for smoother movement
                    player->position.x += dx * lerpFactor;
                    player->position.y += dy * lerpFactor;
                }
                
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