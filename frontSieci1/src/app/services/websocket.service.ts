// src/app/websocket.service.ts
import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable, Subject } from 'rxjs';
import { environment } from '../../environments/environment';
import { NotificationService } from './notification.service';
import { IWsMessage } from '../interfaces/ws-message.interface';

@Injectable({
  providedIn: 'root',
})
export class WebSocketService {
  private socket: WebSocket | null = null;
  private messageSubject = new Subject<IWsMessage>();
  private isConnected = new BehaviorSubject<boolean>(false);
  public isConnected$ = this.isConnected.asObservable();


  constructor(private notificationService: NotificationService) {
 
  }
  initializeConnection(host: string, port: number): void {

    if (this.socket) {
      console.warn('WebSocket connection already exists.');
      return;
    }


    this.socket = new WebSocket(`ws://${host}:${port}/ws`);

    this.socket.onopen = () => {
    this.isConnected.next(true);


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
      this.socket = null; // Ustaw null po zamknięciu połączenia
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
    if(this.socket != null)
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
    this.isConnected.next(false);
  }


  
}
