#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h> //kill

/*TODO:
- odczyt tematów z serwera
- spacje przy inpucie
*/

#define NAME_LENGTH 30
#define MESSAGE_LENGTH 1024
#define SERVER_QUE_NR 12345

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

int take_feedback(int que, int type){
  struct feedback feedback;
  msgrcv(que, &feedback, sizeof(feedback)-sizeof(long), type, 0);
  // printf("Info: %d\n", feedback.info);
    return feedback.info;
}

void print_error(char text[100]){
  printf("\e[0;31m%s\n\e[m", text);
}

void print_info(char text[100]){
  printf("\e[0;34m%s\e[m", text);
}

int get_topics(int server, int id, int que){
  // int nr_of_topics = 0;
  // struct msgbuf msg;
  // msg.type = 5;
  // msg.id = id;
  // msgsnd(server, &msg, sizeof(msg)-sizeof(long), 0);
  //
  // struct text_msg message;
  // printf("\n-------TEMATY NA SERWERZE-------\n");
  // while (MESSAGE_TEXT != "NULL000"){
  //   msgrcv(que, &message, sizeof(message)-sizeof(long), 3, IPC_NOWAIT);
  //   printf("%s\n", message.text);
  //   }
  // }
  // return nr_of_topics();
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

void register_topic(int server, int id, char topic_name[NAME_LENGTH]){
  struct msgbuf topic_msg;
  topic_msg.type = 2;
  topic_msg.id = id;
  strcpy(topic_msg.text, topic_name);
  msgsnd(server, &topic_msg, sizeof(topic_msg)-sizeof(long), 0);
}

void register_sub(int server, int id, int topic, int sublen){ //-1 -> forever
  struct msgbuf sub_msg;
  sub_msg.type = 3;
  sub_msg.id = id;
  sub_msg.topic = topic;
  sub_msg.number = sublen;
  msgsnd(server, &sub_msg, sizeof(sub_msg)-sizeof(long), 0);
}

void send_msg(int server, int id, int topic, char msg_text[MESSAGE_LENGTH]){
  struct msgbuf message;
  message.type = 4;
  message.id = id;
  message.topic = topic;
  strcpy(message.text, msg_text);
  msgsnd(server, &message, sizeof(message)-sizeof(long), 0);
}

void receive_msg_sync(int* pid, int que){
  if (*pid == -1){  //nie odbierał → zaczyna odbierać
    struct text_msg message;
    int x;
    if ((x=fork()) == 0){
      while (1) {
        msgrcv(que, &message, sizeof(message)-sizeof(long), 2, 0);
        printf("%s\n", message.text);
      }
    }
    *pid = x;
    print_info("Włączono synchroniczne odbieranie wiadomości\n");
  }
  else{  //odbierał → kończy odbieranie
    kill(*pid, 15);
    *pid = -1;
      print_info("Wyłączono synchroniczne odbieranie wiadomości\n");
  }
}

void receive_msg_async(int que){
  int size = 0;
  struct text_msg message;
  while (size != -1){
    size = msgrcv(que, &message, sizeof(message)-sizeof(long), 2, IPC_NOWAIT);
    if (size != -1){
      printf("%s\n", message.text);
    }
  }
  print_info("Nie masz więcej nowych wiadomości.\n");
}

void shutdown(int server){
  struct msgbuf message;
  message.type = 5;
  msgsnd(server, &message, sizeof(message)-sizeof(long), 0);
}

int login_menu(int server, int* que){
  int nr_on_server = -1;
  while (nr_on_server < 0){
    //login
    char name[NAME_LENGTH] = "Obi wan";
    print_info("Dzień dobry! Podaj swoją nazwę użytkownika: ");
    scanf("%s", name); //nie może zawierać spacji
    int que_key = login(server, que, name);
    nr_on_server = take_feedback(server, que_key);
    if (nr_on_server == -1){
      print_error("Taka nazwa już istnieje. Podaj inną nazwę.");
    }
    else if(nr_on_server == -2){
      print_error("Na tym serwerze jest zbyt wielu klientów. Nie możesz się zalogować");
      msgctl(*que, IPC_RMID, NULL);
      exit(0);
    }
    else if(nr_on_server >= 0)
      print_info("Zalogowano\n");
  }
  return nr_on_server;
}

void msg_menu(int server, int id, int que){
    char msg[MESSAGE_LENGTH] = "Empty message";
    int topic;
    get_topics(server, id, que);
    print_info("Podaj numer tematu: ");
    scanf("%d", &topic);
    print_info("Wpisz treść wiadomości: \n> "); //nie może zawierać spacji
    scanf("%s", msg);
    send_msg(server, id, topic, msg);

    int feedback = take_feedback(que, 1);
    if (feedback == 1){
      print_error("Taki temat nie istnieje.");
    }
    else if (feedback == 0)
      print_info("Wiadomość wysłana");
}

void topic_menu(int server, int id, int que){
    char topic[NAME_LENGTH] = "New topic";

    print_info("Podaj nazwę tematu: "); //nie może zawierać spacji
    scanf("%s", topic);

    register_topic(server, id, topic);

    int feedback = take_feedback(que, 1);
    if (feedback == 1){
      print_error("Taki temat już istnieje.");
    }
    else if(feedback == 2){
      print_error("Na tym serwerze jest zbyt wiele tematów. Nie możesz dodać kolejnego.");
    }
    else if(feedback == 0)
      print_info("Dodano temat");
}

void sub_menu(int server, int id, int que){
    int topic;
    int length;

    print_info("Podaj numer tematu, który chcesz subskrybować: ");
    scanf("%d", &topic);
    print_info("Ile wiadomości z tego tematu chcesz otrzymać? (-1 → wszystkie): ");
    scanf("%d", &length);

    register_sub(server, id, topic, length);

    int feedback = take_feedback(que, 1);
    if (feedback == 1){
      print_error("Taki temat nie istnieje.");
    }
    else if(feedback == 0)
      print_info("Dodano subkrybcję");
}


int main(int argc, char *argv[]) {
  int server = msgget(SERVER_QUE_NR, 0);
  int que = 0;
  int choice = ' ';
  int child_pid = -1;

  int nr_on_server = login_menu(server, &que);
  // printf("%d %d\n", que, nr_on_server);
  register_topic(server, nr_on_server, "sport");
  take_feedback(que, 1);
  register_sub(server, nr_on_server, 0, -1);
  take_feedback(que, 1);
  get_topics();

  do{
    print_info("\n------------MENU------------------\n1 – utwórz nowy temat\n2 – zapisz się na subskrybcję\n3 – nowa wiadomość\n4 – odbierz wiadomości\n5 – włącz/wyłącz synchroniczne odbieranie wiadomości\n6 – wyłącz system\n7 – wyloguj\n");
    while ((choice = getchar())<=' ');
    switch (choice) {
      case '1':
        topic_menu(server, nr_on_server, que);
        break;
      case '2':
        sub_menu(server, nr_on_server, que);
        break;
      case '3':
        msg_menu(server, nr_on_server, que);
        break;
      case '4':
        receive_msg_async(que);
        break;
      case '5':
        receive_msg_sync(&child_pid, que);
        break;
      case '6':
        shutdown(server);
        break;
      case '7': break;
      default: printf("\e[0;31mNiepoprawny numer '%c'.\n\e[m", choice);
    }
  }while(choice != '7' && choice != '6');

  if (child_pid != -1)
    kill(child_pid, 15);
  print_info("Bywaj!\n");
  return 0;
}
