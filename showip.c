#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
/* show up addresses for a host given on the command line */

int main(int argc, char *argv[]) {
    struct addrinfo hints, *res, *p;

    int status;
    char ipstr[INET6_ADDRSTRLEN]; //create buff for address size of ipv6

    if (argc != 2) {
        fprintf(stderr, "usage: showip hostname\n");
        return 1;
    }

    memset(&hints, 0, sizeof hints); //sets a place in memory with the constant byte c

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; //acccept sock stream

    if ((status = getaddrinfo(argv[1], nullptr, &hints, &res)) != 0) {
        fprintf(stderr, "g a i error: %s\n", gai_strerror(status));
        return 2;
    }

    printf("IP addresses for %s:\n\n", argv[1]);
    for ( p = res; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;
        struct sockaddr_in *ipv4;
        struct sockaddr_in6 *ipv6;

        //get pointer to the address itself
        //different fields in ipv4 and ipv6
        if (p->ai_family == AF_INET) {
            ipv4 = (struct sockaddr_in *) p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        }else {
            ipv6 = (struct sockaddr_in6 *) p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
        //convert the ip to a string and print
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);

    }
    freeaddrinfo(res);
    return 0;
}