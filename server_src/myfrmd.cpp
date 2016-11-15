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

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <string>
#include <unordered_map>
#define MAX_BUFFER 4096
#define MAX_PENDING 0
#define MAX_USERS 16
#define MAX_BOARDS 32
using namespace std;

short int getuser(unordered_map<string,string> &users, const int s, string &user) {
   char buf[MAX_BUFFER];
   int num_sent,num_rec,i;
   string pswrd;

   // send request for username
   memset((char*)&buf,0,sizeof(buf));
   strcpy(buf,"Please enter your username (max of 16 chars):");
   if ((num_sent = send(s,buf,sizeof(buf),0)) == -1) {
      fprintf(stderr,"ERROR: send error\n");
      exit(1);
   }

   // receive username
   memset((char*)&buf,0,sizeof(buf));
   if ((num_rec = recv(s,buf,sizeof(buf),0)) == -1) {
      fprintf(stderr,"ERROR: username receive error\n");
      exit(1);
   }
   user.assign(buf);

   // send request for password (whether new user of not)
   memset((char*)&buf,0,sizeof(buf));
   strcpy(buf,"Please enter your password (max of 16 chars):");
   if ((num_sent = send(s,buf,strlen(buf),0)) == -1) {
      fprintf(stderr,"ERROR: send error\n");
      exit(1);
   }

   // receive password
   memset((char*)&buf,0,sizeof(buf));
   if ((num_rec = recv(s,buf,sizeof(buf),0)) == -1) {
      fprintf(stderr,"ERROR: password receive error\n");
      exit(1);
   }
   pswrd.assign(buf);

   // check to see if user is already in map
   auto got = users.find(user);
   if (got == users.end()) {
      // new user, add info
      users[user] = pswrd;
      return 1;
   } else {
      // check for successful login
      if (got->second == pswrd) {
         // success!
         return 1;
      } else {
         // incorrect password
         return 0;
      }
   }
}

int op_crt (int udp_s, struct sockaddr_in client_addr, socklen_t alen, unordered_map<string,int> &boards, string user) {
   /***********************
    *  CRT OPERATION
    ***********************/
   char buf[MAX_BUFFER];
   ofstream outfile;
   string bname;
   int num_rec;

   // receive board name from client
   memset((char*)&buf,0,sizeof(buf));
   if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }

   // then store name as filename
   bname.assign(buf);
   bname += ".txt";

   // check if board already exists
   if (boards.find(bname) != boards.end()) {
      // board exists
      return -1;
   } else {
      // add board to boards set
      boards[bname] = 0;
   }

   // open file stream and write first line
   outfile.open(bname.c_str());
   outfile << user << endl;

   // close file when done
   outfile.close();
   return 0;
}

int op_msg (int udp_s, struct sockaddr_in client_addr, socklen_t alen, unordered_map<string,int> &boards, string user) {
   /***********************
    *  MSG OPERATION
    ***********************/
   char buf[MAX_BUFFER];
   ofstream outfile;
   string bname;
   int num_rec;

   // receive board name from client
   memset((char*)&buf,0,sizeof(buf));
   if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }

   // then store name as filename
   bname.assign(buf);
   bname += ".txt";

   // receive message from client
   memset((char*)&buf,0,sizeof(buf));
   if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }
   
   // check if board already exists
   if (boards.find(bname) != boards.end()) {
      // board exists open file stream and print formatted message to board
      outfile.open(bname.c_str(), ofstream::app);
      outfile << ++(boards.find(bname)->second) << ": " << user << " - " << buf << endl;
   } else {
      // error since board does not exist
      return -1;
   }

   // close file when done
   outfile.close();
   return 0;
}

int op_dlt (int udp_s, struct sockaddr_in client_addr, socklen_t alen, unordered_map<string,int> &boards, string user) {
   /***********************
    *  DLT OPERATION
    ***********************/
   char buf[MAX_BUFFER];
   ifstream og_file;
   ofstream temp_file;
   string bname,board_user,line,cur_id;
   int num_rec;
   int msg_id;

   // receive board name from client
   memset((char*)&buf,0,sizeof(buf));
   if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }

   // then store name as filename
   bname.assign(buf);
   bname += ".txt";

   // receive message id from client
   if ((num_rec = recvfrom(udp_s,&msg_id,sizeof(msg_id),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }
   msg_id = ntohl(msg_id);

   // check if board exists
   if (boards.find(bname) == boards.end()) {
      // error since board does not exist
      return -1;
   } else {
      // if board exists open file stream and check user
      og_file.open(bname.c_str());
      getline(og_file,board_user);
      if (board_user != user) {
         // error since user is not same user who created board
         return -1;
      }
   }

   // open stream for temp file
   temp_file.open("tempfile.txt");
   temp_file << board_user << "\n";

   // loop through file looking for message id
   while (getline(og_file,line)) {
      stringstream ss(line);
      getline(ss,cur_id,':');
      if (stoi(cur_id) != msg_id) {
         // write line to new file as long as line is not supposed to be deleted
         line += "\n";
         temp_file << line;
      }
   }

   // remove og file
   remove(bname.c_str());
   // rename the tempfile
   rename("tempfile.txt",bname.c_str());

   // close files when done
   og_file.close();
   temp_file.close();
   return 0;
}

int main(int argc, char* argv[]) {
   struct sockaddr_in sin;             // struct for address info
   int slen = sizeof(sin); 
   struct sockaddr_in client_addr;     // struct for client address info
   int alen = sizeof(client_addr);     // size of address stuct
   char buf[MAX_BUFFER];               // message received from client
   int server_port;                    // user input - port num
   int s, new_s, udp_s;                // socket and new socket
   int num_rec, num_sent;              // size of messages sent and received
   string admin_passwd;                // user input - admin password
   unordered_map<string,string> users; // map of usernames to passwords
   short int res;                      // use for sending short int responses
   string user;                        // store current username
   unordered_map<string,int> boards;   // store all created boards

   // handle all arguments
   if (argc == 3) {
      // port number
      server_port = atoi(argv[1]);
      // admin password
      admin_passwd = argv[2];
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
      if ((new_s = accept(s,(struct sockaddr *)&sin,(socklen_t *)&slen)) < 0) {
         fprintf(stderr,"ERROR: accept error\n");
         exit(1);
      }

      memset((struct sockaddr*)&client_addr,0,sizeof(client_addr));

      // call user validation function
      res = getuser(users,new_s,user);

      // send conf to user if login was successful
      res = htons(res);
      if ((num_sent = send(new_s,&res,sizeof(res),0)) == -1) {
         fprintf(stderr,"ERROR: send error\n");
         exit(1);
      }

      // use conf to determine next state for server
      res = ntohs(res);
      if (!res) {
         continue;
      }

      while (1) {
         // receive commands from client (UDP)
         memset((char*)&buf,0,sizeof(buf));
         if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
            fprintf(stderr,"ERROR: receive error\n");
            exit(1);
         }
         cout << "User operation: " << buf << endl;

         // determine operation sent by client
         if (!strcmp(buf,"CRT")) {
            /***********************
             *  CRT OPERATION
             ***********************/
            memset((char*)&buf,0,sizeof(buf));
            if (op_crt(udp_s,client_addr,alen,boards,user) < 0) {
               strcpy(buf,"Board not created successfully\n\tcheck if it already exists\n");
            } else {
               strcpy(buf,"Board created successfully!\n");
            }
            if (sendto(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,sizeof(struct sockaddr)) == -1) {
               fprintf(stderr,"ERROR: server send error: %s\n",strerror(errno));
               exit(1);
            }
            continue;

         } else if (!strcmp(buf,"MSG")) {
            /***********************
             *  MSG OPERATION
             ***********************/
            memset((char*)&buf,0,sizeof(buf));
            if (op_msg(udp_s,client_addr,alen,boards,user) < 0) {
               strcpy(buf,"Message not posted successfully\n\tcheck if board exists\n");
            } else {
               strcpy(buf,"Message posted successfully!\n");
            }
            if (sendto(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,sizeof(struct sockaddr)) == -1) {
               fprintf(stderr,"ERROR: server send error: %s\n",strerror(errno));
               exit(1);
            }
            continue;

         } else if (!strcmp(buf,"DLT")) {
            /***********************
             *  DLT OPERATION
             ***********************/
            memset((char*)&buf,0,sizeof(buf));
            if (op_dlt(udp_s,client_addr,alen,boards,user) < 0) {
               strcpy(buf,"Message not deleted successfully\n\tcheck if board exists\n");
            } else {
               strcpy(buf,"Message deleted successfully!\n");
            }
            if (sendto(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,sizeof(struct sockaddr)) == -1) {
               fprintf(stderr,"ERROR: server send error: %s\n",strerror(errno));
               exit(1);
            }
            continue;
           
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
