#ifndef WEBSOCKET_UTILS_H
#define WEBSOCKET_UTILS_H

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace std::chrono::_V2 {
    struct steady_clock;
}

constexpr size_t BUFFER_SIZE = 102444;

std::string base64_encode(const unsigned char *input, size_t len);
void send_webscoket_message_inframe(int client_fd, const std::string &json_message);
std::string generate_websocket_accept_key(const std::string &key);
void handle_websocket_handshake(int client_fd, const std::string &request);
bool is_websocket_close_frame(const std::vector<uint8_t> &data);
std::string decode_websocket_frame(const char *buffer, int size);
bool is_handshake_request(const char *buffer, size_t bytes_read);
void handle_ack(int client_fd, const nlohmann::json &message);

std::pair<std::string, int> check_ack_timeouts();
void send_changed_pixels(int client_fd, const nlohmann::json &changed_pixels);

// Definicja struktur
struct PendingMessage {
    std::string message;
    std::chrono::time_point<std::chrono::_V2::steady_clock> timestamp;
    int retries;
};

struct ClientContext {
    int next_message_id;
    std::unordered_map<int, PendingMessage> pending_ack;
};

// Deklaracja globalnej mapy do zarządzania klientami
extern std::unordered_map<int, ClientContext> clients_context;

#endif // WEBSOCKET_UTILS_H
