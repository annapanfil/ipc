#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> //bool
#include <unistd.h>
#include <signal.h> //kill
#include <locale.h>
#include <ncursesw/ncurses.h>
#include <math.h> //round

/*TODO:
- kolory
- znikające okno
- wiadomości automatyczne nie w tym oknie (wyślij jedną, przesuń się w menu, wyślij drugą)
- odbiór wiadomości z subskrybcji, które wygasły
*/

#define NAME_LENGTH 30
#define MESSAGE_LENGTH 1024
#define SERVER_QUE_NR 12345
#define TEXT_LEN 100
#define TOPICS_NUM 20
#define FEEDBACK_TYPE 2

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


void print_error(WINDOW* window, char text[TEXT_LEN]){
  int x=0,y=0;
  getyx(window,y,x);
  attron(COLOR_PAIR(2));
  mvwprintw(window, y, x+1, "%s", text);
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
    default: print_error(window, "To developer: wrong type in print_long\n\r");
  }
  free(text);
}

void close_window(WINDOW* window){
  print_info(window, "\n\r");
  print_info(window, "Naciśnij dowolny klawisz, by przejść do menu");
  wgetch(window);

  wclear(window);
  wrefresh(window);
}

void clean_window(WINDOW* window, char* title){
  wclear(window);
  box(window, 0, 0);
  mvwprintw(window, 0, 2, title);
  wmove(window, 1, 0);
  wrefresh(window);
}


int get_int_from_user(WINDOW* window){
  char text[10];
  int number;
  while(1){
    wgetnstr(window, text, sizeof(text));  //zabezbieczyć długość
    if(sscanf(text, "%d", &number) == 1){
      return number;
    }
    else{
      clean_window(window, "BŁĄD!");
      print_error(window, "Podany tekst nie jest liczbą. Spróbuj jeszcze raz: ");
    }
  }
}

char* get_string_from_user(WINDOW* window, int length){
  char* text = malloc(length);
  do{
    wgetnstr(window, text, length);
    if (strcmp(text, "") == 0){ //są równe
      clean_window(window, "BŁĄD");
      print_error(window, "To pole nie może być puste. Spróbuj jeszcze raz: ");
    }
  }while(strcmp(text, "") == 0);
  return text;
}


int take_feedback(int que, int type){
  struct server_msg feedback;
  msgrcv(que, &feedback, sizeof(feedback)-sizeof(long), type, 0);
    return feedback.info;
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

void register_topic(int server, int id, char topic_name[NAME_LENGTH], int topic_nr){
  struct client_msg topic_msg;
  topic_msg.type = 2;
  topic_msg.id = id;
  topic_msg.number = topic_nr;
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

void receive_msg_async(WINDOW* window, int* topics, int topics_num, int que, int* cursorx, int* cursory){
  struct server_msg message;
  int size;
  for(int i=0; i<topics_num; i++){
    size=0;
    do{
      size = msgrcv(que, &message, sizeof(message)-sizeof(long), *(topics+i)+4, IPC_NOWAIT);
      if(size != -1){
        if (wmove(window, *cursory, *cursorx)==ERR){
          print_error(window, "!!!NIE PRZESUNĄŁ!!!!!");
        }
        // wmove(window, 1, 1);
        print_long(window, 'i', message.text, "\n\r");
        int y, x;
        getyx(window, y, x);
        if(y == -1 || x == -1){
            print_error(window, "!!!!NIE POBRAŁ WSPÓŁRZĘDNYCH!!!");
        }
        else{
          char msg[12];
          sprintf(msg,"y, x: %d %d", y, x); //jak tego nie ma, to następna wiadomość pojawia się w głównym oknie (kursor się nie przesuwa) Czemu? Nie mam pojęcia.
          print_info(window, msg);
        }
        *cursorx = x;
        *cursory = y;
      }
    }while (size != -1);
  }
}

int receive_msg_sync(WINDOW* window, int que, int topic){
  struct item {
    struct server_msg msg;
    struct item* next_item;
  } last_item;

  last_item.next_item = NULL;

  struct item* first_ptr = &last_item;

  int size = 0;
  int msg_num = 0;
  struct server_msg message;
  while (size != -1){
    size = msgrcv(que, &message, sizeof(message)-sizeof(long), topic+4, IPC_NOWAIT);

    if (size != -1){
      //sort messages by priority (insertion sort)
      struct item* curr_item = first_ptr;
      struct item* prev_item = NULL;
      while(curr_item -> next_item != NULL && curr_item->msg.info < message.info){
        prev_item = curr_item;
        curr_item = curr_item -> next_item;
      }
      struct item* new_item;
      new_item = (struct item*)malloc(sizeof(struct item));
      new_item->msg = message;
      new_item->next_item = curr_item;

      if (prev_item != NULL){
        prev_item->next_item = new_item;
      }
      else{ //it was the first item
        first_ptr = new_item;
      }
      msg_num++;
    }

  }
  //print all messages
  struct item* curr_item = first_ptr;
  struct item* prev_item = NULL;
  while(curr_item -> next_item != NULL){
    print_long(window, 'i', curr_item->msg.text, "\n\r");

    curr_item = curr_item -> next_item;
    if (prev_item != NULL)
      free(curr_item);
  }
  return msg_num;
}

int get_topics(int server, int id, int que, char topics[][TEXT_LEN], bool subed_only){
  int nr_of_topics = 0;
  struct client_msg msg;
  msg.type = subed_only ? 6 : 5;
  msg.id = id;
  msgsnd(server, &msg, sizeof(msg)-sizeof(long), 0);

  struct server_msg message;
  do{
    msgrcv(que, &message, sizeof(message)-sizeof(long), 1, 0);
    strcpy(topics[nr_of_topics], message.text);
    nr_of_topics++;
  }while (strcmp(message.text, "") != 0);
  return nr_of_topics-1;
}

int get_zombie_subs(int que, char topics[][TEXT_LEN], int nr_of_topics){
  struct server_msg message;
  int size;
  do{
    size = msgrcv(que, &message, sizeof(message)-sizeof(long), 3, IPC_NOWAIT);
    if(size != -1){
      strcpy(topics[nr_of_topics++], message.text);
    }
  }while (size != -1);
  return nr_of_topics;
}

void shutdown(int server){
  struct client_msg message;
  message.type = 7;
  msgsnd(server, &message, sizeof(message)-sizeof(long), 0);
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
  clean_window(login_win, "LOGOWANIE");

  int nr_on_server = -1;
  char name[NAME_LENGTH] = "Obi wan";

  print_info(login_win, "Dzień dobry!\n\r");
  print_info(login_win, "Podaj swoją nazwę użytkownika: ");
  while (nr_on_server < 0){
    strcpy(name, get_string_from_user(login_win, NAME_LENGTH));

    clean_window(login_win, "LOGOWANIE");

    int que_key = login(server, que, name);
    nr_on_server = take_feedback(server, que_key);
    if (nr_on_server == -1){
      print_error(login_win, "Taka nazwa już istnieje. Podaj inną nazwę: ");
    }
    else if(nr_on_server == -2){
      print_error(login_win, "Na tym serwerze jest zbyt wielu klientów. Nie możesz się zalogować\n\r");
      msgctl(*que, IPC_RMID, NULL);
      clear();
      refresh();
      use_default_colors();
      endwin();
      exit(0);
    }
    else if(nr_on_server >= 0)
      print_success(login_win, "Zalogowano");
  }
  close_window(login_win);
  return nr_on_server;
}

void topic_menu(WINDOW* window, int server, int id, int que){
    clean_window(window, "NOWY TEMAT");
    char topic[NAME_LENGTH] = "New topic";

    char topics[TEXT_LEN][TEXT_LEN];
    int topics_nr = get_topics(server, id, que, topics, false);
    if(topics_nr > 0){
      print_info(window, "TEMATY NA SERWERZE:\n\r");
      for (int i=0; i<topics_nr; i++)
      print_long(window, 'i', topics[i], "\n");
    }
    else
      print_info(window, "Na tym serwerze nie ma jeszcze żadnych tematów.\n\r");

    print_info(window,"Podaj nazwę nowego tematu: ");
    strcpy(topic, get_string_from_user(window, NAME_LENGTH));

    register_topic(server, id, topic, topics_nr);
    clean_window(window, "NOWY TEMAT");

    int feedback = take_feedback(que, FEEDBACK_TYPE);
    if (feedback == 1){
      print_error(window, "Taki temat już istnieje\n\r");
    }
    else if(feedback == 2){
      print_error(window, "Na tym serwerze jest zbyt wiele tematów. Nie możesz dodać kolejnego.\n\r");
    }
    else if(feedback == 0)
      print_success(window, "Dodano temat");

    close_window(window);
}

void sub_menu(WINDOW* window, int server, int id, int que){
    int length;

    char topics_all[TEXT_LEN][TEXT_LEN];
    char topics_subed[TEXT_LEN][TEXT_LEN];
    char topics_free[TEXT_LEN][TEXT_LEN];
    int nr_of_topics_all = get_topics(server, id, que, topics_all, false);
    int nr_of_topics_subed = get_topics(server, id, que, topics_subed, true);
    int nr_of_topics_free = 0;
    bool in_subed;
    for(int i=0; i<nr_of_topics_all;i++){
      in_subed = false;
      for (int j=0; j<nr_of_topics_subed;j++){ //dalej już go nie będzie
        if (strcmp(topics_all[i], topics_subed[j]) == 0){
          in_subed = true;
          break;
        }
      }
      if (!in_subed){
        strcpy(topics_free[nr_of_topics_free++], topics_all[i]);
      }
    }

    if (nr_of_topics_free > 0){
      int choice = gui_menu(window, topics_free, nr_of_topics_free);
      int topic; sscanf(topics_free[choice], "%d", &topic);
      clean_window(window, "NOWA SUBSKRYBCJA");
      print_info(window, "Ile wiadomości z tego tematu chcesz otrzymać? (-1 → wszystkie): ");
      length = get_int_from_user(window);
      while (length<-1 || length == 0) {
        clean_window(window, "NOWA SUBSKRYBCJA");
        print_error(window, "Priorytet musi być liczbą całkowitą ≥ -1 i ≠ 0. Spróbuj jeszcze raz: ");
        length = get_int_from_user(window);
      }

      register_sub(server, id, topic, length);
      clean_window(window, "NOWA SUBSKRYBCJA");

      int feedback = take_feedback(que, FEEDBACK_TYPE);
      if (feedback == 1){     //shouldn't happen
        print_error(window, "Taki temat już nie istnieje.\n\r");
      }
      else if(feedback == 0)
        print_success(window, "Dodano subkrybcję.");
    }
    else{
      clean_window(window, "NOWA SUBSKRYBCJA");
      print_info(window, "Na tym serwerze nie ma już tematów, których nie subskrybujesz\n");
    }

    close_window(window);
}

void msg_menu(WINDOW* window, int server, int id, int que){
    char msg[MESSAGE_LENGTH] = "Empty message";
    int priority;

    char topics[TEXT_LEN][TEXT_LEN];
    int nr_of_topics = get_topics(server, id, que, topics, false);
    if (nr_of_topics > 0){
      int topic = gui_menu(window, topics, nr_of_topics); //choice is topic, because there are all topics

    clean_window(window, "NOWA WIADOMOŚĆ");
    print_info(window, "Wpisz treść wiadomości:\n\r");
    print_info(window, "> ");
    strcpy(msg, get_string_from_user(window, MESSAGE_LENGTH));

    print_info(window, "Priorytet wiadomości (1-5, gdzie 1 – najwyższy): ");
    priority = get_int_from_user(window);

    clean_window(window, "NOWA WIADOMOŚĆ");
    while (priority < 1 || priority > 5) {
      print_error(window, "Priorytet musi być równy 1, 2, 3, 4 lub 5. Spróbuj jeszcze raz: ");
      priority = get_int_from_user(window);
    }

    send_msg(server, id, topic, msg, priority);
    clean_window(window, "NOWA WIADOMOŚĆ");

    int feedback = take_feedback(que, FEEDBACK_TYPE);
    if (feedback == 1){
      print_error(window, "Taki temat nie istnieje.\n\r");
    }
    else if (feedback == 0)
      print_success(window, "Wiadomość wysłana");
    }
    else
      print_info(window, "Na tym serwerze nie ma żadnego tematu.\n");

    close_window(window);
}

int receive_msg_sync_menu(WINDOW* menu_window, WINDOW* message_window, int server, int id, int que, char zombie_subs[][TEXT_LEN], int zombie_subs_num){
  char topics[TOPICS_NUM][TEXT_LEN];
  int topics_num = get_topics(server, id, que, topics, true);
  zombie_subs_num = get_zombie_subs(que, zombie_subs, zombie_subs_num);

  char all_topics[TOPICS_NUM][TEXT_LEN];
  int all_topics_num = 0;
  for(int i=0; i<topics_num; i++){
    strcpy(all_topics[all_topics_num++], topics[i]);
  }
  for(int i=0; i<zombie_subs_num; i++){
    strcpy(all_topics[all_topics_num++], zombie_subs[i]);
  }

  if (all_topics_num > 0){
    int choice = gui_menu(menu_window, all_topics, all_topics_num);
    int topic; sscanf(all_topics[choice], "%d", &topic);
    clean_window(menu_window, "Odbiór wiadomości");
    if(receive_msg_sync(message_window, que, topic) > 0)
      print_info(menu_window, "Zobacz nowe wiadomości w skrzynce →\n\r");
    else
      print_info(menu_window, "Nie masz więcej nowych wiadomości w tym temacie\n\r");

    bool is_zombie;
    for(int i=0; i<zombie_subs_num; i++){
      if (strcmp(zombie_subs[i], all_topics[choice])==0){      //was zombie – delete from zombie list
        is_zombie = true;
        zombie_subs_num--;
      }
      if (is_zombie){  //move rest of the topics
        strcpy(zombie_subs[i], zombie_subs[i+1]);
      }
    }
  }
  else
    print_info(menu_window, "Nie subskrybujesz żadnego tematu\n\r");

  close_window(menu_window);
  return zombie_subs_num;
}

int receive_msg_async_menu(WINDOW* window, int* topics_async, int topics_async_num, int server, int id, int que){
  char topics[TEXT_LEN][TEXT_LEN];
  int nr_of_topics = get_topics(server, id, que, topics, true);
  if (nr_of_topics > 0){
    int choice = gui_menu(window, topics, nr_of_topics);
    int topic; sscanf(topics[choice], "%d", &topic);
    clean_window(window, "Automatyczny odbiór wiadomości");
    bool in_array = false;

    for(int i=0; i<topics_async_num; i++){
      if (topics_async[i] == topic){      //was in array
        in_array = true;
        topics_async_num--;
        print_success(window, "WYŁĄCZONO asynchroniczne odbieranie wiadomości\n\r");
      }
      if (in_array){  //move rest of the topics
        *(topics_async + i) = *(topics_async+i+1);
      }
    }
    if(!in_array){  //add to array
        *(topics_async+topics_async_num) = topic;
        topics_async_num++;
        print_success(window, "Włączono asynchroniczne odbieranie wiadomości\n\r");
    }
  }
  else
    print_info(window, "Nie subskrybujesz żadnego tematu\n\r");

  close_window(window);

  return topics_async_num;
}


int child(WINDOW* left_win, WINDOW* right_win, int server, int id, int que){
  bool user_wants_something = false;
  void on_alarm() //user wants something
  {
    user_wants_something = true;
  }

  signal(SIGALRM, on_alarm);

  int topics_async[100];           //WARNING: ograniczona
  int topics_async_num = 0;
  int parent = getppid();
  int cursory = 1;
  int cursorx = 1;

  while(1){
    receive_msg_async(right_win, topics_async, topics_async_num, que, &cursorx, &cursory);
    if (user_wants_something){
      topics_async_num = receive_msg_async_menu(left_win, topics_async, topics_async_num, server, id, que);
      user_wants_something = false;
      kill(parent, SIGALRM);
    }
    // wrefresh(right_win);
  }
}


int main(int argc, char *argv[]) {
  //initialise ncurses
  setlocale(LC_ALL, "pl_PL.UTF-8");
  WINDOW* main_win = initscr();
  echo();
  int terminal_width, terminal_height;
  getmaxyx (stdscr, terminal_height, terminal_width);

  start_color();
  use_default_colors();
  init_pair(1, COLOR_YELLOW, -1);  //for info
  init_pair(2, COLOR_RED, -1);     //for error
  init_pair(3, COLOR_GREEN, -1);   //for success

  //initialise rest of the program
  int server = msgget(SERVER_QUE_NR, 0);
  int que = 0;

  int nr_on_server = login_menu(server, &que);
  int pid = -1;

  WINDOW* left_win = newwin(terminal_height-4, round(2*terminal_width/3)-8, 2, 4);

  if((pid = fork()) == 0){
    WINDOW* right_win = newwin((terminal_height/2)-3, round(terminal_width/3)-8, (terminal_height/2)+1, round(2*terminal_width/3)+4);
    box(right_win, 0, 0);
    mvwprintw(right_win, 0, 2, "TWOJE WIADOMOŚCI");
    wmove(right_win, 1, 0);

    child(left_win, right_win, server, nr_on_server, que);
  }
  else{
    void on_alarm(){}
    signal(SIGALRM, on_alarm);

    WINDOW* right_win = newwin((terminal_height/2)-3, round(terminal_width/3)-8, 2, round(2*terminal_width/3)+4);
    box(right_win, 0, 0);
    mvwprintw(right_win, 0, 2, "TWOJE WIADOMOŚCI");
    wmove(right_win, 1, 0);

    int choice = -1;
    char menu[7][TEXT_LEN] = {"nowy temat", "zapis na subskrybcję", "nowa wiadomość", "odbierz wiadomości", "włącz/wyłącz automatyczne odbieranie wiadomości", "wyłącz system", "zakończ"};
    char zombie_subs[TOPICS_NUM][TEXT_LEN];
    int zombie_subs_num = 0;

    do{
      choice = gui_menu(left_win, menu, 7);

      switch (choice) {
        case 0: topic_menu(left_win, server, nr_on_server, que); break;
        case 1: sub_menu(left_win, server, nr_on_server, que); break;
        case 2: msg_menu(left_win, server, nr_on_server, que); break;
        case 3: zombie_subs_num = receive_msg_sync_menu(left_win, right_win, server, nr_on_server, que, zombie_subs, zombie_subs_num); break;
        case 4: kill(pid, SIGALRM); pause(); break;
        case 5: shutdown(server); break;
        case 6: break;
        default: print_error(left_win, "Niepoprawny wybór w menu głównym\n\r"); //shouldn't happen
      }
    }while(choice != 5 && choice != 6);


    clear();
    refresh();
    wmove(main_win, 8, terminal_width/3);
    print_info(main_win, "Do widzenia!\n");
    getchar();
    use_default_colors();
    delwin(left_win);
    delwin(right_win);
    endwin();
    kill(pid, SIGTERM);
    return 0;
  }
  return 1; //shouldn't happen
}
