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
#include <vector>
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
   bname += ".board";

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
   bname += ".board";

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

int op_dlt_edt (int udp_s, struct sockaddr_in client_addr, socklen_t alen, unordered_map<string,int> &boards, string user, bool edt) {
   /****************************
    *  DLT AND EDIT OPERATIONS
    ****************************/
   char buf[MAX_BUFFER];
   ifstream og_file;
   ofstream temp_file;
   string bname,line_user,line,cur_id,new_line;
   int num_rec;
   int msg_id;
   int res = -1;

   // receive board name from client
   memset((char*)&buf,0,sizeof(buf));
   if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }
   // then store name as filename
   bname.assign(buf);
   bname += ".board";

   // receive message id from client
   if ((num_rec = recvfrom(udp_s,&msg_id,sizeof(msg_id),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }
   msg_id = ntohl(msg_id);

   if (msg_id < 0) {
      // obviously not valid message id
      // client probably detected invalid input
      return -1;
   }

   // if this is an edt call, receive again (for edit)
   if (edt) {
      memset((char*)&buf,0,sizeof(buf));
      if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
         fprintf(stderr,"ERROR: receive error\n");
         exit(1);
      }
      // then store name as filename
      new_line.assign(buf);
   }

   // check if board exists
   auto got = boards.find(bname);
   if (got == boards.end()) {
      // error since board does not exist
      return -1;
      // if msg_id is not valid, also return error
      if (msg_id > got->second) {
         return -1;
      }
   }

   // board exists, open file stream
   og_file.open(bname.c_str());
   getline(og_file,line);

   // open stream for temp file
   temp_file.open("tempfile.txt");
   temp_file << line << "\n";

   // loop through file looking for message id
   while (getline(og_file,line)) {
      stringstream ss(line);
      getline(ss,cur_id,':');
      if (stoi(cur_id) == msg_id) {
         // target line found
         // get space and then username for line
         char c;
         ss.get(c);
         ss >> line_user;
         // check user before doing anything else
         if (line_user == user) {
            if (edt) {
               // if edit flag is set, write edited line to file
               temp_file << msg_id << ": " << user << " - " << new_line << endl;
            }
            // dlt will write nothing
            res = 0;
            // skip writing original line to file
            continue;
         }
         // if user is not correct, no changes will be made to file
         // error is also returned to user
      }
      // write line to new file as long as line is not supposed to be changed
      line += "\n";
      temp_file << line;
   }

   // remove og file
   remove(bname.c_str());
   // rename the tempfile
   rename("tempfile.txt",bname.c_str());

   // close files when done
   og_file.close();
   temp_file.close();
   return res;
}

void op_lis (int udp_s, struct sockaddr_in client_addr, socklen_t alen, const unordered_map<string,int> &boards) {
   /***********************
    *  LIS OPERATION
    ***********************/
   // loop through board names in boards and add them to buffer with new lines
   // then just send buffer
   char buf[MAX_BUFFER];
   size_t ext_loc;
   int space = MAX_BUFFER;

   memset((char*)&buf,0,sizeof(buf));
   // loop through all boards
   for (auto it = boards.begin(); it != boards.end(); ++it) {
      // find where extension begins
      ext_loc = it->first.find(".");
      // add board name to buf to send
      strncat(buf,it->first.substr(0,ext_loc).c_str(),space);
      space = MAX_BUFFER-strlen(buf);
      strncat(buf,"\n",space--);
      if (space <= 0) break;
   }

   // send buf once all boards are added or full
   if (sendto(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,sizeof(struct sockaddr)) == -1) {
      fprintf(stderr,"ERROR: server send error: %s\n",strerror(errno));
      exit(1);
   }
}

void op_rdb_dwn (int udp_s, int tcp_s, struct sockaddr_in client_addr, socklen_t alen, const unordered_map<string,int> &boards, bool dwn) {
   /**************************
    *  RDB AND DWN OPERATIONS
    **************************/
   char buf[MAX_BUFFER];
   int num_rec;
   int fsize;
   ifstream b_file;
   string bname;
   string fname;

   // receive board name from client
   memset((char*)&buf,0,sizeof(buf));
   if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }
   // then store name as filename
   bname.assign(buf);

   if (dwn) {
      // receive file name from client
      memset((char*)&buf,0,sizeof(buf));
      if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
         fprintf(stderr,"ERROR: receive error\n");
         exit(1);
      }
      // then store name
      fname = bname;
      fname += "-";
      fname += buf;
   }
   bname += ".board";

   // check if board exists
   if (boards.find(bname) == boards.end()) {
      // error since board does not exist
      fsize = -1;
   } else {
      // board exists, get filesize
      // open APN file if this is called from dwn
      if (dwn) {
         b_file.open(fname.c_str(),ios::binary);
      } else {
         b_file.open(bname.c_str(),ios::binary);
      }
      // make sure that file exists (in the case of dwn)
      if (b_file.good()) {
         b_file.seekg(0,ios::end);
         fsize = b_file.tellg();
         b_file.close();
      } else {
         fsize = -1;
      }
   }

   // send file size (or negative conf)
   fsize = htonl(fsize);
   if (sendto(udp_s,&fsize,sizeof(fsize),0,(struct sockaddr*)&client_addr,sizeof(struct sockaddr)) == -1) {
      fprintf(stderr,"ERROR: server send error: %s\n",strerror(errno));
      exit(1);
   }
   fsize = ntohl(fsize);

   // if board does not exist, skip sending step
   if (fsize == -1) return;

   // reopen board/file
   if (dwn) {
      b_file.open(fname.c_str());
   } else {
      b_file.open(bname.c_str());
   }

   // read into buffer, 4096 chars at a time, and send
   int count = 0;
   int read;
   while ((read = b_file.readsome(buf,MAX_BUFFER)) > 0) {
      if (send(tcp_s,buf,read,0) == -1) {
         fprintf(stderr,"ERROR: send error\n");
         exit(1);         
      }
      if (b_file.eof()) {cout << "eof" << endl; break;}
      memset((char*)&buf,0,sizeof(buf));
   }

   b_file.close();
   return;
}

void op_apn (int udp_s, int tcp_s, struct sockaddr_in client_addr, socklen_t alen, unordered_map<string,int> &boards, string user, unordered_map<string,vector<string> > &appends) {
   /***********************
    *  APN OPERATION
    ***********************/
   char buf[MAX_BUFFER];
   int num_rec;
   int fsize;
   int res;
   int fname_len;
   ofstream a_file, b_file;
   ifstream infile;
   string bname, fname;

   // receive board name from client
   memset((char*)&buf,0,sizeof(buf));
   if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }
   // then store name
   bname.assign(buf);

   // receive file name from client
   memset((char*)&buf,0,sizeof(buf));
   if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }
   // then store name
   fname_len = strlen(buf);
   fname = bname;
   bname += ".board";
   fname += "-";
   fname += buf;

   // check if board exists
   if (boards.find(bname) == boards.end() ) {
      // error since board does not exist
      res = -1;
   } else {
      // else check if file already exists
      infile.open(fname.c_str());
      if (infile.good()) {
         res = -1;
      } else {
         // else file can be appended to board
         res = 1;
      }
   }
   infile.close();

   res = htonl(res);
   if (sendto(udp_s,&res,sizeof(res),0,(struct sockaddr*)&client_addr,sizeof(struct sockaddr)) == -1) {
      fprintf(stderr,"ERROR: server send error: %s\n",strerror(errno));
      exit(1);
   }
   res = ntohl(res);

   // if file cannot be appeneded, do not receive
   if (res < 0) return;

   // receive file size from client
   if ((num_rec = recvfrom(udp_s,&fsize,sizeof(fsize),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }
   fsize = ntohl(fsize);

   // check for error
   if (fsize < 0) return;

   // open ofstream
   a_file.open(fname.c_str());

   // receive file one buf at a time and write each to local file
   int count = 0;
   int rec_size = sizeof(buf);
   while (1) {
      // check to see what size will be received this loop
      if (fsize-count < rec_size) {
         rec_size = fsize-count;
      }
      // receive data
      memset((char*)&buf,0,sizeof(buf));
      if ((num_rec = recv(tcp_s,buf,rec_size,0)) == -1) {
         fprintf(stderr,"ERROR: receive error\n");
         exit(1);
      }
      count += num_rec;
      // append buf to file
      a_file.write(buf,num_rec);

      // break if whole file has been received
      if (count >= fsize) {
         break;
      }
   }

   // write line to message board file
   string og_name = fname.substr(fname.find("-")+1,fname_len);
   b_file.open(bname.c_str(), ofstream::app);
   b_file << ++(boards.find(bname)->second) << ": " << user << " - " << og_name << " (APN FILE)" << endl;

   // keep track of appended files
   appends[bname].push_back(fname);

   a_file.close();
   b_file.close();
   return;
}

void help_me_destroy(string board, unordered_map<string,vector<string> > appends) {
   // delete all appended files
   auto got = appends.find(board);
   if (got != appends.end()) {
      for (auto it = got->second.begin(); it != got->second.end(); ++it) {
         remove(it->c_str());
      }
   }

   // lastly remove the board itself
   remove(board.c_str());

   return;
}

int op_dst (int udp_s, struct sockaddr_in client_addr, socklen_t alen, unordered_map<string,int> &boards, string user, unordered_map<string,vector<string> > &appends) {
   /***********************
    *  DST OPERATION
    ***********************/
   char buf[MAX_BUFFER];
   ifstream b_file;
   string bname,b_user;
   int num_rec;

   // receive board name from client
   memset((char*)&buf,0,sizeof(buf));
   if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
      fprintf(stderr,"ERROR: receive error\n");
      exit(1);
   }
   // then store name as filename
   bname.assign(buf);
   bname += ".board";

   // check if board exists
   if (boards.find(bname) == boards.end()) {
      // error since board does not exist
      return -1;
   }

   // board exists, open file stream
   b_file.open(bname.c_str());
   getline(b_file,b_user);

   // check if board user is the same as current user
   if (b_user != user) {
      return -1;
   }

   // my helper function for destroying things
   help_me_destroy(bname,appends);

   // get rid of board from maps
   appends.erase(bname);
   boards.erase(bname);

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
   string admin_pswd;                // user input - admin password
   unordered_map<string,string> users; // map of usernames to passwords
   short int res;                      // use for sending short int responses
   string user;                        // store current username
   unordered_map<string,int> boards;   // store all created boards
   unordered_map<string,vector<string> > appends;   // store all appended files

   // handle all arguments
   if (argc == 3) {
      // port number
      server_port = atoi(argv[1]);
      // admin password
      admin_pswd = argv[2];
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

         } else if (!strcmp(buf,"DLT")) {
            /***********************
             *  DLT OPERATION
             ***********************/
            memset((char*)&buf,0,sizeof(buf));
            if (op_dlt_edt(udp_s,client_addr,alen,boards,user,false) < 0) {
               strcpy(buf,"Message not deleted successfully\n\tboard must exist and must be correct user\n");
            } else {
               strcpy(buf,"Message deleted successfully!\n");
            }
            if (sendto(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,sizeof(struct sockaddr)) == -1) {
               fprintf(stderr,"ERROR: server send error: %s\n",strerror(errno));
               exit(1);
            }
           
         } else if (!strcmp(buf,"EDT")) {
            /***********************
             *  EDT OPERATION
             ***********************/
            memset((char*)&buf,0,sizeof(buf));
            if (op_dlt_edt(udp_s,client_addr,alen,boards,user,true) < 0) {
               strcpy(buf,"Message not edited successfully\n\tboard must exist and must be correct user\n");
            } else {
               strcpy(buf,"Message edited successfully!\n");
            }
            if (sendto(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,sizeof(struct sockaddr)) == -1) {
               fprintf(stderr,"ERROR: server send error: %s\n",strerror(errno));
               exit(1);
            }

         } else if (!strcmp(buf,"LIS")) {
            /***********************
             *  LIS OPERATION
             ***********************/
            op_lis(udp_s,client_addr,alen,boards);

         } else if (!strcmp(buf,"RDB")) {
            /***********************
             *  RDB OPERATION
             ***********************/
            op_rdb_dwn(udp_s,new_s,client_addr,alen,boards,false);

         } else if (!strcmp(buf,"APN")) {
            /***********************
             *  APN OPERATION
             ***********************/
            op_apn(udp_s,new_s,client_addr,alen,boards,user,appends);

         } else if (!strcmp(buf,"DWN")) {
            /***********************
             *  DWN OPERATION
             ***********************/
            op_rdb_dwn(udp_s,new_s,client_addr,alen,boards,true);

         } else if (!strcmp(buf,"DST")) {
            /***********************
             *  DST OPERATION
             ***********************/
            memset((char*)&buf,0,sizeof(buf));
            if (op_dst(udp_s,client_addr,alen,boards,user,appends) < 0) {
               strcpy(buf,"Board not destroyed\n\tboard must exist and must be correct user\n");
            } else {
               strcpy(buf,"Board destroyed successfully!\n");
            }
            if (sendto(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,sizeof(struct sockaddr)) == -1) {
               fprintf(stderr,"ERROR: server send error: %s\n",strerror(errno));
               exit(1);
            }

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
            string user_pswd;
            int res;

            memset((char*)&buf,0,sizeof(buf));
            // receive admin pswd from client
            if ((num_rec = recvfrom(udp_s,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,(socklen_t*)&alen)) == -1) {
               fprintf(stderr,"ERROR: receive error\n");
               exit(1);
            }
            user_pswd.assign(buf);
            if (user_pswd == admin_pswd) {
               // set flag/respose to delete everything
               res = 1;
            } else {
               // else passwords dont match...do nothing
               res = -1;
            }
            res = htonl(res);
            if (sendto(udp_s,&res,sizeof(res),0,(struct sockaddr*)&client_addr,sizeof(struct sockaddr)) == -1) {
               fprintf(stderr,"ERROR: server send error: %s\n",strerror(errno));
               exit(1);
            }
            res = ntohl(res);
            if (res == 1) {
               // shutdown delete everything!!
               for (auto it = boards.begin(); it != boards.end(); ++it) {
                  help_me_destroy(it->first,appends);
               }
               close(new_s);
               close(udp_s);
               close(s);
               return 0;
            }
         }
      }
   }
}
