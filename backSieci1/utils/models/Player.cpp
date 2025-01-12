#include "Player.h"
#include <iostream>
#include <thread>
#include <websocket_utils.h>
#include "MessageTypes.h"

Player::Player(int fd, std::string name, bool ready)
    : client_fd(fd), nickname(name), is_ready(ready) {}

nlohmann::json Player::toJson() const
{
    return nlohmann::json{
        {"nickname", nickname},
        {"isReady", is_ready},
        {"roundScore", round_score},
        {"gameScore", game_score},
    };
}

void Player::reset()
{
    this->is_ready = false;
    this->round_score = 0;
    this->game_score = 0;
}

void Player::startReadyTimer(int seconds, std::function<void()> callback)
{
    if (is_timer_running)
    {
        std::cerr << "Timer for player " << nickname << " is already running." << std::endl;
        return;
    }

    is_timer_running = true; // Ustaw flagę
    std::thread([this, seconds, callback]()
                {
                    // Wysyłanie aktualizacji czasu do gracza
                    nlohmann::json timerUpdateMessage = {
                        {"type", WsServerMessageType::ReadyTimer},
                        {"time", seconds}};

                    send_webscoket_message_inframe(this->client_fd, timerUpdateMessage.dump());

                    int remainingTime = seconds;

                    while (remainingTime > 0)
                    {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        remainingTime--;

                        // Jeśli gracz jest gotowy, zatrzymaj timer
                        if (this->is_ready)
                        {
                            std::cout << "Player " << this->nickname << " is ready. Timer stopped." << std::endl;
                            is_timer_running = false;
                            return;
                        }
                    }

                    // Jeśli timer się skończył i gracz nadal nie jest gotowy
                    if (!this->is_ready)
                    {
                        std::cout << "Player " << this->nickname << " did not set ready in time." << std::endl;
                        if (callback)
                        {
                            callback(); // Wywołaj akcję po zakończeniu czasu
                        }
                    }

                    is_timer_running = false; // Zresetuj flagę
                })
        .detach(); // Oddziel wątek, aby nie blokować głównego procesu
}

size_t PlayerHash::operator()(const Player *player) const
{
    return std::hash<std::string>()(player->nickname);
}
