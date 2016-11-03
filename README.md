# WebServer
 HTTP-based	web	server	that	handles	multiple	simultaneous	requests	from	users.

 Usage: ./webserver

 Compile: gcc -Wall -o webServer webServer.c

 Description: HTTP based web server. Server will parse ws.conf to configure. 
 Must have ws.conf to run. Server will parse HTTP packets and will respond
 accordingly. 'GET' is the only method implemented. Server will fork() a new
 process upon each request it gets.
