#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*TODO:
- nowy temat: komunikacja z uzytkownikiem, obsługa błędów
- zapis na subskrybcję: komunikacja z uzytkownikiem, odbiór komunikatu
- otrzymywanie wiadomości synchronicznie
- podział na mniejsze funkcje
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

void register_topic(int server, int id, char topic_name[NAME_LENGTH], int que){
  struct msgbuf topic_msg;
  topic_msg.type = 2;
  topic_msg.id = id;
  strcpy(topic_msg.text, topic_name);
  msgsnd(server, &topic_msg, sizeof(topic_msg)-sizeof(long), 0);
}

void register_sub(int server, int id, int topic, int sublen){ //-1 -> forever
  printf("\e[0;36mⓘ register_sub\e[m\n");
  struct msgbuf sub_msg;
  sub_msg.type = 3;
  sub_msg.id = id;
  sub_msg.topic = topic;
  sub_msg.number = sublen;
  msgsnd(server, &sub_msg, sizeof(sub_msg)-sizeof(long), 0);  //BUG: coś się nie wysyła
}

void send_msg(int server, int id, int topic, char msg_text[MESSAGE_LENGTH]){
  struct msgbuf message;
  message.type = 4;
  message.id = id;
  message.topic = topic;
  strcpy(message.text, msg_text);
  msgsnd(server, &message, sizeof(message)-sizeof(long), 0);
}

void receive_msg_sync(){
}

void receive_msg_async(int que){
  int size;
  while (size == -1){
    struct text_msg message;
    size = msgrcv(que, &message, sizeof(message)-sizeof(long), 2, IPC_NOWAIT);
    if (size != -1){
      printf("%s\n", message.text);
    }
  }
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
    printf("Dzień dobry! Podaj swoją nazwę użytkownika: "); //nie może zawierać spacji
    scanf("%s", name);
    int que_key = login(server, que, name);
    nr_on_server = take_feedback(server, que_key);
    if (nr_on_server == -1){
      printf("Taka nazwa już istnieje. Podaj inną nazwę.\n");
    }
    else if(nr_on_server == -2){
      printf("Na tym serwerze jest zbyt wielu klientów. Nie możesz się zalogować\n");
      msgctl(*que, IPC_RMID, NULL);
      exit(0);
    }
  }
  return nr_on_server;
}

void msg_menu(int server, int id){
    char msg[MESSAGE_LENGTH] = "Empty message";
    int topic;

    printf("Podaj numer tematu: ");
    scanf("%d", &topic);
    printf("Wpisz treść wiadomości: \n> "); //nie może zawierać spacji
    scanf("%s", msg);
    send_msg(server, id, topic, msg);

    // take_feedback(server, que_key);
}

int main(int argc, char *argv[]) {
  int server = msgget(SERVER_QUE_NR, 0);
  int que = 0;
  int choice = ' ';

  int nr_on_server = login_menu(server, &que);
  // printf("%d %d\n", que, nr_on_server);

  do{
    printf("1 – utwórz nowy temat\n2 – zapisz się na subskrybcję\n3 – nowa wiadomość\n4 – odbierz wiadomości\n5 – odbieraj wiadomości synchronicznie\n6 – wyłącz system\n7 – wyloguj\n");
    while ((choice = getchar())<=' ');
    switch (choice) {
      case '1':
        register_topic(server, nr_on_server, "sport", que);
        take_feedback(que, 1);
        break;
      case '2':
        register_sub(server, nr_on_server, 0, -1);
        take_feedback(que, 1);
        break;
      case '3':
        msg_menu(server, nr_on_server);
        break;
      case '4':
        receive_msg_async(que);
      case '5':
        receive_msg_sync();
      case '6':
        shutdown(server);
        printf("Bywaj!\n");
        break;
      case '7':
        printf("Bywaj!\n");
        break;
      default:
        printf("Niepoprawny numer '%c'.\n", choice);
    }
  }while(choice != '7' && choice != '6');

  return 0;
}
