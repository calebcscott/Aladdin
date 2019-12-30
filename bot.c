#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h> //hostent
#include <stdlib.h>
#include <unistd.h> //write
#include <string.h> //strtok
#include <errno.h>
#include <pthread.h>
#include "userinput.h" //username and password
#include <signal.h>
#include "twitch.h"
#include "cmdfile.h"

struct connectionData *connData;


int  analyseInput(char* strinput);
void* readerTHEThread(void* context);
void* writerTHEThread(void* context);
char* returnCommand(char* strinput);
int writeToFile(char* command, char* body);

int main(int argc, char* argv[]){
  struct connectionData conData;
  
  if(argc < 3){
    printf("usage: Aladdin --join <channel name>\n");
    exit(0);
  }
  if (strcmp(argv[1], "--join")!=0){
    printf("usage: Aladdin --join <channel name>\n");
    exit(0);
  }
 
  if(read_commands(NULL)==-1){
    printf("Warning: couldn't load commands\n");
  }
  
  setup();
  char channelName[20];
  sprintf(channelName, "#%s", argv[2]);
  joinChannel(channelName);
  
  pthread_t writerThread;
  pthread_t readerThread;
  
  
  pthread_create(&writerThread, NULL, writerTHEThread, (void *) &conData);
  pthread_create(&readerThread, NULL, readerTHEThread, (void *) &conData);
  
  conData.writerThread = writerThread;
  conData.readerThread = readerThread;
  
  connData = &conData;
  
  pthread_join(writerThread, NULL);
  sleep(1);
  pthread_join(readerThread, NULL);

  
  return 0;
}

//analyses twitch chat for commands
char* returnCommand(char* strinput){
  char* token = strtok(strinput, ":");
  char* retval = malloc(20);
  token = strtok(NULL, ":");
  memcpy(retval, token, 19);
  return retval;
}

//TODO 35: add GOTO for failing conditions
int writeToFile(char* command, char* body){
  FILE* f = fopen("commands.csv", "a");
  if (f == NULL){
    perror("error: couldn't open commands.csv");
    fclose(f);
    return -1;
  }
  if(fprintf(f, "%s,%s\n", command, body)==-1){
    perror("couldn't write to file");
    fclose(f);
    return -1;
  }
  fclose(f);
  return 0;
};

//analyses the user input (streamer side, not input from twitch channel)
int analyseInput(char* strinput){

  char* token = strtok(strinput, " ");
  
  if(strcmp(token, "say")==0){
    char buuf[50];
    sprintf(buuf, "PRIVMSG %s :%s\r\n", current, strtok(NULL, " "));
    if(sendMsg(buuf)==-1){
      return -1;
    } 
  }else if (strcmp(token, "quit\n")==0){
    close_commands();
    pthread_kill(connData->writerThread, SIGTERM);
    pthread_kill(connData->readerThread, SIGTERM);
    printf("-------------------\n");
  }else if(strncmp(token, "addcmd", 6)==0){
    if(strlen(strinput)==7){ //checks for arguments
      printf("addcmd <command> <message>\n");
      return 0;
    }
    char* commandName = strtok(NULL, " ");
    char body[strlen(strinput)-strlen(commandName)-6]; //char body has to be size of command body, so total length - length of commandName - length of strin 'add cmd' (6)
    token = strtok(NULL, " ");
    sprintf(body, ""); 
    while(token != NULL){
      sprintf(body, "%s ", strcat(body, token));
      token = strtok(NULL, " ");
    }
    body[strlen(body)-2] = 0;
    if(strlen(commandName)==1 || strlen(body)==0){
      printf("addcmd <command> <message>\n");
      return 0;
    }
    
    
    //If add_command fails return -1
    if(add_command(commandName, body) > 0) {
      return -1;
    }
    
    /* Code above performs function commenting out for testing
    if(writeToFile(commandName, body)==-1){
      return -1;
    }
    */
    
    //TODO 34: hacky way to add new command to struct of commands
    //add_command should fix commenting out for testing
    //finish();
    //init();


    return 0;
  }
  
  return -1;
}

//thread that reads from socket (twitch chat)
void* readerTHEThread(void* context){
  char buff[500];
  char outputmsg[1024];
  
  for (;;){
    bzero(buff, sizeof(buff));
    read(twitchsock, buff, sizeof(buff));    
    //catches ping from twitch servers 
    if(strcmp(buff, "PING :tmi.twitch.tv\r\n") == 0){

      char* payload = "PONG :tmi.twitch.tv\r\n";
      if(sendMsg(payload)==-1){
	return NULL;
      }
    }else {
      printf("\r%s", buff);
      sleep(0.5);
      printf("[%s]> ", current);
      fflush(stdout);
      
      char* command = returnCommand(buff);
      if(strcmp(command, "!credits\r\n")==0){ 
	//hard coded command
	char payload[100];
	sprintf(payload,"PRIVMSG %s :This bot was written by SamBkamp at: https://github.com/SamBkamp/Aladdin\r\n", current);

	if(sendMsg(payload)==-1){
	  return NULL;
	}
	
      }else if(test_command(command, outputmsg, 100)==1){
        fprintf(stderr, "Found command with message: %s", outputmsg);
	      char* addr = (char *)malloc(1024);
	      sprintf(addr, "PRIVMSG %s :%s\r\n", current, outputmsg); 
        sendMsg(addr);
	      free(addr);
      }
      fprintf(stderr, "Found command not ran\n");
      free(command);
    }
  }
  
  printf("closing writer thread\n");
}

//thread that writes to socket
void* writerTHEThread(void* context){
  char payload[50];
  
  sprintf(payload,"PRIVMSG %s :%s is here! HeyGuys\r\n", current, nick);

  if(sendMsg(payload)==-1){
    return NULL;
  }
  
  for (;;){
    printf("[%s]> ", current);
    char buffer[1024];
    //get streamer inputt
    fgets(buffer, 1024, stdin);
    analyseInput(buffer);
  }
}




