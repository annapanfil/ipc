# Protokół komunikacyjny klient-serwer

[//]: [TOC]

## Informacje ogólne
- Użyto kolejki komunikatów dla serwera o kluczu 12345 i kolejki komunikatów dla każdego klienta – o kluczu opartym o jego PID.
- Klient wysyła wiadomości do kolejki serwera (w trybie blokującym), serwer – do kolejki odpowiedniego klienta (z wyjątkiem błędu logowania)
- Maksymalna liczba zalogowanych użytkowników jest ograniczona stałą `CLIENTS_NR`. Maksymalna liczba utworzonych tematów – `TOPICS_NR`, maksymalna długość nazwy klienta/tematu – `NAME_LENGTH` znaków, maksymalna długość wiadomości: `MESSAGE_LENGTH` znaków.

## Struktura komunikatów
#### Podstawowy komunikat

```c
struct msgbuf{
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

#### Komunikat serwera informujący o stanie zakończenia operacji

```c
struct feedback{
  long type;
  int info;
};
```

- `type` – 1 (lub numer kolejki klienta w przypadku logowania)
- `info` – informacja zwrotna dla klienta

#### Wiadomość tekstowa od serwera
```c
struct text_msg{
  long type;
  char text[MESSAGE_LENGTH+NAME_LENGTH+2];
};
```

- `type` – 2 (wiadomość do tematu) lub 3 (lista tematów na serwerze)
- `text` – treść wiadomości

#### Typy podstawowych komunikatów
1. logowanie
1. rejestracja tematu
1. zapis na subskrybcję
1. nowa wiadomość
1. wyłączenie systemu
1. zapytanie o tematy

## Scenariusze komunikacji
#### Logowanie
  `topic` – niewykorzystane<br>
  `number` – niewykorzystane<br>
  `text` – nazwa klienta

  Serwer zapisuje nowego klienta w tablicy struktur:

  ```c
  struct client{
    int id;
    char name[NAME_LENGTH];
  };
  ```
  Serwer umieszcza w **swojej** kolejce komunikatów wiadomość o typie równym numerowi klienta. W wiadomości znajduje się nowy identyfikator klienta – jego numer w tablicy klientów. Gdy nazwa klienta istnieje już w bazie – zwraca -1, gdy limit klientów został przekroczony: -2.

  Podczas swojej sesji klient może logować się tylko raz.

#### Rejestracja tematu
  `topic` – niewykorzystane<br>
  `number` – niewykorzystane<br>
  `text` – nazwa tematu

  Serwer zapisuje nowy temat w tablicy struktur:

  ```c
  struct topic{
    int id;
    char name[NAME_LENGTH];
    struct sub{
      int client_id;
      int length; //-1 → forever
      struct sub* next_sub;
    }* first_sub;
  };
  ```

  `first_sub` jest początkiem listy jednokierunkowej.

  Gdy nazwa tematu istnieje już w bazie, serwer umieszcza w  kolejce klienta wartość 1, gdy limit tematów został przekroczony: 2. W przeciwnym przypadku: 0.

#### Zapis na subskrybcję
  `number` – długość subskrybcji (-1 w przypadku nieskończonej subskrypcji)<br>
  `text` – niewykorzystane

  Serwer dopisuje klienta do listy subskrybcji w danym temacie (określanym przez strukturę opisaną przy rejestracji tematu).
  Gdy temat nie istnieje, serwer umieszcza w  kolejce klienta wartość 1. W przeciwnym przypadku: 0.

#### Wysłanie nowej wiadomości
  `number` – niewykorzystane<br>
  `text` – treść wiadomości

  Serwer rozsyła wiadomość do kolejek komunikatów wszystkich subskrybentów danego tematu (z wyłączeniem nadawcy wiadomości).
  Gdy temat nie istnieje, serwer umieszcza w  kolejce klienta wartość 1. W przeciwnym przypadku: 0.

#### Odbiór wiadomości w sposób synchroniczny
  Nie angażuje serwera. Tworzy proces potomny, który ciągle sprawdza, czy w kolejce komunikatów danego klienta są wiadomości.

#### Odbiór wiadomości w sposób asynchroniczny
  Nie angażuje serwera. Sprawdza, czy w kolejce komunikatów danego klienta są wiadomości i odbiera wszystkie.

#### Odczyt tematów z serwera

#### Wyłączenie systemu
  Usuwa wszystkie kolejki klientów i kolejkę serwera, kończy program obsługujący serwer. (Uwaga: dane nie zostaną zapisane)

---
©Anna Panfil 2020-2021
