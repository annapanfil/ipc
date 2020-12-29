#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/*TODO:
- logowanie: komunikacja z uzytkownikiem, zwrócenie 2 wartości
- nowy temat: komunikacja z uzytkownikiem
- zapis na subskrybcję: komunikacja z uzytkownikiem
- nowa wiadomość: komunikacja z uzytkownikiem
- otrzymywanie wiadomości
- menu główne
- reakcja na błędy
- podział na mniejsze funkcje?
*/
#define NAME_LENGTH 30
#define MESSAGE_LENGTH 1024
#define SERVER_QUE_NR 12345

union topic_tag{
    char name[NAME_LENGTH];
    int  id;
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

int take_feedback(int que, int type){
  struct feedback feedback;
  msgrcv(que, &feedback, sizeof(feedback)-sizeof(long), type, 0);
  printf("Info: %d\n", feedback.info);
    return feedback.info;
}

int login(int server, int* que, char name[NAME_LENGTH]){
  struct msgbuf login_msg;
  int que_key = getpid();
  *que = msgget(que_key, 0644|IPC_CREAT);
  login_msg.type = 1;
  login_msg.id = que_key;
  strcpy(login_msg.text, name);
  msgsnd(server, &login_msg, sizeof(login_msg)-sizeof(long), 0);
  return que_key;
}

void login_menu(){
  //login
  char name[NAME_LENGTH];
  printf("Dzień dobry! Podaj swoją nazwę użytkownika: "); //nie może zawierać spacji
  name = "Obi wan"
  scanf("%s", name);
  int que_key = login(server, &que, name);
  nr_on_server = take_feedback(server, que_key);
  if (nr_on_server == -1){
    printf("Taka nazwa już istnieje. Podaj inną nazwę.\n");
  }
  else if(nr_on_server == -2){
    printf("Na tym serwerze jest zbyt wielu klientów. Nie możesz się zalogować\n");
    return 0;
  }
}

void register_topic(int server, int id, char topic_name[NAME_LENGTH], int que){
  struct msgbuf topic_msg;
  topic_msg.type = 2;
  topic_msg.id = id;
  strcpy(topic_msg.text, topic_name);
  msgsnd(server, &topic_msg, sizeof(topic_msg)-sizeof(long), 0);

  int feedback = take_feedback(que, 0);
  // if (feedback != 0)
    //obsługa błędów
}


void register_sub(int server, union topic_tag topic, int sublen){ //-1 -> forever
  struct msgbuf sub_msg;
  sub_msg.type = 3;
  sub_msg.topic_tag = topic;
  sub_msg.number = sublen;
  msgsnd(server, &sub_msg, sizeof(sub_msg)-sizeof(long), 0);
}

void send_msg(int server, union topic_tag topic, char msg_text[MESSAGE_LENGTH]){
  struct msgbuf message;
  message.type = 4;
  message.topic_tag = topic;
  strcpy(message.text, msg_text);
  msgsnd(server, &message, sizeof(message)-sizeof(long), 0);
}

void receive_msg_sync(){
}

void receive_msg_async(){ //???
}

void shutdown(int server){
  struct msgbuf message;
  message.type = 5;
  msgsnd(server, &message, sizeof(message)-sizeof(long), 0);
}

int main(int argc, char *argv[]) {
  int server = msgget(SERVER_QUE_NR, 0);
  int que;
  int nr_on_server;

  while (nr_on_server < 0){
    nr_on_server = login_menu();
  }

  printf("%d %d\n", que, nr_on_server);
  register_topic(server, nr_on_server, "sport", que);
  register_topic(server, nr_on_server, "technology", que);
  shutdown(server);

  return 0;
}
