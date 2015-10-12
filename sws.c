#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define MAXPATHLEN 256
#define MAXBUFLEN 256
#define MAXIPLEN 50

int isDirectory(char *Path){
  //printf("P: %s\n",Path);
  if(Path[strlen(Path+1)] == '/'){
    //printf("is directory\n");
    return 1;
  }
  //printf("is file\n");
  return 0;

}

char * getMonth(int month){
  switch(month){
    case(1): return "Jan";
    case(2): return "Feb";
    case(3): return "Mar";
    case(4): return "Apr";
    case(5): return "May";
    case(6): return "Jun";
    case(7): return "Jul";
    case(8): return "Aug";
    case(9): return "Sept";
    case(10): return "Oct";
    case(11): return "Nov";
    case(12): return "Dec";    
  }
  return "";
}


void processRequest(int socket_fd, char *buffer, int port, char *clientip, char *directory, int requestno, struct sockaddr_in client, socklen_t len){
  time_t today;
  struct tm *t;
  int response = 200;
  time(&today);
  t = localtime(&today);
  char Request[10] = "";
  char FilePath[MAXPATHLEN];
  char Header[20] = "";
  char copyBuffer[MAXBUFLEN];
  char responseLog[50] = "";
  char copyDirectory[MAXPATHLEN] = "";

  strcpy(copyBuffer, buffer); 
  strcpy(copyDirectory, directory);

  char *pch = strtok(buffer," ");
  if(pch != NULL){
    strcpy(Request,pch);
    //printf("Request: %s\n",Request);
  }else{
    response = 400;
  }

  pch = strtok(NULL," ");
  if(pch != NULL){
    strcpy(FilePath,pch);  
  }else{
    response = 400;
  }
  //printf("FilePath: %s\n",FilePath);

  pch = strtok(NULL," ");
  if(pch != NULL){
    strcpy(Header,pch);
  }else{
    response = 400;
  }
  //printf("Header: %s\n",Header);

  char *month = getMonth(t->tm_mon);  

  if(strcmp(Header, "HTTP/1.0") != 0){
    response = 400;
  }

  if(strcmp(Request, "GET") != 0){
    response = 400;
    return;
  }

  strcat(copyDirectory, FilePath);
  //printf("CD: %s\n",copyDirectory);
  if(isDirectory(copyDirectory)){
    strcat(copyDirectory, "index.html");
  }
  //printf("nCD: %s\n", copyDirectory);
  int fp;
  if(response == 200){
    fp = open(copyDirectory, O_RDONLY);
    if(fp < 0){
      if(errno == ENOENT){
        response = 404; 
      }
    }
  }
  
  if(strstr(copyDirectory, "..") != NULL){
    response = 400;
  }

  if(response == 200){
    strcpy(responseLog, "HTTP/1.0 200 OK");
  }else if(response == 400){
    strcpy(responseLog, "HTTP/1.0 400 Bad Request");
  }else if(response == 404){
    strcpy(responseLog, "HTTP/1.0 404 Not Found");
  }
  
  char fileBuffer[MAXBUFLEN] = "";
  char endHeader[3] = "\n\n";
  sendto(socket_fd, &responseLog, strlen(responseLog)+1, 0, (struct sockaddr*)&client, len);
  sendto(socket_fd, &endHeader, 3, 0, (struct sockaddr*)&client, len);
  if(response == 200){
    read(fp, &fileBuffer, MAXBUFLEN);
    sendto(socket_fd, &fileBuffer, strlen(fileBuffer)+1, 0, (struct sockaddr*)&client, len);
  }

  printf("%d ", requestno);  
  printf("%d ", 1900+t->tm_year);
  printf("%s ", month); 
  printf("%.2d %.2d:%.2d:%.2d ", t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
  printf("%s:%d ",  clientip, port);
  printf("%s; ",copyBuffer); 
  printf("%s; \n", responseLog);
  fflush(stdout);
}

int main(int argc, char **argv){
  //variables
  struct sockaddr_in server,client;
  int socket_fd;
  int requestno = 0;
  char directory[256];

  //read input params
  if(argc != 3){
    printf("usage ./sws <port> <directory>\n");
    exit(-1);
  }

  int port = atoi(argv[1]);
  if(port == 0){
    printf("usage ./sws <port> <directory>\n");
    exit(-1);
  }
  strcpy(directory,argv[2]);

  //create socket()
  socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(socket_fd <= 0){
    printf("Failed to Create Socket\n");
  }

  server.sin_family = AF_INET; 
  server.sin_port = htons(port);
  server.sin_addr.s_addr = inet_addr("10.10.1.100");

  //set socketopt for reuseaddr
  int optval = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  //bind socket()
  if(bind(socket_fd, (struct sockaddr*)&server, sizeof(server)) == -1){
    printf("Error Binding Socket\n");
    exit(-1);
  }

  printf("sws is running on UDP port %d and serving %s\n", port, directory);
  printf("press ‘q’ then enter to quit ...\n");
  while(1){
    //check to see if q pressed
    fd_set rfds, sfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET(fileno(stdin),&rfds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if(select(1,&rfds,NULL,NULL,&tv) > 0){ //key pressed
      char c;
      read(fileno(stdin), &c, 1);
      if(c == 'q'){
        break;
      }
    }
    //check socket to see if any change
    FD_ZERO(&sfds);
    FD_SET(socket_fd,&sfds);
    int result = select(socket_fd+1,&sfds,NULL,NULL,&tv);

    if(result < 0){
      //error
    }
    else if(result == 1){
      ssize_t bytes;
      char buffer[MAXBUFLEN] = "";
      char clientip[MAXIPLEN] = "";
      socklen_t len = sizeof(client);
      bytes = recvfrom(socket_fd, &buffer, MAXBUFLEN, 0, (struct sockaddr *)&client, &len);
      if(bytes < 0){
        //error
        exit(-1);
      }
      int port = ntohs(client.sin_port);
      strcpy(clientip,inet_ntoa(client.sin_addr));
      if(directory[strlen(directory+1)] == '/'){
        directory[strlen(directory+1)] = '\0';
      }
      processRequest(socket_fd, buffer, port, clientip, directory, requestno, client, len);
      requestno++;

    }
  
    
  }

  return 0;
}//end main


