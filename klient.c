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
*/

union topic_tag{
    char name[30];
    int  id;
};

struct msgbuf{
  long type;
  int id;
  int number;
  char text[1024];
  union topic_tag topic_tag;
};

struct msgserv{
  long type;
  int info;
};

int login(int server){
  struct msgbuf login_msg;
  int que_nr = getpid();
  int que = msgget(que_nr, 0644|IPC_CREAT);
  login_msg.type = 1;
  login_msg.id = que_nr;
  strcpy(login_msg.text, "Obi Wan");
  msgsnd(server, &login_msg, sizeof(login_msg)-sizeof(long), 0);

  struct msgserv feedback;
  msgrcv(server, &feedback, sizeof(feedback)-sizeof(long), que_nr, 0);
  printf("High five: %d\n", feedback.info);
  if (feedback.info >= 0)
    return (que_nr, feedback.info);  //TODO: jak to zwrócić?
  //else
    //obsługa błędów
}

void register_topic(int server, char topic_name[30]){
  struct msgbuf topic_msg;
  topic_msg.type = 2;
  strcpy(topic_msg.text, topic_name);
  msgsnd(server, &topic_msg, sizeof(topic_msg)-sizeof(long), 0);
}

void register_sub(int server, union topic_tag topic, int sublen){ //-1 -> forever
  struct msgbuf sub_msg;
  sub_msg.type = 3;
  sub_msg.topic_tag = topic;
  sub_msg.number = sublen;
  msgsnd(server, &sub_msg, sizeof(sub_msg)-sizeof(long), 0);
}

void send_msg(int server, union topic_tag topic, char msg_text[1024]){
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
  int server = msgget(12345, 0);
  int me = login(server);
  printf("%d\n", me);
  shutdown(server);

  return 0;
}
