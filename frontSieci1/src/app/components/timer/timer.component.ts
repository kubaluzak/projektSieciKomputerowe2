import { Component, EventEmitter, Input, OnChanges, OnDestroy, OnInit, Output, SimpleChanges } from '@angular/core';
import { interval, Subscription, take } from 'rxjs';

@Component({
  selector: 'app-timer',
  standalone: true,
  imports: [],
  templateUrl: './timer.component.html',
  styleUrl: './timer.component.css',
})
export class TimerComponent implements OnChanges, OnDestroy {
  @Input({ required: true }) countdownFrom!: number;
  @Output() timerFinished = new EventEmitter<void>();
  @Input() reset!: boolean;
  // Wartość timera
  timeLeft!: number;
  private subscription!: Subscription;

  ngOnChanges(changes: SimpleChanges): void {
    if (changes['countdownFrom'] || changes['reset']) {
      this.resetTimer();
    }
  }

  startTimer(): void {
    this.subscription = interval(1000) // Odpala się co 1 sekundę
      .pipe(take(this.countdownFrom)) // Ustawia limit do zera
      .subscribe({
        next: () => {
          this.timeLeft--;
          this.countdownFrom--;
        },
        complete: () => this.timerFinished.emit(),
      });
  }

  resetTimer(): void {
    if (this.subscription) {
      this.subscription.unsubscribe();
    }

    this.timeLeft = this.countdownFrom;

    this.subscription = interval(1000) // Co 1 sek
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
