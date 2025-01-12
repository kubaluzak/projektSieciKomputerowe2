Indeksy: 155859, 155870

Odpalenie serwera C [klasycznie]:
g++ -o main main.cpp ./utils/*.cpp ./utils/models/*.cpp -I./utils -I./utils/models -lssl -lcrypto
./main
lub
Ctrl + Shift + B uruchami taska kompilującego main

Gdy zamkniemy przeglądarke klient wysyła frame "close", który ma 8 bajtów
