#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "file.h"
#define MAX_FILES 20
#define MAX_USERS 10

void intializeUserList(User *user_list);
void intializeFileList(File *file_list);
int ifFileExists(File *fl, char *fn);
int authenticateUser(char * un, char *pw, User *ul);   
void createFileEntry(File *file_list, char *fn);


int main(void)
{
    int listenfd = 0;
    int connfd = 0;
    struct sockaddr_in serv_addr;
    char sendBuff[1025];
    int numrv;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    printf("Socket retrieve success\n");

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5002 );

    bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));

    /*Intializing file list with server with random files for test purpose*/
    File *file_list;
    file_list = (File*)malloc(MAX_FILES*sizeof(File));
    intializeFileList(file_list);

    User *user_list;
    user_list = (User*)malloc(MAX_USERS*sizeof(User));
    intializeUserList(user_list);

    if(listen(listenfd, 10) == -1)
    {
        printf("Failed to listen\n");
        return -1;
    }

    while(1)
    {
      //  printf("hello\n");
        char msg[100];
        unsigned char offset_buffer[10] = {'\0'}; 
        unsigned char command_buffer[2] = {'\0'}; 
        int offset;
        int command, auth_res;
        connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL);

        do{
            strcpy(msg, "Enter User Name and Password.\n");
            send(connfd, msg, 100, 0);

            char user_name[10], password[10];
            recv(connfd, user_name, 10, 0); //receive user name
            //printf("user naem receive\n");
            recv(connfd, password, 10, 0);  //receive password
            //check if user name and password is correct
            //invalid user name: -1
            //wrong password: -2
            auth_res = authenticateUser(user_name, password, user_list);   
          //  printf("auth res: %d", auth_res);
            char auth_status[2];

            if(auth_res < 0){
                if(auth_res == -1)//user name
                    strcpy(auth_status, "1");
                else if(auth_res == -2)
                    strcpy(auth_status, "2");

            }

            //sprintf(auth_status, "%d", auth_res);
            send(connfd, auth_status, 2, 0);
        }while(auth_res < 0);


        //print files modified after his last login
        int num_mod = 0, i;
        int file_arr[10];

        for(i=0;i<user_list[auth_res].num_files;i++){
            if(difftime(user_list[auth_res].last_login, file_list[i].time_modified) < 0){
                file_arr[num_mod] = i; 
                num_mod++;
            }
        }

        user_list[auth_res].last_login = time(NULL);

        for(i=0;i<num_mod;i++){
            char f[20];
            strcpy(f, (user_list[auth_res].file_list[i])->file_name);
            send(connfd, f, 20, 0);
        }

        char temp[5];
        sprintf(temp, "%d", num_mod);

        send(connfd, temp, 5, 0);   //sending of files modified
        //record login time



        strcpy(msg, "Enter (1) to download a file, (2) to upload a file\n");
        send(connfd, msg, 100, 0);

        recv(connfd, command_buffer, 2, 0);
        sscanf(command_buffer, "%d", &command); 

        printf("Waiting for client to send the command (Download (1) Upload (2)\n"); 
        unsigned char buff[256]={0};

        if(command == 1){   //wants to download a file

            //add this file to client's data base
            strcpy(msg, "Enter name of file to be downloaded\n");
            send(connfd, msg, 100, 0);
            char file_name[20];
            recv(connfd, file_name, 20, 0);
            int file_index = ifFileExists(file_list, file_name);

            if(file_index != -1){
                //send file
                FILE *fp = fopen(file_name  ,"rb");
                if(fp==NULL)
                {
                    printf("File opern error");
                    return 1;   
                }   
                file_list[file_index].time_accessed = time(NULL);

        
                while(1)
                {
                    /* First read file in chunks of 256 bytes */
                    unsigned char buff[256]={0};
                    int nread = fread(buff,1,256,fp);
                    printf("Bytes read %d \n", nread);        

                    /* If read was success, send data. */
                    if(nread > 0)
                    {
                        printf("Sending \n");
                        write(connfd, buff, nread);
                    }

                    /*
                     * There is something tricky going on with read .. 
                     * Either there was error, or we reached end of file.
                     */
                    if (nread < 256)
                    {
                        if (feof(fp))
                            printf("End of file\n");
                        if (ferror(fp))
                            printf("Error reading\n");
                        break;
                    }


                }

            }
        }

        else if(command == 2){  //wants to upload a file
            //receive header
            //use full ftp to receive file from client
            char fn[20];
            recv(connfd, fn, 20, 0);
            FILE *fp;     
            int ind = ifFileExists(file_list, fn);
            fp = fopen(fn, "ab"); 
            if(NULL == fp)
            {
              printf("Error opening file");
                return 1;
            }
            if(ind == -1){  //file doesn't exist
                createFileEntry(file_list, fn);
            }

            else{
                file_list[ind].time_modified = time(NULL);
            }
        
            /* Receive data in chunks of 256 bytes */
            int bytesReceived;
            //char recvBuff[1024]
            while((bytesReceived = read(connfd, buff, 256)) > 0)
            {
                printf("Bytes received %d\n",bytesReceived);    
                // recvBuff[n] = 0;
                fwrite(buff, 1,bytesReceived,fp);
                // printf("%s \n", recvBuff);
            }

            if(bytesReceived < 0)
            {
                printf("\n Read Error \n");
            }
        }

        close(connfd);
        sleep(1);
        
    }


    return 0;
}

void intializeFileList(File * fl){
    strcpy(fl[0].file_name , "sample1.txt");
    strcpy(fl[1].file_name , "sample2.txt");
    strcpy(fl[2].file_name , "sample3.txt");

    //char time[25];
    //get_time(time);

    fl[0].time_created = time(NULL);
    fl[1].time_created = time(NULL);
    fl[2].time_created = time(NULL);

    fl[0].time_modified = time(NULL);
    fl[1].time_modified = time(NULL);
    fl[2].time_modified = time(NULL);

    fl[0].time_modified = time(NULL);
    fl[1].time_modified = time(NULL);
    fl[2].time_modified = time(NULL);

    num_file = 3;

}


void get_time(char *word){
    time_t curr;
    struct tm * curr_time_str;

    time (&curr);
    curr_time_str = localtime (&curr);
     
    strcpy(word,asctime(curr_time_str));
}


void intializeUserList(User *ul){
    strcpy(ul[0].user_name , "user1");
    strcpy(ul[1].user_name , "user2");
    strcpy(ul[2].user_name , "user3");

    strcpy(ul[0].password , "password1");
    strcpy(ul[1].password , "password2");
    strcpy(ul[2].password , "password3");

    ul[0].last_login = time(NULL);
    ul[1].last_login = time(NULL);
    ul[2].last_login = time(NULL);

    ul[0].num_files = 0;
    ul[1].num_files = 0;
    ul[2].num_files = 0;

    num_user = 3;
}

int authenticateUser(char *user_name, char *password, User *ul){
    //printf("authe\n");
    int i;
    for(i=0;i<num_user;i++){
      //  printf("hell\n");
        if(strcmp(ul[i].user_name, user_name) == 0){
            if(strcmp(ul[i].password, password) == 0)
                return i;//wrong password
            else
                return i;
        }
    }
    //num_user = 3;

    return -1;//no user
}

int ifFileExists(File *fl, char *fname){
    int i;
    for(i=0;i<num_file;i++){
        if(strcmp(fl[i].file_name, fname) == 0)
            return 1;
    }

    return 0;
}

void createFileEntry(File *fl, char *fn){
    strcpy(fl[num_file].file_name, fn);
    fl[num_file].time_modified = time(NULL);
    fl[num_file].time_created = time(NULL);
    fl[num_file].time_accessed = time(NULL);
    num_file++;
}