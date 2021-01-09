#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h> //kill

/*TODO:
- przy priorytecie działa "1,2,3lub4"
*/

#define NAME_LENGTH 30
#define MESSAGE_LENGTH 1024
#define SERVER_QUE_NR 12345

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

int take_feedback(int que, int type){
  struct server_msg feedback;
  msgrcv(que, &feedback, sizeof(feedback)-sizeof(long), type, 0);
  // printf("Info: %d\n", feedback.info);
    return feedback.info;
}

void print_error(char text[100]){
  printf("\e[0;31m%s\n\e[m", text);
}

void print_info(char text[100]){
  printf("\e[1;34m%s\e[m", text);
}

void print_success(char text[100]){
  printf("\e[1;32m%s\e[m\n", text);
}


int get_int_from_user(){
  char text[10];
  int number;
  while(1){
    if(fgets(text, sizeof(text), stdin)){
      if (!(text[strlen(text) - 1] == '\n')){
        scanf("%*[^\n]");scanf("%*c"); //clean buffer
        print_error("Podana liczba jest zbyt długa. Spróbuj jeszcze raz: ");
      }
      else if(sscanf(text, "%d", &number) == 1){
        return number;
      }
      else
        print_error("Podany tekst nie jest liczbą. Spróbuj jeszcze raz: ");
    }
  }
}

char* get_string_from_user(int length){
  char* text = malloc(length);
  fgets(text, length, stdin);
  if ((strlen(text) > 0) && (text[strlen(text) - 1] == '\n'))
    text[strlen(text) - 1] = '\0'; //remove newline
  else if (!(text[strlen(text) - 1] == '\n')){
    printf("\e[0;31mTekst został skrócony do: %s\e[m\n", text);
    scanf("%*[^\n]");scanf("%*c"); //clean buffer
  }
  return text;
}


int choose_topic(int nr_of_topics){
  char choice;
  int topic;
  do{
    choice = 'y';
    print_info("Podaj numer tematu: ");
    topic = get_int_from_user();
    if (topic>=nr_of_topics){
      print_info("Tego tematu nie ma na liście. Czy mimo to chcesz go wybrać? y/n ");
      while ((choice = getchar())<=' ');
      scanf("%*[^\n]");scanf("%*c"); //clean buffer
    }
    else if (topic < 0){
      print_error("Temat nie może być liczbą ujemną. Kochany użytkowniku, wybierz temat, który jest na liście.");
    }

  }while(choice != 'y');

  return topic;
}

int login(int server, int* que, char name[NAME_LENGTH]){
  struct client_msg login_msg;
  int que_key = getpid();
  *que = msgget(que_key, 0644|IPC_CREAT);
  login_msg.type = 1;
  login_msg.id = que_key;
  strcpy(login_msg.text, name);
  msgsnd(server, &login_msg, sizeof(login_msg)-sizeof(long), 0);
  return que_key;
}

void register_topic(int server, int id, char topic_name[NAME_LENGTH]){
  struct client_msg topic_msg;
  topic_msg.type = 2;
  topic_msg.id = id;
  strcpy(topic_msg.text, topic_name);
  msgsnd(server, &topic_msg, sizeof(topic_msg)-sizeof(long), 0);
}

void register_sub(int server, int id, int topic, int sublen){ //-1 -> forever
  struct client_msg sub_msg;
  sub_msg.type = 3;
  sub_msg.id = id;
  sub_msg.topic = topic;
  sub_msg.number = sublen;
  msgsnd(server, &sub_msg, sizeof(sub_msg)-sizeof(long), 0);
}

void send_msg(int server, int id, int topic, char msg_text[MESSAGE_LENGTH], int priority){
  struct client_msg message;
  message.type = 4;
  message.id = id;
  message.topic = topic;
  message.number = priority;
  strcpy(message.text, msg_text);
  msgsnd(server, &message, sizeof(message)-sizeof(long), 0);
}

void receive_msg_async(int* pid, int que){
  if (*pid == -1){  //nie odbierał → zaczyna odbierać
    struct server_msg message;
    int x;
    if ((x=fork()) == 0){
      while (1) {
        msgrcv(que, &message, sizeof(message)-sizeof(long), -5, 0);
        printf("%s\n", message.text);
      }
    }
    *pid = x;
    print_success("Włączono synchroniczne odbieranie wiadomości");
  }
  else{  //odbierał → kończy odbieranie
    kill(*pid, 15);
    *pid = -1;
      print_success("Wyłączono synchroniczne odbieranie wiadomości");
  }
}

void receive_msg_sync(int que){
  int size = 0;
  struct server_msg message;
  while (size != -1){
    size = msgrcv(que, &message, sizeof(message)-sizeof(long), -5, IPC_NOWAIT);
    if (size != -1){
      printf("%s\n", message.text);
    }
  }
  print_success("Nie masz więcej nowych wiadomości.");
}

int get_topics(int server, int id, int que){
  int nr_of_topics = 0;
  struct client_msg msg;
  msg.type = 5;
  msg.id = id;
  msgsnd(server, &msg, sizeof(msg)-sizeof(long), 0);

  struct server_msg message;
  printf("\n-------TEMATY NA SERWERZE-------\n");
  do{
    msgrcv(que, &message, sizeof(message)-sizeof(long), 6, 0);
    printf("%s\n", message.text);
    nr_of_topics++;
  }while (strcmp(message.text, "") != 0);
  return nr_of_topics-1;
}

void shutdown(int server){
  struct client_msg message;
  message.type = 6;
  msgsnd(server, &message, sizeof(message)-sizeof(long), 0);
}


int login_menu(int server, int* que){
  int nr_on_server = -1;
  while (nr_on_server < 0){
    char name[NAME_LENGTH] = "Obi wan";
    print_info("\nDzień dobry!\nPodaj swoją nazwę użytkownika: ");
    strcpy(name, get_string_from_user(NAME_LENGTH));

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
      print_success("Zalogowano");
  }
  return nr_on_server;
}

void topic_menu(int server, int id, int que){
    print_info("\n-----------NOWY TEMAT-----------\n");
    char topic[NAME_LENGTH] = "New topic";

    get_topics(server, id, que);

    print_info("Podaj nazwę nowego tematu: "); //nie może zawierać spacji
    strcpy(topic, get_string_from_user(NAME_LENGTH));

    register_topic(server, id, topic);

    int feedback = take_feedback(que, 7);
    if (feedback == 1){
      printf("Taki temat już istnieje. %s.", topic);
    }
    else if(feedback == 2){
      print_error("Na tym serwerze jest zbyt wiele tematów. Nie możesz dodać kolejnego.");
    }
    else if(feedback == 0)
      print_success("Dodano temat");
}

void sub_menu(int server, int id, int que){
    print_info("\n-----------NOWA SUBSKRYBCJA-----------\n");
    int length;

    int nr_of_topics = get_topics(server, id, que);
    int topic = choose_topic(nr_of_topics);

    print_info("Ile wiadomości z tego tematu chcesz otrzymać? (-1 → wszystkie): ");
    length = get_int_from_user();
    while (length<-1 || length == 0) {
      print_error("Priorytet musi być liczbą całkowitą ≥ -1 i ≠ 0. Spróbuj jeszcze raz: ");
      length = get_int_from_user();
    }

    register_sub(server, id, topic, length);

    int feedback = take_feedback(que, 7);
    if (feedback == 1){
      print_error("Taki temat nie istnieje.");
    }
    else if(feedback == 0)
      print_success("Dodano subkrybcję");
}

void msg_menu(int server, int id, int que){
    print_info("\n-----------NOWA WIADOMOŚĆ-----------\n");
    char msg[MESSAGE_LENGTH] = "Empty message";
    int priority;

    int nr_of_topics = get_topics(server, id, que);
    int topic = choose_topic(nr_of_topics);

    print_info("Wpisz treść wiadomości: \n> ");
    strcpy(msg, get_string_from_user(MESSAGE_LENGTH));

    print_info("Priorytet wiadomości (1-5, gdzie 1 – najwyższy): ");
    priority = get_int_from_user();
    while (priority < 1 || priority > 5) {
      print_error("Priorytet musi być równy 1, 2, 3, 4 lub 5. Spróbuj jeszcze raz: ");
      priority = get_int_from_user();
    }

    send_msg(server, id, topic, msg, priority);

    int feedback = take_feedback(que, 7);
    if (feedback == 1){
      print_error("Taki temat nie istnieje.");
    }
    else if (feedback == 0)
      print_success("Wiadomość wysłana");
}


int main(int argc, char *argv[]) {
  int server = msgget(SERVER_QUE_NR, 0);
  int que = 0;
  int choice = ' ';
  int child_pid = -1;

  int nr_on_server = login_menu(server, &que);

  do{
    print_info("\n------------MENU------------------\n1 – utwórz nowy temat\n2 – zapisz się na subskrybcję\n3 – nowa wiadomość\n4 – odbierz wiadomości\n5 – włącz/wyłącz asynchroniczne odbieranie wiadomości\n6 – wyświetl listę tematów na serwerze\n7 – wyłącz system\n8 – zakończ\n\nPodaj numer i naciśnij enter: ");
    while ((choice = getchar())<=' ');
    scanf("%*[^\n]");scanf("%*c"); //clean buffer
    switch (choice) {
      case '1': topic_menu(server, nr_on_server, que); break;
      case '2': sub_menu(server, nr_on_server, que); break;
      case '3': msg_menu(server, nr_on_server, que); break;
      case '4': receive_msg_sync(que);  break;
      case '5': receive_msg_async(&child_pid, que); break;
      case '6': get_topics(server, nr_on_server, que); break; //TODO: delete
      case '7': shutdown(server); break;
      case '8': break;
      default: printf("\e[0;31mNiepoprawny numer '%c'.\n\e[m", choice);
    }
  }while(choice != '7' && choice != '8');

  if (child_pid != -1)
    kill(child_pid, 15);
  print_info("Bywaj!\n");
  return 0;
}
