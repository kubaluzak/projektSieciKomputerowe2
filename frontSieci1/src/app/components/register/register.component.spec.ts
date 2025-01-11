// src/app/features/register/register.component.spec.ts
import { RegisterComponent } from './register.component';

describe('RegisterComponent', () => {
  let component: RegisterComponent;

  beforeEach(() => {
    component = new RegisterComponent(null as any, null as any);
  });

  it('powinien stworzyć komponent', () => {
    expect(component).toBeTruthy();
  });

  it('powinien ustawić komunikat o błędzie, gdy nick jest pusty', () => {
    component.nickname = '';
    component.sendNickname();
    expect(component.errorMessage).toBe('Proszę wpisać poprawny nick.');
  });
});
