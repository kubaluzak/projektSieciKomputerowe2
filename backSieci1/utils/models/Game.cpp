#include "Game.h"
#include "WordUtils.h"
#include "Lobby.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <MessageTypes.h>
#include <websocket_utils.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Game::Game(Lobby *lobby) : lobby(lobby) {}

bool Game::startNewGame()
{
    if (!this->lobby)
    {
        std::cerr << "Nie ma lobby powiązanego z grą" << std::endl;
        return false;
    }

    this->lobby->is_in_game = true;
    this->startNewRound();

    std::cout << "Nowa gra zaczęła się w lobby o id " << this->lobby->lobby_id
              << ". Pierwszy rysujący: " << current_drawer->nickname << std::endl;

    return true;
}

Player *Game::setRandomDrawer()
{
    if (!this->lobby)
    {
        return nullptr;
    }

    std::vector<Player *> availablePlayers;

    for (auto player : this->lobby->players)
    {
        // Wybiera gracza spośród tych, którzy nie byli wcześniej rysującymi
        if (player != this->current_drawer && this->previous_drawers.find(player) == this->previous_drawers.end())
        {
            availablePlayers.push_back(player);
        }
    }


    if (availablePlayers.empty())
    {
        return nullptr; // Brak graczy mogących zostać rysującymi
    }

    auto new_drawer = availablePlayers[rand() % availablePlayers.size()];

    this->current_drawer = new_drawer;
    this->previous_drawers.insert(new_drawer);
    return this->current_drawer;
}

void Game::reset()
{
    current_drawer = nullptr;
    current_round = 0;
    word_to_draw.clear();
    previous_drawers.clear();
    drawing_board.reset();

    // Resetowanie stanu graczy w lobby
    if (lobby)
    {
        lobby->is_in_game = false;
        for (auto player : lobby->players)
        {
            player->reset();
        }
    }
}

// Timer dla rysującego
void Game::startDrawingTimer(int seconds, std::function<void()> callback)
{
    if (!this->lobby)
    {
        std::cerr << "Brak lobby powiązanego z grą." << std::endl;
        return;
    }

    // Wysyłanie aktualizacji czasu do wszystkich graczy
    // nlohmann::json gameTimerMessage = {
    //     {"type", WsServerMessageType::GameTimer},
    //     {"time", seconds}};

    // for (const auto &player : lobby->players)
    // {
    //     send_webscoket_message_inframe(player->client_fd, gameTimerMessage.dump());
    // }

    std::thread([this, seconds, callback]()
                {
        int remaining_round_time = seconds;

        while (remaining_round_time > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            remaining_round_time--;
        }

        if (callback) {
             callback();
         } })
        .detach();
}

nlohmann::json Game::toJson() const
{
    nlohmann::json j;
    j["current_drawer"] = current_drawer ? current_drawer->toJson() : ""; // Wymaga, aby Player miał toJson
    j["current_round"] = current_round;
    j["word_to_draw"] = word_to_draw;
    j["previous_drawers"] = nlohmann::json::array();
    for (const auto &drawer : previous_drawers)
    {
        j["previous_drawers"].push_back(drawer->toJson()); // Wymaga, aby Player miał toJson
    }
    // j["drawing_board"] = drawing_board.toJSON(); // Dużo danych
    j["max_rounds"] = max_rounds;
    j["lobby_id"] = lobby ? lobby->lobby_id : -1; // Dodanie referencji do lobby, jeśli istnieje
    return j;
}

void Game::startNewRound()
{
    if (lobby == nullptr)
    {
        return;
    }

    // Reset
    current_round++;
    setRandomDrawer();
    word_to_draw = choose_random_word(words_database);

    json round_start_message = {
        {"type", WsServerMessageType::RoundStart},
        {"roundNumber", current_round},
        {"lettersToGuess", word_to_draw.length()},
        {"roundTime", Game::round_time},
        {"drawer", current_drawer->nickname}};

    for (const auto &player : lobby->players)
    {
        send_webscoket_message_inframe(player->client_fd, round_start_message.dump());
    }

    json drawer_message = {
        {"type", WsServerMessageType::DrawerNotification},
        {"word_to_draw", word_to_draw}};

    send_webscoket_message_inframe(current_drawer->client_fd, drawer_message.dump());

    lobby->game.startDrawingTimer(Game::round_time, [&]()
                                  { this->endRound(); });
}

void Game::endRound()
{
    // Jeśli można to zaczynamy nową rundę
    if (current_round < Game::max_rounds)
    {
        drawing_board.reset();
        current_drawer = nullptr;
        word_to_draw.clear();

        // Resetowanie punktacji rundy dla graczy
        if (lobby)
        {
            for (auto player : lobby->players)
            {
                player->game_score += player->round_score;
                player->round_score = 0;
            }
        }

        this->startNewRound();
    }
    // Jeśli nie to kończymy grę
    else
    {
        this->endGame();
    }
}

void Game::endGame()
{
}
