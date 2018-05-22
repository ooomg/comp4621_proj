/*
This is a very simple HTTP server. Default port is 10000 and ROOT for the server is your current working directory..

You can provide command line arguments like:- $./a.aout -p [port] -r [path]

for ex.
$./a.out -p 50000 -r /home/
to start a server at port 50000 with root directory as "/home"

$./a.out -r /home/shadyabhi
starts the server at port 10000 with ROOT as /home/shadyabhi

*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include<pthread.h>
#include "zipg.c"

#define CONNMAX 1000
#define BYTES 1024
#define MAXTHREAD 5

char *ROOT;
int listenfd, clients[CONNMAX];
void error(char *);
void startServer(char *);
void* respond(int);

//Possible media types
struct {
    char *ext;
    char *filetype;
} extensions[] ={
 {"gif", "image/gif" },
 {"txt", "text/plain" },
 {"jpg", "image/jpg" },
 {"jpeg","image/jpeg"},
 {"png", "image/png" },
 {"ico", "image/ico" },
 {"zip", "image/zip" },
 {"gz",  "image/gz"  },
 {"tar", "image/tar" },
 {"htm", "text/html" },
 {"html","text/html" },
 {"php", "text/html" },
 {"pdf","application/pdf"},
 //{"pptx","application/pptx"},
 {"zip","application/octet-stream"},
 {"rar","application/octet-stream"},
 {0,0} };



int main(int argc, char* argv[])
{

	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char c;

	//Default Values PATH = ~/ and PORT=10000
	char PORT[6];
	ROOT = getenv("PWD");
	strcpy(PORT,"10000");

	int slot=0;

	//Parsing the command line arguments
	while ((c = getopt (argc, argv, "p:r:")) != -1)
		switch (c)
		{
			case 'r':
				ROOT = malloc(strlen(optarg));
				strcpy(ROOT,optarg);
				break;
			case 'p':
				strcpy(PORT,optarg);
				break;
			case '?':
				fprintf(stderr,"Wrong arguments given!!!\n");
				exit(1);
			default:
				exit(1);
		}

	printf("Server started at port no. %s%s%s with root directory as %s%s%s\n","\033[92m",PORT,"\033[0m","\033[92m",ROOT,"\033[0m");
	// Setting all elements to -1: signifies there is no client connected
	int i;
	for (i=0; i<CONNMAX; i++)
		clients[i]=-1;
	startServer(PORT);


  int threads_count = 0;
  pthread_t threads[MAXTHREAD];
  char ip_str[INET_ADDRSTRLEN] = {0};

	// ACCEPT connections
	while (1)
	{
		addrlen = sizeof(clientaddr);
		clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

		if (clients[slot]<0)
			error ("accept() error");
		else
		{
			if ( fork()==0 )
			{
				// respond(slot);
				// exit(0);/* print client (remote side) address (IP : port) */
                inet_ntop(AF_INET, &(clientaddr.sin_addr), ip_str, INET_ADDRSTRLEN);
                printf("Incoming connection from %s : %hu with fd: %d\n", ip_str, ntohs(clientaddr.sin_port), clients[slot]);

            		/* create dedicate thread to process the request */
            		if (pthread_create(&threads[threads_count], NULL, respond(slot), (void *)clients[slot]) != 0) {
            			printf("Error when creating thread %d\n", threads_count);
            			return 0;
            		}

            		if (++threads_count >= MAXTHREAD) {
            			break;
            		}
			}
		}

		while (clients[slot]!=-1) slot = (slot+1)%CONNMAX;
	}

	return 0;
}

//start server
void startServer(char *port)
{
	struct addrinfo hints, *res, *p;

	// getaddrinfo for host
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &hints, &res) != 0)
	{
		perror ("getaddrinfo() error");
		exit(1);
	}
	// socket and bind
	for (p = res; p!=NULL; p=p->ai_next)
	{
		listenfd = socket (p->ai_family, p->ai_socktype, 0);
		if (listenfd == -1) continue;
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
	}
	if (p==NULL)
	{
		perror ("socket() or bind()");
		exit(1);
	}

	freeaddrinfo(res);

	// listen for incoming connections
	if ( listen (listenfd, 1000000) != 0 )
	{
		perror("listen() error");
		exit(1);
	}
}

//client connection
void* respond(int n)
{
  //int n; n=(int)args;
	char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999];
	int rcvd, fd, bytes_read;

	memset( (void*)mesg, (int)'\0', 99999 );

	rcvd=recv(clients[n], mesg, 99999, 0);

	if (rcvd<0)    // receive error
		fprintf(stderr,("recv() error\n"));
	else if (rcvd==0)    // receive socket closed
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	else    // message received
	{
		printf("%s", mesg);
		reqline[0] = strtok (mesg, " \t\n");
		if ( strncmp(reqline[0], "GET\0", 4)==0 )
		{
			reqline[1] = strtok (NULL, " \t");
			reqline[2] = strtok (NULL, " \t\n");
			if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
			{
				write(clients[n], "HTTP/1.0 400 Bad Request\n", 25);
			}
			else
			{
				if ( strncmp(reqline[1], "/\0", 2)==0 )
					reqline[1] = "/index.html";        //Because if no file is specified, index.html will be opened by default (like it happens in APACHE...

				strcpy(path, ROOT);
				strcpy(&path[strlen(ROOT)], reqline[1]);
				printf("file: %s\n", path);


        char* finame; finame=path;
        char *pathptr;
        int l = 0;
        pathptr = strstr(finame, "/");
        do{
          l = strlen(pathptr) + 1;
          finame = &finame[strlen(finame)-l+2];
          pathptr = strstr(finame, "/");
        }while(pathptr);

        printf("-----------finame: %s\n", finame);

        int found;
        char openfi[99999];
        int isgzip; isgzip=1;
        if (isgzip==1){
          //if ( (strcmp(finame,".gif")>=0) ) {
          //  strcpy(openfi,path);
          //}else{
            found=zipg(finame);
            strcpy(openfi,"test.gz");
            //if ( (strcmp(finame,".gif")>=0) ) {
            //    strcpy(openfi,path);}
          //}
        }else if (isgzip==0){
          strcpy(openfi,path);
        }

        if(found!=-1){
				if ( ((fd=open(openfi, O_RDONLY))!=-1) )    //FILE FOUND
				{
          printf("-----path :%s\n",path );
          if((isgzip==1)){
            // if ( (strcmp(finame,".gif")>=0)) {
            //    send(clients[n], "HTTP/1.0 200 OK\r\n", 17, 0);
            //    send(clients[n], "Transfer-Encoding: chunked\n\n", 28, 0);
            // }else{
                write(clients[n], "HTTP/1.1 200 OK\n", 16);
  					    write(clients[n], "Content-Encoding: gzip\r\n", 24);
                write(clients[n], "Transfer-Encoding: chunked\r\n", 28);
                write(clients[n], "\r\n", 2);
            //}
          }else if ((isgzip==0)){
                write(clients[n], "HTTP/1.1 200 OK\n", 16);
                write(clients[n], "Transfer-Encoding: chunked\r\n", 28);
                write(clients[n], "\r\n", 2);
          }
          char hexbyte_read[9999];
					while ( (bytes_read=read(fd, data_to_send, BYTES))>0 ){
            sprintf(hexbyte_read, "%x", bytes_read);
            write (clients[n], hexbyte_read, strlen(hexbyte_read));
            write (clients[n], "\r\n", 2);
            write (clients[n], data_to_send, bytes_read);
            write (clients[n], "\r\n",2);
            // if ( (strcmp(finame,".gif")>=0)) {
               //sprintf(hexbyte_read, "%x", bytes_read);
               //write (clients[n], hexbyte_read, strlen(hexbyte_read));
            // }
						// write (clients[n], data_to_send, bytes_read);
            //write (clients[n], "0\r\n",3);
            //write (clients[n], "\r\n",2);
          }
          // write (clients[n], "a\r\n",3);
          // write (clients[n], "This is \r\n",10);
          // write (clients[n], "a\r\n",3);
          // write (clients[n], "the end \r\n",10);
          // write (clients[n], "d\r\n",3);
          // write (clients[n], "of chunked \r\n",13);
          // write (clients[n], "5\r\n",3);
          // write (clients[n], "data.\r\n",5);

           write (clients[n], "0\r\n",3);
           write (clients[n], "\r\n",2);
				}
				else{
            write(clients[n], "HTTP/1.1 404 Not Found\n", 23); //FILE NOT FOUND
            fd=open("notfound.html", O_RDONLY);
            //write(clients[n], "Transfer-Encoding: chunked\r\n", 28);
            //write(clients[n], "\r\n", 2);
            //char hexbyte_read[9999];
  					while ( (bytes_read=read(fd, data_to_send, BYTES))>0 ){
              //sprintf(hexbyte_read, "%x", bytes_read);
              //write (clients[n], hexbyte_read, strlen(hexbyte_read));
              //write (clients[n], "\r\n", 2);
              write (clients[n], data_to_send, bytes_read);
              //write (clients[n], "\r\n",2);
            }
        }
      }else{
        write(clients[n], "HTTP/1.1 404 Not Found\n", 23); //FILE NOT FOUND
        fd=open("notfound.html", O_RDONLY);
        //write(clients[n], "Transfer-Encoding: chunked\r\n", 28);
        //write(clients[n], "\r\n", 2);
        //char hexbyte_read[9999];
        while ( (bytes_read=read(fd, data_to_send, BYTES))>0 ){
          //sprintf(hexbyte_read, "%x", bytes_read);
          //write (clients[n], hexbyte_read, strlen(hexbyte_read));
          //write (clients[n], "\r\n", 2);
          write (clients[n], data_to_send, bytes_read);
          //write (clients[n], "\r\n",2);
        }
      }
      }
		}
	}

	//Closing SOCKET
	shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
	close(clients[n]);
	clients[n]=-1;
}
