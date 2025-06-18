#ifndef WEAPONS_H
#define WEAPONS_H

#include "common.h"

// Weapon functions
WeaponStats* GetCurrentWeaponStats(Player* player);
void SwitchWeapon(Player* player, WeaponType weapon);
void ReloadWeapon(Player* player);
bool CanShoot(Player* player);
void FireWeapon(Player* player);

// Bullet functions
void CreateBullet(const char* ownerId, Vector2 position, float rotation, int damage, Color color);
void UpdateBullets(void);
void DrawBullets(void);

#endif // WEAPONS_H