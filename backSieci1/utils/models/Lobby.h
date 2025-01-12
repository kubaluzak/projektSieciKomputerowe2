#ifndef LOBBY_H
#define LOBBY_H

#include <unordered_set>
#include <string>
#include <nlohmann/json.hpp>
#include "Player.h"
#include "Game.h"

using PlayerSet = std::unordered_set<Player *, PlayerHash>;

struct Lobby
{
    int lobby_id;
    PlayerSet players;
    Game game; // Gra powiązana z tym lobby
    bool is_in_game = false;

    inline static const int min_players = 2; // Minimalna ilość graczy do rozpoczęcia gry
    inline static const int max_players = 3; // Maksymalna ilość graczy w lobby

    Lobby(int id);

    bool addPlayer(Player *player);
    Player *getPlayerByClientFd(int client_fd);
    bool checkIfCanStartGame();
    bool checkIfHasPlayer(Player *player) const;
    nlohmann::json toJsonPlayers() const;
    nlohmann::json toJson() const;
    void addChatMessage(const std::string &sender, const std::string &content);
    int removePlayer(Player *player);
};

#endif // LOBBY_H
