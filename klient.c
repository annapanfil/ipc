#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h> //kill
#include <locale.h>
#include <ncursesw/ncurses.h>
#include <math.h> //round

/*TODO:
old - przy priorytecie działa "1,2,3lub4"
- czyszczenie okna przy błędach
- kolory
- brak enterów przy błędach? (p. sub menu <-1)
*/

#define NAME_LENGTH 30
#define MESSAGE_LENGTH 1024
#define SERVER_QUE_NR 12345
#define TEXT_LEN 100

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
    return feedback.info;
}


void print_error(WINDOW* window, char text[TEXT_LEN]){
  int x=0,y=0;
  getyx(window,y,x);
  attron(COLOR_PAIR(2));
  mvwprintw(window, y, x+1, "%s\n\r", text);
  wrefresh(window);
  attroff(COLOR_PAIR(2));
}

void print_info(WINDOW* window, char text[TEXT_LEN]){
  int x=0,y=0;
  getyx(window,y,x);
  attron(COLOR_PAIR(1));
  mvwprintw(window, y, x+1, "%s", text);
  wrefresh(window);
  attroff(COLOR_PAIR(1));
}

void print_success(WINDOW* window, char text[TEXT_LEN]){
  int x=0,y=0;
  getyx(window,y,x);
  attron(COLOR_PAIR(3));
  mvwprintw(window, y, x+1, "%s\n\r", text);
  wrefresh(window);
  attroff(COLOR_PAIR(3));
}

void print_long(WINDOW* window, char type, char text1[TEXT_LEN], char text2[NAME_LENGTH]){
  char* text = malloc(TEXT_LEN+NAME_LENGTH);
  sprintf(text, "%s %s", text1, text2);
  switch (type) {
    case 'i': print_info(window, text); break;
    case 'e': print_error(window, text); break;
    case 's': print_success(window, text); break;
    default: print_error(window, "To developer: wrong type in print_long");
  }
  free(text);
}


int get_int_from_user(WINDOW* window){
  char text[10];
  int number;
  while(1){
    wgetnstr(window, text, sizeof(text));  //zabezbieczyć długość
    if(sscanf(text, "%d", &number) == 1){
      return number;
      }
    else
      print_error(window, "Podany tekst nie jest liczbą. Spróbuj jeszcze raz: ");
    }
  }

char* get_string_from_user(WINDOW* window, int length){
  char* text = malloc(length);
  wgetnstr(window, text, length);
  return text;
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
        printw("%s\r\n", message.text);
      }
    }
    *pid = x;
    // print_success(window, "Włączono synchroniczne odbieranie wiadomości");
  }
  else{  //odbierał → kończy odbieranie
    kill(*pid, 15);
    *pid = -1;
    // print_success(window, "Wyłączono synchroniczne odbieranie wiadomości");
  }
}

void receive_msg_sync(int que){
  int size = 0;
  struct server_msg message;
  while (size != -1){
    size = msgrcv(que, &message, sizeof(message)-sizeof(long), -5, IPC_NOWAIT);
    if (size != -1){
      printw("%s\r\n", message.text);
    }
  }
  // print_success(window, "Nie masz więcej nowych wiadomości.");
}

int get_topics(int server, int id, int que, char topics[][TEXT_LEN]){
  int nr_of_topics = 0;
  struct client_msg msg;
  msg.type = 5;
  msg.id = id;
  msgsnd(server, &msg, sizeof(msg)-sizeof(long), 0);

  struct server_msg message;
  do{
    msgrcv(que, &message, sizeof(message)-sizeof(long), 6, 0);
    strcpy(topics[nr_of_topics], message.text);
    nr_of_topics++;
  }while (strcmp(message.text, "") != 0);
  return nr_of_topics-1
  ;
}

void shutdown(int server){
  struct client_msg message;
  message.type = 6;
  msgsnd(server, &message, sizeof(message)-sizeof(long), 0);
}

void close_window(WINDOW* window){
  print_info(window, "Naciśnij dowolny klawisz, by przejść do menu");
  wgetch(window);

  wclear(window);
  wrefresh(window);
}

int gui_menu(WINDOW* menu_win, char options[][TEXT_LEN], int options_nr){
  box(menu_win, 0, 0);
  noecho();
  keypad(menu_win, true);  //allows to use arrows

  int choice = -1;
  int highlight = 0;

  while(choice != 10){ //enter
    for(int i=0; i<options_nr; i++){
      if(i == highlight)
        wattron(menu_win, A_REVERSE); //reverse colors - highlight

      mvwprintw(menu_win, i+1, 1, options[i]);
      wattroff(menu_win, A_REVERSE);
    }
    choice = wgetch(menu_win);

    switch (choice) {
      case KEY_UP:
        if (highlight != 0)
          highlight--;
        break;
      case KEY_DOWN:
        if (highlight != options_nr-1)
          highlight++;
        break;
      default: break;
    }
  }

  wclear(menu_win);
  wrefresh(menu_win); // Refresh it (to leave it blank)
  echo();
  return highlight;
}

int login_menu(int server, int* que){
  int terminal_width = getmaxx (stdscr);
  WINDOW* login_win = newwin(15, terminal_width-16, 8, 8);
  box(login_win, 0, 0);
  mvwprintw(login_win, 0, 2, "LOGOWANIE");
  wmove(login_win, 1, 0);
  refresh();
  wrefresh(login_win);

  int nr_on_server = -1;
  char name[NAME_LENGTH] = "Obi wan";

  while (nr_on_server < 0){
    print_info(login_win, "Dzień dobry!\n\r");
    print_info(login_win, "Podaj swoją nazwę użytkownika: ");
    strcpy(name, get_string_from_user(login_win, NAME_LENGTH));

    int que_key = login(server, que, name);
    nr_on_server = take_feedback(server, que_key);
    if (nr_on_server == -1){
      print_error(login_win, "Taka nazwa już istnieje. Podaj inną nazwę.");
    }
    else if(nr_on_server == -2){
      print_error(login_win, "Na tym serwerze jest zbyt wielu klientów. Nie możesz się zalogować");
      msgctl(*que, IPC_RMID, NULL);
      exit(0);
    }
    else if(nr_on_server >= 0)
      print_success(login_win, "Zalogowano");
  }
  close_window(login_win);
  return nr_on_server;
}

void topic_menu(WINDOW* window, int server, int id, int que){
    box(window, 0, 0);
    mvwprintw(window, 0, 2, "NOWY TEMAT");
    wmove(window, 1, 0);
    char topic[NAME_LENGTH] = "New topic";

    char topics[TEXT_LEN][TEXT_LEN];
    int topics_nr =get_topics(server, id, que, topics);
    if(topics_nr > 0){
      print_info(window, "TEMATY NA SERWERZE:\n\r");
      for (int i=0; i<topics_nr; i++)
      print_long(window, 'i', topics[i], "\n");
    }
    else
      print_info(window, "Na tym serwerze nie ma jeszcze żadnych tematów.");

    print_info(window,"Podaj nazwę nowego tematu: ");
    strcpy(topic, get_string_from_user(window, NAME_LENGTH));

    register_topic(server, id, topic);

    int feedback = take_feedback(que, 7);
    if (feedback == 1){
      print_error(window, "Taki temat już istnieje");
    }
    else if(feedback == 2){
      print_error(window, "Na tym serwerze jest zbyt wiele tematów. Nie możesz dodać kolejnego.");
    }
    else if(feedback == 0)
      print_success(window, "Dodano temat");

    close_window(window);
}

void sub_menu(WINDOW* window, int server, int id, int que){
    box(window, 0, 0);
    mvwprintw(window, 0, 2, "NOWA SUBSKRYBCJA");
    wmove(window, 1, 0);
    int length;

    char topics[TEXT_LEN][TEXT_LEN];
    int nr_of_topics = get_topics(server, id, que, topics);
    if (nr_of_topics > 0){
      int topic = gui_menu(window, topics, nr_of_topics);
      box(window, 0, 0);
      mvwprintw(window, 0, 2, "NOWA SUBSKRYBCJA");
      wmove(window, 1, 0);
      print_info(window, "Ile wiadomości z tego tematu chcesz otrzymać? (-1 → wszystkie): ");
      length = get_int_from_user(window);
      while (length<-1 || length == 0) {
        print_error(window, "Priorytet musi być liczbą całkowitą ≥ -1 i ≠ 0. Spróbuj jeszcze raz: ");
        length = get_int_from_user(window);
      }

      register_sub(server, id, topic, length);

      int feedback = take_feedback(que, 7);
      if (feedback == 1){     //shouldn't happen
        print_error(window, "Taki temat już nie istnieje.");
      }
      else if(feedback == 0)
        print_success(window, "Dodano subkrybcję");
    }
    else
      print_info(window, "Na tym serwerze nie ma żadnego tematu\n");

    close_window(window);
}

void msg_menu(int server, int id, int que){
  /*  print_info(window, "\n\r-----------NOWA WIADOMOŚĆ-----------\n\r");
    char msg[MESSAGE_LENGTH] = "Empty message";
    int priority;

    char topics[TEXT_LEN][TEXT_LEN];
    int nr_of_topics = get_topics(server, id, que, topics);
    if (nr_of_topics > 0){
      int topic = gui_menu(topics, nr_of_topics);

    print_info(window, "Wpisz treść wiadomości: \n\r> ");
    strcpy(msg, get_string_from_user(MESSAGE_LENGTH));

    print_info(window, "Priorytet wiadomości (1-5, gdzie 1 – najwyższy): ");
    priority = get_int_from_user();
    while (priority < 1 || priority > 5) {
      print_error(window, "Priorytet musi być równy 1, 2, 3, 4 lub 5. Spróbuj jeszcze raz: ");
      priority = get_int_from_user();
    }

    send_msg(server, id, topic, msg, priority);

    int feedback = take_feedback(que, 7);
    if (feedback == 1){
      print_error(window, "Taki temat nie istnieje.");
    }
    else if (feedback == 0)
      print_success(window, "Wiadomość wysłana");
    }
    else
      print_info(window, "Na tym serwerze nie ma żadnego tematu.\n");

    print_info(window, "Naciśnij dowolny klawisz, by wrócić do menu.\n\r");
    getch();*/
}


int main(int argc, char *argv[]) {
  //initialise ncurses
  setlocale(LC_ALL, "pl_PL.UTF-8");
  initscr();
  echo();
  int terminal_width, terminal_height;
  getmaxyx (stdscr, terminal_height, terminal_width);

  WINDOW* left_win = newwin(terminal_height-8, round(2*terminal_width/3)-8, 4, 4);

  start_color();
  use_default_colors();
  init_pair(1, COLOR_YELLOW, -1);  //for info
  init_pair(2, COLOR_RED, -1);     //for error
  init_pair(3, COLOR_GREEN, -1);   //for success

  int server = msgget(SERVER_QUE_NR, 0);
  int que = 0;
  int child_pid = -1;

  int nr_on_server = login_menu(server, &que);
  int choice = -1;

  char menu[7][TEXT_LEN] = {"nowy temat", "zapis na subskrybcję", "nowa wiadomość", "odbierz wiadomości", "włącz/wyłącz automatyczne odbieranie wiadomości", "wyłącz system", "zakończ"};

  do{
    choice = gui_menu(left_win, menu, 7);

    switch (choice) {
      case 0: topic_menu(left_win, server, nr_on_server, que); break;
      case 1: sub_menu(left_win, server, nr_on_server, que); break;
      case 2: msg_menu(server, nr_on_server, que); break;
      case 3: receive_msg_sync(que);  break;
      case 4: receive_msg_async(&child_pid, que); break;
      case 5: shutdown(server); break;
      case 6: break;
      // default: print_error(window, "Niepoprawny wybór"); //shouldn't happen
    }
  }while(choice != 5 && choice != 6);


  if (child_pid != -1)
    kill(child_pid, 15);
  move(15, round(terminal_width/2)-6);
  // print_info(window, "Do widzenia!\n");
  refresh();
  // getchar();
  use_default_colors();
  delwin(left_win);
  endwin();
  return 0;
}
