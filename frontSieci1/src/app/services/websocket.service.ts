// src/app/websocket.service.ts
import { Injectable } from '@angular/core';
import { Observable, Subject } from 'rxjs';
import { environment } from '../../environments/environment';
import { NotificationService } from './notification.service';
import { IWsMessage } from '../interfaces/ws-message.interface';

@Injectable({
  providedIn: 'root',
})
export class WebSocketService {
  private socket: WebSocket;
  private messageSubject = new Subject<IWsMessage>();

  constructor(private notificationService: NotificationService) {
    this.socket = new WebSocket(
      `ws://${environment.websocketHost}:${environment.websocketPort}/ws`
    );

    this.socket.onopen = () => {
      console.log('WebSocket connection established.');
      this.notificationService.addNotification(
        'success',
        'WebSocket connection established.'
      );
    };

    this.socket.onmessage = (event) => {
      const data = JSON.parse(event.data) as IWsMessage;
      this.messageSubject.next(data);
      this.sendAck(data);
    };

    this.socket.onerror = (error) => {
      console.error('WebSocket Error:', error);
      this.notificationService.addNotification(
        'error',
        'WebSocket Error: Error in connecting with websocket'
      );
    };

    this.socket.onclose = () => {
      console.log('WebSocket connection closed.');
      this.notificationService.addNotification(
        'info',
        'WebSocket connection closed.'
      );
    };
  }
  private sendAck(message: any): void {
    if (message.messageId) {
      const ack = {
        type: 'ACK',
        messageId: message.messageId,
      };
      this.send(ack); // Wyślij ACK do serwera
      console.log('Wysłano ACK dla wiadomości ID:', message.messageId);
    }
  }

  send(message: any): void {
    if (this.socket && this.socket.readyState === WebSocket.OPEN) {
      this.socket.send(JSON.stringify(message));
    }
  }
  sendMessage(message: IWsMessage) {
    if (this.socket.readyState === WebSocket.OPEN) {
      this.socket.send(JSON.stringify(message));
    }
  }

  onMessage(): Observable<IWsMessage> {
    return this.messageSubject.asObservable();
  }
  closeConnection(): void {
    if (this.socket) {
      this.socket.close();
    }
  }
}
