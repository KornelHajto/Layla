#include "../include/common.h"
#include "../include/player.h"
#include "../include/weapons.h"
#include "../include/particles.h"
#include "../include/core.h"

Player* FindPlayer(const char* playerId)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && strcmp(game.players[i].id, playerId) == 0) {
            return &game.players[i];
        }
    }
    return NULL;
}

Player* CreatePlayer(const char* playerId, const char* playerName, bool isLocal)
{
    // First check if player already exists
    Player* existingPlayer = FindPlayer(playerId);
    if (existingPlayer) {
        return existingPlayer;
    }
    
    // Find an empty slot
    int slot = -1;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!game.players[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return NULL; // No empty slots
    }
    
    // Initialize the player
    Player* player = &game.players[slot];
    strcpy(player->id, playerId);
    strcpy(player->name, playerName ? playerName : "Unknown");
    player->position = (Vector2){ SCREEN_WIDTH/2, SCREEN_HEIGHT/2 };
    player->velocity = (Vector2){ 0, 0 };
    player->rotation = 0;
    player->targetRotation = 0;
    player->health = 100.0f;
    player->maxHealth = 100.0f;
    player->team = rand() % 2;  // Randomly assign to team 0 (red) or 1 (blue)
    player->score = 0;
    
    // Generate a deterministic color based on player ID
    unsigned int hashValue = 0;
    for (int i = 0; playerId[i] != '\0'; i++) {
        hashValue = hashValue * 31 + playerId[i];
    }
    
    // Use hash to create a color with good saturation and brightness
    float hue = (hashValue % 360) / 360.0f;
    float saturation = 0.7f + (hashValue % 30) / 100.0f; // 0.7-0.99
    float value = 0.8f + (hashValue % 20) / 100.0f;      // 0.8-0.99
    
    // Convert HSV to RGB
    int hi = (int)(hue * 6);
    float f = hue * 6 - hi;
    float p = value * (1 - saturation);
    float q = value * (1 - f * saturation);
    float t = value * (1 - (1 - f) * saturation);
    
    float r = 0, g = 0, b = 0;
    switch (hi % 6) {
        case 0: r = value; g = t; b = p; break;
        case 1: r = q; g = value; b = p; break;
        case 2: r = p; g = value; b = t; break;
        case 3: r = p; g = q; b = value; break;
        case 4: r = t; g = p; b = value; break;
        case 5: r = value; g = p; b = q; break;
    }
    
    player->color = (Color){
        (unsigned char)(r * 255),
        (unsigned char)(g * 255),
        (unsigned char)(b * 255),
        255
    };
    
    player->isLocal = isLocal;
    player->active = true;
        
    // Set appropriate team colors for team modes
    if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
        if (player->team == 0) {
            player->color = (Color){220, 50, 50, 255}; // Red team
        } else {
            player->color = (Color){50, 50, 220, 255}; // Blue team
        }
    }
    
    // Initialize weapons
    player->currentWeapon = WEAPON_PISTOL;
    for (int i = 0; i < WEAPON_TOTAL; i++) {
        player->ammo[i] = 100;
        player->magazineAmmo[i] = GetCurrentWeaponStats(player)->magazineSize;
    }
    player->fireTimer = 0;
    player->reloadTimer = 0;
    player->isReloading = false;
    
    // Increment player count
    game.playerCount++;
    
    return player;
}

void RemovePlayer(const char* playerId)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && strcmp(game.players[i].id, playerId) == 0) {
            game.players[i].active = false;
            game.playerCount--;
            break;
        }
    }
}

void UpdatePlayers(void)
{
    float dt = GetFrameTime();
    
    // Update all players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active) {
            Player* player = &game.players[i];
            
            if (player->isLocal) {
                // Apply velocity to position
                player->position.x += player->velocity.x * dt;
                player->position.y += player->velocity.y * dt;
                
                // Keep player in bounds
                if (player->position.x < PLAYER_SIZE/2) player->position.x = PLAYER_SIZE/2;
                if (player->position.x > SCREEN_WIDTH - PLAYER_SIZE/2) 
                    player->position.x = SCREEN_WIDTH - PLAYER_SIZE/2;
                if (player->position.y < PLAYER_SIZE/2) player->position.y = PLAYER_SIZE/2;
                if (player->position.y > SCREEN_HEIGHT - PLAYER_SIZE/2) 
                    player->position.y = SCREEN_HEIGHT - PLAYER_SIZE/2;
                
                // Handle player death
                if (player->health <= 0) {
                    // Award points to killer in Deathmatch modes
                    // We're simplifying here - ideally we'd track the last player who damaged this player
                    
                    // Award team points in team modes
                    if (game.mode == MODE_TEAM_DEATHMATCH) {
                        // Award point to the opposing team
                        int opposingTeam = player->team == 0 ? 1 : 0;
                        game.teamScores[opposingTeam]++;
                        
                        char teamName[10];
                        sprintf(teamName, "%s", opposingTeam == 0 ? "RED" : "BLUE");
                        SetStatusMessage("Point for %s team!", teamName);
                    }
                    
                    // Individual scoring in deathmatch
                    if (game.mode == MODE_DEATHMATCH) {
                        // In a real game, we'd track who killed this player
                        // For now, just notify of the death
                        SetStatusMessage("Player %s was eliminated!", player->name);
                    }
                    
                    // Handle CTF flag drop if player was carrying it
                    if (game.mode == MODE_CAPTURE_FLAG) {
                        for (int i = 0; i < 2; i++) {
                            if (game.flags[i].isCaptured && strcmp(game.flags[i].carrierId, player->id) == 0) {
                                // Drop the flag where the player died
                                game.flags[i].position = player->position;
                                game.flags[i].isCaptured = false;
                                strcpy(game.flags[i].carrierId, "");
                                SetStatusMessage("Flag dropped!");
                            }
                        }
                    }
                    
                    player->health = player->maxHealth;  // Respawn with full health
                    
                    // Respawn position - team-based in team modes
                    if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
                        if (player->team == 0) {
                            // Red team spawns on left side
                            player->position.x = PLAYER_SIZE + (float)rand()/(float)RAND_MAX * (SCREEN_WIDTH/3);
                        } else {
                            // Blue team spawns on right side
                            player->position.x = 2*SCREEN_WIDTH/3 + (float)rand()/(float)RAND_MAX * (SCREEN_WIDTH/3 - PLAYER_SIZE);
                        }
                        player->position.y = PLAYER_SIZE + (float)rand()/(float)RAND_MAX * (SCREEN_HEIGHT - 2*PLAYER_SIZE);
                    } else {
                        // Random respawn position for deathmatch
                        player->position.x = PLAYER_SIZE + (float)rand()/(float)RAND_MAX * (SCREEN_WIDTH - 2*PLAYER_SIZE);
                        player->position.y = PLAYER_SIZE + (float)rand()/(float)RAND_MAX * (SCREEN_HEIGHT - 2*PLAYER_SIZE);
                    }
                    
                    // Reset velocity
                    player->velocity = (Vector2){0, 0};
                }
                
                // Apply friction with improved values for better movement feel
                if (Vector2Length(player->velocity) > 0) {
                    // More gradual friction for smoother movement
                    float frictionFactor = (5.0f * dt);
                    if (frictionFactor > 1.0f) frictionFactor = 1.0f;
                    
                    player->velocity.x -= player->velocity.x * frictionFactor;
                    player->velocity.y -= player->velocity.y * frictionFactor;
                    
                    // Stop completely if very slow
                    if (Vector2Length(player->velocity) < 5.0f) {
                        player->velocity = (Vector2){0, 0};
                    }
                }
                
                // Smooth rotation
                float rotationDiff = player->targetRotation - player->rotation;
                // Normalize to [-PI, PI]
                while (rotationDiff > M_PI) rotationDiff -= 2 * M_PI;
                while (rotationDiff < -M_PI) rotationDiff += 2 * M_PI;
                player->rotation += rotationDiff * 10.0f * dt;
            } else {
                // For non-local players, apply simple position prediction
                player->position.x += player->velocity.x * dt;
                player->position.y += player->velocity.y * dt;
                
                // Handle remote player death
                if (player->health <= 0) {
                    player->health = player->maxHealth;  // Respawn with full health
                }
            }
            
            // Update fire timer
            if (player->fireTimer > 0) {
                player->fireTimer -= dt;
            }
            
            // Update reload timer
            if (player->isReloading) {
                player->reloadTimer -= dt;
                if (player->reloadTimer <= 0) {
                    // Reload complete
                    WeaponStats* stats = GetCurrentWeaponStats(player);
                    int ammoNeeded = stats->magazineSize - player->magazineAmmo[player->currentWeapon];
                    int ammoToReload = (player->ammo[player->currentWeapon] < ammoNeeded) ? 
                                      player->ammo[player->currentWeapon] : ammoNeeded;
                    
                    player->magazineAmmo[player->currentWeapon] += ammoToReload;
                    player->ammo[player->currentWeapon] -= ammoToReload;
                    player->isReloading = false;
                }
            }
        }
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
            
            // For team modes, use team colors
            Color playerColor = p->color;
            if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
                playerColor = p->team == 0 ? (Color){220, 50, 50, 255} : (Color){50, 50, 220, 255};
            }
            
            DrawRectangleRec(playerRect, playerColor);
            DrawRectangleLinesEx(playerRect, 2, WHITE);
            
            // Draw team indicator
            if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
                DrawCircle(p->position.x, p->position.y - PLAYER_SIZE/2 - 15, 5, 
                           p->team == 0 ? RED : BLUE);
            }
            
            // Draw flag carrier indicator
            if (game.mode == MODE_CAPTURE_FLAG) {
                for (int i = 0; i < 2; i++) {
                    if (game.flags[i].isCaptured && strcmp(game.flags[i].carrierId, p->id) == 0) {
                        Color flagColor = i == 0 ? RED : BLUE;
                        DrawTriangle(
                            (Vector2){p->position.x, p->position.y - PLAYER_SIZE/2 - 25},
                            (Vector2){p->position.x + 10, p->position.y - PLAYER_SIZE/2 - 15},
                            (Vector2){p->position.x - 10, p->position.y - PLAYER_SIZE/2 - 15},
                            flagColor
                        );
                        break;
                    }
                }
            }
            
            // Draw gun
            Vector2 gunEnd = {
                p->position.x + cosf(p->rotation) * GUN_LENGTH,
                p->position.y + sinf(p->rotation) * GUN_LENGTH
            };
            DrawLineEx(p->position, gunEnd, 3, WHITE);
            
            // Draw health bar
            Rectangle healthBg = {p->position.x - PLAYER_SIZE/2, p->position.y - PLAYER_SIZE/2 - 10, PLAYER_SIZE, 4};
            DrawRectangleRec(healthBg, DARKGRAY);
            Rectangle healthBar = {p->position.x - PLAYER_SIZE/2, p->position.y - PLAYER_SIZE/2 - 10, 
                                 PLAYER_SIZE * (p->health / p->maxHealth), 4};
            Color healthColor = p->health > 50 ? GREEN : (p->health > 25 ? YELLOW : RED);
            DrawRectangleRec(healthBar, healthColor);
            
            // Highlight local player
            if (p->isLocal) {
                DrawCircleLines(p->position.x, p->position.y, PLAYER_SIZE/2 + 5, YELLOW);
            }
            
            // Draw player name
            DrawText(p->name, p->position.x - MeasureText(p->name, 10)/2, p->position.y + PLAYER_SIZE/2 + 5, 10, WHITE);
        }
    }
}