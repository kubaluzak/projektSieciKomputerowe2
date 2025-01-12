// Typy wiadomości WebSocket przesyłane z serwera do klienta
export enum WsServerMessageType {
  LobbyFullInfo = 'lobby_full_info',
  Error = 'error',
  LobbyUpdate = 'lobby_update',
  PixelData = 'pixel_data',
  ClearCanvas = 'clear_canvas',
  DrawingUpdate = 'drawing_update',
  DrawerNotification = 'drawer_notification',
  RoundStart = 'round_start',
  UpdateCanva = 'update_canva',
  ChatMessage = 'chat_message',
  ReadyTimer = 'ready_timer',
  ScoreUpdate = 'score_update',
  Kick = 'kick',
  GameEnd = 'game_end',
  GameTimer = 'game_timer',
}
