#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAXBOOKSIZE 100
#define MYSERVERPORT "43246"
#define MAINSERVERPORT "44246"
#define MAINSERVERIP "localhost"
#define MAXBUFLEN 100

struct library{
    char book_name[MAXBOOKSIZE][100];
    int book_count[MAXBOOKSIZE];
    int num_book;
};

void process_request(struct library *booklist){
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    char buf[MAXBUFLEN];


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYSERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
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
        exit(1);
    }

    freeaddrinfo(servinfo);

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';
    char client_code[100];
    strcpy(client_code, buf);

    close(sockfd);

    int availability = 0;
    int priority = 0;

    if(client_code[0] == 'X'){
        priority = 1;
        client_code[0] = 'H';
        printf("ServerH received an inventory status request for code %s.\n", client_code);
        for(int i = 0; i < booklist->num_book; i++){
            if(strcmp(client_code, booklist->book_name[i]) == 0 && (0 < booklist->book_count[i])){
                availability = booklist->book_count[i];
            }
        }
    }
    
    else{
        printf("ServerH received %s code from the Main Server.\n", client_code);
        for(int i = 0; i < booklist->num_book; i++){
            if(strcmp(client_code, booklist->book_name[i]) == 0 && (0 < booklist->book_count[i])){
                availability = -1;
                booklist->book_count[i] = booklist->book_count[i] - 1;
            }
        }
    }


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(MAINSERVERIP, MAINSERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
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

    if(sendto(sockfd, &availability, sizeof(availability), 0, p->ai_addr, p->ai_addrlen) == -1){
        perror("talker: sendto");
        exit(1);
    }

    if(priority == 1){
        printf("ServerH finished sending the  inventory status to the Main server using UDP on port %s.\n", MYSERVERPORT);
    }
    else{
        printf("ServerH finished sending the availability status of code %s to the Main Server using UDP on port %s.\n", client_code, MYSERVERPORT);    
    }

    close(sockfd);
}


int main()
{
    FILE* ptr = fopen("history.txt", "r");
    if (ptr == NULL) {
        printf("no such file: history.txt");
        return 0;
    }

    struct library booklist;
    booklist.num_book = 0;
    char buf[MAXBUFLEN];
    int int_buf;
    int i = 0;
    while (fscanf(ptr, "%s", buf) == 1){
        buf[strlen(buf) - 1] = '\0';
        strcpy(booklist.book_name[i],buf);
        if(fscanf(ptr, "%d", &int_buf) == 1){
            booklist.book_count[i] = int_buf;
        }
        else{
            printf("error in reading");
            return 0;
        }
        i++;
    }

    booklist.num_book = i;

    printf("ServerH is up and running using UDP on port %s.\n", MYSERVERPORT);

    while(1){
        process_request(&booklist);
    }

    return 0;
}