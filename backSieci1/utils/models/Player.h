#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <nlohmann/json.hpp>

// Struktura reprezentująca gracza
struct Player
{
    const int client_fd;
    bool is_ready; // Czy gracz jest gotowy - to ustawienie tyczy się lobby
    std::string nickname;
    int round_score = 0;           // Punkty zdobyte w rundzie - to ustawienie tyczy się gry
    int game_score = 0;            // Punkty zdobyte w grze - to ustawienie tyczy się gry
    bool is_timer_running = false; // Flaga wskazująca, czy timer działa
    inline static const int ready_timer_seconds = 30;
    bool guessed = false;

    Player(int fd, std::string name, bool ready = false);

    nlohmann::json toJson() const;

    void reset();

    void startReadyTimer(int seconds, std::function<void()> callback);
};

// Funkcja hashująca na potrzeby unikalności w secie graczy
struct PlayerHash
{
    size_t operator()(const Player *player) const;
};

#endif // PLAYER_H
