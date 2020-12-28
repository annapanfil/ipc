//typ1 - logowanie
//typ2 - nowy temat
//typ3 - zapis na subskrybcję
//typ4 - nowa wiadomość
//typ5 - wyłączenie systemu

/*TODO:
- logowanie: sprawdzić poprawność danych i wysłać informację zwrotną
- nowy temat: dodać, sprawdzić poprawność
- zapis na subskrybcję: dodać, sprawdzić poprawność
- nowa wiadomość: dodać, rozesłać
*/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

union topic_tag{
    char name[30];
    int  id;
};

struct client{
  int id;
  char name[30];
};

struct sub{
  int client_id;
  int length; //-1 → forever
  struct sub* next_sub;
};

struct topic{
  //czy jest przypisany do konkretnego klienta czy wszyscy mogą w nim pisać?
  int id;
  char name[30];
  struct sub first_sub;
};

struct msgbuf{
  long type;
  int id;
  int number;
  char text[1024];
  union topic_tag topic_tag;
};

struct msgserv{
  long type;
  int info;
};

void login(struct msgbuf *message, struct client* client_place, int my_que){
  struct client client;
  client.id = msgget(message->id, 0644|IPC_CREAT);
  strcpy(client.name, message->text);
  *client_place = client;
  printf("Hello %s\n", client.name);

  struct msgserv feedback;
  feedback.type = message->id;
  feedback.info = 5;

  msgsnd(my_que, &feedback, sizeof(feedback)-sizeof(long), 0);
}

void add_sub(){
}

void add_topic(struct msgbuf *message, struct topic* topic_place){
  struct topic topic;
  topic.id = message->number;
  strcpy(topic.name, message->text);
  struct sub sub;
  sub.next_sub = NULL;
  topic.first_sub = sub;
  *topic_place = topic;
  printf("New topic: %s\n", topic.name);
}

void send_msgs(){
}

void shutdown(int me, struct client* first_client, struct client* last_client){
  for (;last_client >= first_client; last_client--){
    msgctl(last_client->id, IPC_RMID, NULL);
    printf("Goodbye %s\n", last_client->name);
  }
  msgctl(me, IPC_RMID, NULL);
  printf("Goodbye!\n");
}

int main(int argc, char *argv[]) {
  //create server
  struct client clients[10];
  struct topic topics[10]; //linked list?
  struct client* next_client = clients;  //numer, nie wskaźnik, przekazywać początek i numer
  struct topic* next_topic = topics;
  struct msgbuf message;
  int my_que = msgget(12345, 0644|IPC_CREAT);
  bool running = true;

  while (running) {
    msgrcv(my_que, &message, sizeof(message)-sizeof(long), -5, 0);
    switch (message.type) {
      case 1: login(&message, next_client++, my_que); break;
      case 2: add_topic(&message, next_topic++); break;
      case 3: add_sub(); break;
      case 4: send_msgs(); break;
      case 5: shutdown(my_que, clients, next_client-1); running = false; break;
    }

  }

  return 0;
}
