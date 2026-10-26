#pragma once
#include <Vagante/game.h>
#include <Vagante/input.h>
#include <thread>

struct ClientConnectionData {
    bool isHost;
    bool active;
    bool timedOut;
    int badConnection;
    ulong64 timeBeforeGoodConnectionAgain;
    ulong64 lastBadConnectionTime;
    ulong64 goodConnectionStartTime;
    Netchan netChan;
    sf::IpAddress remoteIp;
    ushort remoteUdpPort;
    ushort remoteTcpPort;
    CSteamID steamIDUser;
    uint frameCountLastData;
    std::string playerName;
    std::shared_ptr<Player> player;
    bool loadedLevel;
    bool hasWrongLevelInfo;
    int hasWrongLevelInfoCooldown;
    std::vector<Snapshot> snapshots;
    std::vector<TileChange> tileChanges;
    std::vector<WaterChange> waterChanges;
    uint deltaMessage;
    int lastSendFullSnapshot;
};

struct Server {
    SteamCallback m_steamcallback_onSteamServersConnected;
    SteamCallback m_steamcallback_onSteamServersConnectFailure;
    SteamCallback m_steamcallback_onSteamServersDisconnected;
    SteamCallback m_steamcallback_onPolicyResponse;
    SteamCallback m_steamcallback_onValidateAuthTicketResponse;
    SteamCallback m_steamcallback_onP2PSessionRequest;
    SteamCallback m_steamcallback_onP2PSessionConnectFail;
    SteamCallResult m_SteamCallResultLobbyCreated;
    bool usingSteam;
    ServerState state;
    Input input;
    std::thread thread;
    ulong64 currentTime;
    uint sentGameFrame;
    sf::UdpSocket mainServerSocket;
    sf::TcpListener tcpListener;
    sf::TcpSocket* nextTcpSocket;
    std::vector<sf::TcpSocket*> tcpConnections;
    CSteamID steamIDLobby;
    CSteamID gameHostSteamID;
    GameEngine gameEngine;
    bool newGameRequested;
    bool publicGame;
    bool defaultHardMode;
    ushort currEntityId;
    std::vector<ulong64> netIdFreeTime;
    std::vector<bool> netIdUsed;
    std::map<int, std::shared_ptr<Entity>> netEntities;
    std::vector<std::shared_ptr<Player>> debugFakePlayers;
    std::vector<int> debugFakePlayerActions;
    std::string serverName;
    bool connectedToSteam;
    bool hostConnected;
    std::vector<ClientConnectionData*> clientData;
    std::vector<ClientConnectionData*> pendingClientData;
};
