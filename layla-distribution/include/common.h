#ifndef COMMON_H
#define COMMON_H

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

// Platform-specific networking headers
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Game constants
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define MAX_PLAYERS 16
#define MAX_BULLETS 256
#define PLAYER_SIZE 20.0f
#define PLAYER_SPEED 200.0f
#define PLAYER_ACCELERATION 1000.0f
#define PLAYER_FRICTION 10.0f
#define BULLET_SIZE 5.0f
#define BULLET_SPEED 800.0f
#define BULLET_LIFETIME 2.0f
#define GUN_LENGTH 20.0f
#define SCREEN_SHAKE_DECAY 0.95f
#define MAX_AMMO_DISPLAY 30
#define MAX_PARTICLES 2000
#define MAX_MUZZLE_FLASHES 50
#define MAX_HIT_EFFECTS 50
#define PARTICLE_LIFETIME 0.5f
#define MUZZLE_FLASH_LIFETIME 0.1f
#define HIT_EFFECT_LIFETIME 0.3f
#define FOV_ANGLE 60.0f
#define FOV_RANGE 500.0f
#define MAX_MESSAGE_SIZE 1024
#define DEFAULT_PORT 7777

// Game state enum
typedef enum {
    GAME_MENU,
    GAME_NAME_INPUT,
    GAME_HOST_SETUP,
    GAME_JOIN_SETUP,
    GAME_PLAYING
} GameState;

// Game mode enum
typedef enum {
    MODE_DEATHMATCH,
    MODE_TEAM_DEATHMATCH,
    MODE_CAPTURE_FLAG,
    MODE_TOTAL
} GameMode;

// Weapon types
typedef enum {
    WEAPON_PISTOL,
    WEAPON_RIFLE,
    WEAPON_SHOTGUN,
    WEAPON_SMG,
    WEAPON_SNIPER,
    WEAPON_TOTAL
} WeaponType;

// Particle types
typedef enum {
    PARTICLE_DEBRIS,
    PARTICLE_BLOOD,
    PARTICLE_SPARK,
    PARTICLE_SMOKE,
    PARTICLE_SHELL
} ParticleType;

// Forward declarations of structs
typedef struct Particle Particle;
typedef struct MuzzleFlash MuzzleFlash;
typedef struct HitEffect HitEffect;
typedef struct WeaponStats WeaponStats;
typedef struct Player Player;
typedef struct Bullet Bullet;
typedef struct Game Game;

// Particle structure
struct Particle {
    Vector2 position;
    Vector2 velocity;
    float rotation;
    float rotationSpeed;
    float size;
    float lifetime;
    float maxLifetime;
    Color color;
    Color startColor;
    Color endColor;
    ParticleType type;
    bool active;
};

// Muzzle flash structure
struct MuzzleFlash {
    Vector2 position;
    float rotation;
    float size;
    float lifetime;
    float maxLifetime;
    Color color;
    bool active;
    char ownerId[32];
};

// Hit effect structure
struct HitEffect {
    Vector2 position;
    float size;
    float lifetime;
    float maxLifetime;
    Color color;
    bool active;
};

// Weapon stats structure
struct WeaponStats {
    char name[32];
    int damage;
    float fireRate;  // Shots per second
    float reloadTime;
    int magazineSize;
    int maxAmmo;
    float spread;
    float bulletSpeed;
    int bulletsPerShot;
    float screenShakeIntensity;
    int particlesPerShot;
    Color muzzleFlashColor;
    float muzzleFlashSize;
    bool automatic;
    bool enabled;
};

// Player structure
struct Player {
    char id[32];
    char name[32];
    Vector2 position;
    Vector2 velocity;
    float rotation;
    float targetRotation;
    float health;
    float maxHealth;
    Color color;
    bool isLocal;
    bool active;
    
    // Team information
    int team;  // 0 = red team, 1 = blue team
    int score; // Individual player score
    int kills; // Number of kills
    int deaths; // Number of deaths
    
    // Weapons
    WeaponType currentWeapon;
    int ammo[WEAPON_TOTAL];
    int magazineAmmo[WEAPON_TOTAL];
    float fireTimer;
    float reloadTimer;
    bool isReloading;
};

// Bullet structure
struct Bullet {
    Vector2 position;
    Vector2 velocity;
    float rotation;
    float lifetime;
    int damage;
    char ownerId[32];
    bool active;
    Color color;
};

// Network message types
typedef enum {
    MSG_PLAYER_JOIN,
    MSG_PLAYER_LEAVE,
    MSG_PLAYER_UPDATE,
    MSG_PLAYER_SHOOT,
    MSG_PING,
    MSG_PONG,
    MSG_GAME_MODE,
    MSG_TEAM_SCORE,
    MSG_FLAG_UPDATE,
    MSG_CHAT
} MessageType;

// Flag structure for Capture the Flag mode
typedef struct {
    Vector2 position;
    Vector2 basePosition;
    bool isCaptured;
    int team;  // 0 = red team, 1 = blue team
    char carrierId[32];  // ID of player carrying the flag
} Flag;

// Network message structure
typedef struct {
    MessageType type;
    char playerId[32];
    union {
        Player player;
        Bullet bullet;
        float ping;
        GameMode gameMode;
        int teamScores[2];
        struct {
            Flag flag;
            int flagIndex;
        };
        struct {
            char chatMessage[256];
            char senderName[32];
        };
    } data;
} NetworkMessage;

// Game structure
struct Game {
    GameState state;
    GameMode mode;
    Player players[MAX_PLAYERS];
    Bullet bullets[MAX_BULLETS];
    int playerCount;
    int bulletCount;
    char localPlayerId[32];
    
    // Team scores (for team modes)
    int teamScores[2];  // 0 = red team, 1 = blue team
    
    // Capture the Flag specific
    Flag flags[2];      // 0 = red flag, 1 = blue flag
    
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
    bool editingPlayerName;
    bool editingChat;
    bool wantsToHost;  // Track if user wants to host or join
    char hostPortStr[8];
    char joinIPStr[16];
    char joinPortStr[8];
    char playerName[32];
    char playerNameInput[32];
    char chatInput[256];
    
    // Chat system
    struct {
        char message[256];
        char senderName[32];
        float displayTime;
    } chatMessages[10];
    int chatMessageCount;
    
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
    
    // Game mode settings
    float modeTimer;           // Timer for game modes
    float modeMaxTime;         // Maximum time for current game mode
    bool showModeInstructions; // Whether to show mode instructions
    
    // Visual effects
    Vector2 screenShake;
    float screenShakeIntensity;
    bool screenShakeEnabled;
    
    // Movement settings
    bool smoothMovement;
    
    // Visual effects
    Particle particles[MAX_PARTICLES];
    MuzzleFlash muzzleFlashes[MAX_MUZZLE_FLASHES];
    HitEffect hitEffects[MAX_HIT_EFFECTS];
    int particleCount;
    int muzzleFlashCount;
    int hitEffectCount;
    bool visualEffectsEnabled;
    float damageFlashTimer;
    Color damageFlashColor;
};

// Global game instance
extern Game game;

#endif // COMMON_H