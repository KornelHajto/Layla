#ifndef PARTICLES_H
#define PARTICLES_H

#include "common.h"

// Particle system functions
void UpdateParticles(void);
void UpdateMuzzleFlashes(void);
void UpdateHitEffects(void);
void DrawParticles(void);
void DrawMuzzleFlashes(void);
void DrawHitEffects(void);

// Particle creation functions
void CreateParticle(Vector2 position, Vector2 velocity, float rotation, float rotationSpeed, 
                    float size, float lifetime, Color startColor, Color endColor, ParticleType type);
void CreateMuzzleFlash(Vector2 position, float rotation, float size, Color color, const char* ownerId);
void CreateHitEffect(Vector2 position, float size, Color color);
void CreateBloodSplatter(Vector2 position, Vector2 direction, int count);
void CreateSparkEffect(Vector2 position, Vector2 direction, int count);

// Visual effects
void AddDamageFlash(Color color);

#endif // PARTICLES_H