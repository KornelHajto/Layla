#include "../include/common.h"
#include "../include/particles.h"
#include "../include/player.h"

void UpdateParticles(void)
{
    float dt = GetFrameTime();
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (game.particles[i].active) {
            Particle* p = &game.particles[i];
            
            // Update position
            p->position.x += p->velocity.x * dt;
            p->position.y += p->velocity.y * dt;
            
            // Apply gravity for some particle types
            if (p->type == PARTICLE_DEBRIS || p->type == PARTICLE_SHELL) {
                p->velocity.y += 500.0f * dt; // Gravity
            }
            
            // Apply drag for some particle types
            if (p->type == PARTICLE_SMOKE) {
                p->velocity.x *= 0.98f;
                p->velocity.y *= 0.98f;
            }
            
            // Update rotation
            p->rotation += p->rotationSpeed * dt;
            
            // Update lifetime
            p->lifetime -= dt;
            
            // Update color based on lifetime percentage
            float lifePercent = p->lifetime / p->maxLifetime;
            p->color.r = (unsigned char)(p->startColor.r * lifePercent + p->endColor.r * (1.0f - lifePercent));
            p->color.g = (unsigned char)(p->startColor.g * lifePercent + p->endColor.g * (1.0f - lifePercent));
            p->color.b = (unsigned char)(p->startColor.b * lifePercent + p->endColor.b * (1.0f - lifePercent));
            p->color.a = (unsigned char)(p->startColor.a * lifePercent + p->endColor.a * (1.0f - lifePercent));
            
            // Deactivate if lifetime expired
            if (p->lifetime <= 0) {
                p->active = false;
                game.particleCount--;
            }
        }
    }
}

void UpdateMuzzleFlashes(void)
{
    float dt = GetFrameTime();
    
    for (int i = 0; i < MAX_MUZZLE_FLASHES; i++) {
        if (game.muzzleFlashes[i].active) {
            MuzzleFlash* m = &game.muzzleFlashes[i];
            
            // Update lifetime
            m->lifetime -= dt;
            
            // Update opacity based on lifetime
            float lifePercent = m->lifetime / m->maxLifetime;
            m->color.a = (unsigned char)(255 * lifePercent);
            
            // Update position based on player's gun position if player exists
            Player* owner = FindPlayer(m->ownerId);
            if (owner && owner->active) {
                m->position.x = owner->position.x + cosf(owner->rotation) * GUN_LENGTH;
                m->position.y = owner->position.y + sinf(owner->rotation) * GUN_LENGTH;
                m->rotation = owner->rotation;
            }
            
            // Deactivate if lifetime expired
            if (m->lifetime <= 0) {
                m->active = false;
                game.muzzleFlashCount--;
            }
        }
    }
}

void UpdateHitEffects(void)
{
    float dt = GetFrameTime();
    
    for (int i = 0; i < MAX_HIT_EFFECTS; i++) {
        if (game.hitEffects[i].active) {
            HitEffect* h = &game.hitEffects[i];
            
            // Update lifetime
            h->lifetime -= dt;
            
            // Update size and opacity based on lifetime
            float lifePercent = h->lifetime / h->maxLifetime;
            h->color.a = (unsigned char)(255 * lifePercent);
            
            // Expand the effect as it fades
            h->size = h->size * (1.0f + dt * 2.0f);
            
            // Deactivate if lifetime expired
            if (h->lifetime <= 0) {
                h->active = false;
                game.hitEffectCount--;
            }
        }
    }
}

void DrawParticles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (game.particles[i].active) {
            Particle* p = &game.particles[i];
            
            switch (p->type) {
                case PARTICLE_DEBRIS:
                    // Draw as small rectangle
                    {
                        Rectangle rect = {
                            p->position.x - p->size/2,
                            p->position.y - p->size/2,
                            p->size,
                            p->size
                        };
                        DrawRectanglePro(rect, (Vector2){p->size/2, p->size/2}, p->rotation * RAD2DEG, p->color);
                    }
                    break;
                    
                case PARTICLE_BLOOD:
                    // Draw as circle
                    DrawCircleV(p->position, p->size, p->color);
                    break;
                    
                case PARTICLE_SPARK:
                    // Draw as line
                    {
                        Vector2 end = {
                            p->position.x + cosf(p->rotation) * p->size * 2,
                            p->position.y + sinf(p->rotation) * p->size * 2
                        };
                        DrawLineEx(p->position, end, p->size / 2, p->color);
                    }
                    break;
                    
                case PARTICLE_SMOKE:
                    // Draw as circle
                    DrawCircleV(p->position, p->size, p->color);
                    break;
                    
                case PARTICLE_SHELL:
                    // Draw as small rectangle
                    {
                        Rectangle rect = {
                            p->position.x - p->size/2,
                            p->position.y - p->size/2,
                            p->size * 2,
                            p->size
                        };
                        DrawRectanglePro(rect, (Vector2){p->size, p->size/2}, p->rotation * RAD2DEG, p->color);
                    }
                    break;
            }
        }
    }
}

void DrawMuzzleFlashes(void)
{
    for (int i = 0; i < MAX_MUZZLE_FLASHES; i++) {
        if (game.muzzleFlashes[i].active) {
            MuzzleFlash* m = &game.muzzleFlashes[i];
            
            // Draw as triangle
            Vector2 v1 = {
                m->position.x,
                m->position.y
            };
            Vector2 v2 = {
                m->position.x + cosf(m->rotation - 0.2f) * m->size,
                m->position.y + sinf(m->rotation - 0.2f) * m->size
            };
            Vector2 v3 = {
                m->position.x + cosf(m->rotation + 0.2f) * m->size,
                m->position.y + sinf(m->rotation + 0.2f) * m->size
            };
            
            DrawTriangle(v2, v1, v3, m->color);
            
            // Draw additional glow
            Color glowColor = m->color;
            glowColor.a = m->color.a / 2;
            DrawCircleV(m->position, m->size / 2, glowColor);
        }
    }
}

void DrawHitEffects(void)
{
    for (int i = 0; i < MAX_HIT_EFFECTS; i++) {
        if (game.hitEffects[i].active) {
            HitEffect* h = &game.hitEffects[i];
            
            // Draw as circle
            DrawCircleV(h->position, h->size, h->color);
        }
    }
}

void CreateParticle(Vector2 position, Vector2 velocity, float rotation, float rotationSpeed,
                    float size, float lifetime, Color startColor, Color endColor, ParticleType type)
{
    // Don't create particles if effects are disabled
    if (!game.visualEffectsEnabled) {
        return;
    }
    
    // Find an empty slot
    int slot = -1;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!game.particles[i].active) {
            slot = i;
            break;
        }
    }
    
    // If no slots available, replace oldest particle
    if (slot == -1) {
        float oldestLifetime = MAX_PARTICLES;
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (game.particles[i].lifetime < oldestLifetime) {
                oldestLifetime = game.particles[i].lifetime;
                slot = i;
            }
        }
    }
    
    // Initialize the particle
    Particle* p = &game.particles[slot];
    p->position = position;
    p->velocity = velocity;
    p->rotation = rotation;
    p->rotationSpeed = rotationSpeed;
    p->size = size;
    p->lifetime = lifetime;
    p->maxLifetime = lifetime;
    p->startColor = startColor;
    p->endColor = endColor;
    p->color = startColor;
    p->type = type;
    p->active = true;
    
    // Increment particle count
    game.particleCount++;
}

void CreateMuzzleFlash(Vector2 position, float rotation, float size, Color color, const char* ownerId)
{
    // Don't create muzzle flashes if effects are disabled
    if (!game.visualEffectsEnabled) {
        return;
    }
    
    // Find an empty slot
    int slot = -1;
    for (int i = 0; i < MAX_MUZZLE_FLASHES; i++) {
        if (!game.muzzleFlashes[i].active) {
            slot = i;
            break;
        }
    }
    
    // If no slots available, replace oldest muzzle flash
    if (slot == -1) {
        float oldestLifetime = MUZZLE_FLASH_LIFETIME;
        for (int i = 0; i < MAX_MUZZLE_FLASHES; i++) {
            if (game.muzzleFlashes[i].lifetime < oldestLifetime) {
                oldestLifetime = game.muzzleFlashes[i].lifetime;
                slot = i;
            }
        }
    }
    
    // Initialize the muzzle flash
    MuzzleFlash* m = &game.muzzleFlashes[slot];
    m->position = position;
    m->rotation = rotation;
    m->size = size;
    m->lifetime = MUZZLE_FLASH_LIFETIME;
    m->maxLifetime = MUZZLE_FLASH_LIFETIME;
    m->color = color;
    m->active = true;
    strcpy(m->ownerId, ownerId);
    
    // Increment muzzle flash count
    game.muzzleFlashCount++;
    
    // Also create some smoke particles
    for (int i = 0; i < 5; i++) {
        Vector2 smokeVel = {
            cosf(rotation) * 50.0f + ((float)rand() / RAND_MAX - 0.5f) * 30.0f,
            sinf(rotation) * 50.0f + ((float)rand() / RAND_MAX - 0.5f) * 30.0f
        };
        
        CreateParticle(
            position,
            smokeVel,
            ((float)rand() / RAND_MAX) * 2 * M_PI,
            ((float)rand() / RAND_MAX - 0.5f) * 2.0f,
            5.0f + ((float)rand() / RAND_MAX) * 5.0f,
            0.5f + ((float)rand() / RAND_MAX) * 0.5f,
            (Color){200, 200, 200, 180},
            (Color){150, 150, 150, 0},
            PARTICLE_SMOKE
        );
    }
}

void CreateHitEffect(Vector2 position, float size, Color color)
{
    // Don't create hit effects if effects are disabled
    if (!game.visualEffectsEnabled) {
        return;
    }
    
    // Find an empty slot
    int slot = -1;
    for (int i = 0; i < MAX_HIT_EFFECTS; i++) {
        if (!game.hitEffects[i].active) {
            slot = i;
            break;
        }
    }
    
    // If no slots available, replace oldest hit effect
    if (slot == -1) {
        float oldestLifetime = HIT_EFFECT_LIFETIME;
        for (int i = 0; i < MAX_HIT_EFFECTS; i++) {
            if (game.hitEffects[i].lifetime < oldestLifetime) {
                oldestLifetime = game.hitEffects[i].lifetime;
                slot = i;
            }
        }
    }
    
    // Initialize the hit effect
    HitEffect* h = &game.hitEffects[slot];
    h->position = position;
    h->size = size;
    h->lifetime = HIT_EFFECT_LIFETIME;
    h->maxLifetime = HIT_EFFECT_LIFETIME;
    h->color = color;
    h->active = true;
    
    // Increment hit effect count
    game.hitEffectCount++;
}

void CreateBloodSplatter(Vector2 position, Vector2 direction, int count)
{
    for (int i = 0; i < count; i++) {
        // Add randomness to particle direction
        Vector2 particleDir = {
            direction.x + ((float)rand() / RAND_MAX - 0.5f) * 0.5f,
            direction.y + ((float)rand() / RAND_MAX - 0.5f) * 0.5f
        };
        
        // Normalize direction
        float length = sqrtf(particleDir.x * particleDir.x + particleDir.y * particleDir.y);
        if (length > 0) {
            particleDir.x /= length;
            particleDir.y /= length;
        }
        
        float particleSpeed = 50.0f + ((float)rand() / RAND_MAX) * 150.0f;
        
        CreateParticle(
            position,
            (Vector2){
                particleDir.x * particleSpeed,
                particleDir.y * particleSpeed
            },
            0,
            0,
            2.0f + ((float)rand() / RAND_MAX) * 3.0f,
            0.5f + ((float)rand() / RAND_MAX) * 0.5f,
            (Color){180, 0, 0, 255},
            (Color){120, 0, 0, 0},
            PARTICLE_BLOOD
        );
    }
    
    // Create hit effect
    CreateHitEffect(position, 10.0f, (Color){180, 0, 0, 150});
}

void CreateSparkEffect(Vector2 position, Vector2 normal, int count)
{
    for (int i = 0; i < count; i++) {
        // Calculate reflection direction with randomness
        Vector2 reflectDir = {
            normal.x + ((float)rand() / RAND_MAX - 0.5f) * 1.5f,
            normal.y + ((float)rand() / RAND_MAX - 0.5f) * 1.5f
        };
        
        // Normalize direction
        float length = sqrtf(reflectDir.x * reflectDir.x + reflectDir.y * reflectDir.y);
        if (length > 0) {
            reflectDir.x /= length;
            reflectDir.y /= length;
        }
        
        float particleSpeed = 100.0f + ((float)rand() / RAND_MAX) * 200.0f;
        float particleAngle = atan2f(reflectDir.y, reflectDir.x);
        
        CreateParticle(
            position,
            (Vector2){
                reflectDir.x * particleSpeed,
                reflectDir.y * particleSpeed
            },
            particleAngle,
            0,
            1.0f + ((float)rand() / RAND_MAX) * 2.0f,
            0.2f + ((float)rand() / RAND_MAX) * 0.3f,
            (Color){255, 230, 150, 255},
            (Color){255, 100, 0, 0},
            PARTICLE_SPARK
        );
    }
    
    // Create hit effect
    CreateHitEffect(position, 8.0f, (Color){255, 200, 100, 180});
}

void AddDamageFlash(Color color)
{
    game.damageFlashTimer = 0.3f;
    game.damageFlashColor = color;
}