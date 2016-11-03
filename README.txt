/********************************************
 * CS4273 Network Systems					*
 * Assignment 2 HTTP Web Server				*
 * Name: Justin Barnitz						*
 * Date: 10/18/2016 						*
 *******************************************/

 Usage: ./webserver

 Compile: gcc -Wall -o webServer webServer.c

 Description: HTTP based web server. Server will parse ws.conf to configure. 
 Must have ws.conf to run. Server will parse HTTP packets and will respond
 accordingly. 'GET' is the only method implemented. Server will fork() a new
 process upon each request it gets.