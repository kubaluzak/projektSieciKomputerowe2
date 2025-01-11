import { Component, OnDestroy, OnInit } from '@angular/core';
import { NotificationService } from '../../services/notification.service';
import { INotification } from '../../interfaces/notification.interface';
import { Subscription } from 'rxjs';
import { NgClass, UpperCasePipe } from '@angular/common';

@Component({
  selector: 'app-notification',
  standalone: true,
  imports: [NgClass, UpperCasePipe],
  templateUrl: './notification.component.html',
  styleUrl: './notification.component.css',
})
export class NotificationComponent implements OnInit, OnDestroy {
  public notifications: INotification[] = [];
  private notificationsSubscription?: Subscription;

  constructor(private notificationService: NotificationService) {}

  ngOnInit() {
    // Subskrypcja na zmiany powiadomień
    this.notificationsSubscription =
      this.notificationService.notificationsObservable.subscribe(
        (notifications: INotification[]) => {
          this.notifications = notifications;
          // this.notifications = [
          //   { id: 0, message: 'Test testowy chuj', type: 'error' },
          //   { id: 1, message: 'Cosiek wiadomosc success', type: 'success' },
          //   { id: 2, message: 'Blue info', type: 'info' },
          //   { id: 3, message: 'Uwaga cos tam', type: 'warning' },
          // ];
        }
      );
  }

  ngOnDestroy() {
    // Anulowanie subskrypcji - czyszczenie pamięci
    if (this.notificationsSubscription) {
      this.notificationsSubscription.unsubscribe();
    }
  }

  // Metoda usuwająca powiadomienie po kliknięciu
  removeNotification(index: number) {
    this.notificationService.removeNotification(index);
  }
}
