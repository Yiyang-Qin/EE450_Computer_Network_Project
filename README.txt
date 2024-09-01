Name: Yiyang Qin
ID:6927580246

What have been done: 
all tasks mentioned in this project, including the optional (extra credit) part.

Code file:
serverM.c: implement serverM, including reading from member.txt, checking login information, TCP connection with client, UDP 
	connection with backend server and processing client request and inventory admin request.
	TCP port: 45246	UDP port: 44246 
serverH.c: implement serverH, including reading from history.txt, UDP connection with serverM, sending availability and inventory 
	status of given book code to serverM. 
	UDP port: 43246
serverL.c: implement serverL, including reading from literature.txt, UDP connection with serverM, sending availability and inventory 
	status of given book code to serverM. 
	UDP port: 42246
serverS.c: implement serverS, including reading from science.txt, UDP connection with serverM, sending availability and inventory 
	status of given book code to serverM. 
	UDP port: 41246
client.c: implement client, including login information input and forwarding, book code input and forwarding, showing message to the client.
	TCP port: dynamically assigned.

Message exchange: 
username and password message exchange: client.c will first send length of input username in the form of int, then send the input username
	to the main server in the form of char array over TCP. The same procedure applies to the input password. The username and the 
	passwords sent by the client are in the encrypted form.
book code: in the TCP connection between client and serverM, as well as the UDP connection between serverM and backend server, book code
	are all exchanged in the form of char array, but for TCP connection, client will first send the length of book code(int), and then send
	the book code(char*), for the UDP connection, only book code will be sent.

Reused code:
For the TCP/UDP socket creating and connection part, I used the code from Beej's Guide. No code from anywhere else is used.
The three backend server file serverH.c, severS.c and serverL.c are almost the same, except for the UDP port, server name and out put message, 
	so comment are only made in the serverS.c file.