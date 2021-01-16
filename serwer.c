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
#define CLIENTS_NR 20
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

struct client_msg{
  long type;
  int id;
  int topic;
  int number;
  char text[MESSAGE_LENGTH];
};

struct server_msg{
  long type;
  int info;
  char text[MESSAGE_LENGTH+NAME_LENGTH+2];
};

void send_feedback(int que, int type, int info){
  struct server_msg feedback;
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


int login(struct client_msg *message, struct client* clients, int* client_nr){
  // printf("\e[0;36mⓘ Login\e[m\n");

  if (*client_nr >= CLIENTS_NR){
    return -2;  //too many clients
  }
  else{
    struct client client;
    client.que = msgget(message->id, 0);
    strcpy(client.name, message->text);
    *(clients+*client_nr) = client;

    for (int i=0; i<*client_nr; i++){
      if (strcmp((clients+i)->name, client.name) == 0){
        return -1;  //existing username
      }
    }

    printf("Hello %s\n", client.name);
    *client_nr = *client_nr + 1;
    return *client_nr-1;
  }

  //UWAGA – teoretycznie 1 klient może się zalogować pod 2 różnymi nazwami – blokada jest po stronie klienta
}


void add_topic(struct client_msg *message, struct topic* topics, int* topic_nr, struct client* clients){
  // printf("\e[0;36mⓘ Add topic\e[m\n");
  if (*topic_nr >= TOPICS_NR){
    send_feedback((clients+message->id)->que, 2, 2);  //too many topics
    return;
  }
  else{
    struct topic topic;
    topic.id = message->number;
    strcpy(topic.name, message->text);

    for (int i=0; i<*topic_nr; i++){
      if (strcmp((topics+i)->name, topic.name) == 0){
        send_feedback((clients+message->id)->que, 2, 1);  //existing name of topic
        return;
      }
    }

    topic.first_sub = NULL;
    *(topics+*topic_nr) = topic;

    printf("New topic: %s\n", topic.name);
    send_feedback((clients+message->id)->que, 2, 0); //OK
    *topic_nr = *topic_nr+1;
  }
}

void add_sub(struct client_msg *message, struct topic* topics, int last_topic, struct client* clients){
  // printf("\e[0;36mⓘ Add_sub\e[m\n");

  struct sub* new_sub;
  new_sub = (struct sub*)malloc(sizeof(struct sub));
  new_sub->client_que = (clients+message->id)->que;
  new_sub->length = message->number;

  if (message->topic > last_topic){
    send_feedback(new_sub->client_que, 2, 1); //topic doesn't exist
    return;
  }

  struct topic* topic = (topics + message->topic);

  //insert at the beginning of the list
  new_sub->next_sub = topic->first_sub;
  topic->first_sub = new_sub;

  send_feedback(new_sub->client_que, 2, 0);
}

void decrement_sub_length(struct sub* sub, struct sub* prev_sub, struct topic* topic, int que){
  if(sub->length != -1){
    sub->length -= 1;
    if (sub->length == 0){ // sub to remove

      struct server_msg msg;  //send msg to client – subscribtion ended
      msg.type = 3;
      sprintf(msg.text, "%d. %s", topic->id, topic->name);
      msgsnd(sub->client_que, &msg, sizeof(msg)-sizeof(long), 0);

      if (prev_sub != NULL){
        prev_sub -> next_sub = sub -> next_sub;
      }
      else{ //it was the first sub
        topic -> first_sub = sub -> next_sub;
      }
      free(sub);
    }
  }
}

void send_msgs(struct client_msg *msg_from_client, struct topic* topics, int last_topic, struct client* clients){
  // printf("\e[0;36mⓘ Send messages\e[m\n");

  if ((msg_from_client->topic) > last_topic){
    send_feedback((clients+msg_from_client->id)->que, 2, 1);  //topic doesn't exist
    return;
  }
  else{
    send_feedback((clients+msg_from_client->id)->que, 2, 0);  //OK

    struct sub* sub = (topics + (msg_from_client->topic)) -> first_sub;
    struct sub* prev_sub = NULL;
    struct server_msg msg;
    msg.type = msg_from_client->topic + 4;
    msg.info = msg_from_client->number;
    sprintf(msg.text, "%s: %s", (topics + (msg_from_client->topic))->name, msg_from_client->text);

    //send messages to all subscribents
    while (sub != NULL){
      if (sub->client_que != (clients+msg_from_client->id)->que){ //don't resend to sender
        msgsnd(sub->client_que, &msg, sizeof(msg)-sizeof(long), 0);
        decrement_sub_length(sub, prev_sub, topics + (msg_from_client->topic), (clients+msg_from_client->id)->que);
      }

      prev_sub = sub;
      sub = sub->next_sub;
    }
  }
}

void send_topics(struct client_msg *msg_from_client, struct topic* topics, int last_topic, struct client* clients){
  int que = (clients + msg_from_client->id) -> que;
  struct server_msg message;
  message.type = 1;
  for (int i=0; i<= last_topic; i++){
    sprintf(message.text, "%d. %s", i, topics[i].name);
    msgsnd(que, &message, sizeof(message)-sizeof(long), 0);
  }
  strcpy(message.text, "");
  msgsnd(que, &message, sizeof(message)-sizeof(long), 0);
}

void send_subed_topics(struct client_msg *msg_from_client, struct topic* topics, int last_topic, struct client* clients){
  int que = (clients + msg_from_client->id) -> que;
  struct server_msg message;
  message.type = 1;
  for (int i=0; i<= last_topic; i++){
    struct sub* sub = topics[i].first_sub;
    while (sub != NULL){
      if (sub->client_que == que){
        sprintf(message.text, "%d. %s", i, topics[i].name);
        msgsnd(que, &message, sizeof(message)-sizeof(long), 0);
        break;
      }
      sub = sub->next_sub;
    }
  }
  strcpy(message.text, "");
  msgsnd(que, &message, sizeof(message)-sizeof(long), 0);
}

void shutdown(int me, struct client* clients, int last_client){
  // printf("\e[0;36mⓘ Shutdown\e[m\n");
  for (int i=0; i<=last_client; i++){
    msgctl((clients+i)->que, IPC_RMID, NULL);
    printf("Goodbye %s\n", (clients+i)->name);
  }

  msgctl(me, IPC_RMID, NULL);
  printf("Goodbye!\n");
}


int main(int argc, char *argv[]) {
  struct client clients[CLIENTS_NR];
  struct topic topics[TOPICS_NR];
  int next_client = 0;
  int next_topic = 0;
  struct client_msg message;
  int my_que = msgget(SERVER_QUE_NR, 0644|IPC_CREAT);
  bool running = true;
  int info;

  while (running) {
    // printf("\e[0;36mⓘ Waiting for message\e[m\n");
    msgrcv(my_que, &message, sizeof(message)-sizeof(long), -7, 0); //WARNING: only 7. Add if needed
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
      case 6: send_subed_topics(&message, topics, next_topic-1, clients); break;
      case 7: shutdown(my_que, clients, next_client-1); running = false; break;
    }
  }

  return 0;
}
