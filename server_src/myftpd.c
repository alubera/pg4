/* Andrew Lubera  | alubera
 * Alan Vuong     | avuong
 * 10-31-16
 * CSE30264
 * Programming Assignment 3 - Program acts as a server for a simple File Transfer
 *    Protocol. The FTP will use a TCP connection to perform the following operations:
 *      REQ (Request), UPL (Upload), LIS (List), MKD (Make Directory), RMD (Remove 
 *      Directory), CHD (Change Directory), and XIT (Exit)
 *    USER INPUTS:
 *      port number (41038, 41039)
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
#include <mhash.h>
#include <dirent.h>
#define MAX_BUFFER 4096
#define MAX_PENDING 0

int my_ls(char* file_list) {
  DIR *dp;
  struct dirent *ep;
  int space = MAX_BUFFER;
  
  dp = opendir("./");
  if (dp != NULL) {
    while (ep = readdir(dp)) {
      strncat(file_list,ep->d_name,space);
      if ((space = space-sizeof(ep->d_name)) < 2) {
        // printf("too many file names to print\n");
        break;
      }
      strcat(file_list,"\n");
    }
    closedir(dp);
  } else {
    return -1;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  struct sockaddr_in sin;           // struct for address info
  int slen = sizeof(sin); 
  char buf[MAX_BUFFER];             // message received from client
  int server_port;                  // user input - port num
  int s, new_s;                     // socket and new socket
  int num_rec, num_sent;            // size of messages sent and received
  MHASH td;                         // for computing MD5 hash
  char hash[16], hash_client[16];   // for sending MD5 hash as 16-byte string

  // handle all arguments
  if (argc == 2) {
    // port number
    server_port = atoi(argv[1]);
  } else {
    fprintf(stderr,"ERROR: invalid number of arguments\n");
    fprintf(stderr,"\tprogram should be called with port number\n");
    exit(1);
  }

  // build address data structure
  memset((char*)&sin,0,sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(server_port);

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

  // variables used in client server communication
  short int fname_length;
  char* fname;
  FILE* fp;
  int file_size;
  int read;
  int response;

  // wait to get a connection request from a client
  while (1) {
    if ((new_s = accept(s,(struct sockaddr *)&sin,&slen)) < 0) {
      fprintf(stderr,"ERROR: accept error\n");
      exit(1);
    }
    // continue to receive messages after client connects until XIT
    while (1) {
      memset((char*)&buf,0,sizeof(buf));
      if ((num_rec = recv(new_s,buf,sizeof(buf)/sizeof(char),0)) == -1) {
        fprintf(stderr,"ERROR: receive error\n");
        exit(1);
      }

      // determine operation sent by client
      if (!strcmp(buf,"REQ")) {
        /**********************************
         *
         *  REQUEST OPERATION
         *
         **********************************/
        // receive file name length
        if ((num_rec = recv(new_s,&fname_length,sizeof(short int),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        fname_length = ntohs(fname_length);

        // use length to set mem for fname
        fname = (char*)malloc(fname_length);
        memset(fname,0,fname_length);

        // receive file name string
        memset((char*)&buf,0,sizeof(buf));
        if ((num_rec = recv(new_s,buf,sizeof(buf)/sizeof(char),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        strcpy(fname,buf);

        // find file and send size
        if ((fp = fopen(fname,"r")) != NULL) {
          // use fseek to get file size
          fseek(fp,0,SEEK_END);
          file_size = ftell(fp);
          fseek(fp,0,SEEK_SET);
        } else {
          // file does not exist, send error response
          file_size = -1;
        }
        file_size = htonl(file_size);
        if ((num_sent = send(new_s,&file_size,sizeof(int),0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1);         
        }
        file_size = ntohl(file_size);
        // if file does not exist, skip sending step
        if (file_size == -1) continue;

        // init mhash
        if ((td = mhash_init(MHASH_MD5)) == MHASH_FAILED) {
          fprintf(stderr,"ERROR: unable to initialize MHASH\n");
          exit(1);
        }

        // read file into buffer, 4096 chars at a time
        // send each buffer as well as adding to MD5 hash
        int count = 0;
        while ((read = fread(buf,sizeof(char),MAX_BUFFER,fp)) > 0) {
          memset((char*)&buf,0,sizeof(buf));
          if ((num_sent = send(new_s,buf,read,0)) == -1) {
            fprintf(stderr,"ERROR: send error\n");
            exit(1);         
          }
          mhash(td,buf,read);
        }

        // compute MD5 hash string and send
        mhash_deinit(td,hash);
        if ((num_sent = send(new_s,hash,sizeof(hash),0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1);         
        }

        free(fname);
        fclose(fp);

      } else if (!strcmp(buf,"UPL")) {
        /**********************************
         *
         *  UPLOAD OPERATION
         *
         **********************************/
        // receive file name length
        if ((num_rec = recv(new_s,&fname_length,sizeof(short int),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        fname_length = ntohs(fname_length);

        // use length to set mem for fname
        fname = (char*)malloc(fname_length);
        memset(fname,0,fname_length);

        // receive file name string
        memset((char*)&buf,0,sizeof(buf));
        if ((num_rec = recv(new_s,buf,sizeof(buf)/sizeof(char),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        strcpy(fname,buf);

        short int ack = htons(1);
        // send acknowledgement that server is ready to receive
        if ((num_sent = send(new_s,&ack,sizeof(ack),0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1);         
        }

        // receive file size
        if ((num_rec = recv(new_s,&file_size,sizeof(file_size),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        file_size = ntohl(file_size);
       
        // open file pointer for received text
        fp = fopen(fname, "a");

        // init mhash
        if ((td = mhash_init(MHASH_MD5)) == MHASH_FAILED) {
          fprintf(stderr,"ERROR: unable to initialize MHASH\n");
          exit(1);
        }

        struct timeval t_sent, t_rec, t_diff;
        gettimeofday(&t_sent,NULL);

        // receive file one buf at a time and write each to local file
        // compute MD5 hash while receiving
        int count = 0;
        int rec_size = sizeof(buf);
        while (1) {
          // check to see what size will be received this loop
          if (file_size-count < rec_size) {
            rec_size = file_size-count;
          }
          // receive data
          memset((char*)&buf,0,sizeof(buf));
          if ((num_rec = recv(new_s,buf,rec_size,0)) == -1) {
            fprintf(stderr,"ERROR: receive error\n");
            exit(1);
          }
          count += num_rec;
          // continue to compute hash
          mhash(td,buf,num_rec);
          // append buf to file
          fwrite(buf,sizeof(char),num_rec,fp);

          // break if whole file has been received
          if (count >= file_size) {
            break;
          }
        }

        // compute time to use in throughput calculation
        gettimeofday(&t_rec,NULL);

        // receive MD5 from client to make sure that transfer was successful
        if ((num_rec = recv(new_s,hash_client,sizeof(hash_client),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }

        // compute throughput
        // result should be in megabytes per second
        // bytes per microsecond == megabytes per second, avoid dividing by small decimal
        timersub(&t_rec,&t_sent,&t_diff);
        float throughput = file_size*1.0/((int)t_diff.tv_sec*10^6+(int)t_diff.tv_usec);

        // compute MD5 hash string and compare them
        mhash_deinit(td,hash);
        int i;
        for (i = 0; i < 16; ++i) {
          if (hash[i] != hash_client[i]) {
            // file transfer is unsuccessful
            break;
          }
        }
        memset((char*)&buf,0,sizeof(buf));
        if (i == 16) {
          // transfer was successful, send throughput to client
          snprintf(buf,sizeof(buf),"%f",throughput);
        } else {
          // transfer was unsuccessful, send a -1 instead of throughput
          snprintf(buf,sizeof(buf),"-1");
        }

        // send throughput back to client
        if ((num_sent = send(new_s,buf,strlen(buf),0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1); 
        }

        free(fname);
        fclose(fp);

      } else if (!strcmp(buf,"DEL")) {
        /**********************************
         *
         *  DELETE OPERATION
         *
         **********************************/
        // receive file name length
        if ((num_rec = recv(new_s,&fname_length,sizeof(short int),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        fname_length = ntohs(fname_length);

        // use length to set mem for fname
        fname = (char*)malloc(fname_length);
        memset(fname,0,sizeof(fname));

        // receive file name string
        if ((num_rec = recv(new_s,fname,fname_length,0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        memset((char*)&buf,0,sizeof(buf));

        // check to see if file exists
        if (access(fname,F_OK) < 0) {
          // could not access file
          response = htonl(-1);
        } else {
          // file exists
          response = htonl(1);
        }

        // send confirm message
        if ((num_sent = send(new_s,&response,sizeof(response),0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1);         
        }
        if (ntohl(response) == -1) {
          continue;
        }

        memset((char*)&buf,0,sizeof(buf));
        // receive confirmation that client wants to delete
        if ((num_rec = recv(new_s,buf,sizeof(buf)/sizeof(char),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        if (!strcmp(buf,"No")) {
          // user DOES NOT want to continue with delete
          continue;
        }
        
        memset((char*)&buf,0,sizeof(buf));
        // use remove to delete file
        if (remove(fname) < 0) {
          // if remove returns < 0 then there was a problem deleting the file
          strcpy(buf,"Failed to delete file\n");
        } else {
          // otherwise file deleted successfully
          strcpy(buf,"File deleted\n");
        }

        // send response message
        if ((num_sent = send(new_s,buf,sizeof(buf),0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1);         
        }

        free(fname);

      } else if (!strcmp(buf,"LIS")) {
        /**********************************
         *
         *  LIST OPERATION
         *
         **********************************/
        memset((char*)&buf,0,sizeof(buf));
        // call listing function
        if (my_ls(buf) < 0) { 
          fprintf(stderr,"ERROR: could not list directory\n");
          continue;
        }

        // send size of listing to client
        file_size = htonl(strlen(buf));
        if ((num_sent = send(new_s,&file_size,sizeof(file_size),0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1);         
        }
        file_size = ntohl(file_size);

        // send listing back to the client
        if ((num_sent = send(new_s,buf,file_size,0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1);         
        }

      } else if (!strcmp(buf,"MKD")) {
        /**********************************
         *
         *  MK DIR OPERATION
         *
         **********************************/
        // receive directory name length
        if ((num_rec = recv(new_s,&fname_length,sizeof(short int),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        fname_length = ntohs(fname_length);

        // use length to set mem for fname
        fname = (char*)malloc(fname_length+2);
        memset(fname,0,sizeof(fname));

        // receive directory name string
        memset((char*)&buf,0,sizeof(buf));
        if ((num_rec = recv(new_s,buf,fname_length,0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        strcpy(fname,"./");
        strncat(fname,buf,fname_length);
        memset((char*)&buf,0,sizeof(buf));

        // use mkdir to make directory
        if (mkdir(fname,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) < 0) {
          // send back -2 if directory already exists
          if (errno == EEXIST) {
            response = htonl(-2);
          // send back -1 if theres another error
          } else {
            response = htonl(-1);
          }
        } else {
          // send back 1 if directory is created successfully
          response = htonl(1);
        }

        // send response message
        if ((num_sent = send(new_s,&response,sizeof(int),0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1);         
        }
        free(fname);

      } else if (!strcmp(buf,"RMD")) {
         /**********************************
         *
         *  RM DIR OPERATION
         *
         **********************************/
        // receive directory name length
        if ((num_rec = recv(new_s,&fname_length,sizeof(short int),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        fname_length = ntohs(fname_length);

        // use length to set mem for fname
        fname = (char*)malloc(fname_length+3);
        memset(fname,0,sizeof(fname));

        // receive directory name string
        memset((char*)&buf,0,sizeof(buf));
        if ((num_rec = recv(new_s,buf,fname_length,0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        strcpy(fname,"./");
        strncat(fname,buf,fname_length);
        strcat(fname,"/");
        memset((char*)&buf,0,sizeof(buf));

        // check to see if directory exists
        if (access(fname,F_OK) < 0) {
          // could not access directory
          response = htonl(-1);
        } else {
          // directory exists
          response = htonl(1);
        }

        // send confirm message
        if ((num_sent = send(new_s,&response,sizeof(response),0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1);         
        }
        if (ntohl(response) == -1) {
          continue;
        }

        memset((char*)&buf,0,sizeof(buf));
        // receive confirmation that client wants to delete
        if ((num_rec = recv(new_s,buf,sizeof(buf)/sizeof(char),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        if (!strcmp(buf,"No")) {
          // user DOES NOT want to continue with delete
          continue;
        }
        
        memset((char*)&buf,0,sizeof(buf));
        // use rmdir to remove directory
        if (rmdir(fname) < 0) {
          // if rmdir returns < 0 then there was a problem deleting the directory
          strcpy(buf,"Failed to delete directory\n");
        } else {
          // otherwise directory deleted successfully
          strcpy(buf,"Directory deleted\n");
        }

        // send response message
        if ((num_sent = send(new_s,buf,sizeof(buf),0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1);         
        }

        free(fname);

      } else if (!strcmp(buf,"CHD")) {
        /**********************************
         *
         *  CH DIR OPERATION
         *
         **********************************/
        // receive directory name length
        if ((num_rec = recv(new_s,&fname_length,sizeof(short int),0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        fname_length = ntohs(fname_length);

        // use length to set mem for fname
        fname = (char*)malloc(fname_length+3);
        memset(fname,0,sizeof(fname));

        // receive directory name string
        memset((char*)&buf,0,sizeof(buf));
        if ((num_rec = recv(new_s,buf,fname_length,0)) == -1) {
          fprintf(stderr,"ERROR: receive error\n");
          exit(1);
        }
        strcpy(fname,"./");
        strncat(fname,buf,fname_length);
        strcat(fname,"/");
        memset((char*)&buf,0,sizeof(buf));

        // check to see if directory exists
        if (access(fname,F_OK) < 0) {
          // could not access directory
          response = htonl(-2);
        } else {
          // directory exists attempt to cd
          if (chdir(fname) < 0) {
            // could not cd
            response = htonl(-1);
          } else {
            // successful cd
            response = htonl(1);
          }
        }

        // send confirm message
        if ((num_sent = send(new_s,&response,sizeof(response),0)) == -1) {
          fprintf(stderr,"ERROR: send error\n");
          exit(1);         
        }

        free(fname);

      } else if (!strcmp(buf,"XIT")) {
        // break out of client loop
        break;
      }
    }
  }

  close(s);

  return 0;
}
