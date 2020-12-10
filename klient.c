#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

struct msgbuf{
  long type;
  int number;
  char text[1024];
};

int login(int server){
  struct msgbuf login_msg;
  int my_id = msgget(12341, 0644|IPC_CREAT); //TODO: nowy numer
  login_msg.type = 1;
  login_msg.number = 12341;
  strcpy(login_msg.text, "Obi Wan");
  msgsnd(server, &login_msg, sizeof(login_msg)-sizeof(long), 0);
  return my_id;
}

void register_sub(int server, int topic, int sublen){ //-1 -> forever
  struct msgbuf sub_msg;
  sub_msg.type = 3;
  sub_msg.number = topic;
  msgsnd(server, &sub_msg, sizeof(sub_msg)-sizeof(long), 0);
}

void register_topic(int server, int topic, char name[30]){
  struct msgbuf topic_msg;
  topic_msg.type = 2;
  topic_msg.number = topic;
  strcpy(topic_msg.text, name);
  msgsnd(server, &topic_msg, sizeof(topic_msg)-sizeof(long), 0);
}

void send_msg(int server, int topic, char msg_text[1024]){
  struct msgbuf message;
  message.type = 4;
  message.number = topic;
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
  int server = msgget(12340, 0);
  int me = login(server);
  shutdown(server);

  return 0;
}
