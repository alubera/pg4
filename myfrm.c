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
        scanf("%s", operation_buf);

        //send the operation buffer to the server

        if ((send_val = sendto(s_udp, operation_buf, strlen(operation_buf), 0, (struct sockaddr*)&sin2, len)) < 0){
            printf("Error with sending operation to server\n");
            exit(1);
        }

        //carry out the function
        if(!strcmp(operation_buf, "CRT")){
            

            printf("What is the name of the board you would like to send?\n");
            scanf("%s", buf);
            
            //send name of the board (string) 
            if((send_val = sendto(s_udp, buf, sizeof(buf), 0, (struct sockaddr*)&sin2, len)) < 0){
                printf("Error with sending name of board to server\n");
                exit(1);
            }
            
            //receive string from server and print results
            
            

        } else if (!strcmp(operation_buf, "LIS")){

            

        } else if (!strcmp(operation_buf, "MSG")){
        
           //send name of board (string)
           
           //send message (string)
           
           //receive string from server


        } else if (!strcmp(operation_buf, "DLT")){

            //send name of board

            //send message id ( short int) to server

            //receive string from server and print

        } else if (!strcmp(operation_buf, "RDB")){ 

            //send name
            //receive int for file size tcp
            //loop use upl req style            


        } else if (!strcmp(operation_buf, "EDT")){

            //send name
            //send message id
            //receive string frm server and print

        } else if (!strcmp(operation_buf, "APN")){

            //send name append to
            //send name to be appended
            //receive +1 or -1 
            //+ means good it passed


        } else if (!strcmp(operation_buf, "DWN")){

            //receive file size or negative

        } else if (!strcmp(operation_buf, "DST")){

        } else if (!strcmp(operation_buf, "XIT")){
            //close sockets
            return 0;

        } else if (!strcmp(operation_buf, "SHT")){
           //udp receive 1 or -1  
        }
    }

}

//use TCP for this s
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
    printf("%i", sizeof(buf));
    //print out request for username
    printf("%s\n received bytes:%i\n", buf, rec_bytes);
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

    printf("password size %i", send_val);
    printf("%s\n", buf);

    short int success;
    //receive short int for success or not
    if ((rec_bytes = recv(s, &success, sizeof(short int), 0)) < 0){
        printf("Error receiving authentication success code!\n");
        exit(1);
    }

    success = ntohs(success);

    return success;
}

             
