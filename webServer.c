/********************************************
 * CS4273 Network Systems					*
 * Assignment 2 HTTP Based Web Server		*
 * Name: Justin Barnitz						*
 * Date: 10/18/2016 						*
 * Usage: ./webserver						*
 * HTTP based web server that handles 		*
 * multiple simultaneous requests.			*
 * Server will parse ws.conf for initial 	*
 * config. Will parse headers and server	*
 *******************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFSIZE 4096
#define MAXLINE 4096 /*max text line length*/
#define LISTENQ 8 /*maximum number of client connections*/
#define NUMFILETYPES 9
#define MAXURI 255

struct Config
{
	int port;
	int numWebPage;
	int numFileTypes;
	char root[BUFSIZE];
	char webPage[10][BUFSIZE];
	char fileTypes[NUMFILETYPES][BUFSIZE];
	char contentType[NUMFILETYPES][BUFSIZE];
} config;

int checkFileType(char *filetype, char *contentType)
{
	int code;
	//check that server can handle that file type
	for (int i = 0; i < NUMFILETYPES; ++i)
	{
		if( strncmp( (config.fileTypes[i] + 1), filetype, 3) )
		{
			// set the code to 501 not implemented
			code = -1;
		}
		else 
		{
			// set the code to 200
			code = 1;
			strcpy(contentType,(config.contentType[i]));
			break;
		}
	}
	return code;
}

int checkVersion(char *version)
{
	int retval = 1;
	// Check HTTP version
	if( (!strncmp(version, "HTTP/1.1", 8)) && (!strncmp(version, "HTTP/1.0", 8)) )
	{
		retval = -1;
	}
	return retval;
}

int checkMethod(char *method)
{
	int retval;

	if(!strncmp(method, "GET", 3))
	{

		retval = 1;
	}
	else if(!strncmp(method, "POST", 4) || !strncmp(method, "HEAD", 4) || !strncmp(method, "DELETE", 6) || !strncmp(method, "OPTIONS", 7))
	{
		//return a 501
		retval = -1;
	}
	else
		retval = -2;
	return retval;
}

char *getFileExt(char *filename)
{
	char *fileExt = strrchr(filename, '.');
	if(!fileExt || fileExt == filename) 
		return "";

	return fileExt + 1;
}

void initConfig()
{
	char buf[BUFSIZE];
	FILE *fp;
	int i, j;
	//open fileopen just for reading
	if( ( fp = fopen("ws.conf", "r") ) == NULL ) 
	{
		printf("File ws.conf not found!\n");
		exit(2);
	}

	//fgets will parse to '\n' or EOF
	while(fgets(buf, sizeof(buf), fp) != NULL)
	{
		if(!strncmp( buf, "Listen", 6 ))
		{
			char temp[5];
			strncpy(temp, (buf + 7), 4);
			config.port = atoi(temp);
		}
		else if(!strncmp( buf, "DocumentRoot", 12 ))
		{
			strncpy(config.root, (buf + 14), BUFSIZE);
			// Use strcspn to remove newline character and " from string
			config.root[strcspn(config.root, "\n")] = 0;
			config.root[strcspn(config.root, "\"")] = 0;
		}
		else if(!strncmp( buf, "DirectoryIndex", 14 ))
		{
			char *tmp;
			i = 0;
			tmp = strtok( (buf + 15)," ");		
			while(tmp != NULL)
			{
				strncpy(config.webPage[i], tmp, BUFSIZE);
				config.webPage[i][strcspn(config.webPage[i], "\n")] = 0;
				tmp = strtok(NULL, " ");
				i++;
			}
			config.numWebPage = i;
		}
		else if(!strncmp( buf, ".", 1 ))
		{
			char *tmp;
			tmp = strtok(buf, " ");
			//printf("token = %s", token);
			strncpy(config.fileTypes[j], tmp, BUFSIZE);
			config.fileTypes[j][strcspn(config.fileTypes[j], "\n")] = 0;
			while(tmp != NULL)
			{
				strncpy(config.contentType[j], tmp, BUFSIZE);
				config.contentType[j][strcspn(config.contentType[j], "\n")] = 0;
				tmp = strtok(NULL, " \n");
			}
			j++;
			config.numFileTypes++;
		}
	}
	fclose(fp);
}

void handleRequest(char * request, int size, int connfd)
{
	char *tmp;
	char *line;
	char *fileBuf;
	char message[BUFSIZE];
	char requestLine[3][BUFSIZE];
	char path[MAXURI];
	char filetype[BUFSIZE];
	char version[40];
	char nameAndPath[MAXURI];
	char contentType[BUFSIZE];
	char append[10];

	size_t nbytes;

	int i = 0;
	// code is a 8 bit value each bit represents an error or its ok (128)
	int code = 0;
	int filesize;
	FILE *fp;

	//get the first line of the request
	tmp = strtok(request, "\n");

	//parse the line
	line = strtok(tmp, " ");
	while(line != NULL)
	{
		strncpy(requestLine[i], line, BUFSIZE);
		requestLine[i][BUFSIZE - 1] = 0;
		line = strtok(NULL, " ");
		i++;
	}

	strncpy(path, requestLine[1], MAXURI);
	//handle default '/' index.html
	if(!strncmp(path, "/\0", 2))
	{
		strcpy(path, "/index.html");
	}

	//check for default webpage
	if( (!strncmp(requestLine[1], "/index.", 7)) && (strlen(requestLine[1]) < 260) )
	{
		for(i = 0; i < config.numWebPage; i++)
		{
			//add 1 to skip over '/'
			if(!strncmp((requestLine[1] + 1), config.webPage[i], 9))
			{
				strcpy(path, "/index.html");
			}
		}
	}
	strcpy(filetype, getFileExt(path));
	
	/*====================================================
	=            Start Checking for Errors               =
	====================================================*/

	// 501 filetype

	if(checkFileType(filetype, contentType) < 1)
	{
		printf("%s 	501	Not	Implemented\n", filetype);
		code = 32;
	}
	
	// 400 version
	if(checkVersion(requestLine[2]) < 1)
	{
		printf("Error 400: Unsupported HTTP type => %s\n", requestLine[2]);
		code = 4;
	}

	// 501 OR 400 Method
	int meth = checkMethod(requestLine[0]);
	if(meth == -1)
	{
		code =  16;
	}
	else if (meth == -2)
	{
		code = 1;
	}

	// 400 invalid URL
	if(strlen(path) > 260)
	{
		code = 2;
	}
	else if(code == 0)
	{
		// build absolute filepath
		strncpy(nameAndPath, config.root, MAXURI);
		strncpy(&nameAndPath[strlen(config.root)], path, strlen(path));

		// 404 check if file exisits
		if( (fp = fopen(nameAndPath,"rb") ) == NULL )
		{
			printf("Error 404 Could not open file at %s\n", nameAndPath);
			code = 8;
		}
		else
		{
			fseek(fp, 0, SEEK_END);
			filesize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
		}
	}

	//create a version string
	if( !strncmp(requestLine[2], "HTTP/1.1", 8) )
	{
		strcpy(version, "HTTP/1.1");
	}
	else if(!strncmp(requestLine[2], "HTTP/1.0", 8)) 
	{
		strcpy(version, "HTTP/1.0");
	}
/*==========================================================
=            Create Header based on error codes            =
==========================================================*/
	//printf("\n_____CODE = %d_________\n", code);
	// OK message
	if( code == 0) // 200 OK
	{
		memset(message, 0, strlen(message));
		strcpy(message, version);
		strcat(message, " 200 OK\n");
		//content-Type
		strcat(message, "Content-Type: ");
		strcat(message, contentType);
		strcat(message, "\nContent-Length:");
		snprintf(append,sizeof(append),"%d\n",filesize);
		strcat(message, append);
		printf("Message 200 = *\n%s*\n", message);

		fileBuf = malloc(filesize);
		//send header
		nbytes = send(connfd, message, BUFSIZE, 0);
		send(connfd, "Connection: keep-alive\n\n", 24, 0);
		//send file
		printf("nbytes from message = %d\n",(int)nbytes);
		while((nbytes = fread(fileBuf, 1, filesize, fp)) > 0)
		{
			write(connfd, fileBuf, (int)nbytes);
			printf("200 nbytes = %d\n", (int)nbytes);
		}
		send(connfd, "\n", 2, 0);
		printf("\n-----------------Done sending 200 OK message----------------\n\n");
	}
	else if( code == 8) // 404 Not Found
	{
		memset(message, 0, strlen(message));
		strcpy(message, version);
		strcat(message, " 404 Not Found\n\n");
		strcat(message, "<html><body>404 NOT Found Reason URL does not exist: ");
		strcat(message, path);
		strcat(message, "</body></html>");
		strcat(message, "\n\n");
		printf("Message 404 = \n*\n%s*\n", message);
		nbytes = send(connfd, message, BUFSIZE, 0);
		printf("404 nbytes = %d\n", (int)nbytes);
		printf("\n-----------------Done sending 404 message----------------\n\n");
	}
	else if( code == 1 ) // 400 Invalid Method
	{
		memset(message, 0, strlen(message));
		strcpy(message, version);
		strcat(message, " 400 Bad Request\n\n");
		strcat(message, "<html><body>400 Bad Request Reason: Invalid Method: ");
		strcat(message, requestLine[0]);
		strcat(message, "</body></html>");
		strcat(message, "\n\n");
		printf("Message 400 Invalid Method = \n*\n%s*\n", message);
		nbytes = send(connfd, message, BUFSIZE, 0);
		printf("400 nbytes = %d\n", (int)nbytes);
		printf("\n-----------------Done sending 400 Invalid Method message----------------\n\n");
	}
	else if( code == 2) // 400 Invalid URL
	{
		memset(message, 0, strlen(message));
		strcpy(message, version);
		strcat(message, " 400 Bad Request\n\n");
		strcat(message, "<html><body>400 Bad Request Reason: Invalid URL: ");
		strcat(message, path);
		strcat(message, "</body></html>");
		strcat(message, "\n\n");
		printf("Message 400 Invalid URL = \n*\n%s*\n", message);
		nbytes = send(connfd, message, BUFSIZE, 0);
		printf("400 nbytes = %d\n", (int)nbytes);
		printf("\n-----------------Done sending 400 Invalid URL message----------------\n\n");
	}
	else if( code == 4)
	{
		memset(message, 0, strlen(message));
		strcpy(message, version);
		strcat(message, " 400 Bad Request\n\n");
		strcat(message, "<html><body>400 Bad Request Reason: Invalid Version: ");
		strcat(message, requestLine[2]);
		strcat(message, "</body></html>");
		strcat(message, "\n\n");
		printf("Message 400 Invalid Version = \n*\n%s*\n", message);
		nbytes = send(connfd, message, BUFSIZE, 0);
		printf("400 nbytes = %d\n", (int)nbytes);
		printf("\n-----------------Done sending 400 Invalid Version message----------------\n\n");
	}
	else if( code == 16 )
	{
		memset(message, 0, strlen(message));
		strcpy(message, version);
		strcat(message, "501 Not Implemented\n\n");
		strcat(message, "<html><body>501 Not Implemented Method: ");
		strcat(message, requestLine[0]);
		strcat(message, "</body></html>");
		strcat(message, "\n\n");
		printf("Message 501 Not Implemented Method = \n*\n%s*\n", message);
		nbytes = send(connfd, message, BUFSIZE, 0);
		printf("501 nbytes = %d\n", (int)nbytes);
		printf("\n-----------------Done sending 501 Not Implemented Method message----------------\n\n");
	}
	else if( code == 32)
	{
		memset(message, 0, strlen(message));
		strcpy(message, version);
		strcat(message, "501 Not Implemented\n\n");
		strcat(message, "<html><body>501 Not Implemented File Type: ");
		strcat(message, filetype);
		strcat(message, "</body></html>");
		strcat(message, "\n\n");
		printf("Message 501 Not Implemented File Type = \n*\n%s*\n", message);
		nbytes = send(connfd, message, BUFSIZE, 0);
		printf("501 nbytes = %d\n", (int)nbytes);
		printf("\n-----------------Done sending 501 Not Implemented File Type message----------------\n\n");
	}
	else
	{
		memset(message, 0, strlen(message));
		strcpy(message, version);
		strcat(message, "500 Internal Server Error\n\n");
		strcat(message, "<html><body>500 Internal Server Error</body></html>");
		strcat(message, "\n\n");
		printf("Message 500 Internal Server Error = \n*\n%s*\n", message);
		nbytes = send(connfd, message, BUFSIZE, 0);
		printf("500 nbytes = %d\n", (int)nbytes);
		printf("\n-----------------Done sending 500 Internal Server Error message----------------\n\n");
	}	

	//printf("closing connfd\n");
	shutdown(connfd, SHUT_RDWR);
	close(connfd);
}



int main()
{
	int listenfd, connfd, n;

	pid_t childPID;
	socklen_t clientLen;
	char buf[MAXLINE];
	struct sockaddr_in clientAddr, serverAddr;

	//Parse ws.conf file
	initConfig();

	//Create a socket for the soclet
 	//If sockfd<0 there was an error in the creation of the socket
 	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) 
 	{
  		perror("Problem in creating the socket\n");
  		exit(2);
 	}

 	if( (int)config.port <= 1024)
 	{
 		printf("Port Number: %d, is too low. Must be greater than 1024.\n", config.port);
 		return -1;
 	}

 	//preparation of the socket address
 	//memset((char *) &serverAddr, 0, sizeof(serverAddr));
 	serverAddr.sin_family = AF_INET;
 	serverAddr.sin_addr.s_addr = INADDR_ANY;
 	serverAddr.sin_port = htons((int)config.port);

 	//bind the socket
 	if (bind (listenfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
 	{
 		printf("Problem in binding socket\n");
 		return -1;
 	}

 	//listen to the socket by creating a connection queue, then wait for clients
 	if (listen (listenfd, LISTENQ) < 0)
 	{
 		printf("Problem in listening!\n");
 		return -1;
 	}

 	printf("%s\n","Server running...waiting for connections.");

 	while(1)
 	{
 		clientLen = sizeof(clientAddr);

 		//accept a connection
 		connfd = accept(listenfd, (struct sockaddr *)&clientAddr, &clientLen);

 		printf("%s\n","Received request...");

 		//Spawn a child process. If PID == 0 its the child
 		if ( (childPID = fork ()) == 0 ) 
 		{
 			printf ("%s\n","Child created for dealing with client requests");

 			//close the listening socket
 			close(listenfd);

 			while( (n = recv(connfd, buf, MAXLINE, 0)) > 0)
 			{
 				printf("%s","\n------String received from and resent to the client:------\n\n");
 				puts(buf);
 				printf("-----------------------------------------------------------\n\n");
 				handleRequest(buf, MAXLINE, connfd);
 			}
 			printf("child is exiting\n\n");
 			exit(0);
 		}
 		//close socket on server
 		close(connfd);
 	}

}