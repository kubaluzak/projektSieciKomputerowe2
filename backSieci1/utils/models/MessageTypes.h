#ifndef MESSAGETYPES_H
#define MESSAGETYPES_H

#include <string>

struct WsClientMessageType
{
    inline static const std::string Register = "register";
    inline static const std::string SetReady = "set_ready";
    inline static const std::string SendPixels = "send_pixels";
};

struct WsServerMessageType
{
    inline static const std::string GameStart = "game_start";
    inline static const std::string LobbyFullInfo = "lobby_full_info";
    inline static const std::string Error = "error";
    inline static const std::string LobbyUpdate = "lobby_update";
    inline static const std::string DrawerNotification = "drawer_notification";
    inline static const std::string DrawingUpdate = "drawing_update";
    inline static const std::string PixelData = "pixel_data";
    inline static const std::string ClearCanvas = "clear_canvas";
    inline static const std::string RoundStart = "round_start";
    inline static const std::string UpdateCanva = "update_canva";
    inline static const std::string ChatMessage = "chat_message";
    inline static const std::string ScoreUpdate = "score_update";
    inline static const std::string Kick = "kick";
    inline static const std::string ReadyTimer = "ready_timer";
    inline static const std::string GameTimer = "game_timer";
    inline static const std::string GameEnd = "game_end";
};

#endif // MESSAGETYPES_H
