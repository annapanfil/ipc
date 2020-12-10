//typ1 - logowanie
//typ2 - nowy temat
//typ3 - zapis na subskrybcję
//typ4 - nowa wiadomość
//typ5 - wyłączenie systemu
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>


struct client{
  int id;
  char name[30];
};

struct topic{
  //czy jest przypisany do konkretnego klienta czy wszyscy mogą w nim pisać?
  int id;
  char name[30];
  int subs[10][2]; //id, length (-1 -> forever)
};

struct msgbuf{
  long type;
  int number;
  char text[1024];
};


void login(struct msgbuf *message, struct client* client_place){
  struct client client;
  client.id = msgget(message->number, 0644|IPC_CREAT);
  strcpy(client.name, message->text);
  *client_place = client;
  printf("Hello %s\n", client.name);
}

void add_sub(){
}

void add_topic(){
}

void send_msgs(){
  //czy powinien wysyłać wiadomość, do klienta, który ją wysłał?
}

void shutdown(int me, struct client* first_client, struct client* last_client){
  for (;last_client >= first_client; last_client--){
    msgctl(last_client->id, IPC_RMID, NULL);
    printf("Closed %s\n", last_client->name);
  }
  msgctl(me, IPC_RMID, NULL);
  printf("Goodbye!\n");
}

int main(int argc, char *argv[]) {
  //create server
  struct client clients[10];
  struct topic topics[10]; //linked list?
  struct client* next_client = clients;
  struct topic* next_topic = topics;
  struct msgbuf message;
  int my_id = msgget(12340, 0644|IPC_CREAT);
  bool running = true;

  while (running) {
    msgrcv(my_id, &message, sizeof(message)-sizeof(long), 0, 0);
    switch (message.type) {
      case 1: login(&message, next_client++); break;
      case 2: add_topic(); break;
      case 3: add_sub(); break;
      case 4: send_msgs(); break;
      case 5: shutdown(my_id, clients, next_client-1); running = false; break;
    }

  }

  return 0;
}
