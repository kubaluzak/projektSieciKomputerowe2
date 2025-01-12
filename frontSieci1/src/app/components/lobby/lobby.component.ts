import { AfterViewInit, Component, ElementRef, ViewChild } from '@angular/core';
import { CommonModule } from '@angular/common';
import { WsClientMessageType } from '../../enums/ws-client-message-type.enum';
import { IWsMessage } from '../../interfaces/ws-message.interface';
import { WebSocketBaseComponent } from '../base/web-socket-base.component';
import { WsServerMessageType } from '../../enums/ws-server-message-type.enum';
import { FormsModule } from '@angular/forms'; // Import FormsModule here
import { LobbyData, User } from '../../services/game-data.service';
import { TimerComponent } from '../timer/timer.component';

@Component({
  standalone: true,
  selector: 'app-lobby',
  templateUrl: './lobby.component.html',
  styleUrls: ['./lobby.component.css'],
  imports: [CommonModule, FormsModule, TimerComponent],
})
export class LobbyComponent extends WebSocketBaseComponent implements AfterViewInit {
  @ViewChild('drawingCanvas')
  public drawingCanvas!: ElementRef<HTMLCanvasElement>;
  private canvasHeight = 400;
  private canvasWidth = 600;
  private pixelSize = 3;
  public currentColor = '#000000'; // Czarny domyślny

  private ctx!: CanvasRenderingContext2D | null;
  private isDrawing = false;
  isDrawer = false;
  private currentPath: { x: number; y: number }[] = [];
  isReady: boolean = false;

  chatMessages: { sender: string; content: string; color?: string }[] = []; // Tablica wiadomości
  newMessage: string = ''; // Aktualna wiadomość użytkownika

  private user!: User; // Aktualny użytkownik
  public lobbyData!: LobbyData; // Dane dotyczące lobby
  public timeToGetReady?: number;
  isGameStarted: boolean = false;
  wordToDraw = '';
  public roundTime?: number; // Czas rundy

  constructor() {
    super();
    const user = this.gameDataService.getUser();
    const lobbyData = this.gameDataService.getLobbyData();

    // W przypadku gdy nie ma tych danych, wychodzimy z komponentu
    // - choć nie powinno dojść do takiej sytuacj bo mamy guarda który to sprawdza
    if (!user || !lobbyData) {
      this.router.navigate(['/']);
      return;
    }

    this.user = user;
    this.lobbyData = lobbyData;
    this.isGameStarted = this.lobbyData?.isGameStarted ?? false;
  }

  ngAfterViewInit(): void {
    const canvas = this.drawingCanvas.nativeElement;
    this.ctx = canvas.getContext('2d');

    if (this.ctx) {
      canvas.width = this.canvasWidth;
      canvas.height = this.canvasHeight;
      this.ctx.fillStyle = 'white';
      this.ctx.fillRect(0, 0, canvas.width, canvas.height);
    }
  }

  startDrawing(): void {
    this.isDrawing = true;
  }

  draw(event: MouseEvent): void {
    if (!this.isDrawing || !this.ctx) return;

    // Pobieranie pozycji kursora
    const rect = this.drawingCanvas.nativeElement.getBoundingClientRect();
    const x = event.clientX - rect.x;
    const y = event.clientY - rect.y;

    // Rysowanie piksela
    this.ctx.fillStyle = this.currentColor;
    this.ctx.fillRect(x, y, this.pixelSize, this.pixelSize);

    // Wysyłanie danych piksela do serwera
    this.sendPixelData(x, y, this.currentColor); // Przekazujemy dane o pozycji i kolorze
  }

  stopDrawing(): void {
    this.isDrawing = false;
  }

  sendPixelData(x: number, y: number, color: string): void {
    this.sendMessage({
      type: WsClientMessageType.SendPixels,
      x,
      y,
      color,
      lobbyId: this.lobbyData.lobbyId,
    } as IWsMessage);
  }

  setReady(): void {
    this.isReady = true;
    this.sendMessage({
      type: WsClientMessageType.SetReady,
      lobbyId: this.lobbyData.lobbyId,
    } as IWsMessage);
  }

  clearCanvas(): void {
    const canvas = this.drawingCanvas.nativeElement;
    if (this.ctx) {
      canvas.width = this.canvasWidth;
      canvas.height = this.canvasHeight;
      this.ctx.fillStyle = 'white';
      this.ctx.fillRect(0, 0, canvas.width, canvas.height);

      // Wyślij wiadomość o czyszczeniu canvasu
    }
  }
  sendMessageOnChat(): void {
    if (this.newMessage.trim() === '') return;

    const message = this.newMessage.trim();

    this.newMessage = '';
    this.sendMessageToServer(message);
  }

  sendMessageToServer(message: string): void {
    this.sendMessage({
      type: WsClientMessageType.ChatMessage,
      lobbyId: this.lobbyData.lobbyId,
      message: message,
    } as IWsMessage);
  }

  override handleServerResponse(data: IWsMessage) {
    super.handleServerResponse(data);

    switch (data.type) {
      case WsServerMessageType.DrawingUpdate:
        if (this.ctx) {
          const { x, y, color } = data;

          this.ctx.fillStyle = color;
          this.ctx.fillRect(x, y, this.pixelSize, this.pixelSize);
        }
        break;
      case WsServerMessageType.LobbyUpdate:
        this.lobbyData.players = data['players'];
        this.lobbyData.lobbyId = data['lobbyId'];
        break;
      case WsServerMessageType.ClearCanvas:
        if (this.ctx) {
          const canvas = this.drawingCanvas.nativeElement;
          this.ctx.clearRect(0, 0, canvas.width, canvas.height);
          this.ctx.fillStyle = 'white';
          this.ctx.fillRect(0, 0, canvas.width, canvas.height);
        }
        break;
      case WsServerMessageType.DrawerNotification:
        this.isDrawer = true;
        this.wordToDraw = data['word_to_draw'];

        break;
      case WsClientMessageType.GameStart:
        this.isGameStarted = true; // Ustawianie ręczne
        break;
      case WsServerMessageType.RoundStart:
        if (this.gameDataService.lobbyData) {
          this.gameDataService.lobbyData.game = {
            drawer: data['drawer'],
            roundNumber: data['roundNumber'],
            lettersToGuess: data['lettersToGuess'],
            wordToDraw: this.wordToDraw,
          };
        }
        this.roundTime = data['roundTime'];
        if (this.gameDataService.lobbyData) {
          this.gameDataService.lobbyData = { ...this.gameDataService.lobbyData };
        }
        break;
      case WsServerMessageType.UpdateCanva:
        if (this.ctx) {
          const pixels = data['changedPixels']; // Otrzymana lista pikseli

          pixels.forEach((pixel: { x: number; y: number; color: string }) => {
            // Rysowanie każdego piksela
            this.ctx!.fillStyle = pixel.color;
            this.ctx!.fillRect(pixel.x, pixel.y, this.pixelSize, this.pixelSize);
          });
        }
        break;
      case WsServerMessageType.ChatMessage:
        let sender = data['sender'];
        const msgMode = data['msg_mode'];
        const content = data['content'];
        let color;

        if (msgMode == 0) {
          // Wiad klienta
        } else if (msgMode == 1) {
          // Wiad servera
          sender = '[SERVER]';
          const colorMode = data['color_mode'];
          switch (colorMode) {
            case 0:
              color = '#0886c4'; //INFO
              break;
            case 1:
              color = '#d48b17'; //WARNING
              break;
            case 2:
              color = '#19b30e'; //SUCCESS
              break;
            default:
          }
        }

        this.chatMessages.push({ sender, content, color });
        break;
      case WsServerMessageType.ScoreUpdate:
        this.lobbyData.players = data['players'];
        break;
      case WsServerMessageType.ReadyTimer:
        this.timeToGetReady = data['time'];
        break;
      case WsServerMessageType.GameTimer:
        const remainingTime = data['time'];
        break;
      case WsServerMessageType.Kick:
        this.gameDataService.reset();
        this.router.navigate(['/']);
        break;
      case WsServerMessageType.GameEnd:
        this.isGameStarted = false;
        this.isDrawer = false;
        this.isReady = false;
        this.clearCanvas();
        this.notificationService.addNotification('info', 'Gra zakończona wracasz do lobby');
        this.lobbyData.players = data['players'];

        break;

      default:
        break;
    }
  }
}
