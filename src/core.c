#include "../include/common.h"
#include "../include/core.h"
#include "../include/player.h"
#include "../include/weapons.h"
#include "../include/particles.h"
#include "../include/network.h"
#include <errno.h>
#include <stdarg.h>

void InitGame(void)
{
    srand(time(NULL));
    
    game.state = GAME_MENU;
    game.mode = MODE_DEATHMATCH;
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
    game.visualEffectsEnabled = true;
    game.particleCount = 0;
    game.muzzleFlashCount = 0;
    game.hitEffectCount = 0;
    game.damageFlashTimer = 0;
    game.damageFlashColor = (Color){255, 0, 0, 0};
    
    // Initialize player name
    strcpy(game.playerName, "Player");
    strcpy(game.playerNameInput, "Player");
    game.editingPlayerName = false;
    game.wantsToHost = false;
    
    // Initialize team scores
    game.teamScores[0] = 0;
    game.teamScores[1] = 0;
    
    // Initialize flags for Capture the Flag mode
    game.flags[0].position = (Vector2){100, SCREEN_HEIGHT/2};
    game.flags[0].basePosition = (Vector2){100, SCREEN_HEIGHT/2};
    game.flags[0].isCaptured = false;
    game.flags[0].team = 0;
    strcpy(game.flags[0].carrierId, "");
    
    game.flags[1].position = (Vector2){SCREEN_WIDTH - 100, SCREEN_HEIGHT/2};
    game.flags[1].basePosition = (Vector2){SCREEN_WIDTH - 100, SCREEN_HEIGHT/2};
    game.flags[1].isCaptured = false;
    game.flags[1].team = 1;
    strcpy(game.flags[1].carrierId, "");
    
    // Game mode settings
    game.modeTimer = 0;
    game.modeMaxTime = 300.0f; // 5 minutes by default
    game.showModeInstructions = true;
    
    // Initialize all particles as inactive
    for (int i = 0; i < MAX_PARTICLES; i++) {
        game.particles[i].active = false;
    }
    for (int i = 0; i < MAX_MUZZLE_FLASHES; i++) {
        game.muzzleFlashes[i].active = false;
    }
    for (int i = 0; i < MAX_HIT_EFFECTS; i++) {
        game.hitEffects[i].active = false;
    }
    
    // Initialize input fields
    strcpy(game.hostPortStr, "12345");
    strcpy(game.joinIPStr, "127.0.0.1");
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
        UpdatePlayers();
        UpdateBullets();
        UpdateParticles();
        UpdateMuzzleFlashes();
        UpdateHitEffects();
        UpdateNetwork();
        UpdateGameMode();
    }
    
    // Update status message timer
    if (game.statusTimer > 0) {
        game.statusTimer -= dt;
    }
    
    // Update damage flash
    if (game.damageFlashTimer > 0) {
        game.damageFlashTimer -= dt;
        float alpha = (game.damageFlashTimer / 0.3f) * 60.0f;
        game.damageFlashColor.a = (unsigned char)(alpha > 255 ? 255 : alpha);
    }
    
    // Update screen shake (much more subtle)
    if (game.screenShakeEnabled && game.screenShakeIntensity > 0) {
        // Reduce intensity by 75% and apply a smoother curve
        float dampenedIntensity = game.screenShakeIntensity * 0.05f;
        game.screenShake.x = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * dampenedIntensity;
        game.screenShake.y = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * dampenedIntensity;
        // Faster decay for less lingering shake
        game.screenShakeIntensity -= (SCREEN_SHAKE_DECAY * 1.5f) * dt;
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
        case GAME_NAME_INPUT:
            DrawNameInput();
            break;
        case GAME_HOST_SETUP:
            DrawHostSetup();
            break;
        case GAME_JOIN_SETUP:
            DrawJoinSetup();
            break;
        case GAME_PLAYING:
            // Draw game mode specific elements
            DrawGameMode();
            
            DrawPlayers();
            DrawBullets();
            if (game.visualEffectsEnabled) {
                DrawParticles();
                DrawMuzzleFlashes();
                DrawHitEffects();
            }
            
            DrawUI();
            
            // Draw damage flash overlay
            if (game.damageFlashTimer > 0) {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, game.damageFlashColor);
            }
            break;
    }
    
    // Remove screen shake offset
    if (game.screenShakeEnabled && game.screenShakeIntensity > 0) {
        rlPopMatrix();
    }
}

void HandleInput(void)
{
    switch (game.state) {
        case GAME_MENU: {
            // Main menu controls
            if (IsKeyPressed(KEY_H) || IsKeyPressed(KEY_ONE)) {
                game.state = GAME_NAME_INPUT;
                game.wantsToHost = true;
                strcpy(game.playerNameInput, game.playerName);
                game.editingPlayerName = true;
            } else if (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_TWO)) {
                game.state = GAME_NAME_INPUT;
                game.wantsToHost = false;
                strcpy(game.playerNameInput, game.playerName);
                game.editingPlayerName = true;
            } else if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_THREE)) {
                DrawGameModeMenu();
            } else if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_ESCAPE)) {
                CloseWindow();
            }
            break;
        }
        
        case GAME_NAME_INPUT: {
            // Name input controls
            if (IsKeyPressed(KEY_ESCAPE)) {
                game.state = GAME_MENU;
                game.editingPlayerName = false;
            }
            
            // Handle name input
            if (game.editingPlayerName) {
                int key = GetKeyPressed();
                if (key >= KEY_A && key <= KEY_Z) {
                    int letter = key - KEY_A;
                    int len = strlen(game.playerNameInput);
                    if (len < 31) {
                        char c = 'A' + letter;
                        if (!IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
                            c = 'a' + letter;
                        }
                        game.playerNameInput[len] = c;
                        game.playerNameInput[len+1] = '\0';
                    }
                } else if (key == KEY_SPACE) {
                    int len = strlen(game.playerNameInput);
                    if (len < 31) {
                        game.playerNameInput[len] = ' ';
                        game.playerNameInput[len+1] = '\0';
                    }
                }
                
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    int len = strlen(game.playerNameInput);
                    if (len > 0) {
                        game.playerNameInput[len-1] = '\0';
                    }
                }
                
                if (IsKeyPressed(KEY_ENTER)) {
                    // Accept the name and proceed
                    if (strlen(game.playerNameInput) > 0) {
                        strcpy(game.playerName, game.playerNameInput);
                    } else {
                        strcpy(game.playerName, "Player");
                    }
                    game.editingPlayerName = false;
                    
                    // Proceed to the correct setup screen
                    if (game.wantsToHost) {
                        game.state = GAME_HOST_SETUP;
                    } else {
                        game.state = GAME_JOIN_SETUP;
                    }
                }
            }
            break;
        }
            
        case GAME_HOST_SETUP: {
            // Host setup controls
            if (IsKeyPressed(KEY_ESCAPE)) {
                game.state = GAME_MENU;
            }
            
            // Toggle editing port field
            if (IsKeyPressed(KEY_TAB) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Rectangle portRect = (Rectangle){SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 20, 200, 40};
                if (CheckCollisionPointRec(GetMousePosition(), portRect)) {
                    game.editingHostPort = true;
                } else {
                    game.editingHostPort = false;
                }
            }
            
            // Edit port field
            if (game.editingHostPort) {
                int key = GetKeyPressed();
                if (key >= KEY_ZERO && key <= KEY_NINE) {
                    int digit = key - KEY_ZERO;
                    int len = strlen(game.hostPortStr);
                    if (len < 5) {  // Keep port within 0-65535
                        game.hostPortStr[len] = '0' + digit;
                        game.hostPortStr[len+1] = '\0';
                    }
                }
                
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    int len = strlen(game.hostPortStr);
                    if (len > 0) {
                        game.hostPortStr[len-1] = '\0';
                    }
                }
            }
            
            // Start hosting button
            Rectangle startRect = (Rectangle){SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50, 200, 40};
            if ((IsKeyPressed(KEY_ENTER) || 
                 (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), startRect))) &&
                strlen(game.hostPortStr) > 0) {
                
                game.hostPort = atoi(game.hostPortStr);
                if (game.hostPort > 0 && game.hostPort < 65536) {
                    int result = StartHost(game.hostPort);
                    if (result == 0) {
                        game.state = GAME_PLAYING;
                        game.isHost = true;
                        game.isConnected = true;
                        
                        // Create local player
                        Player* player = CreatePlayer(game.localPlayerId, game.playerName, true);
                        if (player) {
                            // Set player at a better starting position
                            player->position.x = SCREEN_WIDTH/2;
                            player->position.y = SCREEN_HEIGHT/2;
                        }
                        
                        SetStatusMessage("Hosting game on port %d", game.hostPort);
                    } else {
                        SetStatusMessage("Failed to start host: %s", strerror(errno));
                    }
                } else {
                    SetStatusMessage("Invalid port number");
                }
            }
            break;
        }
            
        case GAME_JOIN_SETUP: {
            // Join setup controls
            if (IsKeyPressed(KEY_ESCAPE)) {
                game.state = GAME_MENU;
            }
            
            // Toggle editing IP/port fields
            if (IsKeyPressed(KEY_TAB)) {
                if (game.editingJoinIP) {
                    game.editingJoinIP = false;
                    game.editingJoinPort = true;
                } else if (game.editingJoinPort) {
                    game.editingJoinPort = false;
                    game.editingJoinIP = true;
                } else {
                    game.editingJoinIP = true;
                }
            }
            
            // Check if clicked on fields
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Rectangle ipRect = (Rectangle){SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 60, 300, 40};
                Rectangle portRect = (Rectangle){SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2, 200, 40};
                
                if (CheckCollisionPointRec(GetMousePosition(), ipRect)) {
                    game.editingJoinIP = true;
                    game.editingJoinPort = false;
                } else if (CheckCollisionPointRec(GetMousePosition(), portRect)) {
                    game.editingJoinIP = false;
                    game.editingJoinPort = true;
                } else {
                    game.editingJoinIP = false;
                    game.editingJoinPort = false;
                }
            }
            
            // Edit IP field
            if (game.editingJoinIP) {
                int key = GetCharPressed();
                if ((key >= 32 && key <= 126) && key != '.') {  // Normal ASCII chars except period
                    int len = strlen(game.joinIPStr);
                    if (len < 15) {  // Limit length
                        game.joinIPStr[len] = (char)key;
                        game.joinIPStr[len+1] = '\0';
                    }
                }
                
                // Allow period for IP address
                if (IsKeyPressed(KEY_PERIOD)) {
                    int len = strlen(game.joinIPStr);
                    if (len < 15) {  // Limit length
                        game.joinIPStr[len] = '.';
                        game.joinIPStr[len+1] = '\0';
                    }
                }
                
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    int len = strlen(game.joinIPStr);
                    if (len > 0) {
                        game.joinIPStr[len-1] = '\0';
                    }
                }
            }
            
            // Edit port field
            if (game.editingJoinPort) {
                int key = GetKeyPressed();
                if (key >= KEY_ZERO && key <= KEY_NINE) {
                    int digit = key - KEY_ZERO;
                    int len = strlen(game.joinPortStr);
                    if (len < 5) {  // Keep port within 0-65535
                        game.joinPortStr[len] = '0' + digit;
                        game.joinPortStr[len+1] = '\0';
                    }
                }
                
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    int len = strlen(game.joinPortStr);
                    if (len > 0) {
                        game.joinPortStr[len-1] = '\0';
                    }
                }
            }
            
            // Connect button
            Rectangle connectRect = (Rectangle){SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50, 200, 40};
            if ((IsKeyPressed(KEY_ENTER) || 
                 (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), connectRect))) &&
                strlen(game.joinIPStr) > 0 && strlen(game.joinPortStr) > 0) {
                
                game.joinPort = atoi(game.joinPortStr);
                if (game.joinPort > 0 && game.joinPort < 65536) {
                    int result = ConnectToServer(game.joinIPStr, game.joinPort);
                    if (result == 0) {
                        game.state = GAME_PLAYING;
                        game.isHost = false;
                        game.isConnected = true;
                        
                        // Create local player
                        Player* player = CreatePlayer(game.localPlayerId, game.playerName, true);
                        if (player) {
                            // Set player at a better starting position
                            player->position.x = SCREEN_WIDTH/2;
                            player->position.y = SCREEN_HEIGHT/2;
                        }
                        
                        // Send join message
                        NetworkMessage joinMsg;
                        joinMsg.type = MSG_PLAYER_JOIN;
                        strcpy(joinMsg.playerId, game.localPlayerId);
                        joinMsg.data.player = *FindPlayer(game.localPlayerId);
                        SendMessage(&joinMsg, &game.serverAddr);
                        
                        SetStatusMessage("Connected to %s:%d", game.joinIPStr, game.joinPort);
                    } else {
                        SetStatusMessage("Failed to connect: %s", strerror(errno));
                    }
                } else {
                    SetStatusMessage("Invalid port number");
                }
            }
            break;
        }
            
        case GAME_PLAYING: {
            Player* localPlayer = FindPlayer(game.localPlayerId);
            
            if (localPlayer && localPlayer->active) {
                // Toggle debug mode
                if (IsKeyPressed(KEY_F1)) {
                    game.debugMode = !game.debugMode;
                }
                
                // Toggle vsync
                if (IsKeyPressed(KEY_F2)) {
                    game.vsyncEnabled = !game.vsyncEnabled;
                    if (game.vsyncEnabled) {
                        SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
                    } else {
                        SetTargetFPS(game.targetFPS); // Restore previous FPS setting
                    }
                }
                
                // Toggle FPS limit
                if (IsKeyPressed(KEY_F3)) {
                    if (game.targetFPS == 0) {
                        game.targetFPS = 60;
                    } else if (game.targetFPS == 60) {
                        game.targetFPS = 144;
                    } else if (game.targetFPS == 144) {
                        game.targetFPS = 240;
                    } else {
                        game.targetFPS = 0; // Uncapped
                    }
                    
                    if (!game.vsyncEnabled) {
                        SetTargetFPS(game.targetFPS);
                    }
                }
                
                // Toggle advanced stats
                if (IsKeyPressed(KEY_F4)) {
                    game.showAdvancedStats = !game.showAdvancedStats;
                }
                
                // Toggle screen shake
                if (IsKeyPressed(KEY_F5)) {
                    game.screenShakeEnabled = !game.screenShakeEnabled;
                }
                
                // Toggle smooth movement
                if (IsKeyPressed(KEY_F6)) {
                    game.smoothMovement = !game.smoothMovement;
                }
                
                // Toggle visual effects
                if (IsKeyPressed(KEY_F7)) {
                    game.visualEffectsEnabled = !game.visualEffectsEnabled;
                }
                
                // Player movement controls
                float horizontalInput = 0;
                float verticalInput = 0;
                
                if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) verticalInput -= 1.0f;
                if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) verticalInput += 1.0f;
                if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) horizontalInput -= 1.0f;
                if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) horizontalInput += 1.0f;
                
                // Normalize input vector if moving diagonally
                if (horizontalInput != 0 && verticalInput != 0) {
                    float length = sqrtf(horizontalInput * horizontalInput + verticalInput * verticalInput);
                    horizontalInput /= length;
                    verticalInput /= length;
                }
                
                // Direct velocity application for better responsiveness
                if (horizontalInput != 0 || verticalInput != 0) {
                    localPlayer->velocity.x = horizontalInput * PLAYER_SPEED;
                    localPlayer->velocity.y = verticalInput * PLAYER_SPEED;
                } else {
                    // Apply deceleration when no input
                    localPlayer->velocity.x *= 0.8f;
                    localPlayer->velocity.y *= 0.8f;
                }
                
                // Update player rotation to face mouse
                Vector2 mousePos = GetMousePosition();
                Vector2 playerScreenPos = (Vector2){
                    localPlayer->position.x, 
                    localPlayer->position.y
                };
                
                localPlayer->targetRotation = atan2f(mousePos.y - playerScreenPos.y, 
                                                    mousePos.x - playerScreenPos.x);
                
                // Weapon selection
                if (IsKeyPressed(KEY_ONE)) SwitchWeapon(localPlayer, WEAPON_PISTOL);
                if (IsKeyPressed(KEY_TWO)) SwitchWeapon(localPlayer, WEAPON_RIFLE);
                if (IsKeyPressed(KEY_THREE)) SwitchWeapon(localPlayer, WEAPON_SHOTGUN);
                if (IsKeyPressed(KEY_FOUR)) SwitchWeapon(localPlayer, WEAPON_SMG);
                if (IsKeyPressed(KEY_FIVE)) SwitchWeapon(localPlayer, WEAPON_SNIPER);
                
                // Mouse wheel weapon switching
                int wheel = GetMouseWheelMove();
                if (wheel != 0) {
                    int newWeapon = (int)localPlayer->currentWeapon + wheel;
                    
                    // Wrap around
                    if (newWeapon < 0) newWeapon = WEAPON_TOTAL - 1;
                    if (newWeapon >= WEAPON_TOTAL) newWeapon = 0;
                    
                    SwitchWeapon(localPlayer, (WeaponType)newWeapon);
                }
                
                // Reload weapon
                if (IsKeyPressed(KEY_R)) {
                    ReloadWeapon(localPlayer);
                }
                
                // Fire weapon
                WeaponStats* stats = GetCurrentWeaponStats(localPlayer);
                
                if (stats && stats->enabled) {
                    bool shouldFire = false;
                    
                    if (stats->automatic) {
                        shouldFire = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
                    } else {
                        shouldFire = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
                    }
                    
                    if (shouldFire && CanShoot(localPlayer)) {
                        FireWeapon(localPlayer);
                    }
                }
                
                // Exit to menu
                if (IsKeyPressed(KEY_ESCAPE)) {
                    game.state = GAME_MENU;
                    CloseNetwork();
                    
                    // Reset game
                    InitGame();
                }
            }
            break;
        }
    }
}

void DrawMenu(void)
{
    DrawText("LAYLA", SCREEN_WIDTH/2 - MeasureText("LAYLA", 60)/2, SCREEN_HEIGHT/4, 60, RAYWHITE);
    DrawText("A 2D Multiplayer Shooter", SCREEN_WIDTH/2 - MeasureText("A 2D Multiplayer Shooter", 20)/2, SCREEN_HEIGHT/4 + 70, 20, LIGHTGRAY);
    
    DrawText("1. Host Game (H)", SCREEN_WIDTH/2 - MeasureText("1. Host Game (H)", 30)/2, SCREEN_HEIGHT/2, 30, WHITE);
    DrawText("2. Join Game (J)", SCREEN_WIDTH/2 - MeasureText("2. Join Game (J)", 30)/2, SCREEN_HEIGHT/2 + 50, 30, WHITE);
    DrawText("3. Game Modes (M)", SCREEN_WIDTH/2 - MeasureText("3. Game Modes (M)", 30)/2, SCREEN_HEIGHT/2 + 100, 30, WHITE);
    DrawText("Q. Quit (ESC)", SCREEN_WIDTH/2 - MeasureText("Q. Quit (ESC)", 30)/2, SCREEN_HEIGHT/2 + 150, 30, WHITE);
    
    // Display current game mode
    char modeText[64];
    sprintf(modeText, "Current Mode: %s", GetGameModeName(game.mode));
    DrawText(modeText, SCREEN_WIDTH/2 - MeasureText(modeText, 20)/2, SCREEN_HEIGHT - 80, 20, GREEN);
    
    DrawText("Made with raylib", SCREEN_WIDTH/2 - MeasureText("Made with raylib", 20)/2, SCREEN_HEIGHT - 50, 20, GRAY);
}

void DrawNameInput(void)
{
    const char* action = game.wantsToHost ? "HOST GAME" : "JOIN GAME";
    DrawText(action, SCREEN_WIDTH/2 - MeasureText(action, 30)/2, SCREEN_HEIGHT/4 - 40, 30, LIGHTGRAY);
    
    DrawText("ENTER YOUR NAME", SCREEN_WIDTH/2 - MeasureText("ENTER YOUR NAME", 40)/2, SCREEN_HEIGHT/4, 40, WHITE);
    
    // Draw name input field
    Rectangle nameRect = (Rectangle){SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 20, 400, 40};
    DrawRectangleRec(nameRect, game.editingPlayerName ? DARKBLUE : DARKGRAY);
    DrawRectangleLinesEx(nameRect, 2, game.editingPlayerName ? BLUE : GRAY);
    
    // Draw name text
    char displayName[64];
    strcpy(displayName, game.playerNameInput);
    if (game.editingPlayerName && ((int)(GetTime() * 2) % 2)) {
        strcat(displayName, "_");  // Blinking cursor
    }
    DrawText(displayName, nameRect.x + 10, nameRect.y + 10, 20, WHITE);
    
    // Draw instructions
    DrawText("Type your name and press ENTER to continue", 
             SCREEN_WIDTH/2 - MeasureText("Type your name and press ENTER to continue", 20)/2, 
             SCREEN_HEIGHT/2 + 60, 20, LIGHTGRAY);
    
    DrawText("Press ESC to return to menu", SCREEN_WIDTH/2 - MeasureText("Press ESC to return to menu", 20)/2, SCREEN_HEIGHT - 50, 20, GRAY);
}

void DrawHostSetup(void)
{
    DrawText("HOST GAME", SCREEN_WIDTH/2 - MeasureText("HOST GAME", 40)/2, SCREEN_HEIGHT/4, 40, WHITE);
    
    DrawText("Port:", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 20, 20, LIGHTGRAY);
    
    // Draw port input field
    Rectangle portRect = (Rectangle){SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 20, 200, 40};
    DrawRectangleRec(portRect, game.editingHostPort ? DARKBLUE : DARKGRAY);
    DrawRectangleLinesEx(portRect, 2, game.editingHostPort ? BLUE : GRAY);
    DrawText(game.hostPortStr, portRect.x + 10, portRect.y + 10, 20, WHITE);
    
    // Draw host button
    Rectangle startRect = (Rectangle){SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50, 200, 40};
    DrawRectangleRec(startRect, DARKBLUE);
    DrawRectangleLinesEx(startRect, 2, BLUE);
    DrawText("START HOSTING", startRect.x + 20, startRect.y + 10, 20, WHITE);
    
    DrawText("Press ESC to return", SCREEN_WIDTH/2 - MeasureText("Press ESC to return", 20)/2, SCREEN_HEIGHT - 50, 20, GRAY);
    
    // Draw status message if set
    if (game.statusTimer > 0) {
        DrawText(game.statusMessage, 10, SCREEN_HEIGHT - 30, 20, RED);
    }
}

void DrawJoinSetup(void)
{
    DrawText("JOIN GAME", SCREEN_WIDTH/2 - MeasureText("JOIN GAME", 40)/2, SCREEN_HEIGHT/4, 40, WHITE);
    
    // Draw IP input field
    DrawText("Server IP:", SCREEN_WIDTH/2 - 230, SCREEN_HEIGHT/2 - 60, 20, LIGHTGRAY);
    Rectangle ipRect = (Rectangle){SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 60, 300, 40};
    DrawRectangleRec(ipRect, game.editingJoinIP ? DARKBLUE : DARKGRAY);
    DrawRectangleLinesEx(ipRect, 2, game.editingJoinIP ? BLUE : GRAY);
    DrawText(game.joinIPStr, ipRect.x + 10, ipRect.y + 10, 20, WHITE);
    
    // Draw port input field
    DrawText("Port:", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2, 20, LIGHTGRAY);
    Rectangle portRect = (Rectangle){SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2, 200, 40};
    DrawRectangleRec(portRect, game.editingJoinPort ? DARKBLUE : DARKGRAY);
    DrawRectangleLinesEx(portRect, 2, game.editingJoinPort ? BLUE : GRAY);
    DrawText(game.joinPortStr, portRect.x + 10, portRect.y + 10, 20, WHITE);
    
    // Draw connect button
    Rectangle connectRect = (Rectangle){SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50, 200, 40};
    DrawRectangleRec(connectRect, DARKBLUE);
    DrawRectangleLinesEx(connectRect, 2, BLUE);
    DrawText("CONNECT", connectRect.x + 50, connectRect.y + 10, 20, WHITE);
    
    DrawText("Press ESC to return", SCREEN_WIDTH/2 - MeasureText("Press ESC to return", 20)/2, SCREEN_HEIGHT - 50, 20, GRAY);
    
    // Draw status message if set
    if (game.statusTimer > 0) {
        DrawText(game.statusMessage, 10, SCREEN_HEIGHT - 30, 20, RED);
    }
}

void DrawUI(void)
{
    // Draw Player UI
    Player* localPlayer = FindPlayer(game.localPlayerId);
    
    if (localPlayer && localPlayer->active) {
        // Health bar
        int healthBarWidth = 200;
        int healthBarHeight = 20;
        float healthPercentage = localPlayer->health / localPlayer->maxHealth;
        
        DrawRectangle(20, 20, healthBarWidth, healthBarHeight, DARKGRAY);
        DrawRectangle(20, 20, (int)(healthBarWidth * healthPercentage), healthBarHeight, RED);
        DrawRectangleLinesEx((Rectangle){20, 20, healthBarWidth, healthBarHeight}, 2, BLACK);
        
        char healthText[32];
        sprintf(healthText, "Health: %.0f/%.0f", localPlayer->health, localPlayer->maxHealth);
        DrawText(healthText, 25, 20, 16, WHITE);
        
        // Team info for team modes
        if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
            const char* teamName = localPlayer->team == 0 ? "RED" : "BLUE";
            Color teamColor = localPlayer->team == 0 ? RED : BLUE;
            DrawText(teamName, 20, 45, 16, teamColor);
        }
        
        // Weapon info
        WeaponStats* stats = GetCurrentWeaponStats(localPlayer);
        if (stats && stats->enabled) {
            char weaponText[64];
            sprintf(weaponText, "%s: %d/%d", 
                    stats->name, 
                    localPlayer->magazineAmmo[localPlayer->currentWeapon],
                    localPlayer->ammo[localPlayer->currentWeapon]);
            DrawText(weaponText, 20, 50, 20, WHITE);
            
            // Visual ammo display (for guns with limited magazines)
            int ammoToShow = localPlayer->magazineAmmo[localPlayer->currentWeapon];
            if (ammoToShow > MAX_AMMO_DISPLAY) ammoToShow = MAX_AMMO_DISPLAY;
            
            for (int i = 0; i < ammoToShow; i++) {
                DrawRectangle(20 + i * 10, 75, 8, 20, YELLOW);
            }
        }
        
        // Draw reloading text
        if (localPlayer->isReloading) {
            DrawText("RELOADING", 20, 100, 20, YELLOW);
        }
    }
    
    // Network info & FPS counter in corner
    if (game.debugMode) {
        char debugText[128];
        sprintf(debugText, "FPS: %d", GetFPS());
        DrawText(debugText, SCREEN_WIDTH - MeasureText(debugText, 20) - 10, 10, 20, LIME);
        
        if (game.isConnected) {
            sprintf(debugText, "Ping: %.1f ms", game.ping);
            DrawText(debugText, SCREEN_WIDTH - MeasureText(debugText, 20) - 10, 35, 20, LIME);
            
            sprintf(debugText, "Players: %d", game.playerCount);
            DrawText(debugText, SCREEN_WIDTH - MeasureText(debugText, 20) - 10, 60, 20, LIME);
            
            sprintf(debugText, "Game Mode: %s", GetGameModeName(game.mode));
            DrawText(debugText, SCREEN_WIDTH - MeasureText(debugText, 20) - 10, 85, 20, LIME);
            
            // Show player name
            char nameText[64];
            sprintf(nameText, "Playing as: %s", game.playerName);
            DrawText(nameText, SCREEN_WIDTH - MeasureText(nameText, 16) - 10, SCREEN_HEIGHT - 25, 16, YELLOW);
            
            if (game.showAdvancedStats) {
                sprintf(debugText, "Packets Sent: %d", game.packetsSent);
                DrawText(debugText, SCREEN_WIDTH - MeasureText(debugText, 20) - 10, 85, 20, LIME);
                
                sprintf(debugText, "Packets Received: %d", game.packetsReceived);
                DrawText(debugText, SCREEN_WIDTH - MeasureText(debugText, 20) - 10, 110, 20, LIME);
                
                sprintf(debugText, "Bullets: %d", game.bulletCount);
                DrawText(debugText, SCREEN_WIDTH - MeasureText(debugText, 20) - 10, 135, 20, LIME);
                
                sprintf(debugText, "Particles: %d", game.particleCount);
                DrawText(debugText, SCREEN_WIDTH - MeasureText(debugText, 20) - 10, 160, 20, LIME);
            }
        }
    } else {
        // Just show FPS when not in debug mode
        char fpsText[32];
        sprintf(fpsText, "FPS: %d", GetFPS());
        DrawText(fpsText, SCREEN_WIDTH - MeasureText(fpsText, 20) - 10, 10, 20, WHITE);
    }
    
    // Draw status message if set
    if (game.statusTimer > 0) {
        DrawText(game.statusMessage, 10, SCREEN_HEIGHT - 30, 20, YELLOW);
    }
    
    // Draw game mode information
    if (game.state == GAME_PLAYING) {
        char modeText[64];
        sprintf(modeText, "Mode: %s", GetGameModeName(game.mode));
        DrawText(modeText, SCREEN_WIDTH/2 - MeasureText(modeText, 20)/2, 10, 20, WHITE);
        
        // Show mode timer if applicable
        if (game.modeMaxTime > 0) {
            int timeRemaining = (int)(game.modeMaxTime - game.modeTimer);
            int minutes = timeRemaining / 60;
            int seconds = timeRemaining % 60;
            char timeText[32];
            sprintf(timeText, "Time: %02d:%02d", minutes, seconds);
            DrawText(timeText, SCREEN_WIDTH/2 - MeasureText(timeText, 20)/2, 35, 20, WHITE);
        }
        
        // Show team scores for team modes
        if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
            char scoreText[64];
            sprintf(scoreText, "RED %d - %d BLUE", game.teamScores[0], game.teamScores[1]);
            DrawText(scoreText, SCREEN_WIDTH/2 - MeasureText(scoreText, 24)/2, 60, 24, WHITE);
        }
        
        // Show individual scoreboard for deathmatch
        if (game.mode == MODE_DEATHMATCH && IsKeyDown(KEY_TAB)) {
            // Draw scoreboard background
            DrawRectangle(SCREEN_WIDTH/2 - 200, 100, 400, 300, (Color){0, 0, 0, 180});
            DrawRectangleLinesEx((Rectangle){SCREEN_WIDTH/2 - 200, 100, 400, 300}, 2, WHITE);
            
            // Title
            const char* title = "SCOREBOARD";
            DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, 24)/2, 110, 24, WHITE);
            
            // Player scores
            int yOffset = 140;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (game.players[i].active) {
                    Player* p = &game.players[i];
                    char scoreText[64];
                    sprintf(scoreText, "%-20s %d", p->name, p->score);
                    Color textColor = p->isLocal ? YELLOW : WHITE;
                    DrawText(scoreText, SCREEN_WIDTH/2 - 180, yOffset, 16, textColor);
                    yOffset += 20;
                    if (yOffset > 380) break; // Don't overflow the scoreboard
                }
            }
            
            // Instructions
            const char* instruction = "Hold TAB to view scoreboard";
            DrawText(instruction, SCREEN_WIDTH/2 - MeasureText(instruction, 14)/2, 385, 14, LIGHTGRAY);
        }
        
        // Show mode instructions temporarily
        if (game.showModeInstructions) {
            const char* instructions = "";
            switch (game.mode) {
                case MODE_DEATHMATCH:
                    instructions = "DEATHMATCH: Eliminate other players to score points!";
                    break;
                case MODE_TEAM_DEATHMATCH:
                    instructions = "TEAM DEATHMATCH: Work with your team to eliminate opponents!";
                    break;
                case MODE_CAPTURE_FLAG:
                    instructions = "CAPTURE THE FLAG: Steal the enemy flag and return it to your base!";
                    break;
                default:
                    break;
            }
            
            DrawRectangle(0, SCREEN_HEIGHT/2 - 20, SCREEN_WIDTH, 40, (Color){0, 0, 0, 150});
            DrawText(instructions, SCREEN_WIDTH/2 - MeasureText(instructions, 20)/2, SCREEN_HEIGHT/2 - 10, 20, WHITE);
            
            // Hide instructions after 5 seconds
            static float instructionTimer = 5.0f;
            instructionTimer -= GetFrameTime();
            if (instructionTimer <= 0) {
                game.showModeInstructions = false;
                instructionTimer = 5.0f;
            }
        }
    }
}

void SetStatusMessage(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(game.statusMessage, sizeof(game.statusMessage), format, args);
    va_end(args);
    
    game.statusTimer = 3.0f;  // Display for 3 seconds
}

// Draw game mode selection menu
void DrawGameModeMenu(void)
{
    bool exitMenu = false;
    
    while (!exitMenu && !WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARKGRAY);
        
        DrawText("GAME MODES", SCREEN_WIDTH/2 - MeasureText("GAME MODES", 40)/2, 100, 40, WHITE);
        
        Rectangle dmRect = (Rectangle){SCREEN_WIDTH/2 - 200, 200, 400, 60};
        Rectangle tdmRect = (Rectangle){SCREEN_WIDTH/2 - 200, 280, 400, 60};
        Rectangle ctfRect = (Rectangle){SCREEN_WIDTH/2 - 200, 360, 400, 60};
        Rectangle backRect = (Rectangle){SCREEN_WIDTH/2 - 100, 440, 200, 40};
        
        Color dmColor = (game.mode == MODE_DEATHMATCH) ? GREEN : DARKBLUE;
        Color tdmColor = (game.mode == MODE_TEAM_DEATHMATCH) ? GREEN : DARKBLUE;
        Color ctfColor = (game.mode == MODE_CAPTURE_FLAG) ? GREEN : DARKBLUE;
        
        // Check for mouse hover
        if (CheckCollisionPointRec(GetMousePosition(), dmRect)) dmColor = BLUE;
        if (CheckCollisionPointRec(GetMousePosition(), tdmRect)) tdmColor = BLUE;
        if (CheckCollisionPointRec(GetMousePosition(), ctfRect)) ctfColor = BLUE;
        
        // Draw mode buttons
        DrawRectangleRec(dmRect, dmColor);
        DrawRectangleRec(tdmRect, tdmColor);
        DrawRectangleRec(ctfRect, ctfColor);
        DrawRectangleRec(backRect, MAROON);
        
        DrawText("Deathmatch", dmRect.x + 140, dmRect.y + 20, 20, WHITE);
        DrawText("Team Deathmatch", tdmRect.x + 110, tdmRect.y + 20, 20, WHITE);
        DrawText("Capture the Flag", ctfRect.x + 110, ctfRect.y + 20, 20, WHITE);
        DrawText("Back", backRect.x + 70, backRect.y + 10, 20, WHITE);
        
        // Draw mode descriptions
        const char* description = "";
        switch (game.mode) {
            case MODE_DEATHMATCH:
                description = "Every player for themselves! Score points by eliminating other players.";
                break;
            case MODE_TEAM_DEATHMATCH:
                description = "Red vs Blue! Work with your team to eliminate opponents.";
                break;
            case MODE_CAPTURE_FLAG:
                description = "Steal the enemy flag and return it to your base to score!";
                break;
            default:
                break;
        }
        
        DrawRectangle(SCREEN_WIDTH/2 - 300, 520, 600, 60, (Color){0, 0, 0, 150});
        DrawText(description, SCREEN_WIDTH/2 - MeasureText(description, 16)/2, 540, 16, WHITE);
        
        // Handle mouse click
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(GetMousePosition(), dmRect)) {
                SwitchGameMode(MODE_DEATHMATCH);
            }
            else if (CheckCollisionPointRec(GetMousePosition(), tdmRect)) {
                SwitchGameMode(MODE_TEAM_DEATHMATCH);
            }
            else if (CheckCollisionPointRec(GetMousePosition(), ctfRect)) {
                SwitchGameMode(MODE_CAPTURE_FLAG);
            }
            else if (CheckCollisionPointRec(GetMousePosition(), backRect)) {
                exitMenu = true;
            }
        }
        
        // Exit on ESC
        if (IsKeyPressed(KEY_ESCAPE)) {
            exitMenu = true;
        }
        
        EndDrawing();
    }
}

// Game mode functions
void InitGameMode(GameMode mode)
{
    // Reset scores
    game.teamScores[0] = 0;
    game.teamScores[1] = 0;
    
    // Reset timer
    game.modeTimer = 0;
    
    // Set maximum time based on mode
    switch (mode) {
        case MODE_DEATHMATCH:
            game.modeMaxTime = 300.0f; // 5 minutes
            break;
        case MODE_TEAM_DEATHMATCH:
            game.modeMaxTime = 300.0f; // 5 minutes
            break;
        case MODE_CAPTURE_FLAG:
            game.modeMaxTime = 600.0f; // 10 minutes
            
            // Reset flags
            game.flags[0].position = (Vector2){100, SCREEN_HEIGHT/2};
            game.flags[0].basePosition = (Vector2){100, SCREEN_HEIGHT/2};
            game.flags[0].isCaptured = false;
            game.flags[0].team = 0;
            strcpy(game.flags[0].carrierId, "");
            
            game.flags[1].position = (Vector2){SCREEN_WIDTH - 100, SCREEN_HEIGHT/2};
            game.flags[1].basePosition = (Vector2){SCREEN_WIDTH - 100, SCREEN_HEIGHT/2};
            game.flags[1].isCaptured = false;
            game.flags[1].team = 1;
            strcpy(game.flags[1].carrierId, "");
            break;
        default:
            break;
    }
    
    // Assign teams for team-based modes
    if (mode == MODE_TEAM_DEATHMATCH || mode == MODE_CAPTURE_FLAG) {
        int teamIndex = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (game.players[i].active) {
                game.players[i].team = teamIndex % 2;
                
                // Set team colors
                if (game.players[i].team == 0) {
                    game.players[i].color = (Color){220, 50, 50, 255}; // Red team
                } else {
                    game.players[i].color = (Color){50, 50, 220, 255}; // Blue team
                }
                
                teamIndex++;
            }
        }
    }
    
    // Show instructions for the new mode
    game.showModeInstructions = true;
}

void UpdateGameMode(void)
{
    float dt = GetFrameTime();
    
    // Update mode timer
    if (game.modeMaxTime > 0) {
        game.modeTimer += dt;
        if (game.modeTimer >= game.modeMaxTime) {
            // Game over - determine winner
            if (game.mode == MODE_DEATHMATCH) {
                // Find player with highest score
                int highestScore = -1;
                char winnerId[32] = "";
                char winnerName[32] = "";
                
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (game.players[i].active && game.players[i].score > highestScore) {
                        highestScore = game.players[i].score;
                        strcpy(winnerId, game.players[i].id);
                        strcpy(winnerName, game.players[i].name);
                    }
                }
                
                if (highestScore > 0) {
                    SetStatusMessage("Game Over! %s wins with %d points!", winnerName, highestScore);
                } else {
                    SetStatusMessage("Game Over! No winner - tied game.");
                }
            } else if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
                // Determine winning team
                if (game.teamScores[0] > game.teamScores[1]) {
                    SetStatusMessage("Game Over! RED TEAM wins with %d points!", game.teamScores[0]);
                } else if (game.teamScores[1] > game.teamScores[0]) {
                    SetStatusMessage("Game Over! BLUE TEAM wins with %d points!", game.teamScores[1]);
                } else {
                    SetStatusMessage("Game Over! TIE GAME - both teams scored %d points!", game.teamScores[0]);
                }
            }
            
            // Reset the game mode
            ResetGameMode();
        }
    }
    
    // Mode-specific logic
    switch (game.mode) {
        case MODE_CAPTURE_FLAG:
            // Update flag positions
            for (int flagIdx = 0; flagIdx < 2; flagIdx++) {
                Flag* flag = &game.flags[flagIdx];
                
                if (flag->isCaptured) {
                    // Update flag position to follow carrier
                    Player* carrier = FindPlayer(flag->carrierId);
                    if (carrier && carrier->active) {
                        flag->position = carrier->position;
                        
                        // Check if carrier reached their base (opponent's flag at carrier's base)
                        if (carrier->team != flag->team) {
                            float distToBase = Vector2Distance(carrier->position, game.flags[carrier->team].basePosition);
                            if (distToBase < 50) {
                                // Score a point for carrier's team
                                game.teamScores[carrier->team]++;
                                
                                // Reset the flag
                                flag->position = flag->basePosition;
                                flag->isCaptured = false;
                                strcpy(flag->carrierId, "");
                                
                                SetStatusMessage("%s team scored a point by capturing the flag!", 
                                                carrier->team == 0 ? "RED" : "BLUE");
                            }
                        }
                    } else {
                        // Carrier disappeared, reset the flag
                        flag->position = flag->basePosition;
                        flag->isCaptured = false;
                        strcpy(flag->carrierId, "");
                    }
                } else {
                    // Check if any player picks up the flag
                    for (int i = 0; i < MAX_PLAYERS; i++) {
                        if (game.players[i].active && game.players[i].team != flag->team) {
                            float dist = Vector2Distance(game.players[i].position, flag->position);
                            if (dist < PLAYER_SIZE) {
                                // Player picks up flag
                                flag->isCaptured = true;
                                strcpy(flag->carrierId, game.players[i].id);
                                
                                SetStatusMessage("%s picked up the %s flag!", 
                                                game.players[i].id,
                                                flag->team == 0 ? "RED" : "BLUE");
                                break;
                            }
                        }
                    }
                }
            }
            break;
            
        default:
            break;
    }
}

void DrawGameMode(void)
{
    switch (game.mode) {
        case MODE_CAPTURE_FLAG:
            // Draw flags
            for (int i = 0; i < 2; i++) {
                Color flagColor = i == 0 ? RED : BLUE;
                Color baseColor = i == 0 ? (Color){255, 200, 200, 100} : (Color){200, 200, 255, 100};
                
                // Draw base
                DrawCircle(game.flags[i].basePosition.x, game.flags[i].basePosition.y, 50, baseColor);
                DrawCircleLines(game.flags[i].basePosition.x, game.flags[i].basePosition.y, 50, flagColor);
                
                // Draw flag
                if (!game.flags[i].isCaptured) {
                    // Draw flag pole
                    DrawRectangle(game.flags[i].position.x - 2, game.flags[i].position.y - 20, 4, 40, GRAY);
                    
                    // Draw flag
                    Vector2 flagPoints[3] = {
                        {game.flags[i].position.x, game.flags[i].position.y - 20},
                        {game.flags[i].position.x + 20, game.flags[i].position.y - 10},
                        {game.flags[i].position.x, game.flags[i].position.y}
                    };
                    DrawTriangle(flagPoints[0], flagPoints[1], flagPoints[2], flagColor);
                }
            }
            break;
            
        default:
            break;
    }
}

const char* GetGameModeName(GameMode mode)
{
    switch (mode) {
        case MODE_DEATHMATCH:
            return "Deathmatch";
        case MODE_TEAM_DEATHMATCH:
            return "Team Deathmatch";
        case MODE_CAPTURE_FLAG:
            return "Capture the Flag";
        default:
            return "Unknown Mode";
    }
}

void SwitchGameMode(GameMode mode)
{
    if (mode >= MODE_DEATHMATCH && mode < MODE_TOTAL) {
        game.mode = mode;
        InitGameMode(mode);
        SetStatusMessage("Game mode changed to %s", GetGameModeName(mode));
    }
}

void ResetGameMode(void)
{
    // Reset the current game mode
    InitGameMode(game.mode);
}