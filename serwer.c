//typ1 - logowanie
//typ2 - nowy temat
//typ3 - zapis na subskrybcję
//typ4 - nowa wiadomość
//typ5 - wyłączenie systemu

//usuwanie wszystkich kolejek:
// ipcs -q | tail -n +4 | tr -s " " | cut -d" " -f 2 | xargs -I\{\} ipcrm -q {}

/*TODO:
- nowy temat: sprawdzić poprawność
- zapis na subskrybcję: dodać, sprawdzić poprawność
- nowa wiadomość: dodać, rozesłać
*/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define NAME_LENGTH 30
#define MESSAGE_LENGTH 1024
#define TOPICS_NR 20
#define CLIENTS_NR 15
#define SERVER_QUE_NR 12345


struct client{
  int que;
  char name[NAME_LENGTH];
};

struct topic{
  //czy jest przypisany do konkretnego klienta czy wszyscy mogą w nim pisać?
  int id;
  char name[NAME_LENGTH];
  struct sub{
    int client_que;
    int length; //-1 → forever
    struct sub* next_sub;
  } *first_sub;
};

struct msgbuf{
  long type;
  int id;
  int topic;
  int number;
  char text[MESSAGE_LENGTH];
};

struct feedback{
  long type;
  int info;
};

struct text_msg{
  long type;
  char text[MESSAGE_LENGTH+NAME_LENGTH+2];
};

void send_feedback(int que, int type, int info){
  struct feedback feedback;
  feedback.type = type;
  feedback.info = info;

  msgsnd(que, &feedback, sizeof(feedback)-sizeof(long), 0);
}

int login(struct msgbuf *message, struct client* clients, int* client_nr){
  printf("\e[0;36mⓘ Login\e[m\n");

  if (*client_nr >= CLIENTS_NR){
    return -2;  //limit klientów przekroczony
  }
  else{
    struct client client;
    client.que = msgget(message->id, 0);
    strcpy(client.name, message->text);
    *(clients+*client_nr) = client;

    for (int i=0; i<*client_nr; i++){
      if (strcmp((clients+i)->name, client.name) == 0){
        return -1;  //nazwa klienta się powtarza
      }
    }

    printf("Hello %s\n", client.name);
    *client_nr = *client_nr + 1;
    return *client_nr-1;
  }

  //UWAGA – teoretycznie 1 klient może się zalogować pod 2 różnymi nazwami – blokada po stronie klienta
}

void add_sub(struct msgbuf *message, struct topic* topics, int last_topic, struct client* clients){
  printf("\e[0;36mⓘ Add_sub\e[m\n");
  struct sub* new_sub;
  new_sub = (struct sub*)malloc(sizeof(struct sub));
  new_sub->client_que = (clients+message->id)->que;
  new_sub->length = message->number;

  if (message->topic > last_topic){
    send_feedback(new_sub->client_que, 1, 1); //temat nie istnieje
    return;
  }

  struct topic* topic = (topics + message->topic);

  // printf("adres kolejnego: %d \n", topic->first_sub->next_sub);
  // struct sub* old_address = topic->first_sub;
  
  //wstawiamy na początek listy
  new_sub->next_sub = topic->first_sub;
  topic->first_sub = new_sub;

  // wypisanie całej listy
  // struct sub* sub = topic->first_sub;
  // while (sub->next_sub != NULL){
  //   printf("\e[0;36m%d ->\e[m", sub->client_que);
  //   sub = sub->next_sub;
  // }
  // printf("\n");

  // printf("powinien być taki adres kolejnego: %d pośrednio: %d jest taki: %d\n", (old_address->next_sub), (new_sub->next_sub->next_sub), (topic->first_sub->next_sub->next_sub));

  send_feedback(new_sub->client_que, 1, 0);
}

void add_topic(struct msgbuf *message, struct topic* topics, int* topic_nr, struct client* clients){
  printf("\e[0;36mⓘ Add topic\e[m\n");
  if (*topic_nr >= TOPICS_NR){
    send_feedback((clients+message->id)->que, 1, 2);  //limit tematów przekroczony
    return;
  }
  else{
    struct topic topic;
    topic.id = message->number;
    strcpy(topic.name, message->text);

    for (int i=0; i<*topic_nr; i++){
      if (strcmp((topics+i)->name, topic.name) == 0){
        send_feedback((clients+message->id)->que, 1, 1);  //nazwa tematu się powtarza
        return;
      }
    }

    struct sub* sub;
    sub = (struct sub*)malloc(sizeof(struct sub));
    sub->next_sub = NULL;
    topic.first_sub = sub;
    *(topics+*topic_nr) = topic;

    printf("New topic: %s\n", topic.name);
    // printf("id: %d\nclient que: %d\n", message->id, (clients+message->id)->que);
    send_feedback((clients+message->id)->que, 1, 0); //wszystko OK
    *topic_nr = *topic_nr+1;
  }
}

void send_msgs(struct msgbuf *msg_from_client, struct topic* topics, struct client* clients){
  printf("\e[0;36mⓘ Add topic\e[m\n");
  struct sub* sub = (topics + (msg_from_client->topic)) -> first_sub;
  while (sub->next_sub != NULL){
    printf("checking %d\n", sub->client_que);
    if (sub->client_que != (clients+msg_from_client->id)->que){ //nie odsyłaj do nadawcy
      //send message
      struct text_msg msg;
      msg.type = 2;
      strcpy(msg.text, msg_from_client->text); //TODO: dodać "<temat>: " na początku wiadomości (sprintf?)

      printf("sending to %d\n", sub->client_que);
      msgsnd(sub->client_que, &msg, sizeof(msg)-sizeof(long), 0);

      //decrement subscription lenght
      if(sub->length != -1){
        sub->length -= 1;
        // if (sub->length == 0){
        //TODO: delete sub from list
        //delete sub object
        // }

      }
    }
    printf("%d\n", sub->next_sub->client_que);
    sub = sub->next_sub;
  }

}

void shutdown(int me, struct client* clients, int last_client){
  printf("\e[0;36mⓘ Shutdown\e[m\n");
  for (int i=0; i<=last_client; i++){
    msgctl((clients+i)->que, IPC_RMID, NULL);
    printf("Goodbye %s\n", (clients+i)->name);
  }

  msgctl(me, IPC_RMID, NULL);
  printf("Goodbye!\n");

  //Czy tu powinnam jeszcze zwolnić pamięć z list subskrybcji?
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
  int info;

  while (running) {
    printf("\e[0;36mⓘ Waiting for message\e[m\n");
    msgrcv(my_que, &message, sizeof(message)-sizeof(long), -5, 0);
    printf("\e[0;36mⓘ Received %ld\e[m\n", message.type);
    switch (message.type) {
      case 1:
        info = login(&message, clients, &next_client);
        send_feedback(my_que, message.id, info);
        break;
      case 2: add_topic(&message, topics, &next_topic, clients); break;
      case 3:
        add_sub(&message, topics, next_topic-1, clients);
        // printf("%d\n", (topics+message.topic)->first_sub.client_que);
        break;
      case 4: send_msgs(&message, topics, clients); break;
      case 5: shutdown(my_que, clients, next_client-1); running = false; break;
    }
  }

  return 0;
}
