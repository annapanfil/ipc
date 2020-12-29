//typ1 - logowanie
//typ2 - nowy temat
//typ3 - zapis na subskrybcję
//typ4 - nowa wiadomość
//typ5 - wyłączenie systemu

/*TODO:
- logowanie: nie zwiększać, jeśli nie dodał
- nowy temat: sprawdzić poprawność
- zapis na subskrybcję: dodać, sprawdzić poprawność
- nowa wiadomość: dodać, rozesłać
- połączyć struktury
*/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define NAME_LENGTH 30
#define MESSAGE_LENGTH 1024
#define TOPICS_NR 20
#define CLIENTS_NR 15
#define SERVER_QUE_NR 12345

union topic_tag{
    char name[NAME_LENGTH];
    int  id;
};

struct client{
  int id;
  char name[NAME_LENGTH];
};

struct sub{
  int client_id;
  int length; //-1 → forever
  struct sub* next_sub;
};

struct topic{
  //czy jest przypisany do konkretnego klienta czy wszyscy mogą w nim pisać?
  int id;
  char name[NAME_LENGTH];
  struct sub first_sub;
};

struct msgbuf{
  long type;
  int id;
  int number;
  char text[MESSAGE_LENGTH];
  union topic_tag topic_tag;
};

struct feedback{
  long type;
  int info;
};

void send_feedback(int que, int type, int info){
  struct feedback feedback;
  feedback.type = type;
  feedback.info = info;

  msgsnd(que, &feedback, sizeof(feedback)-sizeof(long), 0);
}

void login(struct msgbuf *message, struct client* clients, int* client_nr, int my_que){
  printf("\e[0;36mⓘ Login\e[m\n");

  if (*client_nr >= CLIENTS_NR){
    send_feedback(my_que, message->id, -2);  //limit klientów przekroczony
  }
  else{
    struct client client;
    client.id = msgget(message->id, 0);
    strcpy(client.name, message->text);
    *(clients+*client_nr) = client;

    for (int i=0; i<*client_nr; i++){
      if (strcmp((clients+i)->name, client.name) == 0){
        send_feedback(my_que, message->id, -1);  //nazwa klienta się powtarza
        return;
      }
    }

    printf("Hello %s\n", client.name);
    send_feedback(my_que, message->id, *client_nr);
    *client_nr = *client_nr + 1;
  }

  //UWAGA – teoretycznie 1 klient może się zalogować pod 2 różnymi nazwami – blokada po stronie klienta
}

void add_sub(){
}

void add_topic(struct msgbuf *message, struct topic* topics, int* topic_nr, struct client* clients){
  printf("\e[0;36mⓘ Add topic\e[m\n");
  struct topic topic;
  topic.id = message->number;
  strcpy(topic.name, message->text);
  struct sub sub;
  sub.next_sub = NULL;
  topic.first_sub = sub;
  *(topics+*topic_nr) = topic;

  printf("New topic: %s\n", topic.name);
  send_feedback((clients+message->id)->id, 1, *topic_nr);
  *topic_nr = *topic_nr+1;
}

void send_msgs(){
}

void shutdown(int me, struct client* clients, int last_client){
  printf("\e[0;36mⓘ Shutdown\e[m\n");
  for (int i=0; i<=last_client; i++){
    msgctl((clients+i)->id, IPC_RMID, NULL);
    printf("Goodbye %s\n", (clients+i)->name);
  }

  msgctl(me, IPC_RMID, NULL);
  printf("Goodbye!\n");
}

int main(int argc, char *argv[]) {
  //create server
  struct client clients[CLIENTS_NR];
  struct topic topics[TOPICS_NR]; //linked list?
  int next_client = 0;
  int next_topic = 0;
  struct msgbuf message;
  int my_que = msgget(SERVER_QUE_NR, 0644|IPC_CREAT);
  bool running = true;

  while (running) {
    printf("\e[0;36mⓘ Waiting for message\e[m\n");
    msgrcv(my_que, &message, sizeof(message)-sizeof(long), -5, 0);
    switch (message.type) {
      case 1: login(&message, clients, &next_client, my_que); break;
      case 2: add_topic(&message, topics, &next_topic, clients); break;
      case 3: add_sub(); break;
      case 4: send_msgs(); break;
      case 5: shutdown(my_que, clients, next_client-1); running = false; break;
    }

  }

  return 0;
}
