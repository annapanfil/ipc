# Protokół komunikacyjny klient-serwer

[//]: [TOC]

## Informacje ogólne
- Użyto kolejki komunikatów dla serwera o numerze 1234 i kolejki domunikatów dla każdego klienta – o numerze opartym o jego PID.
- Maksymalna liczba zalogowanych użytkowników wynosi 15, maksymalna liczba utworzonych tematów – 20.

## Struktura komunikatów
```c
struct msgbuf{
  long type;
  int id;
  int number;
  char text[1024];
};
```

- `type` – typ wysyłanej wiadomości. Poszczególne typy przedstawiono poniżej
- `id` – identyfikator klienta wysyłającego wiadomość
- `number`, `tekst` – pola do użycia w zależności od typu komunikatu

#### Typy komunikatów
1. logowanie
1. nowy temat
1. zapis na subskrybcję
1. nowa wiadomość
1. wyłączenie systemu

## Scenariusze komunikacji
#### Rejestracja obiorcy
#### Rejestracja tematu
#### Wysłanie nowej wiadomości
#### Odbiór wiadomości w sposób synchroniczny
####  Odbiór wiadomości w sposób asynchroniczny
