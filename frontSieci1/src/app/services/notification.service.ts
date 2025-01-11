import { Injectable } from '@angular/core';
import { BehaviorSubject } from 'rxjs';
import {
  INotification,
  NotificationType,
} from '../interfaces/notification.interface';

@Injectable({
  providedIn: 'root',
})
export class NotificationService {
  private notificationsSubject = new BehaviorSubject<INotification[]>([]);
  notificationsObservable = this.notificationsSubject.asObservable();

  constructor() {}

  public addNotification(type: NotificationType, message: string) {
    const currentNotifications = this.notificationsSubject.value;

    const nextId = currentNotifications.length
      ? Math.max(...currentNotifications.map((n) => n.id)) + 1
      : 1;

    this.notificationsSubject.next([
      ...currentNotifications,
      { id: nextId, message: message, type: type },
    ]);
  }

  public removeNotification(id: number) {
    const currentNotifications = this.notificationsSubject.value.filter(
      (notification) => notification.id !== id
    );
    this.notificationsSubject.next(currentNotifications);
  }
}
