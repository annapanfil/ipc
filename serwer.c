//typ1 - logowanie
//typ2 - nowy temat
//typ3 - zapis na subskrybcję
//typ4 - nowa wiadomość
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>


struct client{
  int id;
  char name[30];
};

struct topic{
  //czy jest przypisany do konkretnego klienta czy wszyscy mogą w nim pisać?
  int id;
  char name[30];
  int subs[10][2]; //id, lenght
};

struct msgbuf{
  long type;
  int number;
  char text[1024];
};


void login(struct msgbuf *message, struct client* client_place){
  printf("Hello there\n");
  struct client client;
  client.id = message->number; //TODO
  strcpy(client.name, message->text);
  *client_place = client;
}

void add_sub(){
}

void add_topic(){
}

void send_msgs(){
  //czy powinien wysyłać wiadomość, do klienta, który ją wysłał?
}

void listen(){
}

int main(int argc, char *argv[]) {
  //create server
  struct client clients[10];
  struct topic topics[10]; //linked list?
  struct client* next_client = clients;
  struct topic* next_topic = topics;
  struct msgbuf message;
  int id = msgget(12340, 0644|IPC_CREAT);


  while (1) {
    msgrcv(id, &message, sizeof(message)-sizeof(long), 0, 0);
    switch (message.type) {
      case 1:
        login(&message, next_client++);
        printf("%s %d\n", (next_client-1)->name, next_client);
        break;
    }

  }

  return 0;
}
