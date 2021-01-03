# IPC - Publish-Subscribe
Projekt na programowanie systemowe i współbieżne – [Treść zadania]

---
## Kompilacja
np.

```
gcc inf145233_s.c -o serwer -Wall
gcc inf145233_k.c -o klient -Wall
```

## Uruchomienie
na początku – uruchomienie serwera: `./serwer`<br>
uruchomienie klientów (dowolna ilość): `./klient`

## Zawartość plików
### inf145233_s:
Plik zawiera wszystkie funkcje niezbędne do działania serwera:

- `main` – odbiór wiadomości i przekierowanie jej do odpowiedniej funkcji
- `login` – przyjęcie logowania nowego klienta
- `add topic` – utworzenie nowego tematu
- `add_sub` – dodanie klienta do listy subskrybentów danego tematu
- `send_msgs` – rozgłoszenie wiadomości
- `send_topics` – wysłanie klientowi listy tematów dostępnych na serwerze
- `shutdown` – usunięcie wszystkich kolejek komunikatów (serwera i klientów)

ponadto funkcje pomocnicze:

- `send_feedback` – wysłanie wiadomości zwrotnej do klienta
- `print_subs` – wyświetlenie listy subskrybcji w danym temacie (do debugowania)
- `decrement_sub_length` – zmniejszenie długości subskrybcji przy subskrybcji przejściowej i usunięcie suskrybcji o długości 0


### inf145233_k:
Plik zawiera wszystkie funkcje niezbędne do komunikacji z serwerem:

- `login` – wysłanie wiadomości dotyczącej logowania na serwer
- `register topic` – wysłanie wiadomości dotyczącej tworzenia nowego tematu na serwer
- `register_sub` – wysłanie wiadomości dotyczącej nowej subskrybcji na serwer
- `send_msg` – wysłanie wiadomości do rozgłoszenia na serwer
- `shutdown` – wysłanie polecenia wyłączenia serwera
- `receive_msg_async` – włączenie/ wyłączenie asynchronicznego odbioru wiadomości
- `receive_msg_sync` – odbiór wiadomości

oraz użytkownikiem:

- `main` – wyświetlenie menu, oczekiwanie na wybór i przekierowanie  do odpowiedniej funkcji
- `get_topics` – wysłanie zapytania na serwer i wyświetlenie listy tematów dostępnych na serwerze
- `login_menu` – wprowadzenie danych logowania i obsługa błędów
- `topic_menu` – wprowadzenie danych do rejestracji tematu i obsługa błędów
- `sub_menu` – wprowadzenie danych do rejestracji subskrybcji i obsługa błędów
- `msg_menu` – wprowadzenie nowej wiadomości i obsługa błędów

ponadto funkcje pomocnicze:

- `take_feedback` – odebranie wiadomości zwrotnej
- `print_error` – wyświetlenie sformatowanej wiadomości o błędzie
- `print_info` – wyświetlenie sformatowanej informacji dla użytkownika

---

© Anna Panfil 2020

<!-- links -->
[Treść zadania]: https://www.cs.put.poznan.pl/akobusinska/downloads/projekt2020.pdf
