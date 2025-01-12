#include "Lobby.h"
#include "Player.h"
#include <iostream>

Lobby::Lobby(int id) : lobby_id(id), game(this) {} // Konstruktor, który inicjalizuje game przekazując wskaźnik na to lobby

struct ChatMessage
{
    std::string sender;
    std::string content;
};
std::vector<ChatMessage> chatMessages; // Lista wiadomości czatu

void Lobby::addChatMessage(const std::string &sender, const std::string &content)
{
    ChatMessage message = {sender, content};
    chatMessages.push_back(message);

    // Opcjonalnie: Możesz ograniczyć liczbę przechowywanych wiadomości, np. do 100:
    if (chatMessages.size() > 100)
    {
        chatMessages.erase(chatMessages.begin());
    }
}

bool Lobby::addPlayer(Player *player)
{
    if (player && players.size() < max_players)
    {
        players.insert(player);
        return true;
    }
    return false;
}

Player *Lobby::getPlayerByClientFd(int client_fd)
{
    for (auto player : players)
    {
        if (player->client_fd == client_fd)
        {
            return player;
        }
    }
    return nullptr;
}
// Jeżeli gra już trwa, nie ma wystarczającej liczby graczy lub któryś z graczy nie jest gotowy
// to gra nie może się rozpocząć
bool Lobby::checkIfCanStartGame()
{
    if (this->is_in_game || this->players.size() < min_players)
    {
        return false;
    }
    for (const auto &player : players)
    {
        if (!player->is_ready)
        {
            return false;
        }
    }
    return true;
}

bool Lobby::checkIfHasPlayer(Player *player) const
{
    return players.count(player);
}

nlohmann::json Lobby::toJsonPlayers() const
{
    nlohmann::json jPlayers = nlohmann::json::array();
    for (const auto &playerPtr : this->players)
    {
        if (playerPtr)
        {
            jPlayers.push_back(playerPtr->toJson());
        }
    }
    return jPlayers;
}

nlohmann::json Lobby::toJson() const
{
    nlohmann::json j;
    j["lobby_id"] = lobby_id;
    j["players"] = nlohmann::json::array();
    for (const auto &player : players)
    {
        if (player)
        {
            j["players"].push_back(player->toJson());
        }
    }
    j["game"] = game.toJson();
    j["is_in_game"] = is_in_game;
    j["min_players"] = min_players;
    j["max_players"] = max_players;
    return j;
}

int Lobby::removePlayer(Player *player)
{
    this->game.previous_drawers.erase(player);
    return this->players.erase(player);
};
