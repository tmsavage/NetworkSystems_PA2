/*
 * tcpechosrv.c - A concurrent TCP echo server using threads
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXLINE  8192 /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */


int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
char *get_filename_ext(char *filename);

int main(int argc, char **argv)
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while (1) {
      // printf("CREATING A NEW SOCKET WOOOOOOOHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH");
	    connfdp = malloc(sizeof(int));
	    *connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, (socklen_t *)&clientlen);
	    pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void * thread(void * vargp)
{
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}

/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    // This is what we need to edit?
    char httpmsg[MAXBUF];

    n = read(connfd, buf, MAXLINE);
    printf("server received the following request:\n%s\n",buf);

    char* reqMethod = strtok(buf, " ");
    char* reqURL = strtok(NULL, " ");
    char* reqVersion = strtok(NULL, "\r");

    char *filename = malloc(100);
    strcat(filename ,"./www");
    strcat(filename ,reqURL);

    printf("File path: %s\n", filename);


    // strcat(httpmsg, reqVersion);

    printf("REQ VERSION: %s \n", reqVersion);
    printf("ReqURL: %s\n",reqURL);
    char* temp = strtok(reqURL, ".");
    char* fileType = get_filename_ext(filename);
    printf("FILETYPE: %s\n",fileType);
    char* fileTypeUpdated = fileType;

    if(strcmp(fileType,"html") == 0){
      fileTypeUpdated = "text/html";
    }
    else if(strcmp(fileType,"txt") == 0){
      fileTypeUpdated = "text/plain";
    }
    else if(strcmp(fileType,"png") == 0){
      fileTypeUpdated = "image/png";
    }
    else if(strcmp(fileType,"gif") == 0){
      fileTypeUpdated = "image/gif";
    }
    else if(strcmp(fileType,"jpg") == 0){
      fileTypeUpdated = "image/jpg";
    }
    else if(strcmp(fileType,"css") == 0){
      fileTypeUpdated = "text/css";
    }
    else if(strcmp(fileType,"js") == 0){
      fileTypeUpdated = "application/javascript";
    }
    // else{
    //   printf("FAILED IDENTIFYING FILE TYPE!");
    //   exit(1);
    // }


    size_t totalBytes = 0;
    char buf2[MAXLINE];
    char fileLength[10];
    FILE *fp;
    // printf("HERE IS THE FILE: %s\n",filename);
    size_t bytesRead;
    if(strcmp(filename,"./www/")==0){
      printf("FILENAME EMPTY");
      filename = "./www/index.html";
    }
    fp = fopen(filename,"rb");
    if(fp != NULL){
        // printf("FILE WAS FOUND!\n\r");
        while( ((bytesRead = fread(buf2,sizeof(char),5000,fp)) > 0)){
            // printf("%s",buf2);
            strcat(httpmsg, reqVersion);
            strcat(httpmsg, " 200 ");
            strcat(httpmsg, " Document Flows \r\n");
            strcat(httpmsg, "Content-Type: ");
            strcat(httpmsg, fileTypeUpdated);
            strcat(httpmsg, "\r\nContent-Length: ");
            sprintf(fileLength,"%zu",bytesRead);
            strcat(httpmsg,fileLength);
            strcat(httpmsg, "\r\n\r\n");
            // strcat(httpmsg, "File Contents: ");
            strcat(httpmsg, buf2);
            // printf("%s", buf2);
            //file length

            // printf("%s\n",httpmsg);
            write(connfd, httpmsg,strlen(httpmsg));
            // write(connfd, httpmsg,strlen(httpmsg));
            totalBytes += bytesRead;
            bzero(buf2,MAXLINE);
            bzero(httpmsg,MAXBUF);
            // bytesRead = 0;
        }
        // printf("Total bytes: %zu",totalBytes);
    }
    else{
        strcat(httpmsg, reqVersion);
        strcat(httpmsg, " 500 ");
        strcat(httpmsg, " Internal Server Error \r\n");
        write(connfd, httpmsg,strlen(httpmsg));
        printf("FILE WAS NOT FOUND!\n");
    }
    fclose(fp);
    if(strcmp(reqVersion, "HTTP/1.0" ) == 0){
      close(connfd);
    }
    // return totalBytes;


}

/*
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure
 */
int open_listenfd(int port)
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */

/* Credit To:
* https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
* For help getting the extension.
*/
char *get_filename_ext(char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}
