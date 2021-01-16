# Protokół komunikacyjny klient-serwer

[//]: [TOC]

## Informacje ogólne
- Użyto kolejki komunikatów dla serwera o kluczu 12345 i kolejki komunikatów dla każdego klienta – o kluczu opartym o jego PID.
- Klient wysyła wiadomości do kolejki serwera (w trybie blokującym), serwer – do kolejki odpowiedniego klienta (z wyjątkiem błędu logowania)
- Maksymalna liczba zalogowanych użytkowników jest ograniczona stałą `CLIENTS_NR`. Maksymalna liczba utworzonych tematów – `TOPICS_NR`, maksymalna długość nazwy klienta/tematu – `NAME_LENGTH` znaków, maksymalna długość wiadomości: `MESSAGE_LENGTH` znaków.
- serwer powinien być uruchamiany jako pierwszy

## Struktura komunikatów
#### Wiadomość od klienta

```c
struct client_msgf{
  long type;
  int id;
  int topic;
  int number;
  char text[MESSAGE_LENGTH];
};
```

- `type` – typ wysyłanej wiadomości. Poszczególne typy przedstawiono poniżej
- `id` – identyfikator klienta wysyłającego wiadomość
- `topic` – numer tematu, którego dotyczy wiadomość
- `number`, `tekst` – pola do użycia w zależności od typu komunikatu (opisane w poszczególnych scenariuszach komunikacji)

#### Wiadomość od serwera
```c
struct text_msg{
  long type;
  int info;
  char text[MESSAGE_LENGTH+NAME_LENGTH+2];
};
```

- `type`
  * 1  – lista tematów na serwerze
  * 2 – informacja zwrotna o stanie operacji
  * 3 – informacja o zakończeniu subskrybcji
  * 4 - ∞ – wiadomość z subskrybowanego tematu (temat + 4)
  * `<nr_kolejki_klienta>` – informacja zwrotna do operacji logowania
- `text`, `info` – treść wiadomości

#### Typy komunikatów od klienta
1. logowanie
1. nowy temat
1. zapis na subskrybcję
1. nowa wiadomość
1. zapytanie o tematy na serwerze
1. zapytanie o subskrybowane tematy
1. wyłączenie systemu

## Scenariusze komunikacji
#### Logowanie
  `id` – klucz kolejki klienta<br>
  `text` – nazwa klienta

  Serwer zapisuje nowego klienta w tablicy struktur:

  ```c
  struct client{
    int que;                //numer kolejki klienta
    char name[NAME_LENGTH]; //login
  };
  ```
  Serwer umieszcza w **swojej** kolejce komunikatów wiadomość o typie równym numerowi klienta. W wiadomości znajduje się nowy identyfikator klienta – jego numer w tablicy klientów. Gdy nazwa klienta istnieje już w bazie – zwraca -1, gdy limit klientów został przekroczony: -2.

  Dany klient może logować się tylko raz.

#### Rejestracja tematu
  `number` – numer tematu<br>
  `text` – nazwa tematu

  Serwer zapisuje nowy temat w tablicy struktur:

  ```c
  struct topic{
    int id;                  // numer tematu
    char name[NAME_LENGTH];  // nazwa tematu
    struct sub{             
      int client_que;        // numer kolejki klienta
      int length;            // długość subskrybcji (-1 → na zawsze)
      struct sub* next_sub;
    }* first_sub;            // subskrybcje tego tematu  
  };
  ```

  `first_sub` jest początkiem listy jednokierunkowej.

  Gdy nazwa tematu istnieje już w bazie, serwer umieszcza w  kolejce klienta wartość 1, gdy limit tematów został przekroczony: 2. W przeciwnym przypadku: 0.

#### Zapis na subskrybcję
  `number` – długość subskrybcji (-1 w przypadku nieskończonej subskrypcji)

  Serwer dopisuje klienta do listy subskrybcji w danym temacie (określanym przez strukturę opisaną przy rejestracji tematu).
  Gdy temat nie istnieje, serwer umieszcza w  kolejce klienta wartość 1. W przeciwnym przypadku: 0.

#### Wysłanie nowej wiadomości
  `number` – priorytet wiadomości<br>
  `text` – treść wiadomości

  Serwer rozsyła wiadomość poprzedzoną nazwą tematu do kolejek komunikatów wszystkich subskrybentów danego tematu (z wyłączeniem nadawcy wiadomości).
  Gdy temat nie istnieje, serwer umieszcza w  kolejce klienta wartość 1. W przeciwnym przypadku: 0.

#### Odbiór wiadomości w sposób synchroniczny
  Nie angażuje serwera. Sprawdza, czy w kolejce komunikatów danego klienta są wiadomości dotyczące konkretnego tematu i odbiera wszystkie.

#### Odbiór wiadomości w sposób asynchroniczny
  Nie angażuje serwera. Utworzony na początku działania programu proces potomny, ciągle sprawdza, czy w kolejce komunikatów danego klienta są wiadomości dotyczące wybranych tematów i wyświetla je. Proces ten odpowiada też za interakcję z użytkownikiem w sprawach dotyczących dodania/ usunięcia tematów z listy do odbioru asynchronicznego.

#### Odczyt tematów z serwera (wszystkich lub subskrybowanych)
  Serwer odsyła `<ilość tematów>+1` wiadomości typu `text_msg`. W polu `text` znajduje się numer tematu i jego nazwa. Ostatnia wiadomość zawiera `""` i oznacza koniec listy tematów.

#### Wyłączenie systemu
  Usuwa wszystkie kolejki klientów i kolejkę serwera, kończy program obsługujący serwer. (Uwaga: dane nie zostaną zapisane)

---
©Anna Panfil 2020-2021
