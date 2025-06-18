#ifndef CORE_H
#define CORE_H

#include "common.h"

// Core game functions
void InitGame(void);
void UpdateGame(void);
void DrawGame(void);
void DrawGameBackground(void);
void HandleInput(void);

// Game mode functions
void InitGameMode(GameMode mode);
void UpdateGameMode(void);
void DrawGameMode(void);
const char* GetGameModeName(GameMode mode);
void SwitchGameMode(GameMode mode);
void ResetGameMode(void);

// UI functions
void DrawMenu(void);
void DrawNameInput(void);
void DrawHostSetup(void);
void DrawJoinSetup(void);
void DrawGameModeMenu(void);
void DrawUI(void);

// Utility functions
void SetStatusMessage(const char* format, ...);
void AddChatMessage(const char* message, const char* senderName);

#endif // CORE_H