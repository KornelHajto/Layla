#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"

// Player management functions
Player* FindPlayer(const char* playerId);
Player* CreatePlayer(const char* playerId, const char* playerName, bool isLocal);
void RemovePlayer(const char* playerId);
void UpdatePlayers(void);
void DrawPlayers(void);

#endif // PLAYER_H