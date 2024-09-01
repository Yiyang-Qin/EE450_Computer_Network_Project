#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define MAINSERVERPORT "45246" // the port client will be connecting to 
#define MAINSERVERIP "localhost"

#define MAXDATASIZE 100 // max number of bytes we can get at once 


void encrypting(char* encrypted, char* origin)
/* encrypt the input username and password */
{
    char chipher[100];
    for(int i = 0; i < strlen(origin); i++){
        if((96 < origin[i])&&(origin[i] < 118) || (64 < origin[i])&&(origin[i] < 86)){
            chipher[i] = origin[i] + 5;
        }
        else if(('u' < origin[i])&&(origin[i] < '{') || ('U' < origin[i])&&(origin[i] < '[')){
            chipher[i] = origin[i] - 21;
        }
        else if (('/' < origin[i])&&(origin[i] < '5')){
            chipher[i] = origin[i] + 5;
        }
        else if(('4' < origin[i])&&(origin[i] < ':')){
            chipher[i] = origin[i] - 5;
        }
        else{
            chipher[i] = origin[i];
        }
    }
    chipher[strlen(origin)] = '\0';
    strcpy(encrypted, chipher);
}

void login(const int sockfd, const int myport, char* Final_username, int* priority){
    /* Handle login process 
        INPUT:
            sockfd: socket descriptor
        MODIFIES: 
            myport: client's port number received from the serverM;
            Final_username: the correctly input username;
            priority: admin right or not; 
    */
    int numbytes;
    char buf[MAXDATASIZE];
    char username[100];
    char password[100];
    char encrypted_username[100];
    char encrypted_password[100];

    printf("Please enter the username:");
    scanf("%s",username);
    printf("Please enter the password:");
    scanf("%s",password);
    encrypting(encrypted_username,username);
    encrypting(encrypted_password,password);

    int send_len = strlen(encrypted_username);
    /*  first send length of input username in the form of int, 
        then send the input username to the main server in the form of char array over TCP. 
    */
    if(send(sockfd, &send_len, sizeof send_len, 0) == -1){
        perror("send send_len");
        exit(1);
    }
    if(send(sockfd, encrypted_username, send_len, 0) == -1){
        perror("send username");
        exit(1);
    }

    send_len = strlen(encrypted_password);

    if(send(sockfd, &send_len, sizeof send_len, 0) == -1){
        perror("send send_len");
        exit(1);
    }
    if(send(sockfd, encrypted_password, send_len, 0) == -1){
        perror("send password");
        exit(1);
    }

    printf("%s sent an authentication request to the Main Server.\n", username);

    if((numbytes = recv(sockfd, buf, 6, 0)) == -1){
        perror("receive authentication message");
        exit(1);
    }

    buf[numbytes] = '\0';
    /* check the message returned from serverM to determine autehtication succeed or not */
    if(strcmp(buf, "Pmatch") == 0){
        printf("%s received the result of authentication from Main Server using TCP over port %d. Authentication is successful.\n", username, myport);
        strcpy(Final_username, username);
    }
    else if(strcmp(buf, "Smatch") == 0){
        printf("%s received the result of authentication from Main Server using TCP over port %d. Authentication is successful.\n", username, myport);
        strcpy(Final_username, username);
        *priority = 1;
    }
    else if(strcmp(buf, "Umatch") == 0){
        printf("%s received the result of authentication from Main Server using TCP over port %d. Authentication failed: Password does not match.\n", username, myport);
        login(sockfd, myport, Final_username, priority);
    }
    else if(strcmp(buf, "Nmatch") == 0){
        printf("%s received the result of authentication from Main Server using TCP over port %d. Authentication failed: Username not found.\n", username, myport);
        login(sockfd, myport, Final_username, priority);
    }
    else{
        perror("Wrong authentication message");
        exit(1);
    }

}

void make_request(const int sockfd, const char* username, const int myport, int priority){
    /* Get the input book code, forward it to the serverM and process the response from serverM */
    printf("Please enter book code to query:\n");
    char book_code[100];
    scanf("%s",book_code);

    /*first send the length of input book code(int) and then send the book code(char*)*/
    int send_len = strlen(book_code);
    if(send(sockfd, &send_len, sizeof send_len, 0) == -1){
        perror("send send_len");
        exit(1);
    }
    if(send(sockfd, book_code, send_len, 0) == -1){
        perror("send book code");
        exit(1);
    }

    if(priority == 1){
        printf("Request sent to the Main Server with Admin rights.\n");
    }
    else{
        printf("%s sent the request to the Main Server.\n", username);
    }

    int reply = 0;
    if(recv(sockfd, &reply, sizeof reply, 0) == -1){
        perror("receive reply from main server");
        exit(1);
    }
    printf("Response received from the Main Server on TCP port: %d.\n", myport);

    /* 
        Without admin right, availability = 0 means NOT available, -1 means available, -2 means Not found;
        With admin right, availability = -2 means NOT found, other non-negative number means the number of books available
        The availability code is determined by the backend server or serverM
        */
    if(priority == 0){
        switch (reply)
        {
        case 0:
            printf("The requested book %s is NOT available in the library.\n", book_code);
            break;
        
        case -1:
            printf("The requested book %s is available in the library.\n", book_code);
            break;

        case -2:
            printf("Not able to find the book-code %s in the system.\n", book_code);
            break;
        
        default:
            break;
        }    
    }

    else{
        switch (reply)
        {
        case -2:
            printf("Not able to find the book-code %s in the system.\n", book_code);
            break;
        
        default:
            printf("Total number of book %s available = %d.\n", book_code, reply);
            break;
        }
    }
}

int main()
{
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    // following part for TCP socket creating and connection are from code in Beej's Guide
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    printf("Client is up and running.\n");

    if ((rv = getaddrinfo(MAINSERVERIP, MAINSERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }


    freeaddrinfo(servinfo); // all done with this structure

    int myport = 0;
    if(recv(sockfd, &myport, sizeof myport, 0) == -1){
        perror("receive client port");
        exit(1);
    }
    
    char username[100];

    int priority = 0;
    login(sockfd, myport, username, &priority);

    while(1){
        make_request(sockfd, username, myport, priority);
        printf("--- Start a new query ---\n");
    }

    close(sockfd);

    return 0;
}