// Typy wiadomości WebSocket przesyłane z klienta do serwera
export enum WsClientMessageType {
  Register = 'register',
  SetReady = 'set_ready',
  SendPixels = 'send_pixels',
  ClearCanvas = 'clear_canvas',
  GameStart = 'game_start',
  ChatMessage = "chat_message",
}
