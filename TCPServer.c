/******************************
*		徐健
*	  SA14226136
*******************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define    MAXLINE         1024*16   /*ip packet size*/
#define    SERVER_PORT     8000  
#define    OFFSET         1024*1024*128      /*offset in file pointer */
#define    FILE_NAME_MAX_SIZE 512 

void fileTransfer(void *arg);    /*function that every thread execute*/

static  int  threadCount = 0;  //public resources used to count thread created
pthread_mutex_t alock = PTHREAD_MUTEX_INITIALIZER;  //thread lock
char  file_name[FILE_NAME_MAX_SIZE];   

int main(int argc,char **argv) 
{
    struct sockaddr_in    serv_addr;
    struct sockaddr_in    clie_addr;
    int                   sock_id;
    int                   clie_addr_len;
    pthread_t             thread_id;
    int                   *link_id; 

    if ((sock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("Create socket failed\n");
        exit(0);
    }
    /*initialize server address*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /*bind socket with server address*/
    if (bind(sock_id, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    {
        perror("Bind socket failed\n");
        exit(0);
    }
    if (-1 == listen(sock_id, 10)) 
    {
        perror("Listen socket failed\n");
        exit(0);
    }
    /*waiting for connect request*/
    while (1) 
    {
        clie_addr_len = sizeof(clie_addr);
	link_id = (int *)malloc(sizeof(int));
	/*create link socket when connect request arrived*/
        *link_id = accept(sock_id, (struct sockaddr *)&clie_addr, &clie_addr_len);  
    	if (-1 == *link_id) 
	{
            perror("Accept socket failed\n");
            continue;
        }
	/*create thread to deal with every connect request*/
	while (pthread_create (&thread_id, NULL, (void *)fileTransfer, (void *)link_id) != 0)   
        {    
             printf ("create thread error: %s.\n\n", strerror(errno));
	     break;          
        }
    }

    printf("file %s finished transfer!\n ", file_name);
    close(sock_id); 
    return 0;
}


void fileTransfer(void *arg)
{ 
	int                    *link_id;
        FILE                   *fp;
   	int                    read_len;
   	int                    send_len;
    	int                    recv_len;
        char                   buf[MAXLINE];
	int                    threadIndex;
 //  int                    blockCount = 0;

	link_id = (int *)malloc(sizeof(int));
	*link_id = *((int*)arg);

	/*action on public on resources*/
	pthread_mutex_lock(&alock);
	threadCount++;
	threadIndex = threadCount;
	pthread_mutex_unlock(&alock);

	/*receive filename to transfer*/
	if((recv_len = recv(*link_id, file_name, FILE_NAME_MAX_SIZE, 0)) == -1) 
	{
		printf("recv file name error, error = %d\n", errno);
		exit(1);
	}
	/*open file to read*/
	if ((fp = fopen(file_name, "r")) == NULL)
	{
		perror("Open file failed\n");
		exit(0);
	}
	/*positon the read-pointer*/
	fseek(fp, OFFSET*(threadIndex-1), 0);

	/*send thread index to client*/
	sprintf(buf, "%d", threadIndex);
	if((send_len = send(*link_id, buf, sizeof(buf), 0)) < sizeof(buf))
	{
		perror("Send thread_id failed\n");
		exit(0);
	}
	
	bzero(buf, MAXLINE);
	while ((read_len = fread(buf, sizeof(char), MAXLINE, fp)) >0)  /*read content to buf from file */
	{
		send_len = send(*link_id, buf, read_len, 0);   /*send buf content*/
		if ( send_len < read_len ) 
		{
			perror("Send file failed\n");
			exit(0);
		}
		//blockCount++;	 
		//printf("--thread index = %d, len = %d,link_id = %d, block id = %d\n",  threadIndex, send_len, *link_id, blockCount);

	        bzero(buf, MAXLINE);
		if(ftell(fp) >= OFFSET*threadIndex)    /*when read-pointer arrive the boundary, exit and terminated the thread*/
			break;
	}

        fclose(fp);
        close(*link_id);
	pthread_exit(NULL);
}

