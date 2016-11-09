/* Andrew Lubera  | alubera
 * Alan Vuong     | avuong
 * 11-14-16
 * CSE30264
 * Programming Assignment 4 - Program acts as a server for a meesage board forum. 
 *      Both TCP and UDP will be used to perform the following operations:
 *      CRT (Create Board), LIS (List Boards), MSG (Leave Message), DLT (Delete Message),
 *      RDB (Read Board), EDT (Edit Message), APN (Append File), DWN (Download File),
 *      DST (Destroy Board), XIT (Exit), and SHT (Shutdown Server)
 *    USER INPUTS:
 *      port number (41038, 41039) and admin password
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <dirent.h>
#define MAX_BUFFER 4096
#define MAX_PENDING 0
#define MAX_USERS 16

typedef struct {
  char user[16];
  char pswd[16];
} user_info;

typedef struct {
  user_info info[MAX_USERS];  // because realloc'ing would be a pain
  int num;
} all_users;

short int getuser(all_users*,int,char[]);

int main(int argc, char* argv[]) {
  struct sockaddr_in sin;           // struct for address info
  int slen = sizeof(sin); 
  struct sockaddr_in client_addr;   // struct for client address info
  int alen = sizeof(client_addr);   // size of address stuct
  char buf[MAX_BUFFER];             // message received from client
  int server_port;                  // user input - port num
  int s, new_s, udp_s;              // socket and new socket
  int num_rec, num_sent;            // size of messages sent and received
  char* admin_passwd;               // user input - admin password
  all_users current_users;          // structure for storing user info
  short int res;                    // use for sending short int responses
  char user[16];                    // store current username

  // handle all arguments
  if (argc == 3) {
    // port number
    server_port = atoi(argv[1]);
    // admin password
    admin_passwd = malloc(strlen(argv[2]));
    strcpy(admin_passwd,argv[2]);
    // init for user struct
    current_users.num = 0;
  } else {
    fprintf(stderr,"ERROR: invalid number of arguments\n");
    fprintf(stderr,"\tprogram should be called with port number and admin password\n");
    exit(1);
  }

  // build address data structure
  memset((char*)&sin,0,sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(server_port);

  // create UDP socket
  udp_s = socket(PF_INET,SOCK_DGRAM,0);
  if (udp_s < 0) {
    fprintf(stderr,"ERROR: unable to open socket\n");
    exit(1);
  }

  // bind UDP socket to address
  if ((bind(udp_s,(struct sockaddr*)&sin,slen)) < 0) {
    fprintf(stderr,"ERROR: unable to bind socket\n");
    exit(1);
  }

  // create TCP socket
  s = socket(PF_INET,SOCK_STREAM,0);
  if (s < 0) {
    fprintf(stderr,"ERROR: unable to create socket\n");
    exit(1);
  }

  // set reuse option on socket
  int opt = 1;
  if ((setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))) < 0) {
    fprintf(stderr,"ERROR: unable to set reuse option on socket\n");
    exit(1);
  }

  // bind socket to address
  if ((bind(s,(struct sockaddr*)&sin,slen)) < 0) {
    fprintf(stderr,"ERROR: unable to bind socket\n");
    exit(1);
  }

  // open the TCP socket by listening
  if ((listen(s,MAX_PENDING)) < 0) {
    fprintf(stderr,"ERROR: unable to open socket\n");
    exit(1);
  }

  // wait to get a connection request from a client
  while (1) {
    // use TCP for initial connection, user verification
    if ((new_s = accept(s,(struct sockaddr *)&sin,&slen)) < 0) {
      fprintf(stderr,"ERROR: accept error\n");
      exit(1);
    }

    memset((struct sockaddr*)&client_addr,0,sizeof(client_addr));
    memset((char*)&user,0,sizeof(user));

    // call user validation function
    res = getuser(&current_users,new_s,user);

    // send conf to user if login was successful
    res = htons(res);
    if ((num_sent = send(new_s,&res,sizeof(res),0)) == -1) {
      fprintf(stderr,"ERROR: send error\n");
      exit(1);
    }

    // use conf to determine next state for server
    res = ntohs(res);
    if (res) {
      printf("user connected!\t%s\n",user);
    } else {
      printf("user not connected\n");
      continue;
    }
    while (1) {
      // receive commands from client (UDP)
      memset((char*)&buf,0,sizeof(buf));
      if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
        fprintf(stderr,"ERROR: receive error\n");
        exit(1);
      }
      printf("User operation: %s\n",buf);
      // determine operation sent by client
      if (!strcmp(buf,"CRT")) {
        /***********************
         *  CRT OPERATION
         ***********************/
        // client sends name of board
        // server sends conf message
//        char buf[MAX_BUFFER];
        memset((char*)&buf,0,sizeof(buf));
        if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }

      } else if (!strcmp(buf,"MSG")) {
        /***********************
         *  MSG OPERATION
         ***********************/
        // client sends name of board
        // client sends message to post
        // server sends conf message
      } else if (!strcmp(buf,"DLT")) {
        /***********************
         *  DLT OPERATION
         ***********************/
        // client sends name of board
        // client sends message ID to delete (short int)
        // server sends conf message
      } else if (!strcmp(buf,"EDT")) {
        /***********************
         *  EDT OPERATION
         ***********************/
        // client sends name of board
        // client sends message ID to edit (short int)
        // server sends conf message
      } else if (!strcmp(buf,"LIS")) {
        /***********************
         *  LIS OPERATION
         ***********************/
        // TBD
      } else if (!strcmp(buf,"RDB")) {
        /***********************
         *  RDB OPERATION
         ***********************/
        // SEND TCP
        // client sends name of board to read
        // server sends filesize/boardsize (int) or neg
        // server sends file in chunks (use UPL/REQ)
      } else if (!strcmp(buf,"APN")) {
        /***********************
         *  APN OPERATION
         ***********************/
        // SEND TCP
        // client sends name of board
        // client sends name of file to be appended
        // server sends conf message (int)
        // if conf was positive, client sends file in chunks
      } else if (!strcmp(buf,"DWN")) {
        /***********************
         *  DWN OPERATION
         ***********************/
        // SEND TCP
        // client sends name of board
        // client sends name of file to be downloaded
        // server sends filesize (int) or neg
        // server sends file in chunks
      } else if (!strcmp(buf,"DST")) {
        /***********************
         *  DST OPERATION
         ***********************/
        // client sends name of board to be destroyed
        // server needs to check if this is user who created board
        // server sends conf message
      } else if (!strcmp(buf,"XIT")) {
        /***********************
         *  XIT OPERATION
         ***********************/
        // server closes TCP socket
        close(new_s);
        break;
      } else if (!strcmp(buf,"SHT")) {
        /***********************
         *  SHT OPERATION
         ***********************/
        // client sends admin password to server
        // send conf int if passwords match
      }
    }
  }
  close(s);
  return 0;
}

short int getuser(all_users* users_ptr, int s, char user[16]) {
  char buf[MAX_BUFFER];
  int num_sent,num_rec,i;

  // send request for username
  memset((char*)&buf,0,sizeof(buf));
  strcpy(buf,"Please enter your username (max of 16 chars):");
  if ((num_sent = send(s,buf,sizeof(buf),0)) == -1) {
    fprintf(stderr,"ERROR: send error\n");
    exit(1);
  }

  // receive username
  //memset((char*)&user,0,sizeof(user));
  if ((num_rec = recv(s,user,sizeof(user),0)) == -1) {
    fprintf(stderr,"ERROR: username receive error\n");
    exit(1);
  }

  // check to see if existing user
  printf("check to see if existing user\n");
  for (i = 0; i < users_ptr->num; ++i) {
    printf("\t%i\t%s\n",i,users_ptr->info[i].user);
    if (!strcmp(users_ptr->info[i].user,user)) {
      break;
    }
  }
  printf("\n");
  // if user was not found, i will equal users_ptr->num

  memset((char*)&buf,0,sizeof(buf));
  // send request for password (whether new user of not)
  strcpy(buf,"Please enter your password (max of 16 chars):");
  if ((num_sent = send(s,buf,strlen(buf),0)) == -1) {
    fprintf(stderr,"ERROR: send error\n");
    exit(1);
  }

  // receive password
  memset((char*)&buf,0,sizeof(buf));
  if ((num_rec = recv(s,buf,sizeof(buf),0)) == -1) {   // hardcoded password length here
    fprintf(stderr,"ERROR: password receive error\n");
    exit(1);
  }

  printf("%s\t%s\n",user,buf);

  if (i < MAX_USERS) {
    if (i == users_ptr->num) {
      // new user, add to user info
      strcpy(users_ptr->info[i].user,user);
      strcpy(users_ptr->info[i].pswd,buf);
      users_ptr->num++;
      return 1;
    } else {
      // check for successful login
      if (!strcmp(users_ptr->info[i].pswd,buf)) {
        // successful login!
        return 1;
      } else {
        // incorrect password
        return 0;
      }
    }
  } else {
    // too many users, new login failed
    return 0;
  }

}
