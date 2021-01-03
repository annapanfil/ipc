//typ1 - logowanie
//typ2 - nowy temat
//typ3 - zapis na subskrybcję
//typ4 - nowa wiadomość
//typ5 - odczyt tematów
//typ6 - wyłączenie systemu

//usuwanie wszystkich kolejek:
// ipcs -q | tail -n +4 | tr -s " " | cut -d" " -f 2 | xargs -I\{\} ipcrm -q {}

/*
TODO: powtarzające się subskrybcje
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

void print_subs(struct topic* topic){
  struct sub* sub = topic -> first_sub;
  printf("temat %s: ", topic->name);
  while (sub != NULL){
    printf("%d (%d) -> ", sub->client_que, sub->length);
    sub = sub->next_sub;
  }
  printf("\n");
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

    topic.first_sub = NULL;
    *(topics+*topic_nr) = topic;

    printf("New topic: %s\n", topic.name);
    // printf("id: %d\nclient que: %d\n", message->id, (clients+message->id)->que);
    send_feedback((clients+message->id)->que, 1, 0); //wszystko OK
    *topic_nr = *topic_nr+1;
  }
}

void add_sub(struct msgbuf *message, struct topic* topics, int last_topic, struct client* clients){
  printf("\e[0;36mⓘ Add_sub\e[m\n");

  print_subs(topics + message->topic);
  struct sub* new_sub;
  new_sub = (struct sub*)malloc(sizeof(struct sub));
  new_sub->client_que = (clients+message->id)->que;
  new_sub->length = message->number;

  if (message->topic > last_topic){
    send_feedback(new_sub->client_que, 1, 1); //temat nie istnieje
    return;
  }

  struct topic* topic = (topics + message->topic);

  //wstawiamy na początek listy
  new_sub->next_sub = topic->first_sub;
  topic->first_sub = new_sub;

  print_subs(topics + message->topic);
  send_feedback(new_sub->client_que, 1, 0);
}

void decrement_sub_length(struct sub* sub, struct sub* prev_sub, struct topic* topic){
  if(sub->length != -1){
    sub->length -= 1;
    if (sub->length == 0){ // sub to remove
      if (prev_sub != NULL){
        prev_sub -> next_sub = sub -> next_sub;
      }
      else{ //it was first sub
        topic -> first_sub = sub -> next_sub;
      }
      free(sub);
    }
  }
}

void send_msgs(struct msgbuf *msg_from_client, struct topic* topics, int last_topic, struct client* clients){
  printf("\e[0;36mⓘ Send messages\e[m\n");

  if ((msg_from_client->topic) > last_topic){
    send_feedback((clients+msg_from_client->id)->que, 1, 1);  //temat nie istnieje
    return;
  }
  else{
    send_feedback((clients+msg_from_client->id)->que, 1, 0);  //OK

    // print_subs(topics + (msg_from_client->topic));
    struct sub* sub = (topics + (msg_from_client->topic)) -> first_sub;
    struct sub* prev_sub = NULL;
    struct text_msg msg;
    msg.type = 2;
    sprintf(msg.text, "%s: %s", (topics + (msg_from_client->topic))->name, msg_from_client->text);

    //send messages to all subscribents
    while (sub != NULL){
      if (sub->client_que != (clients+msg_from_client->id)->que){ //nie odsyłaj do nadawcy
        msgsnd(sub->client_que, &msg, sizeof(msg)-sizeof(long), 0);
        decrement_sub_length(sub, prev_sub, topics + (msg_from_client->topic));
      }

      prev_sub = sub;
      sub = sub->next_sub;
    }
  }
  // print_subs(topics + (msg_from_client->topic));
}

void send_topics(struct msgbuf *msg_from_client, struct topic* topics, int last_topic, struct client* clients){
  int que = (clients + msg_from_client->id) -> que;
  struct text_msg message;
  message.type = 3;
  for (int i=0; i<= last_topic; i++){
    sprintf(message.text, "%d. %s", i, (topics + i)-> name);
    printf("%s\n", message.text);
    msgsnd(que, &message, sizeof(message)-sizeof(long), 0);
  }
  strcpy(message.text, "");
  msgsnd(que, &message, sizeof(message)-sizeof(long), 0);
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
  struct topic topics[TOPICS_NR];
  int next_client = 0;
  int next_topic = 0;
  struct msgbuf message;
  int my_que = msgget(SERVER_QUE_NR, 0644|IPC_CREAT);
  bool running = true;
  int info;

  while (running) {
    // printf("\e[0;36mⓘ Waiting for message\e[m\n");
    msgrcv(my_que, &message, sizeof(message)-sizeof(long), -6, 0);
    // printf("\e[0;36mⓘ Received %ld\e[m\n", message.type);
    switch (message.type) {
      case 1:
        info = login(&message, clients, &next_client);
        send_feedback(my_que, message.id, info);
        break;
      case 2: add_topic(&message, topics, &next_topic, clients); break;
      case 3: add_sub(&message, topics, next_topic-1, clients); break;
      case 4: send_msgs(&message, topics, next_topic-1, clients); break;
      case 5: send_topics(&message, topics, next_topic-1, clients); break;
      case 6: shutdown(my_que, clients, next_client-1); running = false; break;
    }
  }

  return 0;
}
