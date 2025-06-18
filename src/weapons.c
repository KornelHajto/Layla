#include "../include/common.h"
#include "../include/weapons.h"
#include "../include/particles.h"
#include "../include/network.h"
#include "../include/player.h"
#include "../include/core.h"
#include <math.h>

// Define weapon stats for each weapon type
static WeaponStats weaponStats[WEAPON_TOTAL] = {
    // WEAPON_PISTOL
    {
        .name = "Pistol",
        .damage = 25,
        .fireRate = 5.0f,  // 5 shots per second
        .reloadTime = 1.0f,
        .magazineSize = 12,
        .maxAmmo = 120,
        .spread = 0.015f,
        .bulletSpeed = 900.0f,
        .bulletsPerShot = 1,
        .screenShakeIntensity = 0.5f,
        .particlesPerShot = 5,
        .muzzleFlashColor = {255, 200, 100, 255},
        .muzzleFlashSize = 15.0f,
        .automatic = false,
        .enabled = true
    },
    // WEAPON_RIFLE
    {
        .name = "Rifle",
        .damage = 35,
        .fireRate = 8.0f,  // 8 shots per second
        .reloadTime = 1.8f,
        .magazineSize = 30,
        .maxAmmo = 150,
        .spread = 0.025f,
        .bulletSpeed = 1100.0f,
        .bulletsPerShot = 1,
        .screenShakeIntensity = 0.8f,
        .particlesPerShot = 7,
        .muzzleFlashColor = {255, 180, 80, 255},
        .muzzleFlashSize = 20.0f,
        .automatic = true,
        .enabled = true
    },
    // WEAPON_SHOTGUN
    {
        .name = "Shotgun",
        .damage = 18,
        .fireRate = 1.5f,  // 1.5 shots per second
        .reloadTime = 2.0f,
        .magazineSize = 8,
        .maxAmmo = 64,
        .spread = 0.2f,
        .bulletSpeed = 800.0f,
        .bulletsPerShot = 8,
        .screenShakeIntensity = 1.2f,
        .particlesPerShot = 15,
        .muzzleFlashColor = {255, 160, 60, 255},
        .muzzleFlashSize = 25.0f,
        .automatic = false,
        .enabled = true
    },
    // WEAPON_SMG
    {
        .name = "SMG",
        .damage = 18,
        .fireRate = 15.0f,  // 15 shots per second
        .reloadTime = 1.6f,
        .magazineSize = 30,
        .maxAmmo = 180,
        .spread = 0.045f,
        .bulletSpeed = 950.0f,
        .bulletsPerShot = 1,
        .screenShakeIntensity = 0.6f,
        .particlesPerShot = 6,
        .muzzleFlashColor = {255, 190, 90, 255},
        .muzzleFlashSize = 18.0f,
        .automatic = true,
        .enabled = true
    },
    // WEAPON_SNIPER
    {
        .name = "Sniper",
        .damage = 90,
        .fireRate = 1.0f,  // 1 shot per second
        .reloadTime = 2.0f,
        .magazineSize = 5,
        .maxAmmo = 40,
        .spread = 0.002f,
        .bulletSpeed = 1500.0f,
        .bulletsPerShot = 1,
        .screenShakeIntensity = 1.0f,
        .particlesPerShot = 8,
        .muzzleFlashColor = {240, 220, 110, 255},
        .muzzleFlashSize = 22.0f,
        .automatic = false,
        .enabled = true
    }
};

WeaponStats* GetCurrentWeaponStats(Player* player)
{
    if (!player || player->currentWeapon < 0 || player->currentWeapon >= WEAPON_TOTAL) {
        return NULL;
    }
    
    return &weaponStats[player->currentWeapon];
}

void SwitchWeapon(Player* player, WeaponType weapon)
{
    if (!player || weapon < 0 || weapon >= WEAPON_TOTAL) {
        return;
    }
    
    // Only switch if the weapon is enabled
    if (weaponStats[weapon].enabled) {
        player->currentWeapon = weapon;
        player->isReloading = false;
    }
}

void ReloadWeapon(Player* player)
{
    if (!player || player->isReloading) {
        return;
    }
    
    WeaponStats* stats = GetCurrentWeaponStats(player);
    if (!stats) {
        return;
    }
    
    // Only reload if we have ammo and aren't full
    if (player->ammo[player->currentWeapon] > 0 && 
        player->magazineAmmo[player->currentWeapon] < stats->magazineSize) {
        player->isReloading = true;
        player->reloadTimer = stats->reloadTime;
    }
}

bool CanShoot(Player* player)
{
    if (!player || player->isReloading) {
        return false;
    }
    
    WeaponStats* stats = GetCurrentWeaponStats(player);
    if (!stats) {
        return false;
    }
    
    return player->fireTimer <= 0 && player->magazineAmmo[player->currentWeapon] > 0;
}

void FireWeapon(Player* player)
{
    if (!player || !CanShoot(player)) {
        return;
    }
    
    WeaponStats* stats = GetCurrentWeaponStats(player);
    if (!stats) {
        return;
    }
    
    // Update cooldown
    player->fireTimer = 1.0f / stats->fireRate;
    
    // Reduce ammo
    player->magazineAmmo[player->currentWeapon]--;
    
    // Apply screen shake
    if (game.screenShakeEnabled) {
        game.screenShakeIntensity += stats->screenShakeIntensity;
    }
    
    // Create bullets
    for (int i = 0; i < stats->bulletsPerShot; i++) {
        // Calculate spread
        float spreadAngle = ((float)rand() / RAND_MAX - 0.5f) * stats->spread;
        float bulletAngle = player->rotation + spreadAngle;
        
        // Create bullet
        Vector2 bulletPos = {
            player->position.x + cosf(player->rotation) * GUN_LENGTH,
            player->position.y + sinf(player->rotation) * GUN_LENGTH
        };

        CreateBullet(
            player->id,
            bulletPos,
            bulletAngle,
            stats->damage,
            player->color
        );
    }
    
    // Create muzzle flash
    CreateMuzzleFlash(
        (Vector2){
            player->position.x + cosf(player->rotation) * GUN_LENGTH,
            player->position.y + sinf(player->rotation) * GUN_LENGTH
        },
        player->rotation,
        stats->muzzleFlashSize,
        stats->muzzleFlashColor,
        player->id
    );
    
    // Create shell casing particles
    Vector2 shellDirection = {
        cosf(player->rotation + M_PI/2),  // Eject to the right of gun
        sinf(player->rotation + M_PI/2)
    };
    
    for (int i = 0; i < stats->particlesPerShot; i++) {
        // Add randomness to particle direction
        Vector2 particleDir = {
            shellDirection.x + ((float)rand() / RAND_MAX - 0.5f) * 0.3f,
            shellDirection.y + ((float)rand() / RAND_MAX - 0.5f) * 0.3f
        };
        
        float particleSpeed = 50.0f + ((float)rand() / RAND_MAX) * 100.0f;
        
        CreateParticle(
            (Vector2){
                player->position.x + cosf(player->rotation) * (GUN_LENGTH * 0.7f),
                player->position.y + sinf(player->rotation) * (GUN_LENGTH * 0.7f)
            },
            (Vector2){
                particleDir.x * particleSpeed,
                particleDir.y * particleSpeed
            },
            ((float)rand() / RAND_MAX) * 2 * M_PI,  // Random rotation
            ((float)rand() / RAND_MAX - 0.5f) * 10.0f,  // Random spin
            2.0f + ((float)rand() / RAND_MAX) * 2.0f,   // Random size
            0.5f + ((float)rand() / RAND_MAX) * 0.5f,   // Random lifetime
            (Color){255, 200, 100, 255},  // Start color (brass shell)
            (Color){200, 150, 50, 0},     // End color (fade out)
            PARTICLE_SHELL
        );
    }
    
    // Auto reload if out of ammo
    if (player->magazineAmmo[player->currentWeapon] <= 0 && player->ammo[player->currentWeapon] > 0) {
        ReloadWeapon(player);
    }
    
    // Send shoot message for network play
    if (player->isLocal && game.isConnected) {
        NetworkMessage shootMsg;
        shootMsg.type = MSG_PLAYER_SHOOT;
        strcpy(shootMsg.playerId, player->id);
        
        // Create bullet data for the message
        Bullet bulletData;
        bulletData.position = (Vector2){
            player->position.x + cosf(player->rotation) * GUN_LENGTH,
            player->position.y + sinf(player->rotation) * GUN_LENGTH
        };
        bulletData.rotation = player->rotation;
        bulletData.damage = stats->damage;
        strcpy(bulletData.ownerId, player->id);
        bulletData.color = player->color;
        
        shootMsg.data.bullet = bulletData;
        
        if (game.isHost) {
            // Send to all clients
            for (int i = 0; i < game.clientCount; i++) {
                SendMessage(&shootMsg, &game.clientAddrs[i]);
            }
        } else {
            // Send to server
            SendMessage(&shootMsg, &game.serverAddr);
        }
    }
}

void CreateBullet(const char* ownerId, Vector2 position, float rotation, int damage, Color color)
{
    // Find an empty slot
    int slot = -1;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!game.bullets[i].active) {
            slot = i;
            break;
        }
    }
    
    // If no slots available, overwrite the oldest bullet
    if (slot == -1) {
        float oldestLifetime = 0;
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (game.bullets[i].lifetime > oldestLifetime) {
                oldestLifetime = game.bullets[i].lifetime;
                slot = i;
            }
        }
    }
    
    // Initialize the bullet
    Bullet* bullet = &game.bullets[slot];
    bullet->position = position;
    bullet->velocity = (Vector2){
        cosf(rotation) * BULLET_SPEED,
        sinf(rotation) * BULLET_SPEED
    };
    bullet->rotation = rotation;
    bullet->lifetime = BULLET_LIFETIME;
    bullet->damage = damage;
    strcpy(bullet->ownerId, ownerId);
    bullet->active = true;
    
    // Set bullet color based on owner's team in team modes
    Player* owner = FindPlayer(ownerId);
    if ((game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) && owner && owner->active) {
        bullet->color = owner->team == 0 ? RED : BLUE;
    } else {
        bullet->color = color;
    }
    
    // Increment bullet count
    game.bulletCount++;
}

void UpdateBullets(void)
{
    float dt = GetFrameTime();
    
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game.bullets[i].active) {
            Bullet* bullet = &game.bullets[i];
            
            // Store previous position for better collision detection
            Vector2 prevPosition = bullet->position;
            
            // Update position
            bullet->position.x += bullet->velocity.x * dt;
            bullet->position.y += bullet->velocity.y * dt;
            
            // Update lifetime
            bullet->lifetime -= dt;
            if (bullet->lifetime <= 0) {
                bullet->active = false;
                game.bulletCount--;
                continue;
            }
            
            // Check for collisions with walls
            bool hitWall = false;
            Vector2 normal = {0, 0};
            
            if (bullet->position.x < 0) {
                bullet->position.x = 0;
                normal = (Vector2){1, 0};
                hitWall = true;
            }
            else if (bullet->position.x > SCREEN_WIDTH) {
                bullet->position.x = SCREEN_WIDTH;
                normal = (Vector2){-1, 0};
                hitWall = true;
            }
            
            if (bullet->position.y < 0) {
                bullet->position.y = 0;
                normal = (Vector2){0, 1};
                hitWall = true;
            }
            else if (bullet->position.y > SCREEN_HEIGHT) {
                bullet->position.y = SCREEN_HEIGHT;
                normal = (Vector2){0, -1};
                hitWall = true;
            }
            
            if (hitWall) {
                CreateSparkEffect(bullet->position, normal, 15);
                bullet->active = false;
                game.bulletCount--;
                continue;
            }
            
            // Check for collisions with players using line-circle intersection for better accuracy
            bool hitPlayer = false;
            
            for (int j = 0; j < MAX_PLAYERS; j++) {
                if (game.players[j].active && strcmp(game.players[j].id, bullet->ownerId) != 0) {
                    Player* player = &game.players[j];
                    
                    // Line-circle collision for better accuracy
                    // Check for friendly fire in team modes
                    bool canDamage = true;
                    
                    if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
                        Player* shooter = FindPlayer(bullet->ownerId);
                        if (shooter && shooter->active && shooter->team == player->team) {
                            canDamage = false; // No friendly fire
                        }
                    }
                    
                    // Simple circle-circle collision
                    Vector2 d = {
                        bullet->position.x - prevPosition.x,
                        bullet->position.y - prevPosition.y
                    };
                    
                    Vector2 f = {
                        prevPosition.x - player->position.x,
                        prevPosition.y - player->position.y
                    };
                    
                    float a = d.x * d.x + d.y * d.y;
                    float b = 2 * (f.x * d.x + f.y * d.y);
                    float c = f.x * f.x + f.y * f.y - (PLAYER_SIZE/2 + BULLET_SIZE) * (PLAYER_SIZE/2 + BULLET_SIZE);
                    
                    float discriminant = b * b - 4 * a * c;
                    
                    if (discriminant >= 0 && canDamage) {
                        discriminant = sqrtf(discriminant);
                        
                        float t1 = (-b - discriminant) / (2 * a);
                        float t2 = (-b + discriminant) / (2 * a);
                        
                        if ((t1 >= 0 && t1 <= 1) || (t2 >= 0 && t2 <= 1)) {
                            // Hit the player
                            player->health -= bullet->damage;
                            
                            // Update score for the shooter in deathmatch
                            Player* shooter = FindPlayer(bullet->ownerId);
                            if (shooter && shooter->active) {
                                if (game.mode == MODE_DEATHMATCH && player->health <= 0) {
                                    shooter->score++;
                                    SetStatusMessage("%s eliminated %s (+1 point)", shooter->name, player->name);
                                }
                            }
                            
                            // Vibrant blood splatter
                            Vector2 direction = {
                                -bullet->velocity.x / BULLET_SPEED,
                                -bullet->velocity.y / BULLET_SPEED
                            };
                            CreateBloodSplatter(bullet->position, direction, 30);
                            
                            // Enhanced damage effect
                            if (player->isLocal) {
                                AddDamageFlash((Color){255, 0, 0, 180});
                            }
                            
                            // Check if player died - death handling now in player.c
                            if (player->health <= 0) {
                                player->health = 0;
                                
                                // Award team points in team deathmatch
                                if (game.mode == MODE_TEAM_DEATHMATCH) {
                                    Player* shooter = FindPlayer(bullet->ownerId);
                                    if (shooter && shooter->active) {
                                        game.teamScores[shooter->team]++;
                                        
                                        // Update shooter's personal score
                                        shooter->score++;
                                    }
                                }
                            }
                            
                            // Deactivate bullet
                            bullet->active = false;
                            game.bulletCount--;
                            hitPlayer = true;
                            break;
                        }
                    }
                }
            }
            
            if (hitPlayer) {
                continue;
            }
        }
    }
}

void DrawBullets(void)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game.bullets[i].active) {
            Bullet* b = &game.bullets[i];
            
            // Draw bullet as a small circle with a white outline for better visibility
            DrawCircleV(b->position, BULLET_SIZE + 1, WHITE);
            
            // Use team colors in team modes
            Color bulletColor = b->color;
            if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
                Player* owner = FindPlayer(b->ownerId);
                if (owner && owner->active) {
                    bulletColor = owner->team == 0 ? RED : BLUE;
                }
            }
            
            DrawCircleV(b->position, BULLET_SIZE, bulletColor);
            
            // Enhanced trail effect
            Vector2 trailEnd = {
                b->position.x - cosf(b->rotation) * BULLET_SIZE * 6,
                b->position.y - sinf(b->rotation) * BULLET_SIZE * 6
            };
            
            // Make trail semi-transparent
            Color trailColor = bulletColor;
            trailColor.a = 150;
            DrawLineEx(b->position, trailEnd, BULLET_SIZE * 1.8f, trailColor);
            
            // Add a glow effect
            Color glowColor = bulletColor;
            glowColor.a = 80;
            DrawCircleV(b->position, BULLET_SIZE * 2.5f, glowColor);
        }
    }
}