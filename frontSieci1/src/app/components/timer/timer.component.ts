import { Component, EventEmitter, Input, OnDestroy, OnInit, Output } from '@angular/core';
import { interval, Subscription, take } from 'rxjs';

@Component({
  selector: 'app-timer',
  standalone: true,
  imports: [],
  templateUrl: './timer.component.html',
  styleUrl: './timer.component.css',
})
export class TimerComponent implements OnInit, OnDestroy {
  @Input({ required: true }) countdownFrom!: number;
  @Output() timerFinished = new EventEmitter<void>();
  // Wartość timera
  timeLeft!: number;
  private subscription!: Subscription;

  ngOnInit(): void {
    this.timeLeft = this.countdownFrom;
    this.startTimer();
  }

  startTimer(): void {
    this.subscription = interval(1000) // Odpala się co 1 sekundę
      .pipe(take(this.countdownFrom)) // Ustawia limit do zera
      .subscribe({
        next: () => this.timeLeft--,
        complete: () => this.timerFinished.emit(),
      });
  }

  ngOnDestroy(): void {
    if (this.subscription) {
      this.subscription.unsubscribe();
    }
  }
}
