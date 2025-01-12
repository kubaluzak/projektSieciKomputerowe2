import { Injectable } from '@angular/core';

export interface User {
  nickname: string;
}

export interface Player {
  nickname: string;
  isReady: boolean;
  roundScore: number;
  gameScore: number;
}

export interface LobbyData {
  lobbyId: number;
  players: Player[];
  isGameEnded: boolean;
  isGameStarted: boolean;
  settings: { playersMin: number; playersMax: number };
  game?: {
    roundNumber: number;
    drawer: string;
    lettersToGuess?: number; // Puste - dla rysującego
    wordToDraw?: string; // Puste - dla zgadujących
  };
}

@Injectable({
  providedIn: 'root',
})
export class GameDataService {
  private player: User | null = null;
  public lobbyData: LobbyData | null = null;

  constructor() {}

  setUser(player: User): void {
    this.player = player;
  }

  getUser(): User | null {
    return this.player;
  }

  setLobbyData(data: LobbyData): void {
    this.lobbyData = data;
  }

  getLobbyData(): LobbyData | null {
    return this.lobbyData;
  }

  clearUser(): void {
    this.player = null;
  }

  clearLobbyData(): void {
    this.lobbyData = null;
  }

  reset(): void {
    this.lobbyData = null;
    this.player = null;
  }
}
