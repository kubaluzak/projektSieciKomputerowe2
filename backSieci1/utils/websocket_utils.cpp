#include "websocket_utils.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <cstring>
#include <MessageTypes.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// funkcja która zmienia dane binarne na dane tekstowe , ponieważ są łatwiejsze do przesłania w takiej formie http

std::string base64_encode(const unsigned char *input, size_t len)
{
    static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int i = 0;
    unsigned char char_array_3[3], char_array_4[4];

    while (len--)
    {
        char_array_3[i++] = *(input++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; ++i)
                result += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (int j = i; j < 3; ++j)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; j < i + 1; ++j)
            result += base64_chars[char_array_4[j]];

        while (i++ < 3)
            result += '=';
    }

    return result;
}

void send_webscoket_message_inframe(int client_fd, const std::string &json_message)
{
    auto &context = clients_context[client_fd];
    int message_id = context.next_message_id++;

    // Dodanie `messageId` do wiadomości
    std::string message_with_id = json_message;
    message_with_id.insert(1, "\"messageId\":" + std::to_string(message_id) + ",");

    size_t message_len = message_with_id.size();
    unsigned char frame[4096] = {0};
    size_t frame_len = 0;

    frame[0] = 0x81;
    if (message_len <= 125)
    {
        frame[1] = message_len;
        frame_len = 2;
    }
    else if (message_len <= 65535)
    {
        frame[1] = 126;
        frame[2] = (message_len >> 8) & 0xFF;
        frame[3] = message_len & 0xFF;
        frame_len = 4;
    }
    else
    {
        frame[1] = 127;
        for (int i = 0; i < 8; ++i)
        {
            frame[9 - i] = (message_len >> (i * 8)) & 0xFF;
        }
        frame_len = 10;
    }

    memcpy(&frame[frame_len], message_with_id.c_str(), message_len);
    frame_len += message_len;

    // Wysyłanie wiadomości

    // Przechowuj wiadomość w pending_ack
    context.pending_ack[message_id] = {
        .message = message_with_id,
        .timestamp = std::chrono::steady_clock::now(),
        .retries = 0};

    // static bool random_initialized = false;
    // if (!random_initialized)
    // {
    //     srand(static_cast<unsigned int>(time(nullptr)));
    //     random_initialized = true;
    // }
    //
    // // Symulacja szans na utratę pakietu (1/5 = 20%)
    // if (rand() % 5 == 0) // Wartość z przedziału 0–4; 0 oznacza "utrata pakietu"
    // {
    //     std::cout << "Symulacja: wiadomość nie została wysłana (1/5 szans)." << std::endl;
    //     return; // Nie wysyłaj wiadomości
    // }
    send(client_fd, frame, frame_len, 0);
}

void send_changed_pixels(int client_fd, const nlohmann::json &changed_pixels) {
    const size_t max_pixels_per_message = 40;


    size_t total_pixels = changed_pixels.size();

    for (size_t i = 0; i < total_pixels; i += max_pixels_per_message) {
        size_t end = std::min(i + max_pixels_per_message, total_pixels);

        nlohmann::json message = {
            {"type", WsServerMessageType::UpdateCanva},
            {"changedPixels", nlohmann::json::array()}
        };

        for (size_t j = i; j < end; ++j) {
            message["changedPixels"].push_back(changed_pixels[j]);
        }

        send_webscoket_message_inframe(client_fd, message.dump());
    }
}
void resend_webscoket_message_inframe(int client_fd, int message_id) {
    // Pobierz kontekst klienta
    auto &context = clients_context[client_fd];
    auto it = context.pending_ack.find(message_id);

    if (it == context.pending_ack.end())
    {
        // Jeśli wiadomość o podanym `message_id` nie istnieje w pending_ack
        std::cerr << "Nie można ponownie wysłać wiadomości. ID: " << message_id << " nie istnieje." << std::endl;
        return;
    }

    // Pobierz treść wiadomości
    const std::string &message_with_id = it->second.message;

    // Przygotuj ramkę WebSocket z istniejącą wiadomością
    size_t message_len = message_with_id.size();
    unsigned char frame[4096] = {0};
    size_t frame_len = 0;

    frame[0] = 0x81; // FIN bit set and opcode = 0x1 (text frame)
    if (message_len <= 125)
    {
        frame[1] = message_len;
        frame_len = 2;
    }
    else if (message_len <= 65535)
    {
        frame[1] = 126;
        frame[2] = (message_len >> 8) & 0xFF;
        frame[3] = message_len & 0xFF;
        frame_len = 4;
    }
    else
    {
        frame[1] = 127;
        for (int i = 0; i < 8; ++i)
        {
            frame[9 - i] = (message_len >> (i * 8)) & 0xFF;
        }
        frame_len = 10;
    }

    memcpy(&frame[frame_len], message_with_id.c_str(), message_len);
    frame_len += message_len;

    // Wyślij wiadomość
    send(client_fd, frame, frame_len, 0);

    // Zaktualizuj timestamp i zwiększ liczbę prób
    it->second.timestamp = std::chrono::steady_clock::now();
    it->second.retries++;

    std::cout << "Ponownie wysłano wiadomość ID: " << message_id
              << " do klienta " << client_fd << ". Próba: " << it->second.retries << std::endl;
}

void check_ack_timeouts()
{
    auto now = std::chrono::steady_clock::now();

    for (auto &[client_fd, context] : clients_context)
    {
        for (auto it = context.pending_ack.begin(); it != context.pending_ack.end();)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp).count();
            if (elapsed > 0.1)
            {
                if (it->second.retries >= 3)
                {
                    std::cerr << "Nie otrzymano ACK dla wiadomości ID: " << it->first << ". Usuwanie wiadomości." << std::endl;
                    it = context.pending_ack.erase(it);
                }
                else
                {
                    std::cerr << "Ponawianie wysyłki wiadomości ID: " << it->first << std::endl;
                    resend_webscoket_message_inframe(client_fd, it->first);
                    it->second.timestamp = now;
                    it->second.retries++;
                    ++it;
                }
            }
            else
            {
                ++it;
            }
        }
    }
}

void handle_ack(int client_fd, const nlohmann::json &message)
{
    int message_id = message.at("messageId");

    auto &context = clients_context[client_fd];
    auto it = context.pending_ack.find(message_id);

    if (it != context.pending_ack.end())
    {
        std::cout << "ACK otrzymane dla wiadomości ID: " << message_id << std::endl;
        context.pending_ack.erase(it); // Usuwanie wiadomości z mapy
    }
    else
    {
        std::cerr << "Nieoczekiwane ACK dla wiadomości ID: " << message_id << std::endl;
    }
}

std::unordered_map<int, ClientContext> clients_context;

// łączy string który wysłał klient z "magic stringiem" czyli tym który mamy na sztywno w kodzie (ten magic string to jest taki ciąg znaków
// , że on jest taki sam dla każdego serwera websocket, to nie jest coś unikalnego i jest powszechnie znany Klient później oblicza
// sobie czy serwer wysłał mu dobry accept key tzn "unikalny kod klienta który wygenerował przy żądaniu http + magic string"
// kazdy klient ma swój uniklany kod który wysyła do serwera dlatego kazdy klient bedzie mial tez unikalny accept key
std::string generate_websocket_accept_key(const std::string &key)
{
    const std::string magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string concatenated = key + magic_string;

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(concatenated.c_str()), concatenated.size(), hash);

    return base64_encode(hash, SHA_DIGEST_LENGTH);
}

void handle_websocket_handshake(int client_fd, const std::string &request)
{
    std::istringstream request_stream(request);
    std::string line;
    std::string websocket_key;

    while (std::getline(request_stream, line) && line != "\r")
    {
        if (line.find("Sec-WebSocket-Key:") != std::string::npos)
        {
            websocket_key = line.substr(19);                                  // Skip "Sec-WebSocket-Key: "
            websocket_key.erase(websocket_key.find_last_not_of(" \r\n") + 1); // Trim whitespace
        }
    }

    if (!websocket_key.empty())
    {
        std::string accept_key = generate_websocket_accept_key(websocket_key);
        std::string response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: " +
            accept_key + "\r\n\r\n";

        send(client_fd, response.c_str(), response.size(), 0);
        std::cout << "WebSocket handshake completed with client: " << client_fd << std::endl;
        std::string json_message = R"({"message": "Welcome, new client!"})";
        send_webscoket_message_inframe(client_fd, json_message);
    }
    else
    {
        std::cerr << "WebSocket handshake failed: Sec-WebSocket-Key not found" << std::endl;
        close(client_fd);
    }
}

// funkcja do sprawdzenia czy to ta ramka , żeby zamknąć połączenie

bool is_websocket_close_frame(const std::vector<uint8_t> &data)
{
    // Minimalna długość ramki CLOSE to 2 bajty nagłówka.
    if (data.size() < 2)
    {
        return false;
    }

    // Sprawdź pierwszy bajt (FIN bit i opcode)
    uint8_t fin = data[0] & 0x80;    // Sprawdź bit FIN (powinien być ustawiony)
    uint8_t opcode = data[0] & 0x0F; // Sprawdź opcode (powinien być 0x8)

    if (fin == 0 || opcode != 0x8)
    {
        return false; // Nie jest to ramka zamknięcia
    }

    // Sprawdź drugi bajt (Mask bit i Payload length)
    uint8_t mask = data[1] & 0x80; // Sprawdź, czy ustawiony jest bit Mask (powinien być ustawiony dla klientów)
    uint8_t payload_length = data[1] & 0x7F;

    if (mask == 0)
    {
        std::cerr << "Brak maskowania, co wskazuje na nieprawidłową ramkę klienta." << std::endl;
        return false;
    }

    // Jeśli ramka jest poprawnie sformatowana jako CLOSE, zwróć true
    return true;
}

// websocket wysyła wiadomosci w ramkach wiec gdy te wiadomosci do nas dojda musimy je odkodowac zebysmy widzieli json to robi ta funkcja

std::string decode_websocket_frame(const char *buffer, int size)
{
    // WebSocket frame structure is:
    // [FIN, RSV1, RSV2, RSV3, Opcode, Mask, Payload length, Masking Key, Payload Data]
    // For simplicity, let's assume we're handling only text frames and no fragmentation.

    if (size < 2)
    {
        return ""; // Not enough data for a WebSocket frame
    }

    unsigned char first_byte = buffer[0];
    unsigned char second_byte = buffer[1];

    // Payload length is the 7 bits of the second byte
    int payload_length = second_byte & 0x7F;

    // If the payload length is 126 or 127, handle extended payload length (not implemented here)
    // In a real implementation, you would read additional bytes for large payloads.

    if (payload_length > size - 2)
    {
        return ""; // Not enough data for the full message
    }

    // If the frame is masked, we need to unmask the payload data (only applicable for client-to-server messages)
    if (first_byte & 0x80)
    { // Mask bit is set
        unsigned char mask_key[4];
        memcpy(mask_key, buffer + 2, 4);

        std::string decoded_data;
        for (int i = 0; i < payload_length; ++i)
        {
            decoded_data += buffer[6 + i] ^ mask_key[i % 4]; // XOR with mask key
        }
        return decoded_data;
    }
    else
    {
        // If no mask, return the payload directly (for server-to-client messages)
        return std::string(buffer + 2, buffer + 2 + payload_length);
    }
}

bool is_handshake_request(const char *buffer, size_t bytes_read)
{
    // Sprawdzamy, czy wiadomość zawiera nagłówek WebSocket "Sec-WebSocket-Key"
    return strstr(buffer, "Sec-WebSocket-Key") != nullptr;
}