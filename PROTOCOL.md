# Protokół komunikacyjny klient-serwer

[//]: [TOC]

## Informacje ogólne
- Użyto kolejki komunikatów dla serwera o numerze 12345 i kolejki komunikatów dla każdego klienta – o numerze opartym o jego PID.
- Klient wysyła wiadomości do kolejki serwera (w trybie blokującym), serwer – do kolejki odpowiedniego klienta (z wyjątkiem błędu logowania)
- Maksymalna liczba zalogowanych użytkowników wynosi 15, maksymalna liczba utworzonych tematów – 20, maksymalna długość nazwy klienta/tematu – 30 znaków

## Struktura komunikatów
Podstawowy komunikat:

```c
struct msgbuf{
  long type;
  int id;
  union topic_tag topic_tag;
  int number;
  char text[1024];
};
```

- `type` – typ wysyłanej wiadomości. Poszczególne typy przedstawiono poniżej
- `id` – identyfikator klienta wysyłającego wiadomość
- `topic_tag` – identyfikacja tematu przez id lub nazwę

    ```c
    union topic_tag{
      char name[30];
      int  id;
    };
    ```

- `number`, `tekst` – pola do użycia w zależności od typu komunikatu (opisane w poszczególnych scenariuszach komunikacji)


Komunikat serwera informujący o stanie zakończenia operacji:

```c
struct msgserv{
  long type;
  int info;
};
```

- `type` odpowiada typowi operacji (lub numerowi kolejki klienta w przypadku logowania)
- `info` – informacja zwrotna dla klienta


#### Typy komunikatów
1. logowanie
1. rejestracja tematu
1. zapis na subskrybcję
1. nowa wiadomość
1. wyłączenie systemu

## Scenariusze komunikacji
#### Logowanie
  `topic_tag` – niewykorzystane<br>
  `number` – niewykorzystane<br>
  `text` – nazwa klienta

  Serwer zapisuje nowego klienta w tablicy struktur:

  ```c
  struct client{
    int id;
    char name[30];
  };
  ```
  Serwer umieszcza w **swojej** kolejce komunikatów wiadomość o typie = numerowi klienta. W wiadomości znajduje się nowy identyfikator klienta – jego numer w tablicy klientów. Gdy nazwa klienta istnieje już w bazie – zwraca -1, gdy limit klientów został przekroczony – -2.

  //Co jeżeli kolejka już istnieje?

#### Rejestracja tematu
  `topic_tag` – niewykorzystane<br>
  `number` – niewykorzystane<br>
  `text` – nazwa tematu

  Serwer zapisuje nowy temat w tablicy struktur:

  ```c
  struct topic{
    int id;
    char name[30];
    struct sub first_sub;
  };
  ```

`first_sub` jest początkiem listy jednokierunkowej z następującymi strukturami:

  ```c
  struct sub{
    int client_que; //numer kolejki klienta
    int length; //długość subskrybcji – ilość wiadomości lub -1 w przypadku subskrypcji na zawsze
    struct sub* next_sub;
  };
  ```

  Gdy nazwa tematu istnieje już w bazie, serwer umieszcza w  kolejce klienta wartość 1, gdy ilość tematów jest za duża – 2. W przeciwnym przypadku – 0.

#### Zapis na subskrybcję
  `topic_tag` – temat<br>
  `number` – długość subskrybcji (-1 w przypadku nieskończonej subskrypcji)<br>
  `text` – niewykorzystane

  Serwer dopisuje klienta do listy subskrybcji w danym temacie (określanym przez strukturę opisaną przy rejestracji tematu).
  Gdy temat nie istnieje, serwer umieszcza w  kolejce klienta wartość 1. W przeciwnym przypadku – 0.

#### Wysłanie nowej wiadomości
  `topic_tag` – temat<br>
  `number` – niewykorzystane<br>
  `text` – treść wiadomości

  Serwer rozsyła wiadomość do kolejek komunikatów wszystkich subskrybentów danego tematu (z wyłączeniem nadawcy wiadomości).
  Nie zwraca żadnej informacji do nadawcy.

#### Odbiór wiadomości w sposób synchroniczny


#### Odbiór wiadomości w sposób asynchroniczny
  Nie angażuje serwera. Sprawdza czy w kolejce komunikatów danego klienta są wiadomości określonego typu.

#### Wyłączenie systemu
  Usuwa wszystkie kolejki klientów i kolejkę serwera, kończy program obsługujący serwer. (Uwaga: dane nie zostaną zapisane)

---
©Anna Panfil 2020-2021
