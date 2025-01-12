import { Component, inject, OnDestroy, OnInit } from '@angular/core';
import { Subscription } from 'rxjs';
import { WebSocketService } from '../../services/websocket.service';
import { IWsMessage } from '../../interfaces/ws-message.interface';
import { BaseComponent } from './base.component';
import { WsServerMessageType } from '../../enums/ws-server-message-type.enum';

@Component({
  template: '',
  standalone: true,
})
export abstract class WebSocketBaseComponent extends BaseComponent implements OnInit, OnDestroy {
  private wsSubscription?: Subscription;
  private wsService: WebSocketService;

  constructor() {
    super();
    this.wsService = inject(WebSocketService);
  }

  ngOnInit(): void {
    this.wsSubscription = this.wsService.onMessage().subscribe((data) => {
      this.handleServerResponse(data);
    });
  }

  ngOnDestroy(): void {
    if (this.wsSubscription) {
      this.wsSubscription.unsubscribe();
    }
  }

  sendMessage(data: IWsMessage): void {
    this.wsService.sendMessage(data);
  }


  protected handleServerResponse(data: IWsMessage): void {
    console.log('[WS received]' + data.type);
    // Ogólne typy wiadomości
    switch (data.type) {
      case WsServerMessageType.Error:
        this.notificationService.addNotification('error', data['error']);
        break;
      default:
        break;
    }
  }

  closeConnection(): void {
    this.wsService.closeConnection;
  }
}
