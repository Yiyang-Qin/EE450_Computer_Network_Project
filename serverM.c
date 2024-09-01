/*
** listener.c -- a datagram sockets "server" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXBOOKSIZE 100
#define MYPORT_TCP "45246" //TCP port of serverM
#define MYPORT_UDP "44246" //UDP port of serverM
#define HPORT "43246"//UDP port of serverH
#define LPORT "42246"//UDP port of serverL
#define SPORT "41246"//UDP port of serverS
#define MAXBUFLEN 100 //maximum lenth of the buffer
#define BACKLOG 10   // how many pending connections queue will hold
#define MAXDATASIZE 100 // max number of bytes we can get at once
#define SERVERIP "localhost"

struct member
/* This is a structure used to hold the member username and password in member.txt*/
{
    char username[MAXBOOKSIZE][100];
    char password[MAXBOOKSIZE][100];
    int member_count;
};

/* The following two functions are from code in Beej’s Guide*/
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

in_port_t get_in_port(struct sockaddr *sa)
/* Get the port number of client socket. */
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in*)sa)->sin_port);

    return (((struct sockaddr_in6*)sa)->sin6_port);
}

void login(int new_fd, const struct member *memberlist, int* priority){
    /* Process the login process.
        INPUT: 
            new_fd: socket descriptor of the TCP socket;
            memberlist: username and password loaded from member.txt;
            priority: admin right indicator, changed to 1 if admin right detected.
        MODIFIES: priority
        RETURN: The function runs recursively and won't return until correct login information is reverived from client. 
        */
    char buf[MAXBUFLEN];
    char client_username[100];
    char client_password[100];
    int numbytes;

    int recv_len = 0;
    /* 
        client.c will first send length of input username in the form of int, then send the input username
	    to the main server in the form of char array over TCP. The same procedure applies to the input password.
        */
    if(recv(new_fd, &recv_len, sizeof recv_len, 0) == -1){
        perror("Error in recv recv_len\n");
        exit(1);
    }
    if ((numbytes = recv(new_fd, buf, recv_len, 0)) == -1){
        perror("receive username");
        exit(1);
    }
    buf[numbytes] = '\0';
    strcpy(client_username ,buf);
    printf("%s\n", client_username);

    if(recv(new_fd, &recv_len, sizeof recv_len, 0) == -1){
        perror("Error in recv recv_len\n");
        exit(1);
    }
    if ((numbytes = recv(new_fd, buf, recv_len, 0)) == -1){
        perror("receive password");
        exit(1);
    }
    buf[numbytes] = '\0';
    strcpy(client_password ,buf);
    printf("%s\n", client_password);

    printf("Main Server received the username and password from the client using TCP over port %s.\n", MYPORT_TCP);

    //printf("Username: %s\n", client_username);
    //printf("Password: %s\n", client_password);

    int username_found = 0;
    for(int i = 0; i < memberlist->member_count; i++){
        if(strcmp(memberlist->username[i], client_username) == 0){
            if(strcmp(memberlist->password[i], client_password) == 0){
                printf("Password %s matches the username. Send a reply to the client.\n", client_password);
                if((strcmp(client_username, "firns") == 0) && (strcmp(client_password, "Firns") == 0)){
                    /* chech admin right */
                    *priority = 1;
                    /* Smatch means username and password match with admin right */
                    if(send(new_fd, "Smatch", 6, 0) == -1){
                        perror("send password match message");
                        exit(1);
                    }   
                }
                else{
                    /* Pmatch means username and password match */
                    if(send(new_fd, "Pmatch", 6, 0) == -1){
                        perror("send password match message");
                        exit(1);
                    }   
                }
            }
            else{
                printf("Password %s does not match the username. Send a reply to the client.\n", client_password);
                /* Umatch means username match and password not match */
                if(send(new_fd, "Umatch", 6, 0) == -1){
                    perror("send password not match message");
                    exit(1);
                }
                login(new_fd, memberlist, priority);
            }
            username_found = 1;
        }
    }

    if(!username_found){
        printf("%s is not registered. Send a reply to the client.\n", client_username);
        /* Nmatch means username not match */
        if(send(new_fd, "Nmatch", 6, 0) == -1){
            perror("send username not match message");
            exit(1);
        }
        login(new_fd, memberlist, priority);
    }
}

int backend_connect(char servername, char* book_code, int priority){
    /* Connect with backend server using UDP
        INPUT:
            servername: 'S' means connect to serverS; 'L' means connect to serverL; 'H' means connect to serverH;
        RETURN: book availability sent by the bakend server*/
    int sockfd_udp;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    char serverport[100];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;

    switch (servername)
    {
    case 'S':
        for(int i = 0; i < strlen(SPORT); i++){
            serverport[i] = SPORT[i];
        }
        serverport[strlen(SPORT)] = '\0';
        break;

    case 'H':
        for(int i = 0; i < strlen(HPORT); i++){
            serverport[i] = HPORT[i];
        }
        serverport[strlen(HPORT)] = '\0';
        break;

    case 'L':
        for(int i = 0; i < strlen(LPORT); i++){
            serverport[i] = LPORT[i];
        }
        serverport[strlen(LPORT)] = '\0';
        break;
    
    default:
        break;
    }

    // following part for UDP socket creating and connection are from code in Beej's Guide
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(SERVERIP, serverport, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd_udp = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        exit(1);
    }

    printf("Found %s located at Server%c. Send to server%c.\n", book_code, servername, servername);

    /* book code start with X means admin right inventory status check*/
    if(priority == 1){
        book_code[0] = 'X';
    }

    if ((numbytes = sendto(sockfd_udp, book_code, strlen(book_code), 0,
            p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    close(sockfd_udp);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT_UDP, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd_udp = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd_udp, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd_udp);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        exit(1);
    }

    freeaddrinfo(servinfo);

    int availability = 0;
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd_udp, &availability, sizeof(availability) , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    printf("Main Server received from server%c the book status result using UDP over port %s.\n",servername, MYPORT_UDP);

    close(sockfd_udp);

    return availability;
}

void process_request(int new_fd, int priority){
    /* Receive book code input by client and print check result.*/
    char buf[MAXBUFLEN];
    char book_code[100];
    int recv_len = 0;
    int numbytes = 0;

    /* client will first send the length of input book code(int) and then send the book code(char*)*/
    if(recv(new_fd, &recv_len, sizeof recv_len, 0) == -1){
        perror("Error in recv recv_len\n");
        exit(1);
    }
    if ((numbytes = recv(new_fd, buf, recv_len, 0)) == -1){
        perror("receive username");
        exit(1);
    }
    buf[numbytes] = '\0';
    strcpy(book_code ,buf);
    //printf("%s\n", book_code);
    printf("Main Server received the book request from client using TCP over port %s.\n", MYPORT_TCP);

    int check_result = 0;
    if(book_code[0] == 'S' || book_code[0] == 'H' || book_code[0] == 'L'){
        check_result = backend_connect(book_code[0], book_code, priority);
    }
    else{
        check_result = -2;
        printf("Did not found %s in the book code list.\n", book_code);
    }

    if(send(new_fd, &check_result, sizeof check_result, 0) == -1){
        perror("send check result");
        exit(1);
    }
    printf("Main Server sent the book status to the client.\n");
}

int main(void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    // The following commented out code are for deleted phase 1 part
 
    // Data structure used to store the backend book statuses
    /*
    int store_status = 3;
    char S_book_name[MAXBOOKSIZE][100];
    int S_book_count[MAXBOOKSIZE];
    char L_book_name[MAXBOOKSIZE][100];
    int L_book_count[MAXBOOKSIZE];
    char H_book_name[MAXBOOKSIZE][100];
    int H_book_count[MAXBOOKSIZE];
    int S_idx = 0, L_idx = 0, H_idx = 0;
    

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    */

    printf("Main Server is up and running\n");
    /*
    addr_len = sizeof their_addr;
    while(store_status != 0) {
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        else {
            if(buf[0] == 'S'){
                strcpy(S_book_name[S_idx], buf);
                if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                    (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                    perror("recvfrom");
                    exit(1);
                }
                else{
                    buf[numbytes] = '\0';
                    int number;
                    sscanf(buf, "%d", &number);
                    S_book_count[S_idx] = number;
                    S_idx ++;
                }
            }
            else if(buf[0] == 'L'){
                strcpy(L_book_name[L_idx], buf);
                if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                    (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                    perror("recvfrom");
                    exit(1);
                }
                else{
                    buf[numbytes] = '\0';
                    int number;
                    sscanf(buf, "%d", &number);
                    L_book_count[L_idx] = number;
                    L_idx ++;
                }
            }
            else if(buf[0] == 'H'){
                strcpy(H_book_name[H_idx], buf);
                if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                    (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                    perror("recvfrom");
                    exit(1);
                }
                else{
                    buf[numbytes] = '\0';
                    int number;
                    sscanf(buf, "%d", &number);
                    H_book_count[H_idx] = number;
                    H_idx ++;
                }
            }
            else if(buf[0]=='e' && buf[1]=='n' && buf[2]=='d' && buf[3]=='S'){
                store_status--;
                printf("Main Server received the book code list from serverS using UDP over port %s.\n", MYPORT);
            }
            else if(buf[0]=='e' && buf[1]=='n' && buf[2]=='d' && buf[3]=='L'){
                store_status--;
                printf("Main Server received the book code list from serverL using UDP over port %s.\n", MYPORT);
            }
            else if(buf[0]=='e' && buf[1]=='n' && buf[2]=='d' && buf[3]=='H'){
                store_status--;
                printf("Main Server received the book code list from serverH using UDP over port %s.\n", MYPORT);
            }
        }
    }


    close(sockfd);
    */
    /*======== PHASE2 ========*/
    FILE* ptr = fopen("member.txt", "r");
    if (ptr == NULL) {
        printf("no such file: member.txt");
        return 0;
    }

    /* read member.txt */
    struct member memberlist;
    memberlist.member_count = 0;
    while (fscanf(ptr, "%s", buf) == 1){
        buf[strlen(buf) - 1] = '\0';
        strcpy(memberlist.username[memberlist.member_count], buf);
        if(fscanf(ptr, "%s", buf) == 1){
            strcpy(memberlist.password[memberlist.member_count], buf);
        }
        else{
            printf("error in reading");
            return 0;
        }
        memberlist.member_count++;
    }

    for(int i = 0; i < memberlist.member_count; i++){
        for(int j = 0; j < strlen(memberlist.username[i]); j++){
            if('A' <= memberlist.username[i][j] && memberlist.username[i][j] <= 'Z'){
                memberlist.username[i][j] += 32;
            }
        }
    }

    printf("Main Server loaded the member list.\n");

    // following part for TCP socket creating and connection are from code in Beej's Guide
    int new_fd;  // listen on sock_fd, new connection on new_fd
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT_TCP, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    //printf("server: waiting for connections...\n");


    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    if (new_fd == -1) {
        perror("accept");
    }

    /*inet_ntop(their_addr.ss_family,
        get_in_addr((struct sockaddr *)&their_addr),
        s, sizeof s);
    printf("server: got connection from %s\n", s);*/

    /* Get the dynamicly assigned client port number and send it to the client. */
    int client_port =  ntohs(get_in_port((struct sockaddr *)&their_addr));
    if(send(new_fd, &client_port, sizeof client_port, 0) == -1){
        perror("send client port number");
        exit(1);
    }

    close(sockfd); 

    int priority = 0;
    /* login */
    login(new_fd, &memberlist, &priority);

    /* receiver book code and process request*/
    while(1){
        process_request(new_fd, priority);    
    }

    close(new_fd); 

    return 0;
}