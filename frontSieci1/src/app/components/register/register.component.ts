// src/app/features/register/register.component.ts
import { Component } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { WsClientMessageType } from '../../enums/ws-client-message-type.enum';
import { IWsMessage } from '../../interfaces/ws-message.interface';
import { WebSocketBaseComponent } from '../base/web-socket-base.component';
import { WsServerMessageType } from '../../enums/ws-server-message-type.enum';
import { LobbyData } from '../../services/game-data.service';
import { WebSocketService } from '../../services/websocket.service';
import { CommonModule } from '@angular/common';

@Component({
  standalone: true,
  selector: 'app-register',
  templateUrl: './register.component.html',
  styleUrls: ['./register.component.css'],
  imports: [CommonModule, FormsModule],
})
export class RegisterComponent extends WebSocketBaseComponent {
  public nickname: string = '';
  public errorMessage: string = '';
  public websocketHost: string = 'localhost';
  public websocketPort: number = 8080;
  public step: string = 'configWS';

  constructor(private webSocketService: WebSocketService) {
    super();
  }

  proceedToRegistration() {
    if (this.websocketHost.trim() !== '' && this.websocketPort > 0) {
      // Ustaw WebSocket na podstawie podanych danych
      this.webSocketService.initializeConnection(this.websocketHost, this.websocketPort);   
      this.webSocketService.isConnected$.subscribe((isConnected: any) => {
        if (isConnected) {
          this.step = 'register'; // Przejdź do etapu rejestracji
        } 
      });
    } else {
      this.setErrorMessage('Proszę podać poprawne dane konfiguracyjne.');
    }
  }
  sendNickname() {
    const nickname = this.nickname.trim();
    if (nickname !== '') {
      this.sendMessage({
        type: WsClientMessageType.Register,
        nick: nickname,
      } as IWsMessage);
    } else {
      this.setErrorMessage('Proszę wpisać poprawny nick.');
    }
  }

  override handleServerResponse(data: IWsMessage) {
    super.handleServerResponse(data);

    switch (data.type) {
      case WsServerMessageType.LobbyFullInfo:

        // Trick żeby od razu móc dane sparsować do modelu
        console.log((data as object).toString ());
        this.gameDataService.setLobbyData(data as unknown as LobbyData);
        if (this.gameDataService.lobbyData) {
          // this.gameDataService.lobbyData.game = {};
        };
        this.gameDataService.setUser({ nickname: this.nickname });
        this.router.navigate(['/lobby']);
        break;
      case WsServerMessageType.Error:
        this.setErrorMessage(data['error']);
        break;
      default:
        break;
    }
  }

  setErrorMessage(message: string) {
    this.errorMessage = message;
  }
}
