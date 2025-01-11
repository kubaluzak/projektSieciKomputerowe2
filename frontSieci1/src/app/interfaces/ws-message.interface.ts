import { WsClientMessageType } from '../enums/ws-client-message-type.enum';
import { WsServerMessageType } from '../enums/ws-server-message-type.enum';

export interface IWsMessage {
  type: WsClientMessageType | WsServerMessageType;
  [key: string]: any; // Dopuszczamy wszelakie inne właściwości
}
