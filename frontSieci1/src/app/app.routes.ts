import { Routes } from '@angular/router';
import { RegisterComponent } from './components/register/register.component';
import { LobbyComponent } from './components/lobby/lobby.component';
import { GameDataGuard } from './guards/game-data.guard';

export const routes: Routes = [
  { path: '', component: RegisterComponent },
  { path: 'lobby', component: LobbyComponent, canActivate: [GameDataGuard] },
  { path: '**', redirectTo: '', pathMatch: 'full' },
];
