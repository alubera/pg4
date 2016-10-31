//Alan Vuong and Andrew Lubera
//netid: avuong and alubera
//TCP Client that will connect to our ftp server
//prompt the user to enter an operation
//send that operation to the server and carry out 
//the operation, displaying the result back
//to the user

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
#include <mhash.h>

int main(int argc, const char* argv[]){

    //check for valid input
    //ex.) ./myftp Server_name [port]
    if(argc != 3){
        printf("Error: Usage ./myftp <hostname> <portnumber>\n");    
        exit(1);
    }

    //initialize variables
    struct hostent *hp;
    struct sockaddr_in sin; //struct to hold info about sockets
    const char* host_name = argv[1];
    int port_num = atoi(argv[2]);
    int s; //socket return code    
    char buf[4096];
    struct sockaddr_in client_addr;      
    struct timeval begin, end, diff;
     
    //creating the socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Error creating the socket");
        exit(1);
    }

    //translate host name into peer's IP address
    hp = gethostbyname(host_name);
    if (!hp){
        fprintf(stderr, "simplex-talk unknown host: %s\n", host_name);
        exit(1);
    }        

    //set addess and port number
    memset((char*)&sin, 0, sizeof(sin)); //make sure to clear mem beforehand
    sin.sin_family = AF_INET;
    bcopy((char *)hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(port_num);

    //create connection
    if (connect(s, (struct sockaddr *) &sin, sizeof(sin)) < 0){
        printf("Error with connecting\n");
        exit(1);
    }


    char operation_buf[4]; //operation buf to send to server
    short int name_length;
    char name_length_str[4096];
    char file_name[4096];
    int send_val;
    int rec_bytes; //number of bytes received
    int file_size;
    socklen_t addr_len;
    int response;

    while(1){
        //Prompt user for operation
        printf("Hello, please enter an operation: REQ, UPL, DEL, LIS, MKD, RMD, CHD, XIT:\n");
        fflush(stdout);

        bzero(operation_buf, sizeof(operation_buf));
        scanf("%s",operation_buf);


        //send the buffer to the server
        if ((send_val = send(s, operation_buf, strlen(operation_buf),0)) < 0){
            printf("Error with write function");
            exit(1);
        }

        if (!strcmp(operation_buf, "REQ")){ 
            printf("What is the file name you want to request?\n");
            scanf("%s", file_name);
            name_length = strlen(file_name);

            //send the file name length
            name_length = htons(name_length);
            if ((send_val = send(s,&name_length,sizeof(short int),0)) < 0){
                printf("Error writing file length to the server\n");      
                exit(1);
            }
            
            //send the file name
            if ((send_val = send(s, file_name, strlen(file_name) ,0)) < 0){
                printf("Error writing file name to the server\n");
                exit(1);
            }

            addr_len = sizeof(client_addr);
   
            //recieve the 32-bit size, decode
            if((rec_bytes = recv(s,&file_size,sizeof(int),0)) < 0){
                printf("Error receiving!\n");
                exit(1);
            }
            file_size = ntohl(file_size);


            if(file_size == -1) {
                printf("File does not exist on the server\n");
                continue;
            }
           
            //open new file pointer and keep writing to the file
            FILE *f;
            f = fopen(file_name, "a");
            if (f == NULL){
                printf("Error opening the file\n");
            }
            //read "file size" bytes from the server
            //loop while reading the file and exit after last buffer
            //also compute the hash 

            char md5_hash_c[16];
            MHASH td;
            td = mhash_init(MHASH_MD5);
            if (td == MHASH_FAILED){
                printf("MHASH failed\n");
                exit(1);
            }
            int total_rec = 0;
            int rec_size = 0;
            gettimeofday(&begin,NULL);
            int rec_count = 0;
            int j;
            while(1){

                if(file_size - total_rec < 4096){
                    rec_size = file_size - total_rec;
                } else {
                    rec_size = sizeof(buf);
                }

                //receive the data
                bzero(buf, sizeof(buf));
                if((rec_bytes = recv(s, buf, rec_size, 0)) < 0){
                    printf("Error receiving file\n");
                    exit(1);
                }
                rec_count++;
                total_rec = total_rec + rec_bytes;
               
                mhash(td,buf,rec_size); 
                fwrite(buf, sizeof(char), rec_bytes, f);             

                if(total_rec >= file_size){
                    break;
                }
            }
            mhash_deinit(td, md5_hash_c); 
            gettimeofday(&end,NULL);
            timersub(&end, &begin, &diff);
            float throughput;
            //get throughput in MB
            //receive the hash of the file and save
            throughput = file_size*1.0/((int)diff.tv_sec*10^6+(int)diff.tv_usec);
            printf("%i\t%i\n",file_size,(int)diff.tv_sec*10^6+(int)diff.tv_usec);
            char md5_hash_s[16]; 
            if((rec_bytes = recv(s, md5_hash_s, sizeof(md5_hash_s), 0)) < 0){
                printf("Error receiving hash\n");
                exit(1);
            }

            fclose(f); 
           
            int i;
            //compare hashes
            for (i=0; i<16; i++){
                if(md5_hash_s[i] != md5_hash_c[i]){
                    printf("File transfer unsuccessful\n");
                    break;
                }
            }
    
            if(i == 16){
                printf("File transfer successful\n");
                printf("The throughput was %f megabytes per sec\n", throughput);
            }
        } else if (!strcmp(operation_buf,"UPL")) {
            char file_name[4096];
            short int file_length;
            printf("What is the file name you would like to upload?\n");
            scanf("%s", file_name);
            file_length = strlen(file_name);
            file_length = htons(file_length);
            MHASH td;
            int file_size;

            //send length of the file name
            if ((send_val = send(s, &file_length, sizeof(short int), 0)) < 0){
                printf("Error sending file name length\n");
                exit(1);
            }

            //send file name
            if ((send_val = send(s, file_name, strlen(file_name), 0)) < 0){
                printf("Error sending file name\n");
                exit(1);
            }
            short int ack = 0;

            //receive 1-short int for acknowledgement
            if ((rec_bytes = recv(s, &ack, sizeof(short int), 0)) <0){
                printf("Error receiving!\n");
                exit(1);
            }

            ack = ntohs(ack);

            if (ack != 1){
                printf("Server did not send ACK...going back to main menu\n");
                continue;    
            }
            FILE* fp;
            //find file and send size
            if ((fp = fopen(file_name, "r")) != NULL){
                //use fseek to get the file size
                fseek(fp,0,SEEK_END);
                file_size = ftell(fp);
                fseek(fp,0,SEEK_SET);
            } else{
                //file doesnt exist send error resp
                file_size = -1;
                printf("Error file doesn't exist\n");
                continue;
            }

            file_size = htonl(file_size);
            if ((send_val = send(s, &file_size, sizeof(int),0)) == -1){
                fprintf(stderr, "ERROR: send error\n");
                exit(1);
            } 

            file_size = ntohl(file_size);
            if (file_size == -1) continue;
            
            char hash[16];
            //init mhash
            if ((td = mhash_init(MHASH_MD5)) == MHASH_FAILED) {
                fprintf(stderr,"ERROR: unable to initialize MHASH\n");
                exit(1);
            }

            //read file into buffer, 4096 chars at a time
            //send each buffer as well as adding to MD5 hash

            int count = 0;
            int read;
            memset((char*)&buf,0,sizeof(buf));
            while ((read = fread(buf, sizeof(char), sizeof(buf), fp)) > 0){
                if ((send_val = send(s, buf, read, 0)) == -1){
                    fprintf(stderr,"ERROR: send error\n");
                    exit(1);
                }

                mhash(td,buf,read);
                memset((char*)&buf,0,sizeof(buf));
            }

            //compute md5 hash string and send
            mhash_deinit(td,hash);
            if ((send_val = send(s, hash,sizeof(hash), 0)) == -1){
                fprintf(stderr,"ERROR: send error\n");
                exit(1);
           }

            char throughput_rec[4096];
            memset((char*)&throughput_rec,0,sizeof(throughput_rec));
            //recieve throughput and output -1 will be
            if ((rec_bytes = recv(s, &throughput_rec, sizeof(throughput_rec) , 0)) < 0){
                printf("Throughput received failed\n");
                exit(1);
            }
            
            if (!strcmp(throughput_rec,"-1")){
                printf("Unsuccessful upload\n");
                continue;
            } else{
                printf("Successful upload\n");
                printf("Throughput was %s megabytes per second\n", throughput_rec);
            } 
              
            fclose(fp);

        } else if (!strcmp(operation_buf,"DEL")) {

            short int file_length;
            char file_name[4096];
            
            printf("What is the file name you would like to delete?\n");
            scanf("%s", file_name);

            file_length = strlen(file_name);
            file_length = htons(file_length);
            //send file name length
            if ((send_val = send(s, &file_length, sizeof(short int), 0)) <0){
                printf("Error sending file name length\n");
                exit(1);
            }

            //send file name
            if ((send_val = send(s, file_name, sizeof(file_name), 0)) < 0){ 
                printf("Error sending file name\n");
                exit(1);
            }

            int confirm;
            //receive confirm from server
            if ((rec_bytes = recv(s, &confirm, sizeof(int), 0)) < 0){
                printf("Error receiving confirm from server\n");
                exit(1);
            }
            
            confirm = ntohl(confirm);
            if (confirm == -1){
                printf("File doesn't exist\n");
                continue;
            } else{
                while(1){
                    char delete_confirm[5];
                    printf("Are you sure you want to delete this file? Yes or No?\n");
                    scanf("%s", delete_confirm);
                    if(!strcmp(delete_confirm, "Yes")){
                        if((send_val = send(s, "Yes", 3, 0)) < 0){
                            printf("Error sending to server\n");
                            exit(1);
                        }
                        bzero(buf, sizeof(buf));
                        if ((rec_bytes = recv(s, buf, sizeof(buf), 0)) < 0){
                            printf("Error receiving!\n");
                            exit(1);
                        }
                        printf("%s", buf);
                        break;
                    } else if (!strcmp(delete_confirm, "No")){
                        printf("Delete abandoned by the user!\n");
                        if((send_val = send(s, "No", 2, 0)) < 0){
                            printf("Error sending to the server\n");
                            exit(1);
                        }
                        break;
                }else{
                    printf("You did not enter a valid response, try again\n");
                }
            }
        }

        } else if (!strcmp(operation_buf,"LIS")){

            int resp2;

            //recieve int and char array
            if ((rec_bytes = recv(s, &resp2, sizeof(int), 0)) < 0){   
                printf("Error receiving\n");
                exit(1);
            }
           
            resp2 = ntohl(resp2);
            bzero(buf, sizeof(buf));
            if ((rec_bytes = recv(s, buf, resp2, 0)) < 0){
                printf("Error receiving\n");
                exit(1);
            }     

            printf("%s\n", buf);

        } else if (!strcmp(operation_buf,"MKD")) {

            char directory_name[4096];
            short int dir_length;
            char dir_length_str[4096];

            printf("What is the directory name you would like to create? ");
            scanf("%s", directory_name);
            dir_length = strlen(directory_name);

            dir_length = htons(dir_length);        
            //send the directory name length
            if ((send_val = send(s, &dir_length, sizeof(short int) ,0)) < 0){
                printf("Error writing file length to the server\n");
                exit(1);
            }

            //send the directory name
            if ((send_val = send(s, directory_name, strlen(directory_name) ,0)) < 0){
                printf("Error writing the file length to the server\n");
                exit(1);
            }

            //receive confirm from server
            if ((rec_bytes = recv(s, &response, sizeof(int), 0)) < 0){
                printf("Error receiving!\n");
                exit(1);
            }
            response = ntohl(response);

            //check confirms for return message
            if(response == -2){
                printf("The directory already exists on server!\n");
            } else if (response == -1){
                printf("Error in making directory\n");
            } else {
                printf("The directory was successfully made\n");
            }

        } else if (!strcmp(operation_buf,"RMD")) {
            char directory_name[4096];
            short int dir_length;
            char dir_length_str[4096];

            printf("What is the directory name you would like to remove?\n");
            scanf("%s", directory_name);
            dir_length = strlen(directory_name);
            dir_length = htons(dir_length);
        
            //send the directory name length
            if ((send_val = send(s, &dir_length, sizeof(short int) ,0)) < 0){
                printf("Error writing file length to the server\n");
                exit(1);
            }

            //send the directory name
            if ((send_val = send(s, directory_name, strlen(directory_name) ,0)) < 0){
                printf("Error writing the file length to the server\n");
                exit(1);
            }
            int rec_bytes; //number of bytes received
            bzero(buf, sizeof(buf));
            int resp; // the server's confirm
            //receive confirm from server
            if ((rec_bytes = recv(s, &resp, sizeof(int), 0)) < 0){
                printf("Error receiving!\n");
                exit(1);
            }
       
            resp = ntohl(resp);
            char confirm[5];
            //check confirms for return message
            if(resp == -1){
                printf("The directory does not exist on server!\n");
                continue;
            } else {
                while(1){
                    printf("Are you sure you want to delete the directory? Yes or No? \n");
                    scanf("%s", confirm);
                    if(!strcmp(confirm, "Yes")){
                        //send stuff to server wait display info 
                        if((send_val = send(s, "Yes", 3, 0)) < 0){
                            printf("Error sending to server\n");
                            exit(1);
                        }
                        bzero(buf, sizeof(buf));
                        if ((rec_bytes = recv(s, buf, sizeof(buf), 0)) < 0){
                            printf("Error receiving!\n");
                            exit(1); 
                        }
                        //print out the delete confirmation msg from server
                        printf("%s", buf);
                        break; 

                    } else if (!strcmp(confirm, "No")){
                        printf("Delete abandoned by the user!\n");
                        if((send_val = send(s, "No", 2, 0)) < 0){
                            printf("Error sending to the server\n");
                            exit(1);
                        }
                        break;
                    } else{
                        printf("You did not enter a valid response, try again\n");
                    }
                }
            } 

        } else if (!strcmp(operation_buf,"CHD")) {

            char directory_name[4096];
            short int dir_length;
           
            printf("What is the directory you would like to change to?\n");
            scanf("%s", directory_name);
            dir_length = strlen(directory_name);
            dir_length = htons(dir_length); 
            //send the directory name length
            if ((send_val = send(s, &dir_length, sizeof(short int) ,0)) < 0){
                printf("Error writing file length to the server\n");
                exit(1);
            }
           
            //send the directory name 
            if ((send_val = send(s, directory_name, strlen(directory_name) ,0)) < 0){
                printf("Error writing the file length to the server\n");
                exit(1);
            }  

            int resp1;
            if ((rec_bytes = recv(s, &resp1, sizeof(int), 0)) < 0){
                printf("Error receiving!\n");
                exit(1);
            }

            resp1 = ntohl(resp1);

            //confirms from the server
            if(resp1 == -2){
                printf("The directory does not exist on the server!\n");
            } else if (resp1 == -1){
                printf("Error in changing directory\n");
            } else {
                printf("The directory was successfully transfered to\n");
            }            

        } else if (!strcmp(operation_buf,"XIT")) {
            close(s);
            printf("Session has been closed, have a nice day.\n");
            break;
        } else {
            printf("Command not recognized, try again\n");
        }
    }

    return 0;
}
