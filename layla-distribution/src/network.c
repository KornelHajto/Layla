#include "../include/common.h"
#include "../include/network.h"
#include "../include/player.h"
#include "../include/weapons.h"
#include "../include/core.h"
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

// Platform-specific includes
#ifdef _WIN32
    #include <io.h>
    #define fcntl(fd, cmd, ...) 0  // Simplified for Windows
    #define O_NONBLOCK 0
#else
    #include <fcntl.h>
    #include <unistd.h>
#endif

int StartHost(int port)
{
    // Close existing socket if it's open
    if (game.socket_fd >= 0) {
        close(game.socket_fd);
    }
    
    // Create UDP socket
    game.socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (game.socket_fd < 0) {
        return -1;
    }
    
    // Set socket to non-blocking
    int flags = fcntl(game.socket_fd, F_GETFL, 0);
    fcntl(game.socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Initialize server address
    memset(&game.serverAddr, 0, sizeof(game.serverAddr));
    game.serverAddr.sin_family = AF_INET;
    game.serverAddr.sin_port = htons(port);
    game.serverAddr.sin_addr.s_addr = INADDR_ANY;
    
    // Bind socket to port
    if (bind(game.socket_fd, (struct sockaddr*)&game.serverAddr, sizeof(game.serverAddr)) < 0) {
        close(game.socket_fd);
        game.socket_fd = -1;
        return -1;
    }
    
    // Initialize client list
    game.clientCount = 0;
    game.hostPort = port;
    
    // Reset packet counters
    game.packetsSent = 0;
    game.packetsReceived = 0;
    
    return 0;
}

int ConnectToServer(const char* ip, int port)
{
    // Close existing socket if it's open
    if (game.socket_fd >= 0) {
        close(game.socket_fd);
    }
    
    // Create UDP socket
    game.socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (game.socket_fd < 0) {
        return -1;
    }
    
    // Set socket to non-blocking
    int flags = fcntl(game.socket_fd, F_GETFL, 0);
    fcntl(game.socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Initialize server address
    memset(&game.serverAddr, 0, sizeof(game.serverAddr));
    game.serverAddr.sin_family = AF_INET;
    game.serverAddr.sin_port = htons(port);
    
    // Convert IP address (accept both localhost and IP addresses)
    if (strcmp(ip, "localhost") == 0) {
        // Use 127.0.0.1 for localhost
        if (inet_pton(AF_INET, "127.0.0.1", &game.serverAddr.sin_addr) <= 0) {
            close(game.socket_fd);
            game.socket_fd = -1;
            return -1;
        }
    } else if (inet_pton(AF_INET, ip, &game.serverAddr.sin_addr) <= 0) {
        close(game.socket_fd);
        game.socket_fd = -1;
        return -1;
    }
    
    // Store server info
    strcpy(game.joinIP, ip);
    game.joinPort = port;
    
    // Reset packet counters
    game.packetsSent = 0;
    game.packetsReceived = 0;
    
    return 0;
}

void CloseNetwork(void)
{
    if (game.socket_fd >= 0) {
        // If we're connected, send a leave message
        if (game.isConnected) {
            NetworkMessage leaveMsg;
            leaveMsg.type = MSG_PLAYER_LEAVE;
            strcpy(leaveMsg.playerId, game.localPlayerId);
            
            if (game.isHost) {
                // Send to all clients
                for (int i = 0; i < game.clientCount; i++) {
                    SendMessage(&leaveMsg, &game.clientAddrs[i]);
                }
            } else {
                // Send to server
                SendMessage(&leaveMsg, &game.serverAddr);
            }
        }
        
        close(game.socket_fd);
        game.socket_fd = -1;
    }
    
    game.isConnected = false;
    game.isHost = false;
}

void UpdateNetwork(void)
{
    if (!game.isConnected || game.socket_fd < 0) {
        return;
    }
    
    // Send regular updates
    static float updateTimer = 0;
    static float pingTimer = 0;
    static float reconnectTimer = 0;
    static float gameModeTimer = 0;
    static float flagUpdateTimer = 0;
    static int failedPackets = 0;
    
    float dt = GetFrameTime();
    updateTimer += dt;
    pingTimer += dt;
    reconnectTimer += dt;
    gameModeTimer += dt;
    flagUpdateTimer += dt;
    
    // Send player updates every 33ms (30Hz) for smoother gameplay
    if (updateTimer >= 0.033f) {
        updateTimer = 0;
        
        Player* localPlayer = FindPlayer(game.localPlayerId);
        if (localPlayer && localPlayer->active) {
            NetworkMessage updateMsg;
            updateMsg.type = MSG_PLAYER_UPDATE;
            strcpy(updateMsg.playerId, game.localPlayerId);
            updateMsg.data.player = *localPlayer;
            
            if (game.isHost) {
                // Send to all clients
                for (int i = 0; i < game.clientCount; i++) {
                    SendMessage(&updateMsg, &game.clientAddrs[i]);
                }
            } else {
                // Send to server
                SendMessage(&updateMsg, &game.serverAddr);
            }
        }
    }
    
    // Send ping every 1 second
    if (pingTimer >= 1.0f) {
        pingTimer = 0;
        
        NetworkMessage pingMsg;
        pingMsg.type = MSG_PING;
        strcpy(pingMsg.playerId, game.localPlayerId);
        pingMsg.data.ping = 0;
        
        game.pingStartTime = GetTime();
        
        if (game.isHost) {
            // Send to all clients
            for (int i = 0; i < game.clientCount; i++) {
                SendMessage(&pingMsg, &game.clientAddrs[i]);
            }
        } else {
            // Send to server
            SendMessage(&pingMsg, &game.serverAddr);
        }
    }
    
    // Send game mode updates periodically (only if host)
    if (game.isHost && gameModeTimer >= 5.0f) {
        gameModeTimer = 0;
        
        // Send current game mode
        NetworkMessage modeMsg;
        modeMsg.type = MSG_GAME_MODE;
        strcpy(modeMsg.playerId, game.localPlayerId);
        modeMsg.data.gameMode = game.mode;
        
        // Send team scores
        NetworkMessage scoreMsg;
        scoreMsg.type = MSG_TEAM_SCORE;
        strcpy(scoreMsg.playerId, game.localPlayerId);
        scoreMsg.data.teamScores[0] = game.teamScores[0];
        scoreMsg.data.teamScores[1] = game.teamScores[1];
        
        for (int i = 0; i < game.clientCount; i++) {
            SendMessage(&modeMsg, &game.clientAddrs[i]);
            SendMessage(&scoreMsg, &game.clientAddrs[i]);
        }
    }
    
    // Send flag updates for Capture the Flag mode
    if (game.mode == MODE_CAPTURE_FLAG && flagUpdateTimer >= 0.5f) {
        flagUpdateTimer = 0;
        
        // Send flag states
        for (int i = 0; i < 2; i++) {
            NetworkMessage flagMsg;
            flagMsg.type = MSG_FLAG_UPDATE;
            strcpy(flagMsg.playerId, game.localPlayerId);
            flagMsg.data.flag = game.flags[i];
            flagMsg.data.flagIndex = i;
            
            if (game.isHost) {
                for (int j = 0; j < game.clientCount; j++) {
                    SendMessage(&flagMsg, &game.clientAddrs[j]);
                }
            } else {
                SendMessage(&flagMsg, &game.serverAddr);
            }
        }
    }
    
    // Receive messages
    struct sockaddr_in senderAddr;
    socklen_t senderAddrLen = sizeof(senderAddr);
    NetworkMessage recvMsg;
    
    while (1) {
        int bytesReceived = recvfrom(game.socket_fd, &recvMsg, sizeof(recvMsg), 0,
                                    (struct sockaddr*)&senderAddr, &senderAddrLen);
        
        if (bytesReceived <= 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                // Error
                failedPackets++;
                SetStatusMessage("Network error: %s", strerror(errno));
                
                // If too many consecutive failed packets, try to reconnect
                if (failedPackets > 20 && reconnectTimer >= 5.0f) {
                    reconnectTimer = 0;
                    if (game.isHost) {
                        // Restart hosting
                        int port = game.hostPort;
                        CloseNetwork();
                        if (StartHost(port) == 0) {
                            SetStatusMessage("Network connection reestablished (host)");
                            game.isHost = true;
                            game.isConnected = true;
                            failedPackets = 0;
                        }
                    } else {
                        // Reconnect to server
                        char ip[16];
                        int port = game.joinPort;
                        strncpy(ip, game.joinIP, sizeof(ip));
                        CloseNetwork();
                        if (ConnectToServer(ip, port) == 0) {
                            SetStatusMessage("Network connection reestablished (client)");
                            game.isHost = false;
                            game.isConnected = true;
                            failedPackets = 0;
                            
                            // Resend join message
                            NetworkMessage joinMsg;
                            joinMsg.type = MSG_PLAYER_JOIN;
                            strcpy(joinMsg.playerId, game.localPlayerId);
                            Player* localPlayer = FindPlayer(game.localPlayerId);
                            if (localPlayer) {
                                joinMsg.data.player = *localPlayer;
                                SendMessage(&joinMsg, &game.serverAddr);
                            }
                        }
                    }
                }
            }
            break;
        }
        
        // Process the message
        ProcessMessage(&recvMsg, &senderAddr);
        game.packetsReceived++;
        failedPackets = 0; // Reset failed packets counter on successful receive
    }
}

void SendMessage(NetworkMessage* message, struct sockaddr_in* destAddr)
{
    if (!game.isConnected || game.socket_fd < 0) {
        return;
    }
    
    // Send the message
    sendto(game.socket_fd, message, sizeof(NetworkMessage), 0,
           (struct sockaddr*)destAddr, sizeof(struct sockaddr_in));
    
    game.packetsSent++;
}

void ProcessMessage(NetworkMessage* message, struct sockaddr_in* senderAddr)
{
    // Validate message before processing
    if (!message || !senderAddr) {
        return;
    }
    
    // Add a timestamp for debugging
    static char timeBuffer[64];
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", timeinfo);
    
    // For storing game mode data
    GameMode receivedMode;
    Flag receivedFlag;
    
    switch (message->type) {
        case MSG_PLAYER_JOIN: {
            // Add the player
            Player* player = CreatePlayer(message->playerId, message->data.player.name, false);
            
            if (player) {
                // Copy player data
                *player = message->data.player;
                player->isLocal = false;
                player->active = true;
                
                // If we're the host, add the client to our list
                if (game.isHost) {
                    bool clientExists = false;
                    for (int i = 0; i < game.clientCount; i++) {
                        if (game.clientAddrs[i].sin_addr.s_addr == senderAddr->sin_addr.s_addr &&
                            game.clientAddrs[i].sin_port == senderAddr->sin_port) {
                            clientExists = true;
                            break;
                        }
                    }
                    
                    if (!clientExists && game.clientCount < MAX_PLAYERS) {
                        game.clientAddrs[game.clientCount] = *senderAddr;
                        game.clientCount++;
                        
                        // Send all existing players to the new client
                        for (int i = 0; i < MAX_PLAYERS; i++) {
                            if (game.players[i].active && strcmp(game.players[i].id, message->playerId) != 0) {
                                NetworkMessage playerMsg;
                                playerMsg.type = MSG_PLAYER_JOIN;
                                strcpy(playerMsg.playerId, game.players[i].id);
                                playerMsg.data.player = game.players[i];
                                SendMessage(&playerMsg, senderAddr);
                            }
                        }
                    
                        // Send current game mode to the new client
                        NetworkMessage modeMsg;
                        modeMsg.type = MSG_GAME_MODE;
                        strcpy(modeMsg.playerId, game.localPlayerId);
                        modeMsg.data.gameMode = game.mode;
                        SendMessage(&modeMsg, senderAddr);
                    
                        // Send team scores to the new client
                        NetworkMessage scoreMsg;
                        scoreMsg.type = MSG_TEAM_SCORE;
                        strcpy(scoreMsg.playerId, game.localPlayerId);
                        scoreMsg.data.teamScores[0] = game.teamScores[0];
                        scoreMsg.data.teamScores[1] = game.teamScores[1];
                        SendMessage(&scoreMsg, senderAddr);
                    
                        // If it's capture the flag mode, send flag states
                        if (game.mode == MODE_CAPTURE_FLAG) {
                            for (int i = 0; i < 2; i++) {
                                NetworkMessage flagMsg;
                                flagMsg.type = MSG_FLAG_UPDATE;
                                strcpy(flagMsg.playerId, game.localPlayerId);
                                flagMsg.data.flag = game.flags[i];
                                flagMsg.data.flagIndex = i;
                                SendMessage(&flagMsg, senderAddr);
                            }
                        }
                    }
                }
                
                SetStatusMessage("Player %s joined", message->playerId);
            }
            break;
        }
            
        case MSG_PLAYER_LEAVE: {
            // Remove the player
            RemovePlayer(message->playerId);
            SetStatusMessage("Player %s left", message->playerId);
            break;
        }
            
        case MSG_PLAYER_UPDATE: {
            // Update player
            Player* player = FindPlayer(message->playerId);
            if (player && !player->isLocal) {
                // Copy position, velocity, rotation
                player->position = message->data.player.position;
                player->velocity = message->data.player.velocity;
                player->rotation = message->data.player.rotation;
                player->health = message->data.player.health;
                player->currentWeapon = message->data.player.currentWeapon;
                player->isReloading = message->data.player.isReloading;
                player->reloadTimer = message->data.player.reloadTimer;
                
                // Forward this to all clients if we're the host
                if (game.isHost) {
                    for (int i = 0; i < game.clientCount; i++) {
                        if (game.clientAddrs[i].sin_addr.s_addr != senderAddr->sin_addr.s_addr ||
                            game.clientAddrs[i].sin_port != senderAddr->sin_port) {
                            SendMessage(message, &game.clientAddrs[i]);
                        }
                    }
                }
            }
            break;
        }
            
        case MSG_PLAYER_SHOOT: {
            // Create bullet
            CreateBullet(
                message->playerId,
                message->data.bullet.position,
                message->data.bullet.rotation,
                message->data.bullet.damage,
                message->data.bullet.color
            );
            
            // Forward this to all clients if we're the host
            if (game.isHost) {
                for (int i = 0; i < game.clientCount; i++) {
                    if (game.clientAddrs[i].sin_addr.s_addr != senderAddr->sin_addr.s_addr ||
                        game.clientAddrs[i].sin_port != senderAddr->sin_port) {
                        SendMessage(message, &game.clientAddrs[i]);
                    }
                }
            }
            break;
        }
            
        case MSG_PING: {
            // Respond with pong
            if (game.isHost || (strcmp(message->playerId, game.localPlayerId) != 0)) {
                NetworkMessage pongMsg;
                pongMsg.type = MSG_PONG;
                strcpy(pongMsg.playerId, message->playerId);
                pongMsg.data.ping = message->data.ping;
                SendMessage(&pongMsg, senderAddr);
            }
            break;
        }
            
        case MSG_PONG: {
            // Calculate ping
            if (strcmp(message->playerId, game.localPlayerId) == 0) {
                double now = GetTime();
                game.ping = (float)((now - game.pingStartTime) * 1000.0);
                game.lastPingTime = now;
            }
            break;
        }
        
        case MSG_GAME_MODE: {
            // Update game mode
            receivedMode = (GameMode)message->data.gameMode;
            
            // Only host can change game mode, or accept from host if client
            if ((game.isHost && strcmp(message->playerId, game.localPlayerId) != 0) || 
                (!game.isHost && game.mode != receivedMode)) {
                
                SwitchGameMode(receivedMode);
                
                // Forward to other clients if we're the host
                if (game.isHost) {
                    for (int i = 0; i < game.clientCount; i++) {
                        if (game.clientAddrs[i].sin_addr.s_addr != senderAddr->sin_addr.s_addr ||
                            game.clientAddrs[i].sin_port != senderAddr->sin_port) {
                            SendMessage(message, &game.clientAddrs[i]);
                        }
                    }
                }
            }
            break;
        }
        
        case MSG_TEAM_SCORE: {
            // Update team scores
            if (game.mode == MODE_TEAM_DEATHMATCH || game.mode == MODE_CAPTURE_FLAG) {
                game.teamScores[0] = message->data.teamScores[0];
                game.teamScores[1] = message->data.teamScores[1];
                
                // Forward to other clients if we're the host
                if (game.isHost) {
                    for (int i = 0; i < game.clientCount; i++) {
                        if (game.clientAddrs[i].sin_addr.s_addr != senderAddr->sin_addr.s_addr ||
                            game.clientAddrs[i].sin_port != senderAddr->sin_port) {
                            SendMessage(message, &game.clientAddrs[i]);
                        }
                    }
                }
            }
            break;
        }
        
        case MSG_FLAG_UPDATE: {
            // Update flag state for CTF mode
            if (game.mode == MODE_CAPTURE_FLAG) {
                int flagIndex = message->data.flagIndex;
                if (flagIndex >= 0 && flagIndex < 2) {
                    game.flags[flagIndex] = message->data.flag;
                    
                    // Forward to other clients if we're the host
                    if (game.isHost) {
                        for (int i = 0; i < game.clientCount; i++) {
                            if (game.clientAddrs[i].sin_addr.s_addr != senderAddr->sin_addr.s_addr ||
                                game.clientAddrs[i].sin_port != senderAddr->sin_port) {
                                SendMessage(message, &game.clientAddrs[i]);
                            }
                        }
                    }
                }
            }
            break;
        }
        
        case MSG_CHAT: {
            // Add received chat message
            AddChatMessage(message->data.chatMessage, message->data.senderName);
            
            // Forward to other clients if we're the host
            if (game.isHost) {
                for (int i = 0; i < game.clientCount; i++) {
                    if (game.clientAddrs[i].sin_addr.s_addr != senderAddr->sin_addr.s_addr ||
                        game.clientAddrs[i].sin_port != senderAddr->sin_port) {
                        SendMessage(message, &game.clientAddrs[i]);
                    }
                }
            }
            break;
        }
    }
}

// Function moved to core.c - declared in core.h

void GeneratePlayerId(char* playerId)
{
    static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    // Use current time and pid to create better randomness
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)getpid();
    seed ^= (uintptr_t)playerId; // Mix in the pointer address for better entropy
    srand(seed);
    
    // Generate a unique prefix to reduce collision chance
    unsigned long uniquifier = (unsigned long)time(NULL) % 100000;
    char prefix[6];
    snprintf(prefix, sizeof(prefix), "%05lu", uniquifier);
    
    // Copy prefix
    for (int i = 0; i < 5; i++) {
        playerId[i] = prefix[i];
    }
    
    // Generate random part for the rest of the ID
    for (int i = 5; i < 15; i++) {
        playerId[i] = chars[rand() % (sizeof(chars) - 1)];
    }
    playerId[15] = '\0';
}