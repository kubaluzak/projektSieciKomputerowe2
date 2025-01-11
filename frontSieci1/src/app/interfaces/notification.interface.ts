export interface INotification {
  id: number;
  message: string;
  type: NotificationType;
}

export type NotificationType = 'success' | 'error' | 'warning' | 'info';
