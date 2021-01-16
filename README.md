# IPC - Publish-Subscribe
Projekt na programowanie systemowe i współbieżne – [Treść zadania]

---
## Kompilacja
np.

```
gcc inf145233_s.c -o serwer -Wall
gcc inf145233_k.c -o klient -Wall -lncursesw
```

Wymaga biblioteki ncursesw (ewentualnie ncurses, ale nie będą działać polskie znaki)

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
- `send_subed_topics` – wysłanie klientowi listy subskrybowanych przez niego tematów
- `shutdown` – usunięcie wszystkich kolejek komunikatów (serwera i klientów)

ponadto funkcje pomocnicze:

- `send_feedback` – wysłanie wiadomości zwrotnej do klienta
- `print_subs` – wyświetlenie listy subskrybcji w danym temacie (do debugowania)
- `decrement_sub_length` – zmniejszenie długości subskrybcji przy subskrybcji przejściowej. Usunięcie suskrybcji o długości 0 i przesłanie wiadomości o tym do klienta.


### inf145233_k:
Plik zawiera wszystkie funkcje niezbędne do komunikacji z serwerem:

- `login` – wysłanie wiadomości dotyczącej logowania na serwer
- `register topic` – wysłanie wiadomości dotyczącej tworzenia nowego tematu na serwer
- `register_sub` – wysłanie wiadomości dotyczącej nowej subskrybcji na serwer
- `send_msg` – wysłanie wiadomości do rozgłoszenia na serwer
- `receive_msg_async` – odbiór wiadomości z danego tematu (jeśli dostępne) i wyświetlenie ich
- `receive_msg_sync` – odbiór wiadomości z danego tematu (jeśli dostępne), posortowanie ich po priorytecie i wyświetlenie
- `get_topics` – wysłanie zapytania na serwer i wyświetlenie listy tematów dostępnych na serwerze (lub subskrybowanych przez użytkownika)
- `get_zombie_subs` – odebranie wiadomości o subskrybcjach, które niedawno się zakończyły
- `shutdown` – wysłanie polecenia wyłączenia serwera

oraz użytkownikiem:

- `gui_menu` – wyświetlenie graficznego menu i jego obsługa
- `login_menu` – wprowadzenie danych logowania i obsługa błędów
- `topic_menu` – wprowadzenie danych do rejestracji tematu i obsługa błędów
- `sub_menu` – wprowadzenie danych do rejestracji subskrybcji i obsługa błędów
- `msg_menu` – wprowadzenie nowej wiadomości i obsługa błędów
- `receive_msg_sync_menu` – wprowadzenie danych wiadomości do odebrania i obsługa błędów
- `receive_msg_async_menu` – włączenie lub wyłączenie asynchronicznego odbioru wiadomości z konkretnego tematu przez użytkownika

ponadto funkcje pomocnicze:

- `main` – inicjalizacja, utworzenie procesu potomnego, przekierowywanie  do odpowiednich funkcji
- `child` – pętla procesu potomnego – asynchroniczne odczytywanie wiadomości i komunikacja z użytkownikiem w sprawach ich dotyczących
- `take_feedback` – odebranie wiadomości zwrotnej o powodzeniu lub błędzie
- `print_error`, `print_info`, `print_success`, `print_long` – wyświetlenie odpowiednio sformatowanej wiadomości
- `get_int_from_user`, `get_string_from_user` – pobranie odpowiedniej wartości od użytkownika
- `close_window` – czekanie na naciśnięcie dowolnego klawisza, wyczyszczenie okna
- `clean_window` – wyczyszczenie okna, narysowanie ramki z jego nazwą i umieszczenie kursora w lewym górnym rogu

---

© Anna Panfil 2020-2021

<!-- links -->
[Treść zadania]: https://www.cs.put.poznan.pl/akobusinska/downloads/projekt2020.pdf
