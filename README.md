# cHttp
re-learning basic socket/network programming by building a basic http server


## Basic http server 

This is an example of a basic http server in C. It listens on port 8080 and serves a simple HTML page when accessed.

This is complied on a Linux machine, sorry window users :)

### What is a socket?
A socket is a way to speak to other programs using the standard Unix file descriptors

Everything in Unix is a file, that is why every type of I/O with unix systems they operate by reading or writing to a file descriptor
A file descriptor is simply an integer associated with an open file, and that can literally be anything: network connection, FIFO, pipe, terminal a real on disk file. So when you want to communicate with another pgoram over the Internat you're going to do it through a file descriptor.

### Two main types of sockets
Stream sockets and Datagram sockets.

Stream sockets are reliable two way connected communication streams. If you output two items into the socket in order "1, 2", they will arive in the order "1, 2" at the opposite end

applications such as telnet or ssh use stream sockets. Web browsers use the HTTP Hypertext Transfer Protocol, which uses stream sockets to get pages.

Stream sockets use the Transmission Control Protocol (TCP) https://datatracker.ietf.org/doc/html/rfc793

Datagram sockets use UDP User Datagram Protocol. UDP is a connectionless protocol, and it is not reliable. If you output two items into the socket in order "1, 2", they may arrive in the order "2, 1" at the opposite end. Or they may not arrive at all.

You make the packet, put an ip header on it with destination information, and send it out. No connection needed. They
are connectionless because you do not need to maintain an open connection as you do with stream sockets.
Generally used either when a TCP stack is unavailable or when a few dropped packets are not the end of the world.
Applications such as tftp (trivial file transfer protocol), dhcpcd (dhcp client), multiplayer games, streaming, video conferencing, and voice over ip use datagram sockets.

Sometimes these applications have their own protocol on top of UDP. The tftp says  that for each packet that gets sent, the recipient has to send back a packet that confirms they have recieved it.
If no ACK is received by the sender after a certain amount of time, it will re-transmit the packet until it finally gets an ACK.

## Networking garble that is useful from the guide im following
see here: https://beej.us/guide/bgnet/html/split/ip-addresses-structs-and-data-munging.html

## Structs
socket descriptors are just of type `int`

This struct is used to prep the socket addresses structures for use. It is also used in host name lookups and service name lookups

```c++
struct addrinfo{
    int         ai_flags; // AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST
    int         ai_family; // AF_INET, AF_INET6, AF_UNSPEC
    int         ai_socktype; // SOCK_STREAM, SOCK_DGRAM
    int         ai_protocol; // use 0 for auto
    size_t      ai_addrlen; // size of ai_addr in bytes
    struct      sockaddr *ai_addr; // struct sockaddr_in or _in6 that contains ip and port
    char        *ai_canonname; // full canonical hostname
    struct      addrinfo *ai_next; // linked list of results
}
```
Once this gets loaded, call `getaddrinfo()`. It will return a pointer to a new linked list of these structures filled out with what you need

The struct sockaddr holds socket address information for many types of sockets.
```c++
struct sockaddr {
    unsigned short  sa_family; // address family, AF_xxx
    char            sa_data[14]; // 14 bytes of protocol address
};
```
`sa_family` is the address family, which is always AF_INET for IPv4 addresses, and AF_INET6 for IPv6 addresses. The `sa_data` field contains the actual address data, which is 14 bytes long. For IPv4 addresses, the first 4 bytes of `sa_data` contain the IP address, and the next 2 bytes contain the port number. For IPv6 addresses, the first 16 bytes of `sa_data` contain the IP address, and the next 2 bytes contain the port number.


For `struct sockaddr_in` to be used with ipv4
```c++
struct sockaddr_in {
    short int    sin_family; // Address family, AF_INET
    unsigned short int sin_port; // Port number
    struct in_addr sin_addr;    // Internet address
    unsigned char sin_zero[8]; // Padding
};
```

this makes it easy to reference elements of the socket address.
`sin_port` must be in Network Byte Order (Big endian) by using `htons()`

### IpAddresses
if you have an ip address "10.12.110.57" that you want to store into it. The function to use 
use called `inet_pton()` which stands for presentation to network. It converts the ip address from the human readable form into the binary form that the computer can understand. It takes three arguments: the address family (AF_INET for IPv4), the string containing the ip address, and a pointer to a buffer where the binary form of the address will be stored.

```c++
struct sockaddr_in sa; //ipv4

inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr));
```

Now we can convert string ip addresses to their binary representations, what about the other way

you use the function `inet_ntop()` which stands for network to presentation. It converts the binary form of an ip address back into the human readable form. It takes four arguments: the address family (AF_INET for IPv4), a pointer to the binary form of the address, a buffer where the string form of the address will be stored, and the size of the buffer.

```c++
char ip4[INET_ADDRSTRLEN]; // space to hold the string form of the ip address
struct sockaddr_in sa; // assume this is loaded with something

inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN);
printf("the ipv4 address is: %s\n", ip4);
```
these functions only work on numeric ip addresses, not hostnames. If you want to convert a hostname to an ip address, you can use the `getaddrinfo()` function, which is a more modern and flexible way to do hostname resolution. It takes a hostname and a service name (or port number) and returns a linked list of `struct addrinfo` structures that contain the resolved addresses.

### private and disconnected networks
firewalls hides networks from the rest of the world for their own protection
Often times the firewall translates "internal" ip addresses to "external" ip addresses using a process called Network Address Translation, or NAT.

NAT is taken care of transparently for us by the router, but it can cause problems for certain applications that need to know their own ip address or that need to accept incoming connections from the outside world. For example, if you want to run a web server on your home network, you will need to configure your router to forward incoming requests on port 80 to the internal ip address of your web server. This is called port forwarding.

For instance, a firewall at home. You are given 2 static ipv4 addresses allocated to you
by the dsl company. One is for the router, and the other is for your computer. The router is connected to the internet, and it has a public ip address that can be accessed from anywhere in the world. Your computer is connected to the router, and it has a private ip address that can only be accessed from within your home network. The router uses NAT to translate between the public ip address and the private ip address, so that when you access a website from your computer, it appears to come from the public ip address of the router.


## System calls
This section is about the system calls that allow you to access the network functionality of a Unix box,
or any box that supports the sockets API. when you call one of these functions, the kernel takes over and does the work.

### getaddrinfo
```c++
SYNOPSIS
     #include <sys/types.h>
     #include <sys/socket.h>
     #include <netdb.h>

     int getaddrinfo(const char *restrict node,     // "www.example.com" or IP
                     const char *restrict service,  // "http" or port number
                     const struct addrinfo *restrict hints,
                     struct addrinfo **restrict res);
```
You give this function three input parameters and it gives you a pointer to a linked-list, `res` of results.

`Node` parameter is the host name to connect to, or an ip addresss
`service` can be a port number or the name of a particular service found in the IANA Port List or the /etc/services file
`hints` parameter points to a struct addrinfo that you've already filled out with relevant information

