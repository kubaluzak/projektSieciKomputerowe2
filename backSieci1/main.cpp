#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>
#include <openssl/sha.h>
#include <iomanip>
#include <map>
#include <set>
#include <thread>
#include <unordered_set>
#include <unordered_map>
#include <WordUtils.h>
#include <nlohmann/json.hpp>

#include "utils/websocket_utils.h"
#include "utils/models/Player.h"
#include "utils/models/Game.h"
#include "utils/models/Lobby.h"
#include "utils/models/DrawingBoard.h"
#include "utils/models/MessageTypes.h"
#include <sys/timerfd.h>

#define MAX_EVENTS 10
#define PORT 1234
using json = nlohmann::json;

// mapa z istniejacymi lobby, po usunieciu lobby takze usuwamy je z tej mapy
std::unordered_map<int, std::unique_ptr<Lobby>> lobbies;
// do tego setu trafiaja usuniete lobby, zebysmy wiedzieli ze mozna nadac ich id kolejny raz
std::set<int> free_lobby_ids;
// Mapa deskryptorów gniazd połączeń klienta z obiektem gracza w programie
// Wygodne by przechowywać to tu (poza PlayerSetem players w Lobby oczywiście)
std::unordered_map<int, Player *> client_to_player;
// Sprawdza czy w secie free_lobby_ids znajduje się jakieś wolne id , jeśli tak to je zwraca,
// a jeśli nie no to zwracaja nastepny wolny numerek
int get_next_lobby_id()
{
    if (!free_lobby_ids.empty())
    {
        int id = *free_lobby_ids.begin();
        free_lobby_ids.erase(free_lobby_ids.begin());
        return id;
    }
    return lobbies.size() + 1;
}
// Usuwa lobby jeśli jest puste i dodaje je do setu pustych lobby
void remove_empty_lobby(int lobby_id)
{
    if (lobbies.find(lobby_id) != lobbies.end())
    {
        lobbies.erase(lobby_id);
        free_lobby_ids.insert(lobby_id); // Numer lobby jest znowu dostępny
        std::cout << "Lobby " << lobby_id << " removed and ID recycled." << std::endl;
    }
}
// Wysyła wiadomość do graczy w lobby o zmianie stanu graczy (dołączenie, wyjście, gotowość do gry)
void send_lobby_players_update(int lobby_id)
{
    // Chwila wyjaśnienia - at() zwraca referencję. Lobby &lobby ma ampersant przed sobą bo chcemy zachować to jako referencję (nie tworzyć nowego obiektu)
    // - nie ma takiej potrzeby a tak jest optymalniej. Dodatkowo const zapewnia że dane nie zostaną omyłkowo zmodyfikowane. To samo dla players
    const auto &lobby = lobbies.at(lobby_id);
    const auto &players = lobby->players;
    json notification = {
        {"type", WsServerMessageType::LobbyUpdate},
        {"players", lobby->toJsonPlayers()},
        {"lobbyId", lobby_id}};

    for (const auto &player : players)
    {
        send_webscoket_message_inframe(player->client_fd, notification.dump());
    }
}
// Funkcja drukująca wszystkie lobby i graczy w nich znajdujących się (pomocnicza bo nie wiem jak to zdebugować xd)
void print_all_lobbies_and_players()
{
    std::cout << "Lista wszystkich lobby i graczy w nich znajdujących się:" << std::endl;

    if (lobbies.empty())
    {
        std::cout << "Brak aktywnych lobby." << std::endl;
        return;
    }

    for (const auto &[lobby_id, lobby] : lobbies)
    {
        std::cout << "Lobby ID: " << lobby_id << std::endl;
        if (lobby->players.empty())
        {
            std::cout << "  Brak graczy w tym lobby." << std::endl;
        }
        else
        {
            std::cout << "  Gracze:" << std::endl;
            for (const auto &player : lobby->players)
            {
                std::cout << "    - " << player->nickname << std::endl;
            }
        }
    }
}
int assign_player_to_lobby(int client_fd, Player *player)
{
    if (player == nullptr)
    {
        return -1;
    }

    for (auto &[lobby_id, lobby] : lobbies)
    {
        if (lobby->players.size() < Lobby::max_players)
        {
            // Wyjaśnienie - referencja do nick w argumencie funkcji nam pasuje, bo nie tworzymy nowej lokalnej kopii stringa,
            // a konstruktor obiektu i tak tworzy nam kopię
            lobby->addPlayer(player);
            std::cout << "Gracz " << player->nickname << " przydzielony do istniejącego lobby: " << lobby_id << std::endl;

            // Powiadom graczy w lobby o dołączeniu nowego gracza
            send_lobby_players_update(lobby_id);

            return lobby->lobby_id;
        }
    }

    // Tworzenie nowego lobby
    int new_lobby_id = get_next_lobby_id();
    auto new_lobby = std::make_unique<Lobby>(new_lobby_id);

    if (new_lobby->addPlayer(player))
    {
        lobbies.emplace(new_lobby_id, std::move(new_lobby));
    }
    else
    {
        std::cerr << "Nie udało się dodać gracza do lobby: " << player->nickname << std::endl;
        return -1;
    }

    std::cout << "Gracz " << player->nickname << " przydzielony do nowego lobby: " << new_lobby_id << std::endl;
    print_all_lobbies_and_players();
    // std::cout << new_lobby.toJson() << std::endl;
    std::cout << new_lobby_id << std::endl;
    return new_lobby_id;
}

void handle_disconnect(int client_fd)
{
    std::cout << "Rozłączanie klienta z fd: " << client_fd << std::endl;

    // Sprawdź, czy deskryptor istnieje w mapie
    if (client_to_player.count(client_fd) == 0)
    {
        std::cerr << "Deskryptor fd: " << client_fd << " nie istnieje w client_to_player." << std::endl;
        close(client_fd);
        return;
    }

    // Pobierz wskaźnik do gracza
    Player *playerToRemove = client_to_player[client_fd];

    // Usuń gracza z lobby
    if (playerToRemove != nullptr)
    {
        for (auto &[lobby_id, lobby] : lobbies)
        {
            if (lobby->removePlayer(playerToRemove))
            {
                std::cout << "Gracz " << playerToRemove->nickname << " usunięty z lobby " << lobby_id << std::endl;

                // Usuń puste lobby
                if (lobby->players.empty())
                {
                    remove_empty_lobby(lobby_id);
                }
                else
                {
                    send_lobby_players_update(lobby_id);
                }
                break;
            }
        }

        // Usuń obiekt gracza
        delete playerToRemove;
    }
    else
    {
        std::cerr << "Ostrzeżenie: Gracz powiązany z fd: " << client_fd << " to nullptr." << std::endl;
    }

    // Usuń wpis z mapy
    client_to_player.erase(client_fd);

    // Zamknij deskryptor po usunięciu
    close(client_fd);
    std::cout << "Klient rozłączony i usunięty: fd " << client_fd << std::endl;
}

void handle_round_info_for_new_players(int client_fd, Lobby &lobby)
{
    const int lobby_id = lobby.lobby_id;
    std::cout << lobby.toJson() << std::endl;
    json round_start_message = {
        {"type", WsServerMessageType::RoundStart},
        {"roundNumber", lobby.game.current_round},
        {"lettersToGuess", lobby.game.word_to_draw.length()}
        // remaining czas

    };
    send_webscoket_message_inframe(client_fd, round_start_message.dump());
}

void handle_chat_message_sent(int client_fd, const json &message)
{
    std::string content = message.at("message");
    Player *sender = client_to_player[client_fd];

    if (sender)
    {
        // Znajdź lobby, do którego należy gracz
        int lobby_id = -1;
        for (const auto &[id, lobby] : lobbies)
        {
            if (lobby->checkIfHasPlayer(sender))
            {
                lobby_id = id;
                break;
            }
        }

        if (lobby_id != -1)
        {
            auto &lobby = lobbies.at(lobby_id);

            // Dodaj wiadomość do lobby
            lobby->addChatMessage(sender->nickname, content);

            // Rozgłoś wiadomość do graczy w tym lobby
            json chatMessage = {
                {"type", WsServerMessageType::ChatMessage},
                {"msg_mode", 0},
                {"sender", sender->nickname},
                {"content", content}};

            if (!(content == lobby->game.word_to_draw))
            {
                for (const auto &player : lobby->players)
                {
                    send_webscoket_message_inframe(player->client_fd, chatMessage.dump());
                }
            }
            else if (lobby->game.current_drawer != sender)
            {
                json chatMessage = {
                    {"type", WsServerMessageType::ChatMessage},
                    {"msg_mode", 1}, // Wiadomość serwera
                    {"color_mode", 2},
                    {"content", "Brawo zgadłeś!"}};
                send_webscoket_message_inframe(client_fd, chatMessage.dump());
                sender->game_score += 1;
                lobby->game.current_drawer->game_score += 1;

                json scoreUpdate = {
                    {"type", WsServerMessageType::ScoreUpdate},
                    {"players", lobby->toJsonPlayers()}};

                for (const auto &player : lobby->players)
                {
                    send_webscoket_message_inframe(player->client_fd, scoreUpdate.dump());
                }
            }
        }
        else
        {
            std::cerr << "Nie znaleziono lobby dla gracza: " << sender->nickname << std::endl;
        }
    }
}

bool handle_registration(int client_fd, const json &message)
{
    try
    {

        const std::string nick = message.at("nick");

        // Sprawdź, czy nick jest unikalny
        for (const auto &pair : client_to_player)
        {

            if (pair.second->nickname == nick)
            {
                // Odpowiadamy klientowi, że nick jest zajęty
                json response = {
                    {"type", WsServerMessageType::Error},
                    {"error", "Nickname already taken."}};

                send_webscoket_message_inframe(client_fd, response.dump());

                std::cerr << "Nick " << nick << " jest już zajęty." << std::endl;
                return false;
            }
        }

        Player *new_player = new Player(client_fd, nick);

        int lobbyId = assign_player_to_lobby(client_fd, new_player);

        if (lobbyId < 0)
        {
            std::cerr << "Nick " << nick << " jest już zajęty." << std::endl;
            return false;
        }

        client_to_player[client_fd] = new_player;
        auto &lobby = lobbies.at(lobbyId);

        json settings;
        settings["playersMin"] = Lobby::min_players;
        settings["playersMax"] = Lobby::max_players;

        json response = {
            {"type", WsServerMessageType::LobbyFullInfo},
            {"lobbyId", lobbyId},
            {"players", lobby->toJsonPlayers()},
            {"settings", settings},
            {"isGameStarted", lobby->is_in_game}};

        std::cout << "Sending response: " << response.dump(4) << std::endl; // Debug JSON z wcięciem

        send_webscoket_message_inframe(client_fd, response.dump());

        if (!lobby->is_in_game && lobby->players.size() >= Lobby::min_players)
        {
            for (auto *player : lobby->players)
            {
                if (!player->is_ready && !player->is_timer_running)
                {
                    player->startReadyTimer(Player::ready_timer_seconds, [player, lobbyId]()
                                            {
                                            // to sie wykona jak timer sie skonczy a nie powinien sie skonczyc, bo sie przerwie kiedy da ziomal ready
                                            json kickMessage = {
                                                {"type", WsServerMessageType::Kick},
                                            };
                                            auto &lobby = lobbies.at(lobbyId);
                                            if (lobby->removePlayer(player))
                                            {
                                                send_lobby_players_update(lobbyId);
                                                client_to_player.erase(player->client_fd);
                                                delete player;
                                                send_webscoket_message_inframe(player->client_fd, kickMessage.dump());
                                                // json chatMessage = {
                                                //     {"type", WsServerMessageType::ChatMessage},
                                                //     {"msg_mode", 1}, // Wiadomość serwera
                                                //     {"color_mode", 1},
                                                //     {"content", "Wyrzucono gracza '" + player->nickname + "' z powodu nieaktywności."}};
                                                // for (auto *player : lobby->players){
                                                //     send_webscoket_message_inframe(player->client_fd, chatMessage.dump());  
                                                // }
                                            } });
                }
            }
        }

        if (lobby->is_in_game)
        {
            json updateCanva = {
                {"changedPixels", lobby->game.drawing_board.getChangedPixels()},
                {"type", WsServerMessageType::UpdateCanva}};
            // send_webscoket_message_inframe(client_fd, updateCanva.dump());
            send_changed_pixels(client_fd, lobby->game.drawing_board.getChangedPixels());

            handle_round_info_for_new_players(client_fd, *lobby);
        }
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Błąd podczas rejestracji: " << e.what() << std::endl;
        return false;
    }

    return false; // Jeśli typ nie jest 'register', zwróć false
}

void handle_game_end(Lobby &lobby)
{
    lobby.is_in_game = false;
    lobby.game.current_round = 1;
    lobby.game.previous_drawers.clear();
    lobby.game.drawing_board.pixels.clear();

    for (const auto &player : lobby.players)
    {
        player->is_ready = false;
        player->round_score = 0;
    }

    nlohmann::json endGameMessage = {
        {"type", WsServerMessageType::GameEnd},
        {"message", "Game over! Returning to lobby."},
        {"players", lobby.toJsonPlayers()},
        {"lobbyId", lobby.lobby_id}};

    int lobbyId = lobby.lobby_id;
    for (const auto &player : lobby.players)
    {
        send_webscoket_message_inframe(player->client_fd, endGameMessage.dump());

        player->startReadyTimer(Player::ready_timer_seconds, [player, lobbyId]()
                                {
                                            // to sie wykona jak timer sie skonczy a nie powinien sie skonczyc, bo sie przerwie kiedy da ziomal ready
                                            json kickMessage = {
                                                {"type", WsServerMessageType::Kick},
                                            };
                                            auto &lobby = lobbies.at(lobbyId);
                                            if (lobby->removePlayer(player))
                                            {
                                                send_lobby_players_update(lobbyId);
                                                client_to_player.erase(player->client_fd);
                                                delete player;
                                                send_webscoket_message_inframe(player->client_fd, kickMessage.dump());
                                                // json chatMessage = {
                                                //     {"type", WsServerMessageType::ChatMessage},
                                                //     {"msg_mode", 1}, // Wiadomość serwera
                                                //     {"color_mode", 1},
                                                //     {"content", "Wyrzucono gracza '" + player->nickname + "' z powodu nieaktywności."}};
                                                // for (auto *player : lobby->players){
                                                //     send_webscoket_message_inframe(player->client_fd, chatMessage.dump());  
                                                // }
                                            } });
    }
};

void handle_game_start(Lobby &lobby)
{
    const int lobby_id = lobby.lobby_id;
    std::cout << lobby.toJson() << std::endl;

    if (lobby.game.startNewGame())
    {
        // Powiadomienie graczy o rozpoczęciu gry i wybraniu rysującego
        json game_start_message = {
            {"type", WsServerMessageType::GameStart},
            {"lobbyId", lobby_id}};

        for (const auto &player : lobby.players)
        {
            if (player != nullptr)
            {
                send_webscoket_message_inframe(player->client_fd, game_start_message.dump());
            }
        }
    }
}

void handle_set_ready(int client_fd, const json &message)
{
    int lobby_id = message.at("lobbyId");

    auto it = client_to_player.find(client_fd);
    if (it != client_to_player.end())
    {
        Player *player = it->second;
        auto lobby_it = lobbies.find(lobby_id);
        if (lobby_it != lobbies.end())
        {
            auto &lobby = lobby_it->second;
            if (!lobby->checkIfHasPlayer(player))
            {
                std::cerr << "Podane lobby nie posiada gracza " << player->nickname << std::endl;
                return;
            }
            player->is_ready = true;

            std::cout << "CHECK START" << std::endl;
            std::cout << lobby->checkIfCanStartGame() << std::endl;

            if (lobby->checkIfCanStartGame())
            {
                handle_game_start(*lobby);
            }

            send_lobby_players_update(lobby_id);
        }
    }
}

void handle_drawing(int client_fd, const json &message)
{
    try
    {
        // Pobranie danych o rysowaniu
        int lobby_id = message.at("lobbyId");
        int x = message.at("x").get<int>();
        int y = message.at("y").get<int>();

        std::string color = message.at("color").get<std::string>();

        auto lobby_it = lobbies.find(lobby_id);
        if (lobby_it != lobbies.end())
        {
            auto &lobby = lobby_it->second;

            Player *current_player = lobby->getPlayerByClientFd(client_fd);
            if (current_player && current_player == lobby->game.current_drawer)
            {
                // Aktualizacja planszy
                lobby->game.drawing_board.setPixel(x, y, color);

                // Powiadomienie innych graczy o zmianach na planszy
                json drawing_update = {
                    {"type", WsServerMessageType::DrawingUpdate},
                    {"x", x},
                    {"y", y},
                    {"color", color}};

                for (const auto &player : lobby->players)
                {
                    if (player != current_player)
                    {
                        send_webscoket_message_inframe(player->client_fd, drawing_update.dump());
                    }
                }
            }
            else
            {
                std::cerr << "Gracz o nicku " << current_player->nickname << " nie jest rysującym" << std::endl;
            }
        }
        else
        {
            std::cerr << "Nie znaleziono lobby o ID " << lobby_id << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Błąd przy obsłudze rysowania: " << e.what() << std::endl;
    }
}

void handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read > 0)
    {
        std::cout << "Odebrano " << bytes_read << " bajtów od klienta" << std::endl;

        std::string request(buffer);

        std::vector<uint8_t> data(buffer, buffer + bytes_read); // Kopiowanie danych do wektora uint8_t
        if (is_websocket_close_frame(data))
        {
            std::cout << "Klient wysłał ramkę WebSocket close. Zamykanie połączenia." << std::endl;
            handle_disconnect(client_fd);
            return; // Po zamknięciu połączenia nie kontynuujemy obsługi klienta
        }

        // If WebSocket handshake is not completed, handle it
        if (request.find("Upgrade: websocket") != std::string::npos)
        {
            handle_websocket_handshake(client_fd, request);
        }
        else
        {
            // After the handshake, decode the WebSocket frame before parsing JSON
            try
            {
                std::string decoded_message = decode_websocket_frame(buffer, bytes_read); // Decode WebSocket frame
                std::cout << "Decoded message: " << decoded_message << std::endl;

                json j = json::parse(decoded_message); // Parse the decoded JSON message
                // send_webscoket_message_inframe(client_fd, j.dump()); // Serializing JSON and sending it

                std::string type = j.at("type");

                if (type == WsClientMessageType::Register)
                {
                    // Handle player registration
                    if (handle_registration(client_fd, j))
                    {
                        std::cout << "Gracz zarejestrowany: " << client_fd << std::endl;
                    }
                    else
                    {
                        std::cerr << "Błąd rejestracji gracza: " << client_fd << std::endl;
                    }
                }
                else if (type == WsClientMessageType::SetReady)
                {
                    std::cerr << "=== CHECK SET READY " << client_fd << std::endl;
                    handle_set_ready(client_fd, j);
                }
                else if (type == WsClientMessageType::SendPixels)
                {
                    handle_drawing(client_fd, j);
                }
                else if (type == WsServerMessageType::ChatMessage)
                {
                    handle_chat_message_sent(client_fd, j);
                }
                else if (type == "ACK")
                {
                    handle_ack(client_fd, j);
                }
                else
                {
                    std::cerr << "Nieznany typ wiadomości: " << type << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Błąd przy obsłudze klienta: " << e.what() << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "Błąd odczytu od klienta." << std::endl;
        close(client_fd);
    }
}

int main()
{
    std::cerr << "Serwer wystartował" << std::endl;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Socket creation failed");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) == -1)
    {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("Epoll creation failed");
        close(server_fd);
        return 1;
    }

    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
    {
        perror("Epoll control failed");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    // Tworzenie timerfd
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1)
    {
        perror("Timerfd creation failed");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    // Konfiguracja timerfd
    itimerspec timer_spec{};
    timer_spec.it_value.tv_sec = 0;
    timer_spec.it_value.tv_nsec = 100 * 1000000; // 100 ms
    timer_spec.it_interval.tv_sec = 0;
    timer_spec.it_interval.tv_nsec = 100 * 1000000; // 100 ms

    if (timerfd_settime(timer_fd, 0, &timer_spec, nullptr) == -1)
    {
        perror("Timerfd settime failed");
        close(timer_fd);
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    // Dodanie timerfd do epoll
    epoll_event timer_event{};
    timer_event.events = EPOLLIN;
    timer_event.data.fd = timer_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &timer_event) == -1)
    {
        perror("Epoll control for timer failed");
        close(timer_fd);
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    epoll_event events[MAX_EVENTS];

    while (true)
    {
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count == -1)
        {
            perror("Epoll wait failed");
            break;
        }
        // std::cout << "Epoll zwrócił " << event_count << " zdarzeń" << std::endl;

        for (int i = 0; i < event_count; ++i)
        {
            // std::cout << "Sprawdzam fd: " << events[i].data.fd << std::endl; // Debugging

            if (events[i].data.fd == server_fd)
            {
                sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                if (client_fd == -1)
                {
                    perror("Accept failed");
                    continue;
                }

                epoll_event client_event{};
                client_event.events = EPOLLIN;
                client_event.data.fd = client_fd;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1)
                {
                    perror("Epoll control for client failed");
                    close(client_fd);
                }
                else
                {
                    std::cout << "New client connected: " << client_fd << std::endl;
                    clients_context[client_fd] = ClientContext{.next_message_id = 1};
                }
            }
            else if (events[i].data.fd == timer_fd)
            {
                // Odczytaj dane z timer_fd, aby wyczyścić zdarzenie
                uint64_t expirations;
                ssize_t s = read(timer_fd, &expirations, sizeof(expirations));
                if (s != sizeof(expirations))
                {
                    perror("Read from timer_fd failed");
                }

                // Wywołaj funkcję obsługi timeoutów ACK
                check_ack_timeouts();
            }

            else
            {
                handle_client(events[i].data.fd);
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    return 0;
}
