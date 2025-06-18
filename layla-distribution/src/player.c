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
    player->kills = 0;
    player->deaths = 0;
    
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
                    player->deaths++; // Increment death counter
                    
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
            
            // Enhanced player body design
            Vector2 center = p->position;
            
            // Draw shadow
            DrawCircle(center.x + 2, center.y + 2, PLAYER_SIZE/2 + 2, (Color){0, 0, 0, 60});
            
            // For team modes, use enhanced team colors
            Color playerColor = p->color;
            Color outlineColor = WHITE;
            if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
                playerColor = p->team == 0 ? (Color){220, 50, 50, 255} : (Color){50, 120, 220, 255};
                outlineColor = p->team == 0 ? (Color){255, 100, 100, 255} : (Color){100, 160, 255, 255};
            }
            
            // Draw player body as hexagon for better look
            Vector2 hexPoints[6];
            float hexRadius = PLAYER_SIZE/2;
            for (int h = 0; h < 6; h++) {
                float angle = (h * 60.0f) * DEG2RAD;
                hexPoints[h] = (Vector2){
                    center.x + cosf(angle) * hexRadius,
                    center.y + sinf(angle) * hexRadius
                };
            }
            
            // Draw hexagon body
            for (int h = 0; h < 6; h++) {
                Vector2 p1 = hexPoints[h];
                Vector2 p2 = hexPoints[(h + 1) % 6];
                Vector2 p3 = center;
                DrawTriangle(p1, p2, p3, playerColor);
            }
            
            // Draw hexagon outline
            for (int h = 0; h < 6; h++) {
                Vector2 p1 = hexPoints[h];
                Vector2 p2 = hexPoints[(h + 1) % 6];
                DrawLineEx(p1, p2, 2.0f, outlineColor);
            }
            
            // Draw directional indicator (front of player)
            Vector2 frontPoint = {
                center.x + cosf(p->rotation) * (hexRadius + 3),
                center.y + sinf(p->rotation) * (hexRadius + 3)
            };
            DrawCircle(frontPoint.x, frontPoint.y, 3, outlineColor);
            
            // Enhanced weapon rendering
            Vector2 gunStart = {
                center.x + cosf(p->rotation) * (hexRadius - 2),
                center.y + sinf(p->rotation) * (hexRadius - 2)
            };
            Vector2 gunEnd = {
                center.x + cosf(p->rotation) * (GUN_LENGTH + hexRadius),
                center.y + sinf(p->rotation) * (GUN_LENGTH + hexRadius)
            };
            
            // Draw weapon based on type
            WeaponStats* stats = GetCurrentWeaponStats(p);
            Color weaponColor = LIGHTGRAY;
            float weaponWidth = 4.0f;
            
            if (stats) {
                switch (p->currentWeapon) {
                    case WEAPON_PISTOL:
                        weaponColor = GRAY;
                        weaponWidth = 3.0f;
                        break;
                    case WEAPON_RIFLE:
                        weaponColor = DARKGRAY;
                        weaponWidth = 5.0f;
                        break;
                    case WEAPON_SHOTGUN:
                        weaponColor = BROWN;
                        weaponWidth = 6.0f;
                        break;
                    case WEAPON_SMG:
                        weaponColor = DARKBLUE;
                        weaponWidth = 4.0f;
                        break;
                    case WEAPON_SNIPER:
                        weaponColor = BLACK;
                        weaponWidth = 4.0f;
                        // Draw scope
                        Vector2 scopePos = {
                            gunStart.x + cosf(p->rotation) * 8,
                            gunStart.y + sinf(p->rotation) * 8
                        };
                        DrawRectanglePro((Rectangle){scopePos.x, scopePos.y, 6, 3}, 
                                       (Vector2){3, 1.5f}, p->rotation * RAD2DEG, DARKGRAY);
                        break;
                    default:
                        break;
                }
            }
            
            DrawLineEx(gunStart, gunEnd, weaponWidth, weaponColor);
            DrawLineEx(gunStart, gunEnd, 1.0f, WHITE); // Highlight line
            
            // Enhanced team indicator
            if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
                Color teamIndicatorColor = p->team == 0 ? RED : BLUE;
                Vector2 indicatorPos = {center.x, center.y - hexRadius - 12};
                
                // Draw team badge
                DrawRectanglePro((Rectangle){indicatorPos.x, indicatorPos.y, 12, 8}, 
                               (Vector2){6, 4}, 0, teamIndicatorColor);
                DrawRectangleLinesEx((Rectangle){indicatorPos.x - 6, indicatorPos.y - 4, 12, 8}, 
                                   1, WHITE);
                
                // Draw team letter
                const char* teamLetter = p->team == 0 ? "R" : "B";
                DrawText(teamLetter, indicatorPos.x - 3, indicatorPos.y - 3, 8, WHITE);
            }
            
            // Enhanced flag carrier indicator
            if (game.mode == MODE_CAPTURE_FLAG) {
                for (int f = 0; f < 2; f++) {
                    if (game.flags[f].isCaptured && strcmp(game.flags[f].carrierId, p->id) == 0) {
                        Color flagColor = f == 0 ? RED : BLUE;
                        Vector2 flagPos = {center.x - 8, center.y - hexRadius - 20};
                        
                        // Draw flag pole
                        DrawLineEx((Vector2){flagPos.x, flagPos.y}, 
                                 (Vector2){flagPos.x, flagPos.y + 15}, 2, BROWN);
                        
                        // Draw flag
                        Vector2 flagPoints[3] = {
                            {flagPos.x, flagPos.y},
                            {flagPos.x + 12, flagPos.y + 4},
                            {flagPos.x, flagPos.y + 8}
                        };
                        DrawTriangle(flagPoints[0], flagPoints[1], flagPoints[2], flagColor);
                        DrawTriangleLines(flagPoints[0], flagPoints[1], flagPoints[2], WHITE);
                        break;
                    }
                }
            }
            
            // Enhanced health bar
            float healthWidth = PLAYER_SIZE + 4;
            float healthHeight = 4;
            Vector2 healthPos = {center.x - healthWidth/2, center.y - hexRadius - 8};
            
            // Health background
            DrawRectangleRounded((Rectangle){healthPos.x, healthPos.y, healthWidth, healthHeight}, 
                               0.3f, 8, (Color){40, 40, 40, 200});
            
            // Health bar
            float healthPercentage = p->health / p->maxHealth;
            Color healthColor = healthPercentage > 0.6f ? GREEN : 
                               healthPercentage > 0.3f ? YELLOW : RED;
            
            if (healthPercentage > 0) {
                DrawRectangleRounded((Rectangle){healthPos.x + 1, healthPos.y + 1, 
                                               (healthWidth - 2) * healthPercentage, healthHeight - 2}, 
                                   0.3f, 8, healthColor);
            }
            
            // Highlight local player
            if (p->isLocal) {
                // Animated ring around local player
                float pulseRadius = hexRadius + 8 + sinf(GetTime() * 4) * 3;
                for (int ring = 0; ring < 3; ring++) {
                    DrawCircleLines(center.x, center.y, pulseRadius + ring, 
                                  (Color){255, 255, 0, 100 - ring * 30});
                }
            }
            
            // Enhanced player name display
            int nameWidth = MeasureText(p->name, 12);
            Vector2 namePos = {center.x - nameWidth/2, center.y + hexRadius + 8};
            
            // Name background
            DrawRectangleRounded((Rectangle){namePos.x - 4, namePos.y - 2, nameWidth + 8, 16}, 
                               0.3f, 8, (Color){0, 0, 0, 150});
            
            // Name text with outline
            Color nameColor = p->isLocal ? YELLOW : WHITE;
            DrawText(p->name, namePos.x + 1, namePos.y + 1, 12, BLACK); // Shadow
            DrawText(p->name, namePos.x, namePos.y, 12, nameColor);
        }
    }
}