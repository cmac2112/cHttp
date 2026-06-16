#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MYPORT "3490" //port users will connect to
#define BACKLOG 3     // how many pending connections queue holds

int main() {
    int status;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC; //either ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM; //tcp
    hints.ai_flags = AI_PASSIVE; //fill my ip for me
    printf("getaddrinfo-ing\n");
    if ((status = getaddrinfo(NULL, MYPORT, &hints, &res)) != 0) {
        fprintf(stderr, "g a i error: %s\n", gai_strerror(status));
        return 2;
    }
   //lets get a socket from thiis?

    int s;
    int b;
    //socket returns us a socket descriptor we can use later ot bind to
    printf("socketing\n");
    if ((s = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        fprintf(stderr,"Socket creation failed: errno %s\n", strerror(errno));
        return 3;
    }

    //once we have the socket we can associate it with a port on our machine
    printf("binding\n");
    if (( b = bind(s, res->ai_addr, res->ai_addrlen)) == -1) {
        fprintf(stderr, "bind failed: %s\n", strerror(errno));
        return 4;
    }

    //listen and loop awiating calls on port 3490
    int l;
    printf("Listening on port: %s\n", MYPORT);
    if ((l = listen(s, BACKLOG)) == -1) {
        fprintf(stderr, "listen failed %s\n", strerror(errno));
    }

    int newfd;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    addr_size = sizeof their_addr;
    newfd = accept(s, (struct sockaddr *)&their_addr, &addr_size);
    //lets send a connection back
    char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello World im bold</h1><p>But im not!</p>";
    int len, bytes_sent;
    len = strlen(msg);
    bytes_sent = send(newfd, msg, len, 0);

    printf("bytes_sent: %d\n", bytes_sent);


    freeaddrinfo(res);
    return 0;
}