# helper file

## usage of these functions 📌

`#include <arpa/inet.h>`

`uint32_t htonl(uint32_t hostlong);`
**function converts the unsigned integer hostlong from host byte order to network byte order.**
`uint16_t htons(uint16_t hostshort);`
**function converts the unsigned short integer hostshort from host byte order to network byte order.**
`uint32_t ntohl(uint32_t netlong);`
**function converts the unsigned integer netlong from network byte order to host byte order.**
`uint16_t ntohs(uint16_t netshort);`
**function converts the unsigned short integer netshort from network byte order to host byte order.**

### Return Value
**Upon successful completion, socket() shall return a non-negative integer, the socket file descriptor. Otherwise, a value of -1 shall be returned and errno set to indicate the error.**


## Socket 📌

### Socket Creation 🟢

`int sockfd = socket(domain, type, protocol);`

**sockfd** : socket descriptor, an integer (like a file handle)

**domain** : integer, specifies communication domain. We use AF_ LOCAL as defined in the POSIX standard for communication between processes on the same host. For communicating between processes on different hosts connected by IPV4, we use AF_INET and AF_I NET 6 for processes connected by IPV6.

**type** : communication type
1. SOCK_STREAM: TCP(reliable, connection-oriented)
2. SOCK_DGRAM: UDP(unreliable, connectionless)

**protocol** : Protocol value for Internet Protocol(IP), which is 0. This is the same number that appears on the protocol field in the IP header of a packet.(man protocols for more details)


## Sockaddr_in struct 📌

`_address.sin_addr`
`_address.sin_family`
`_address.sin_port`
`_address.sin_zero`