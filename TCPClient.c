/******************************
*		徐健
*	  SA14226136
*******************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define    MAXLINE        1024*16  /*ip packet size*/
#define    SERVER_PORT    8000    
#define    OFFSET         1024*1024*128      /*offset in file pointer */
#define    FILE_NAME_MAX_SIZE 512    
#define    THREAD_NUM       8             /*number of thread*/

char  file_name[FILE_NAME_MAX_SIZE];  /*filename on server*/
void* connectToServer(void* arg);   /*thread function*/

int main(int argc,char **argv)
{
    struct sockaddr_in     serv_addr;
    time_t                 t_start,t_end;
    pthread_t              tempthread_id[THREAD_NUM]; 
    int                    i; 

    /*initialize server address*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

     /*input file name on server*/
     bzero(file_name, FILE_NAME_MAX_SIZE);    
     printf("Please Input File Name On Server: ");    
     scanf("%s", file_name);  

     /* start count time*/
     t_start=time(NULL);   
     for (i=0; i<THREAD_NUM; i++) 
     {   
          
         while (pthread_create (&tempthread_id[i], NULL, (void *)connectToServer, (void *)&serv_addr) != 0)     //create pthread
         {    
               printf ("create thread error: %s.\n\n", strerror(errno));          
         } 
     } 

    /*wait for pthread to exit*/
    for (i=0; i<THREAD_NUM; i++)  
    {
	pthread_join(tempthread_id[i], NULL);
	printf(" thread id = %d exit!\n", tempthread_id[i]);
    } 

    /*finish count time*/
    t_end=time(NULL);  
    printf("共用时%.0fs\n",difftime(t_end,t_start));
    printf("File:%s Transfer Successful!\n", file_name); 
    return 0;
}


void *connectToServer(void *arg)
{
    char                   buf[MAXLINE];  /*received file buffer*/
    int                    sock_id;
    int                    recv_len;
    int                    write_len;
    int                    send_len;
    FILE                   *fp;
    int                    i_ret;
    int                    threadIndex;         /*server thread index to position write-pointer*/
    char                   client_file_name[FILE_NAME_MAX_SIZE] = "file";   //received filename 
    //int             	   blockCount = 0;        
 
    if((sock_id = socket (AF_INET, SOCK_STREAM, 0)) == 0)  
    {   
        printf ("obtain a socket error: %s.\n\n", strerror(errno));  
    }   

     /* open received file*/   
    fp = fopen(client_file_name, "w");    
    if(NULL == fp)    
    {      
	printf("File:\t%s Can Not Open To Write\n", client_file_name);       
	exit(1);    
    } 
    /*connect request*/
    i_ret = connect(sock_id, (struct sockaddr*)arg, sizeof(struct sockaddr));
    if (-1 == i_ret) 
    {
        printf("Connect socket failed\n");
	return NULL;
    }
    /*send file name to server*/
    if((send_len = send(sock_id, file_name, sizeof(file_name), 0)) < sizeof(file_name))
    {
	printf("send file name error\n");
	exit(1); 
    }
    /*receive threadIndex from server*/
    if(recv(sock_id, buf, MAXLINE, 0) < 0)  
    {
	printf("thread id = %d, recv thread_id error\n", pthread_self());
	exit(1); 
    }

    threadIndex = atoi(buf);
    /*position file write-pointer*/
    fseek(fp, OFFSET*(threadIndex-1), 0);

    bzero(buf, MAXLINE);
    while (recv_len = recv(sock_id, buf, MAXLINE, 0)) 
    {   
        if(recv_len <= 0) 
	{
            printf("Recieve Data From Server Failed!\n");
            break;
        }

	//blockCount++; 
       // printf("----server thread index = %d, recv_len = %d, block id = %d\n", threadIndex, recv_len, blockCount);

        write_len = fwrite(buf, sizeof(char), recv_len, fp);    /*write buffer to file*/
        if (write_len < recv_len ) 
	{
            printf("Write file failed\n");
            break;
        }
        bzero(buf, MAXLINE);
    }
     
    fclose(fp);
    close(sock_id);
    pthread_exit(NULL);
}

