#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

/*void get_time(char *word){


time_t curr;
struct tm * curr_time_str;

time (&curr);
curr_time_str = localtime (&curr);
 
strcpy(word,asctime(curr_time_str));

//printf("%s",word);

} */



int main(void)
{   int authen=0;                     //flag variable to allow user authentication
    int a;
    int sockfd = 0;                   //for eastablishing the connection 
    int bytesReceived = 0;            //for allowing files to be sent/uploaded to server 
    char recvBuff[256];               //for allowing files to be received from server
    char buffer[1024];	              // general buffer to carry operations
    char username[10];                // client's/user's name can be at most 20 characters long, last char '\0'. Used for authentication
    char pwd[10];                     // client's password needed for authentication 
    char filename[20];                // names of file max 20 chars, last char '\0'
    unsigned char buff_offset[10];   // buffer to send the File offset value
    unsigned char buff_command[2];   // buffer to send the Complete File (0) or Partial File Command (1). 
    int offset;                      // required to get the user input for offset in case of partial file command
     int command;                     // required to get the user input for command
    memset(recvBuff, '0', sizeof(recvBuff));
    struct sockaddr_in serv_addr;

    // Create a socket first 
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    // Initialize sockaddr_in data structure 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5002); // port
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Attempt a connection 
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }

//for user authentication
     do{
     recv(sockfd, buffer, 1024, 0);       //Receive "enter username and password." from server       
     printf("%s\n",buffer);	          //Printing above thing.
     scanf("%s",username);                //getting username from client
    // username[9]='\0';
     strcpy(buffer,username);              
     send(sockfd, buffer, 10, 0 );        //send username to server                                
     scanf("%s",pwd);                     //getting password from client
     //pwd[9]='\0';
    // memset(buffer, '\0', sizeof(buffer));
     strcpy(buffer,pwd);
     send(sockfd,buffer,10,0);            //send password to server
     recv(sockfd,buffer,1024,0);          // receive response from server telling whether username, pwd combination correct or not
     
     if(buffer[0]=='1'){
     printf("Invalid username\n");       //invalid username
     }
     else if(buffer[0]=='2'){
     printf("wrong password.Enter username and password again\n");}           //valid username but wrong password.
     else authen=1;                      // valid username & pwd, login successsful , authen flag changed to 1

}while(authen==0);                       //comes out of this loop once login is sucessfull.



/* Following code gives information to each client about the number and names of files which have been modified since last login 
 by this particular user to maintain complete synchronization */


recv(sockfd,buffer,100, 0);            // server sending clients about number of files modified since last login,client receiving this number
int b=buffer[0];
printf("numbers of files modified since last login= %d",b);           //printing the number of such files
int i;
for(i=0;i<b;i++){                                                     
	recv(sockfd,buffer,20, 0);                         //receive names one by one & print sequentially
	buffer[20]='\0';
	printf("Names of files modified since last login:%s\n",buffer);          //printimg names of files modified since last login
}


/* Following code gives user the choice to upload(push) or download(pull) any particular file from the server 
  1)  if user/client chooses option 1 => he can download the file from server and make modifications.
  2)  if user/client chooses option 2 => he can upload the file on server after making modifications.
*/
while(1){ 
	recv(sockfd,buffer,100,0);           // getting "ENETR 1 TO DOWNLOAD & 2 TO UPLOAD" from server  
	printf("%s\n",buffer);               // Printing the above statement
	scanf("%d",&a);                      // Getting the choice from the user.
	buffer[0]=a;
	buffer[1]='\0';
	send(sockfd,buffer,2,0);             // sending user's choice to server

// Downloading file from server
	if(a==1)                             
		{ recv(sockfd,buffer,100,0);    // getting "Enter name of file to download " from server
		  printf("%s\n",buffer);        //printig the above statement
	          scanf("%s",filename);     // scanning file from user
                  filename[19]='\0';     
                  strcpy(buffer,filename);
		  send(sockfd,buffer,21,0);           //sending filename from client to server for downloading purpose
		  

       /* Create file where data will be stored */
        FILE *fp;     
        fp = fopen("destination_file.txt", "ab"); 
        if(NULL == fp)
        {
          printf("Error opening file");
            return 1;
        }
        
    /* Receive data in chunks of 256 bytes */
    while((bytesReceived = read(sockfd, recvBuff, 256)) > 0)
    {
        printf("Bytes received %d\n",bytesReceived);    
        // recvBuff[n] = 0;
        fwrite(recvBuff, 1,bytesReceived,fp);
        // printf("%s \n", recvBuff);
    }

    if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
    }
//modify user database

}

/* Following code is meant for uploading files from client to server. 

*/
else if(a==2){
        
 /* Open the file that we wish to transfer */
        printf("Enter the name of the file in current directory that you wish to upload/send to server");
        scanf("%s",filename);               //scanning filename to upload on server  
        filename[20]='\0';
        strcpy(buffer,filename);
        send(sockfd,buffer,21,0);           //sending filename to server
        FILE *fp = fopen("source_file.txt","rb");
        if(fp==NULL)
        {
            printf("File opern error");
            return 1;   
        }   

        
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
                write(sockfd, buff, nread);
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

//modify header


        }



}                            

else printf("Invalid option chosen.");

}
        
    /* Create file where data will be stored 
        FILE *fp;     
        fp = fopen("destination_file.txt", "ab"); 
        if(NULL == fp)
        {
          printf("Error opening file");
            return 1;
        }
        fseek(fp, 0, SEEK_END);
        offset = ftell(fp);
        fclose(fp);
        fp = fopen("destination_file.txt", "ab"); 
        if(NULL == fp)
        {
          printf("Error opening file");
            return 1;
        }
    
    printf("Enter (0) to get complete file, (1) to specify offset, (2) calculate the offset value from local file\n");
    scanf("%d", &command);
    sprintf(buff_command, "%d", command);
    write(sockfd, buff_command, 2);
    
    
   
    if(command == 1 || command == 2)   // We need to specify the offset
    {
    
        if(command == 1)  // get the offset from the user
        {
        printf("Enter the value of File offset\n");
        scanf("%d", &offset);
        }
        // otherwise offset = size of local partial file, that we have already calculated
        sprintf(buff_offset, "%d", offset);
        /* sending the value of file offset */
        //write(sockfd, buff_offset, 10);
    
    
    // Else { command = 0 then no need to send the value of offset }
    
    
    /* Receive data in chunks of 256 bytes 
    while((bytesReceived = read(sockfd, recvBuff, 256)) > 0)
    {
        printf("Bytes received %d\n",bytesReceived);    
        // recvBuff[n] = 0;
        fwrite(recvBuff, 1,bytesReceived,fp);
        // printf("%s \n", recvBuff);
    }

   /* if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
    }

*/
    return 0;
}
