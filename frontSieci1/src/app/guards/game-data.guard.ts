import { Injectable } from '@angular/core';
import { CanActivate, Router } from '@angular/router';
import { GameDataService } from '../services/game-data.service';

@Injectable({
  providedIn: 'root',
})
export class GameDataGuard implements CanActivate {
  constructor(private gameDataService: GameDataService, private router: Router) {}

  canActivate(): boolean {
    if (this.gameDataService.getUser() && this.gameDataService.getLobbyData()) {
      return true;
    }

    this.router.navigate(['/']);
    return false;
  }
}
