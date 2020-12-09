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
  strcpy(login_msg.text, "my_name");
  msgsnd(server, &login_msg, sizeof(login_msg)-sizeof(long), 0);
  return my_id;
}

void register_sub(int topic, int sublen){ //-1 - forever
}

void register_topic(int topic){
}

void send_msg(){
}

void receive_msg_sync(){
}

void receive_msg_async(){ //???
}

int main(int argc, char *argv[]) {
  int server = msgget(12340, 0);
  int me = login(server);

  return 0;
}
