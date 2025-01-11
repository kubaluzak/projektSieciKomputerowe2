import { Component, inject } from '@angular/core';
import { NotificationService } from '../../services/notification.service';
import { GameDataService } from '../../services/game-data.service';
import { Router } from '@angular/router';

@Component({
  template: '',
  standalone: true,
})
export abstract class BaseComponent {
  protected router: Router;
  protected notificationService: NotificationService;
  protected gameDataService: GameDataService;

  constructor() {
    this.router = inject(Router);
    this.notificationService = inject(NotificationService);
    this.gameDataService = inject(GameDataService);
  }
}
