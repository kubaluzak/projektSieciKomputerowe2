#ifndef GAME_H
#define GAME_H

#include <unordered_set>
#include <string>
#include "Player.h"
#include "DrawingBoard.h"

struct Lobby; // Forward declaration dla Lobby, żeby uniknąć cyklicznych zależności

struct Game
{
    Player *current_drawer = nullptr;
    int current_round = 0;
    int MAX_ROUNDS = 8;
    bool is_timer_running = false; // Flaga wskazująca, czy timer działa
    std::string word_to_draw;
    std::unordered_set<Player *> previous_drawers; // Zbiór graczy, którzy już byli rysującymi
    DrawingBoard drawing_board;                    // Obiekt planszy do rysowania
    std::string checkSum;
    inline static const int round_time = 5;
    int remaining_round_time;
    inline static const int max_rounds = 8; // Maksymalna liczba rund w grze
    Lobby *lobby = nullptr;                 // Wskaźnik na powiązane lobby
    std::function<void(Player*, int)> kickInactivePlayerCallback;
    
    void startDrawingTimer(int seconds, std::function<void()> callback);
    void startNewRound(std::function<void(Player *, int)> kickInactivePlayerCallback);
    void endGame(std::function<void(Player*, int)> kickInactivePlayerCallback);
    void endRound(std::function<void(Player *, int)> kickInactivePlayerCallback);

    Game(Lobby *lobby);

    bool startNewGame(std::function<void(Player *, int)> kickInactivePlayerCallback);       // Rozpocznij nową grę
    Player *setRandomDrawer(); // Wybierz nowego gracza do rysowania
    void reset();              // Zresetuj grę do stanu początkowego
    nlohmann::json toJson() const;
};

#endif // GAME_H
