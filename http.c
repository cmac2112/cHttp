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

#define PORT "3490" //port users will connect to
#define BACKLOG 3     // how many pending connections queue holds
void sigchld_handler(int s)
{
    (void)s;

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
int main() {
    int addrInfo;
    int socketFd, new_fd; //listens on socketFd, new connection on new_fd
    struct addrinfo hints, *serverInfo, *p;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];

    ssize_t recvFd;

    memset(&hints, 0, sizeof hints); //set our hints to be empty

    //assign flags we need
    hints.ai_family = AF_UNSPEC; //either ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM; //tcp
    hints.ai_flags = AI_PASSIVE; //fill my ip for me


    printf("getaddrinfo-ing\n");
    if ((addrInfo = getaddrinfo(NULL, PORT, &hints, &serverInfo)) != 0) {
        fprintf(stderr, "g a i error: %s\n", gai_strerror(addrInfo));
        return 1;
    }

    //loop through all results from getaddrinfo -- remember this is a linked list
    for (p = serverInfo; p != NULL; p = p->ai_next) {
        if ((socketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }
        if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(socketFd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socketFd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(serverInfo);
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(socketFd, BACKLOG) == -1) {
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

    printf("server: waiting for connections...\n");
    while (1) {
        sin_size = sizeof their_addr;
        new_fd = accept(socketFd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }else {
            printf("Connection received\n");
        }
        const char *inetop;
        inetop = inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("from: %s\n", inetop);
        if (!fork()) {
            close(socketFd);
            char buf[2048];
            recvFd = recv(new_fd, buf, sizeof(buf), 0);
            if (recvFd == -1) {
                perror("recv");
                const char *body = "<h1>Oops your request did not work 404</h1>";
                char msg[512];
                snprintf(msg, sizeof(msg),
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: %zu\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "%s", strlen(body), body);
                if (send(new_fd, msg, strlen(msg), 0) == -1) {
                    perror("send");
                }
                close(new_fd);
                exit(0);
            }
                //determine what is in buf
                const char *home = "GET /home";
                char *val;


                //strstr reads until the \0 character, setup our buf to have one
                buf[recvFd] = '\0';
                printf("%s", buf);
                if ((val = strstr(buf, home)) != NULL){
                    //no /home string found
                    const char *body =
                        "<h1>Hey you hit the /home request worked!</h1>"
                        "<button onclick=\"hitUsers()\">click</button>"
                        "<pre id=\"result\"></pre>"
                        "<script>"
                        "function hitUsers() {"
                        "  fetch('/users')"
                        "    .then(function(r) { return r.text(); })"
                        "    .then(function(text) { document.getElementById('result').textContent = text; })"
                        "    .catch(function(err) { document.getElementById('result').textContent = 'Error: ' + err; });"
                        "}"
                        "</script>";
                    char msg[1024];
                    snprintf(msg, sizeof(msg),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s", strlen(body), body);
                    if (send(new_fd, msg, strlen(msg), 0) == -1) {
                        perror("send");
                    }
                    printf("200 OK /home\n");
                    close(new_fd);
                    exit(0);
                }

                //now lets check against a param request and return json
                const char *user = "GET /users";
                if (strstr(buf, user) != NULL) {
                    const char *data = "json data here";
                    char msg[512];
                    snprintf(msg, sizeof(msg),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s", strlen(data), data);
                    if (send(new_fd, msg, strlen(msg), 0) == -1) {
                        perror("send");
                    }
                    printf("200 OK /users\n");
                    close(new_fd);
                    exit(0);
                }

            const char *body = "<h1>Hello World im sending you this from C</h1><p>This is some basic html</p>";
            char msg[512];
            snprintf(msg, sizeof(msg),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %zu\r\n"
                "Connection: close\r\n"
                "\r\n"
                "%s", strlen(body), body);
            if (send(new_fd, msg, strlen(msg), 0) == -1) {
                perror("send");
            }
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }
    return 0;
}
