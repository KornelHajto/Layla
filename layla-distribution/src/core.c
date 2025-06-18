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
    
    // Initialize chat system
    game.editingChat = false;
    strcpy(game.chatInput, "");
    game.chatMessageCount = 0;
    for (int i = 0; i < 10; i++) {
        strcpy(game.chatMessages[i].message, "");
        strcpy(game.chatMessages[i].senderName, "");
        game.chatMessages[i].displayTime = 0;
    }
    
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
            // Draw enhanced background
            DrawGameBackground();
            
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

void DrawGameBackground(void)
{
    // Enhanced background with grid pattern
    Color bgColor = (Color){25, 30, 35, 255};
    ClearBackground(bgColor);
    
    // Draw grid pattern for better visual reference
    Color gridColor = (Color){40, 45, 50, 255};
    int gridSize = 50;
    
    // Vertical lines
    for (int x = 0; x < SCREEN_WIDTH; x += gridSize) {
        DrawLineEx((Vector2){x, 0}, (Vector2){x, SCREEN_HEIGHT}, 1, gridColor);
    }
    
    // Horizontal lines
    for (int y = 0; y < SCREEN_HEIGHT; y += gridSize) {
        DrawLineEx((Vector2){0, y}, (Vector2){SCREEN_WIDTH, y}, 1, gridColor);
    }
    
    // Draw border walls
    Color wallColor = (Color){60, 70, 80, 255};
    Color wallHighlight = (Color){80, 90, 100, 255};
    
    // Top wall
    DrawRectangle(0, 0, SCREEN_WIDTH, 20, wallColor);
    DrawRectangle(0, 18, SCREEN_WIDTH, 2, wallHighlight);
    
    // Bottom wall
    DrawRectangle(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, wallColor);
    DrawRectangle(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 2, wallHighlight);
    
    // Left wall
    DrawRectangle(0, 0, 20, SCREEN_HEIGHT, wallColor);
    DrawRectangle(18, 0, 2, SCREEN_HEIGHT, wallHighlight);
    
    // Right wall
    DrawRectangle(SCREEN_WIDTH - 20, 0, 20, SCREEN_HEIGHT, wallColor);
    DrawRectangle(SCREEN_WIDTH - 20, 0, 2, SCREEN_HEIGHT, wallHighlight);
    
    // Add some decorative elements
    if (game.mode == MODE_CAPTURE_FLAG) {
        // Draw flag bases
        Color redBaseColor = (Color){100, 30, 30, 150};
        Color blueBaseColor = (Color){30, 30, 100, 150};
        
        // Red base (left)
        DrawRectangle(30, SCREEN_HEIGHT/2 - 60, 80, 120, redBaseColor);
        DrawRectangleLines(30, SCREEN_HEIGHT/2 - 60, 80, 120, RED);
        DrawText("RED BASE", 35, SCREEN_HEIGHT/2 - 5, 12, WHITE);
        
        // Blue base (right)
        DrawRectangle(SCREEN_WIDTH - 110, SCREEN_HEIGHT/2 - 60, 80, 120, blueBaseColor);
        DrawRectangleLines(SCREEN_WIDTH - 110, SCREEN_HEIGHT/2 - 60, 80, 120, BLUE);
        DrawText("BLUE BASE", SCREEN_WIDTH - 105, SCREEN_HEIGHT/2 - 5, 12, WHITE);
    }
    
    // Add some ambient lighting effects
    Vector2 lightPos = {SCREEN_WIDTH/2, SCREEN_HEIGHT/4};
    Color lightColor = (Color){255, 255, 200, 20};
    DrawCircleGradient(lightPos.x, lightPos.y, 200, lightColor, (Color){0, 0, 0, 0});
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
            // Chat system
            if (IsKeyPressed(KEY_ENTER) && !game.editingChat) {
                game.editingChat = true;
                strcpy(game.chatInput, "");
            } else if (IsKeyPressed(KEY_ENTER) && game.editingChat) {
                // Send chat message
                if (strlen(game.chatInput) > 0) {
                    // Add to local chat
                    AddChatMessage(game.chatInput, game.playerName);
                    
                    // Send over network if connected
                    if (game.isConnected) {
                        NetworkMessage chatMsg;
                        chatMsg.type = MSG_CHAT;
                        strcpy(chatMsg.playerId, game.localPlayerId);
                        strcpy(chatMsg.data.chatMessage, game.chatInput);
                        strcpy(chatMsg.data.senderName, game.playerName);
                        
                        if (game.isHost) {
                            // Host sends to all clients
                            for (int i = 0; i < game.clientCount; i++) {
                                SendMessage(&chatMsg, &game.clientAddrs[i]);
                            }
                        } else {
                            // Client sends to host
                            SendMessage(&chatMsg, &game.serverAddr);
                        }
                    }
                }
                game.editingChat = false;
                strcpy(game.chatInput, "");
            } else if (IsKeyPressed(KEY_ESCAPE) && game.editingChat) {
                game.editingChat = false;
                strcpy(game.chatInput, "");
            }
            
            // Handle chat input
            if (game.editingChat) {
                int key = GetKeyPressed();
                if (key >= KEY_A && key <= KEY_Z) {
                    int letter = key - KEY_A;
                    int len = strlen(game.chatInput);
                    if (len < 255) {
                        char c = 'A' + letter;
                        if (!IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
                            c = 'a' + letter;
                        }
                        game.chatInput[len] = c;
                        game.chatInput[len+1] = '\0';
                    }
                } else if (key >= KEY_ZERO && key <= KEY_NINE) {
                    int len = strlen(game.chatInput);
                    if (len < 255) {
                        game.chatInput[len] = '0' + (key - KEY_ZERO);
                        game.chatInput[len+1] = '\0';
                    }
                } else if (key == KEY_SPACE) {
                    int len = strlen(game.chatInput);
                    if (len < 255) {
                        game.chatInput[len] = ' ';
                        game.chatInput[len+1] = '\0';
                    }
                }
                
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    int len = strlen(game.chatInput);
                    if (len > 0) {
                        game.chatInput[len-1] = '\0';
                    }
                }
                
                // Don't process game input while chatting
                return;
            }
            
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
    // Modern gradient background
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){10, 15, 25, 255}, (Color){25, 35, 50, 255});
    
    // Animated title with glow effect
    float titleGlow = 0.8f + 0.2f * sinf(GetTime() * 2.0f);
    const char* title = "LAYLA";
    int titleSize = 80;
    int titleWidth = MeasureText(title, titleSize);
    
    // Title shadow layers for depth
    for (int i = 0; i < 3; i++) {
        DrawText(title, SCREEN_WIDTH/2 - titleWidth/2 + i*2, SCREEN_HEIGHT/4 + i*2, titleSize, 
                (Color){0, 0, 0, 60 - i*20});
    }
    
    // Main title with animated glow
    DrawText(title, SCREEN_WIDTH/2 - titleWidth/2, SCREEN_HEIGHT/4, titleSize, 
            (Color){255, 255, 255, (int)(255 * titleGlow)});
    
    // Subtitle with typewriter effect
    const char* subtitle = "Advanced 2D Multiplayer Combat Arena";
    static float subtitleTimer = 0;
    subtitleTimer += GetFrameTime();
    int visibleChars = (int)(subtitleTimer * 30) % (int)(strlen(subtitle) + 20);
    if (visibleChars > (int)strlen(subtitle)) visibleChars = (int)strlen(subtitle);
    
    char visibleSubtitle[100];
    strncpy(visibleSubtitle, subtitle, visibleChars);
    visibleSubtitle[visibleChars] = '\0';
    
    DrawText(visibleSubtitle, SCREEN_WIDTH/2 - MeasureText(subtitle, 24)/2, SCREEN_HEIGHT/4 + 90, 24, 
            (Color){180, 200, 255, 200});
    
    // Modern menu buttons with hover effects
    Rectangle buttons[4];
    const char* buttonTexts[] = {"🚀 Host Game", "🌐 Join Game", "⚙️ Game Modes", "❌ Quit"};
    const char* buttonKeys[] = {"(H)", "(J)", "(M)", "(Q)"};
    
    for (int i = 0; i < 4; i++) {
        buttons[i] = (Rectangle){SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 + i*70, 400, 50};
        
        // Button animation based on time and index
        float buttonPulse = 0.9f + 0.1f * sinf(GetTime() * 2.0f + i * 0.5f);
        
        // Button shadow
        DrawRectangleRounded((Rectangle){buttons[i].x + 3, buttons[i].y + 3, buttons[i].width, buttons[i].height}, 
                           0.2f, 8, (Color){0, 0, 0, 80});
        
        // Button background with gradient
        Color bgColor = (Color){40, 50, 70, (int)(200 * buttonPulse)};
        Color borderColor = (Color){80, 120, 180, (int)(255 * buttonPulse)};
        
        DrawRectangleRounded(buttons[i], 0.2f, 8, bgColor);
        DrawRectangleRoundedLines(buttons[i], 0.2f, 8, borderColor);
        
        // Button text
        int keyWidth = MeasureText(buttonKeys[i], 18);
        
        DrawText(buttonTexts[i], buttons[i].x + 30, buttons[i].y + 13, 24, WHITE);
        DrawText(buttonKeys[i], buttons[i].x + buttons[i].width - keyWidth - 30, buttons[i].y + 16, 18, 
                (Color){150, 170, 200, 200});
    }
    
    // Current game mode display with modern styling
    char modeText[64];
    sprintf(modeText, "Current Mode: %s", GetGameModeName(game.mode));
    Rectangle modeBg = {SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT - 120, 300, 35};
    
    DrawRectangleRounded(modeBg, 0.3f, 8, (Color){20, 30, 45, 180});
    DrawRectangleRoundedLines(modeBg, 0.3f, 8, (Color){100, 150, 200, 150});
    
    DrawText(modeText, SCREEN_WIDTH/2 - MeasureText(modeText, 18)/2, SCREEN_HEIGHT - 110, 18, 
            (Color){100, 255, 150, 255});
    
    // Credits with fade animation
    float creditAlpha = 0.6f + 0.2f * sinf(GetTime() * 1.5f);
    const char* credits = "Built with ❤️ using Raylib • Enhanced by AI";
    DrawText(credits, SCREEN_WIDTH/2 - MeasureText(credits, 16)/2, SCREEN_HEIGHT - 40, 16, 
            (Color){120, 140, 160, (int)(255 * creditAlpha)});
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
        
        // Enhanced weapon info display
        WeaponStats* stats = GetCurrentWeaponStats(localPlayer);
        if (stats && stats->enabled) {
            // Weapon panel background
            Rectangle weaponPanel = {15, 45, 300, 70};
            DrawRectangleRounded(weaponPanel, 0.2f, 8, (Color){20, 25, 30, 200});
            DrawRectangleRoundedLines(weaponPanel, 0.2f, 8, (Color){60, 80, 120, 255});
            
            // Weapon name with icon
            const char* weaponIcon = "🔫";
            switch (localPlayer->currentWeapon) {
                case WEAPON_PISTOL: weaponIcon = "🔫"; break;
                case WEAPON_RIFLE: weaponIcon = "🏹"; break;
                case WEAPON_SHOTGUN: weaponIcon = "💥"; break;
                case WEAPON_SMG: weaponIcon = "⚡"; break;
                case WEAPON_SNIPER: weaponIcon = "🎯"; break;
                default: break;
            }
            
            char weaponTitle[64];
            sprintf(weaponTitle, "%s %s", weaponIcon, stats->name);
            DrawText(weaponTitle, 25, 55, 18, WHITE);
            
            // Ammo count with modern styling
            char ammoText[32];
            sprintf(ammoText, "%d / %d", 
                    localPlayer->magazineAmmo[localPlayer->currentWeapon],
                    localPlayer->ammo[localPlayer->currentWeapon]);
            DrawText(ammoText, 200, 55, 16, LIGHTGRAY);
            
            // Enhanced visual ammo display
            int maxAmmo = stats->magazineSize;
            int currentAmmo = localPlayer->magazineAmmo[localPlayer->currentWeapon];
            float ammoPercentage = maxAmmo > 0 ? (float)currentAmmo / maxAmmo : 0;
            
            // Ammo bar background
            Rectangle ammoBar = {25, 80, 200, 8};
            DrawRectangleRounded(ammoBar, 0.5f, 8, (Color){40, 40, 40, 200});
            
            // Ammo bar fill
            Color ammoColor = ammoPercentage > 0.5f ? GREEN : 
                             ammoPercentage > 0.2f ? YELLOW : RED;
            Rectangle ammoFill = {25, 80, 200 * ammoPercentage, 8};
            DrawRectangleRounded(ammoFill, 0.5f, 8, ammoColor);
            
            // Individual bullet indicators for clarity
            int bulletsToShow = currentAmmo > 20 ? 20 : currentAmmo;
            for (int i = 0; i < bulletsToShow; i++) {
                int bulletX = 25 + i * 9;
                DrawRectangleRounded((Rectangle){bulletX, 95, 6, 12}, 0.3f, 4, YELLOW);
                DrawRectangleRoundedLines((Rectangle){bulletX, 95, 6, 12}, 0.3f, 4, GOLD);
            }
            
            // Show "..." if more bullets than displayed
            if (currentAmmo > 20) {
                DrawText("...", 25 + 20 * 9, 98, 12, YELLOW);
            }
        }
        
        // Enhanced reloading indicator
        if (localPlayer->isReloading) {
            Rectangle reloadPanel = {15, 120, 150, 30};
            float reloadProgress = 1.0f - (localPlayer->reloadTimer / GetCurrentWeaponStats(localPlayer)->reloadTime);
            
            // Background
            DrawRectangleRounded(reloadPanel, 0.3f, 8, (Color){60, 60, 20, 200});
            
            // Progress bar
            Rectangle progressBar = {20, 125, 140 * reloadProgress, 20};
            DrawRectangleRounded(progressBar, 0.3f, 8, ORANGE);
            
            // Text
            DrawText("RELOADING...", 25, 130, 12, WHITE);
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
        
        // Enhanced Production-Level Scoreboard for Deathmatch
        if (game.mode == MODE_DEATHMATCH && IsKeyDown(KEY_TAB)) {
            // Animated background with blur effect
            float boardWidth = 600;
            float boardHeight = 400;
            Rectangle boardBg = {SCREEN_WIDTH/2 - boardWidth/2, 80, boardWidth, boardHeight};
            
            // Multiple shadow layers for depth
            for (int i = 0; i < 5; i++) {
                Rectangle shadow = {boardBg.x + i*2, boardBg.y + i*2, boardBg.width, boardBg.height};
                DrawRectangleRounded(shadow, 0.1f, 12, (Color){0, 0, 0, 15});
            }
            
            // Main background with gradient effect
            DrawRectangleRounded(boardBg, 0.08f, 12, (Color){15, 20, 30, 240});
            
            // Animated border with glow
            float glowPulse = 0.7f + 0.3f * sinf(GetTime() * 2.0f);
            DrawRectangleRoundedLines(boardBg, 0.08f, 12, (Color){70, 130, 200, (int)(180 * glowPulse)});
            DrawRectangleRoundedLines(boardBg, 0.08f, 12, (Color){120, 180, 255, (int)(120 * glowPulse)});
            
            // Modern header with icon and styling
            float headerY = boardBg.y + 20;
            const char* title = "🏆 LEADERBOARD";
            int titleWidth = MeasureText(title, 28);
            DrawText(title, SCREEN_WIDTH/2 - titleWidth/2, headerY, 28, (Color){255, 215, 0, 255});
            
            // Underline with gradient
            Rectangle underline = {SCREEN_WIDTH/2 - titleWidth/2, headerY + 35, titleWidth, 3};
            DrawRectangleGradientH(underline.x, underline.y, underline.width, underline.height, 
                                 (Color){255, 215, 0, 100}, (Color){255, 215, 0, 255});
            DrawRectangleGradientH(underline.x, underline.y, underline.width/2, underline.height, 
                                 (Color){255, 215, 0, 255}, (Color){255, 215, 0, 100});
            
            // Enhanced column headers with icons
            float headerRowY = headerY + 55;
            DrawText("🎖️", boardBg.x + 30, headerRowY, 18, (Color){255, 215, 0, 255});
            DrawText("PLAYER", boardBg.x + 60, headerRowY, 18, (Color){200, 220, 255, 255});
            DrawText("SCORE", boardBg.x + 280, headerRowY, 18, (Color){200, 220, 255, 255});
            DrawText("K/D", boardBg.x + 360, headerRowY, 18, (Color){200, 220, 255, 255});
            DrawText("RATIO", boardBg.x + 440, headerRowY, 18, (Color){200, 220, 255, 255});
            DrawText("STATUS", boardBg.x + 520, headerRowY, 18, (Color){200, 220, 255, 255});
            
            // Header separator line
            Rectangle separator = {boardBg.x + 20, headerRowY + 25, boardBg.width - 40, 2};
            DrawRectangleGradientH(separator.x, separator.y, separator.width, separator.height, 
                                 (Color){70, 130, 200, 50}, (Color){70, 130, 200, 200});
            
            // Sort players by score (descending)
            Player sortedPlayers[MAX_PLAYERS];
            int activePlayers = 0;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (game.players[i].active) {
                    sortedPlayers[activePlayers] = game.players[i];
                    activePlayers++;
                }
            }
            
            // Simple bubble sort by score
            for (int i = 0; i < activePlayers - 1; i++) {
                for (int j = 0; j < activePlayers - i - 1; j++) {
                    if (sortedPlayers[j].score < sortedPlayers[j + 1].score) {
                        Player temp = sortedPlayers[j];
                        sortedPlayers[j] = sortedPlayers[j + 1];
                        sortedPlayers[j + 1] = temp;
                    }
                }
            }
            
            // Player entries with ranking and animations
            float entryY = headerRowY + 40;
            for (int i = 0; i < activePlayers && i < 8; i++) {
                Player* p = &sortedPlayers[i];
                
                // Alternating row backgrounds
                Rectangle rowBg = {boardBg.x + 15, entryY - 5, boardBg.width - 30, 30};
                Color rowColor = (i % 2 == 0) ? (Color){25, 30, 40, 100} : (Color){20, 25, 35, 100};
                DrawRectangleRounded(rowBg, 0.2f, 6, rowColor);
                
                // Highlight local player with special effects
                if (p->isLocal) {
                    float highlightPulse = 0.6f + 0.4f * sinf(GetTime() * 4.0f);
                    DrawRectangleRounded(rowBg, 0.2f, 6, (Color){255, 215, 0, (int)(30 * highlightPulse)});
                    DrawRectangleRoundedLines(rowBg, 0.2f, 6, (Color){255, 215, 0, (int)(150 * highlightPulse)});
                }
                
                // Rank with special styling for top 3
                char rankText[8];
                sprintf(rankText, "#%d", i + 1);
                Color rankColor = WHITE;
                if (i == 0) rankColor = (Color){255, 215, 0, 255};      // Gold
                else if (i == 1) rankColor = (Color){192, 192, 192, 255}; // Silver  
                else if (i == 2) rankColor = (Color){205, 127, 50, 255};  // Bronze
                
                DrawText(rankText, boardBg.x + 30, entryY, 16, rankColor);
                
                // Player name with special formatting for local player
                Color nameColor = p->isLocal ? (Color){255, 255, 100, 255} : WHITE;
                if (p->isLocal) {
                    DrawText("👤", boardBg.x + 60, entryY, 16, (Color){255, 215, 0, 255});
                    DrawText(p->name, boardBg.x + 85, entryY, 16, nameColor);
                } else {
                    DrawText(p->name, boardBg.x + 60, entryY, 16, nameColor);
                }
                
                // Score with highlighting for high scores
                char scoreText[16];
                sprintf(scoreText, "%d", p->score);
                Color scoreColor = p->score >= 10 ? (Color){100, 255, 100, 255} : WHITE;
                DrawText(scoreText, boardBg.x + 280, entryY, 16, scoreColor);
                
                // K/D with color coding
                char kdText[16];
                sprintf(kdText, "%d/%d", p->kills, p->deaths);
                Color kdColor = p->kills > p->deaths ? (Color){100, 255, 100, 255} : 
                               p->kills < p->deaths ? (Color){255, 150, 150, 255} : WHITE;
                DrawText(kdText, boardBg.x + 360, entryY, 16, kdColor);
                
                // K/D Ratio with performance indicators
                char ratioText[16];
                float ratio = p->deaths > 0 ? (float)p->kills / p->deaths : (float)p->kills;
                sprintf(ratioText, "%.2f", ratio);
                Color ratioColor = ratio >= 2.0f ? (Color){0, 255, 0, 255} :
                                  ratio >= 1.0f ? (Color){255, 255, 0, 255} :
                                  (Color){255, 100, 100, 255};
                DrawText(ratioText, boardBg.x + 440, entryY, 16, ratioColor);
                
                // Status indicator
                const char* status = p->health > 75 ? "🟢" : p->health > 25 ? "🟡" : "🔴";
                DrawText(status, boardBg.x + 520, entryY, 16, WHITE);
                
                entryY += 35;
            }
            
            // Enhanced footer with instructions and stats
            Rectangle footerBg = {boardBg.x + 20, boardBg.y + boardBg.height - 60, boardBg.width - 40, 40};
            DrawRectangleRounded(footerBg, 0.3f, 8, (Color){30, 40, 50, 150});
            
            const char* instruction = "Hold TAB to view • ESC for menu";
            int instrWidth = MeasureText(instruction, 14);
            DrawText(instruction, SCREEN_WIDTH/2 - instrWidth/2, footerBg.y + 12, 14, (Color){180, 200, 220, 255});
            
            // Live stats
            char statsText[64];
            sprintf(statsText, "Players Online: %d", activePlayers);
            DrawText(statsText, footerBg.x + 10, footerBg.y + 25, 12, (Color){150, 170, 190, 255});
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
    
    // === PRODUCTION-LEVEL CHAT SYSTEM ===
    if (game.state == GAME_PLAYING) {
        // Update chat message timers and fade effects
        for (int i = 0; i < game.chatMessageCount; i++) {
            if (game.chatMessages[i].displayTime > 0) {
                game.chatMessages[i].displayTime -= GetFrameTime();
            }
        }
        
        // Modern Chat Messages Display with animations
        int chatY = 15;
        for (int i = 0; i < game.chatMessageCount && i < 5; i++) {
            if (game.chatMessages[i].displayTime > 0) {
                // Calculate fade and slide animations
                float fadeAlpha = game.chatMessages[i].displayTime < 2.0f ? 
                                 (game.chatMessages[i].displayTime / 2.0f) : 1.0f;
                float slideOffset = game.chatMessages[i].displayTime > 8.0f ? 
                                   (10.0f - game.chatMessages[i].displayTime) * 20 : 0;
                
                char chatLine[320];
                sprintf(chatLine, "%s: %s", game.chatMessages[i].senderName, game.chatMessages[i].message);
                
                int textWidth = MeasureText(chatLine, 16);
                int nameWidth = MeasureText(game.chatMessages[i].senderName, 16);
                
                // Modern chat bubble with rounded corners and shadow
                Rectangle chatBg = {15 - slideOffset, chatY - 3, textWidth + 25, 24};
                Rectangle shadow = {chatBg.x + 2, chatBg.y + 2, chatBg.width, chatBg.height};
                
                // Drop shadow
                DrawRectangleRounded(shadow, 0.4f, 8, (Color){0, 0, 0, (int)(40 * fadeAlpha)});
                
                // Main background with gradient effect
                DrawRectangleRounded(chatBg, 0.4f, 8, (Color){25, 25, 35, (int)(180 * fadeAlpha)});
                DrawRectangleRoundedLines(chatBg, 0.4f, 8, (Color){70, 130, 180, (int)(120 * fadeAlpha)});
                
                // Player name with unique color based on name hash
                unsigned int nameHash = 0;
                for (int j = 0; game.chatMessages[i].senderName[j]; j++) {
                    nameHash = nameHash * 31 + game.chatMessages[i].senderName[j];
                }
                Color nameColor = {
                    (unsigned char)(100 + (nameHash % 155)),
                    (unsigned char)(150 + (nameHash * 7 % 105)),
                    (unsigned char)(200 + (nameHash * 13 % 55)),
                    (unsigned char)(255 * fadeAlpha)
                };
                
                // Draw text with proper spacing
                DrawText(game.chatMessages[i].senderName, chatBg.x + 10, chatY, 16, nameColor);
                DrawText(": ", chatBg.x + 10 + nameWidth, chatY, 16, (Color){180, 180, 180, (int)(255 * fadeAlpha)});
                DrawText(game.chatMessages[i].message, chatBg.x + 10 + nameWidth + 12, chatY, 16, 
                        (Color){255, 255, 255, (int)(255 * fadeAlpha)});
                
                chatY += 28;
            }
        }
        
        // Enhanced Chat Input Box
        if (game.editingChat) {
            Rectangle chatInputBg = {15, SCREEN_HEIGHT - 65, SCREEN_WIDTH - 30, 45};
            Rectangle inputField = {chatInputBg.x + 5, chatInputBg.y + 5, chatInputBg.width - 10, 35};
            
            // Animated background with pulsing glow
            float glowIntensity = 0.5f + 0.3f * sinf(GetTime() * 3.0f);
            
            // Shadow
            DrawRectangleRounded((Rectangle){chatInputBg.x + 3, chatInputBg.y + 3, chatInputBg.width, chatInputBg.height}, 
                               0.3f, 10, (Color){0, 0, 0, 60});
            
            // Main background
            DrawRectangleRounded(chatInputBg, 0.3f, 10, (Color){30, 35, 45, 220});
            
            // Animated border with glow
            DrawRectangleRoundedLines(chatInputBg, 0.3f, 10, 
                                    (Color){100, 150, 255, (int)(150 + 50 * glowIntensity)});
            
            // Inner input field
            DrawRectangleRounded(inputField, 0.2f, 8, (Color){20, 25, 35, 200});
            DrawRectangleRoundedLines(inputField, 0.2f, 8, (Color){60, 60, 80, 255});
            
            // Chat prompt with icon
            DrawText("💬", chatInputBg.x + 15, chatInputBg.y + 10, 20, (Color){150, 200, 255, 255});
            DrawText("Say:", chatInputBg.x + 45, chatInputBg.y + 12, 16, (Color){150, 170, 190, 255});
            
            // Input text with animated cursor
            char displayInput[300];
            strcpy(displayInput, game.chatInput);
            if ((int)(GetTime() * 2.5f) % 2) {
                strcat(displayInput, "│");  // Modern cursor
            }
            DrawText(displayInput, inputField.x + 10, inputField.y + 8, 16, WHITE);
            
            // Modern instruction text
            const char* instruction = "↵ ENTER to send • ESC to cancel";
            DrawText(instruction, chatInputBg.x + 15, chatInputBg.y + 30, 12, (Color){120, 140, 160, 200});
        } else {
            // Subtle chat hint with fade animation
            float hintAlpha = 0.3f + 0.2f * sinf(GetTime() * 1.5f);
            DrawText("💬 Press ENTER to chat", 15, SCREEN_HEIGHT - 25, 14, 
                    (Color){100, 120, 140, (int)(255 * hintAlpha)});
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

void AddChatMessage(const char* message, const char* senderName)
{
    // Shift existing messages up
    for (int i = 9; i > 0; i--) {
        strcpy(game.chatMessages[i].message, game.chatMessages[i-1].message);
        strcpy(game.chatMessages[i].senderName, game.chatMessages[i-1].senderName);
        game.chatMessages[i].displayTime = game.chatMessages[i-1].displayTime;
    }
    
    // Add new message at top
    strcpy(game.chatMessages[0].message, message);
    strcpy(game.chatMessages[0].senderName, senderName);
    game.chatMessages[0].displayTime = 10.0f; // Display for 10 seconds
    
    if (game.chatMessageCount < 10) {
        game.chatMessageCount++;
    }
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