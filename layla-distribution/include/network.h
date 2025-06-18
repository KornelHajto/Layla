#ifndef NETWORK_H
#define NETWORK_H

#include "common.h"
// Socket headers are already included in common.h

// Network message types and functions
int StartHost(int port);
int ConnectToServer(const char* ip, int port);
void CloseNetwork(void);
void UpdateNetwork(void);
void SendMessage(NetworkMessage* message, struct sockaddr_in* destAddr);
void ProcessMessage(NetworkMessage* message, struct sockaddr_in* senderAddr);

// Network utility functions
void GeneratePlayerId(char* playerId);

#endif // NETWORK_H