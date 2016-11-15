//Alan Vuong and Andrew Lubera
//netid: avuong and alubera
//Client of a message board forum
//using TCP and UDP to transfer data
//UDP for small transfers and
//TCP for larger transfers

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#define MAX_BUFFER 4096

//function prototypes
int authentication(int s);
int create(int s_udp, struct sockaddr_in sin2);
int message(int s_udp, struct sockaddr_in sin2);
int delete(int s_udp, struct sockaddr_in sin2);
int edit(int s_udp, struct sockaddr_in sin2);
int read_board(int s_udp, struct sockaddr_in sin2, int s_tcp);
int destroy_board(int s_udp, struct sockaddr_in sin2);
int shut_down(int s_udp, struct sockaddr_in sin2, int s_tcp);

int main(int argc, const char* argv[]){

    //check for valid input
    //ex.) ./myftp Server_name [port]
    if(argc != 3){
        printf("Error: Usage ./myftp <hostname> <portnumber>\n");
        exit(1);
    }

    //initialize variables
    int port_num = atoi(argv[2]);
    int s_tcp, s_udp; //socket return code
    struct hostent *hp;
    struct sockaddr_in sin; //struct to hold info about sockets
    const char* host_name = argv[1];
    char buf[MAX_BUFFER];
    struct sockaddr_in client_addr;
    int send_val;

    //TCP socket

    //creating the socket
    if ((s_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Error creating the socket");
        exit(1);
    }

    //translate host name into peers IP address
    hp = gethostbyname(host_name);
    if (!hp){
        fprintf(stderr, "simplex-talk unknown host: %s\n", host_name);
        exit(1);
    }

    //set address and port number
    memset((char*)&sin, 0, sizeof(sin)); //make sure to clear mem beforehand
    sin.sin_family = AF_INET;
    bcopy((char *)hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(port_num);

    //create connection
    if (connect(s_tcp, (struct sockaddr *) &sin, sizeof(sin)) < 0){
        printf("Error with connecting\n");
        exit(1);
    }

    //create UDP socket same var names but with 2 appended
    struct hostent *hp2;
    struct sockaddr_in sin2;

    hp2 = gethostbyname(host_name);
    if (!hp2){
        fprintf(stderr, "simplex-talk unknown host: %s\n", host_name);
        exit(1);
    }

    if ((s_udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        printf("Sorry. Error creating the socket\n");
        exit(0);
    }

    //set address and port num
    memset((char*)&sin2, 0, sizeof(sin2));
    sin2.sin_family = AF_INET;
    sin2.sin_port = htons(port_num);
    bcopy(hp2->h_addr, (char *)&sin2.sin_addr, hp2->h_length);
    socklen_t len = sizeof(struct sockaddr_in);
   
    //reprompt while sign in fails
    if (authentication(s_tcp) != 1){
        printf("Sign in failed. Client exiting...\n");
        exit(1);
    }

    printf("Login successful\n");
    char operation_buf[4];
    //go into loop and prompt for user operation
    while(1){
        printf("Hello, please enter an operation: CRT, LIS, MSG, DLT, RDB, EDT, APN, DWN, DST, XIT, SHT:\n");
        fflush(stdout);

        bzero(operation_buf, sizeof(operation_buf));
        fflush(stdin);
        scanf("%s", operation_buf);

        //send the operation buffer to the server

        if ((send_val = sendto(s_udp, operation_buf, strlen(operation_buf), 0, (struct sockaddr*)&sin2, len)) < 0){
            printf("Error with sending operation to server\n");
            exit(1);
        }

        //carry out the function
        if(!strcmp(operation_buf, "CRT")){
            
            if (create(s_udp, sin2) < 0){
                printf("CRT operation failed\n");
                exit(1);
            }    

        } else if (!strcmp(operation_buf, "LIS")){

            

        } else if (!strcmp(operation_buf, "MSG")){
 
            if (message(s_udp, sin2) < 0){
                printf("MSG operation failed\n");
                exit(1);
            }       

        } else if (!strcmp(operation_buf, "DLT")){

            if (delete(s_udp, sin2) < 0){
                printf("DLT operation failed\n");
                exit(1);
            }

        } else if (!strcmp(operation_buf, "RDB")){ 

            if(read_board(s_udp, sin2, s_tcp) < 0){
                printf("RDB operation failed\n");
                exit(1);
            }

        } else if (!strcmp(operation_buf, "EDT")){

            if (edit(s_udp, sin2) < 0){
                printf("EDT operation failed\n");
                exit(1);
            }

        } else if (!strcmp(operation_buf, "APN")){

            //send name append to
            //send name to be appended
            //receive +1 or -1 
            //+ means good it passed


        } else if (!strcmp(operation_buf, "DWN")){

            //receive file size or negative

        } else if (!strcmp(operation_buf, "DST")){

            if (destroy_board(s_udp, sin2) < 0){
                printf("DST operation failed\n");
                exit(1);
            }

        } else if (!strcmp(operation_buf, "XIT")){
            //close sockets
            close(s_udp);
            close(s_tcp);
            return 0;

        } else if (!strcmp(operation_buf, "SHT")){
        
            if(shut_down(s_udp, sin2, s_tcp) < 0){
                printf("SHT operation failed\n");
                exit(1);
            }
        } else{
           
           printf("Did not enter a valid operation, please try again\n");
           printf("Make sure input is in all caps\n");
        }
   
    }

}

//use TCP for this socket
int authentication(int s){

    int rec_bytes; //number of bytes received
    int send_val; //send number of bytes
    char buf[MAX_BUFFER];
    bzero(buf, sizeof(buf));
       
    //receive string prompting for user name
    if((rec_bytes = recv(s, buf, sizeof(buf), 0)) < 0){
        printf("Error receiving user prompt!\n");
        exit(1);
    }
    //print out request for username
    printf("%s\n", buf, rec_bytes);
    bzero(buf, sizeof(buf));
    scanf("%s", buf);
    //send username to the server
    if ((send_val = send(s, buf, strlen(buf), 0 )) < 0){
        printf("Error sending username\n");
        exit(1);
    }

    bzero(buf, sizeof(buf));
    //receive password request prompt
    if ((rec_bytes = recv(s, buf, sizeof(buf), 0)) < 0){
        printf("Error receiving password prompt!\n");
        exit(1);
    }

    printf("%s\n", buf);
    bzero(buf, sizeof(buf));
    scanf("%s", buf);

    //send password to server
    if ((send_val = send(s, buf, strlen(buf), 0)) < 0){
        printf("Error sending password to server\n");
        exit(1);
    }

    short int success;
    //receive short int for success or not
    if ((rec_bytes = recv(s, &success, sizeof(short int), 0)) < 0){
        printf("Error receiving authentication success code!\n");
        exit(1);
    }

    success = ntohs(success);

    return success;
}

int create(int s_udp, struct sockaddr_in sin2){

    int rec_bytes; //number of bytes received
    int send_val; //send number of bytes
    char buf[MAX_BUFFER];   
    socklen_t len = sizeof(struct sockaddr_in); 
    
    bzero(buf, sizeof(buf));
    printf("What is the name of the board you would like to create?\n");
    scanf("%s", buf); 

    //send name of the board (string)
    if((send_val = sendto(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error with sending name of board to server\n");
        return -1;
    }

    bzero(buf, sizeof(buf));
    //receive string from server and print results
    if((rec_bytes = recvfrom(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, &len)) < 0){
        printf("Error with receiving string from server\n");
        return -1;
    }

    printf("%s", buf);
    return 0;
}

int message(int s_udp, struct sockaddr_in sin2){

    int rec_bytes; //number of bytes received
    int send_val; //send number of bytes
    char buf[MAX_BUFFER];
    socklen_t len = sizeof(struct sockaddr_in);
    char dummy_char; //soak in newline char

    bzero(buf, sizeof(buf));
    printf("What is the name of the board you would like to leave a message on\n");
    scanf("%s", buf);
    dummy_char = getc(stdin);
    //send name of the board
    if((send_val = sendto(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error with sending name of board to server\n");
        return -1;
    }

    bzero(buf, sizeof(buf));
    printf("What is the message you would like to send?\n");
    fgets(buf, sizeof(buf), stdin);
    
    //remove trailing newline
    char *pos;
    if ((pos = strchr(buf, '\n')) !=NULL){
        *pos = '\0';
    }

    //send message to server
    if((send_val = sendto(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error sending message to the server\n");
        return -1;
    }

    bzero(buf, sizeof(buf));
    //recieve confirmation string from server
    if((rec_bytes = recvfrom(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, &len)) < 0){
        printf("Error with receiving string from server\n");
        return -1;
    }
    
    printf("%s", buf);
    return 0;
}

int delete(int s_udp, struct sockaddr_in sin2){

    int rec_bytes; //number of bytes received
    int send_val; //send number of bytes
    char buf[MAX_BUFFER];
    socklen_t len = sizeof(struct sockaddr_in);

    bzero(buf, sizeof(buf));
    printf("What is the name of the board you would like to delete on?\n");
    scanf("%s", buf);

    //send name of the board
    if((send_val = sendto(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error with sending name of board to server\n");
        return -1;
    }

    int message_id;
    char term;
    printf("What is the message you would like to delete (ID number)?\n");
    if(scanf("%d%c", &message_id, &term) !=2 || term != '\n'){
        printf("Please make sure message ID is a valid integer\n");
        message_id = -1;
        if((send_val = sendto(s_udp, &message_id, sizeof(int),0, (struct sockaddr*)&sin2, len)) < 0){
            printf("Error with sending message id to the server\n");
            return -1;
        }
        return 0;
    } 

    //send message id (int) to server
    message_id = htonl(message_id);
   
    if((send_val = sendto(s_udp, &message_id, sizeof(int),0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error with sending message id to the server\n");
        return -1;
    }
  
    printf("number of bytes sent for message_id: %i\n", send_val);
     
    bzero(buf, sizeof(buf)); 
    //recieve confirmation string from server
    if((rec_bytes = recvfrom(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, &len)) < 0){
        printf("Error with receiving string from server\n");
        return -1;
    }

    printf("%s", buf);
    return 0;
}

int edit(int s_udp, struct sockaddr_in sin2){

    int rec_bytes; //number of bytes received
    int send_val; //send number of bytes
    char buf[MAX_BUFFER];
    socklen_t len = sizeof(struct sockaddr_in);
    char dummy_char; //soak in newline char

    bzero(buf, sizeof(buf));
    printf("What is the name of the board you would like to send?\n");
    scanf("%s", buf);
    dummy_char = getc(stdin);

    //send name of the board
    if((send_val = sendto(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error with sending name of board to server\n");
        return -1;
    }

    int message_id;
    char term;
    printf("What is the message you would like to edit (ID number)?\n");
    if(scanf("%d%c", &message_id, &term) !=2 || term != '\n'){
        printf("Please make sure message ID is a valid integer\n");
        message_id = -1;
        if((send_val = sendto(s_udp, &message_id, sizeof(int),0, (struct sockaddr*)&sin2, len)) < 0){
            printf("Error with sending message id to the server\n");
            return -1;
        }
        return 0;
    }
    //send message id (int) to server
    message_id = htonl(message_id);

    if((send_val = sendto(s_udp, &message_id, sizeof(int),0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error with sending message id to the server\n");
        return -1;
    }

    //send new message to be used
    bzero(buf, sizeof(buf));
    printf("What is the edited message you would like to send?\n");
    fgets(buf, sizeof(buf), stdin);

    //remove trailing newline
    char *pos;
    if ((pos = strchr(buf, '\n')) !=NULL){
        *pos = '\0';
    }

    if((send_val = sendto(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error sending message to the server\n");
        return -1;
    } 

    bzero(buf, sizeof(buf));
    //receive confirmation string and print it
    if((rec_bytes = recvfrom(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, &len)) < 0){
        printf("Error receiving confirmation!\n");
        return -1;
    } 
    printf("%s\n", buf);

    return 0;
}

int read_board(int s_udp, struct sockaddr_in sin2, int s_tcp){

    int rec_bytes; //number of bytes received
    int send_val; //send number of bytes
    char buf[MAX_BUFFER];
    socklen_t len = sizeof(struct sockaddr_in);

    bzero(buf, sizeof(buf));
    //ask user for name of board to read and send it
    printf("What is the message you would like to read?\n");
    scanf("%s", buf);
    
    if((send_val = sendto(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error with sending name of board to server\n");
        return -1;
    }

    int file_size;

    if((rec_bytes = recvfrom(s_udp, &file_size, sizeof(int), 0, (struct sockaddr*)&sin2, &len)) < 0){
       printf("Error with receiving file size from the server\n");
       return -1;
    }
   
    file_size = ntohl(file_size); 
    //if file_size is negative that means board doesnt exist
    if(file_size < 0){
        printf("Board does not exist on server\n");
        return 0;
    }
   
    //enter loop and starts receiving the contents of the board and
    //printing them until the total data received == the filesize  
    int total_rec = 0;
    int rec_count = 0;
    int rec_size;
    while(1){
    
        if(file_size - total_rec < MAX_BUFFER){
            rec_size = file_size - total_rec;
        } else {
            rec_size = sizeof(buf);
        }
        
        //receive the data
        bzero(buf, sizeof(buf));
        
        if((rec_bytes = recv(s_tcp, buf, rec_size, 0)) < 0){
            printf("Error receiving data\n");
            return -1;
        }
       
        //print out the data
        printf("%s\n", buf);
        rec_count++;       
        total_rec = total_rec + rec_bytes;
        
        if(total_rec >= file_size){
            break;
        }
    }

        return 0;               
}

int destroy_board(int s_udp, struct sockaddr_in sin2){
    
    int rec_bytes;
    int send_val;
    char buf[MAX_BUFFER];
    socklen_t len = sizeof(struct sockaddr_in);

    bzero(buf, sizeof(buf));
    printf("Please enter the name of the board you would like to destroy.\n");
    scanf("%s", buf);
  
    //send board name to the server 
    if((send_val = sendto(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error with sending the name of the board to the server\n");
        return -1;
    }  

    //receive message from server and print it out
    bzero(buf, sizeof(buf));
    if((rec_bytes = recvfrom(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, &len)) < 0){
        printf("Error with receiving message from the server\n");
        return -1;
    }

    printf("%s\n", buf);

}

int shut_down(int s_udp, struct sockaddr_in sin2, int s_tcp){
    
    int rec_bytes; //number of bytes received
    int send_val; //send number of bytes
    char buf[MAX_BUFFER];
    socklen_t len = sizeof(struct sockaddr_in);
    
    //prompts for admin pass and send to server  
    bzero(buf, sizeof(buf));
    printf("Please enter the admin password for the server\n");
    scanf("%s", buf);
    
    if((send_val = sendto(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error with sending admin password to server\n"); 
        return -1;
    }

    short int confirm;
    //recieve confirmation int from server to see what next steps to take
    //if -1 this means failed and go back to prompt for user operation

    if((rec_bytes = recvfrom(s_udp, &confirm, sizeof(short int), 0, (struct sockaddr*)&sin2, &len)) < 0){
        printf("Error with receiving confirmation from server\n");
        return -1;
    }
    
    bzero(buf, sizeof(buf));
    //recieve message from the server
    if((rec_bytes = recvfrom(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, &len)) < 0){
        printf("Error with receiving message from the server\n");
        return -1;
    }

    printf("%s\n", buf);

    if(confirm == -1){
        return 0;
    }

    if(confirm == 1){
        close(s_udp);
        close(s_tcp);
        exit(0);
    }
     
    return 0;   
} 

int append_board(int s_udp, struct sockaddr_in sin2, int s_tcp){
    
    int rec_bytes; //number of bytes received
    int send_val; //send number of bytes
    char buf[MAX_BUFFER];
    char file_name[MAX_BUFFER];
    socklen_t len = sizeof(struct sockaddr_in);   
    
    bzero(buf, sizeof(buf));
    
    printf("What is the name of the board you would like to append to?\n");
    scanf("%s\n", buf);
    
    //send name of the board to append to
    if((send_val = sendto(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error with sending name of board to server\n");
        return -1;
    }
    
    bzero(file_name, sizeof(file_name));
   
    //send name of file to use to append
    printf("What is the name of the file you would like to append?\n");
    scanf("%s\n", file_name);
    
    //send file name
    if((send_val = sendto(s_udp, file_name, sizeof(file_name), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error with sending name of board to server\n");
        return -1;
    }

    int confirm;
    //server checks if board exists and sends confirmation
    if((rec_bytes = recvfrom(s_udp, &confirm, sizeof(int), 0, (struct sockaddr*)&sin2, &len)) < 0){
        printf("Error with receiving confirm from the server\n");
        return -1;
    }

    confirm = ntohl(confirm);

    //if the board doesnt exist go back to the beginning
    if (confirm < 0){
        printf("Board does not exist, returning to main menu\n");
        return 0;
    }
    
    //find file and send size
    int file_size;
    FILE* fp;
    if ((fp = fopen(file_name, "r")) != NULL){
        //use fseek to get the file size
        fseek(fp,0,SEEK_END);
        file_size = ftell(fp);
        fseek(fp,0,SEEK_SET);
    } else{
        //file doesnt exist send error resp
        file_size = -1;
        printf("Error file doesn't exist\n");
    }
   
    //send file size to server (if negative that means file didnt exist)
    //and the server should go back to wait for client
    file_size = htonl(file_size);
    if((send_val = sendto(s_udp, &file_size, sizeof(int), 0, (struct sockaddr*)&sin2, len)) < 0){
        printf("Error sending file size to the server\n");
        return -1;
    }
    
    file_size = ntohl(file_size);
    
    if (file_size == -1){
        return 0;
    }
    
    //start sending the file
    //read file into a buffer, 4096 chars at a time       
}    
