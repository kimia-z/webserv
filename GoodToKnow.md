# -----helper file----

## 📌 usage of these functions 📌

`uint32_t htonl(uint32_t hostlong);`
**function converts the unsigned integer hostlong from host byte order to network byte order.**
`uint16_t htons(uint16_t hostshort);`
**function converts the unsigned short integer hostshort from host byte order to network byte order.**
`uint32_t ntohl(uint32_t netlong);`
**function converts the unsigned integer netlong from network byte order to host byte order.**
`uint16_t ntohs(uint16_t netshort);`
**function converts the unsigned short integer netshort from network byte order to host byte order.**


## 📌 Socket 📌

### 🟢 Socket Creation 🟢

`int sockfd = socket(domain, type, protocol);`

*The socket() function shall create an unbound socket in a communications domain, and return a file descriptor that can be used in later function calls that operate on sockets.*

**sockfd** : socket descriptor, an integer (like a file handle)

**domain** : integer, specifies communication domain. We use AF_ LOCAL as defined in the POSIX standard for communication between processes on the same host. For communicating between processes on different hosts connected by IPV4, we use AF_INET and AF_INET6 for processes connected by IPV6.

**type** : communication type
1. SOCK_STREAM: TCP(reliable, connection-oriented)
2. SOCK_DGRAM: UDP(unreliable, connectionless)

**protocol** : Protocol value for Internet Protocol(IP), which is 0. This is the same number that appears on the protocol field in the IP header of a packet.(man protocols for more details)

### 💡 Return Value 💡
**Upon successful completion, socket() shall return a non-negative integer, the socket file descriptor. Otherwise, a value of -1 shall be returned and errno set to indicate the error.**


### 🟢 Bind (Server Side)🟢

`int bind(int socket, const struct sockaddr *address,socklen_t address_len);`

*The bind() function shall assign a local socket address address to a socket identified by descriptor socket that has no local socket address assigned. Sockets created with the socket() function are initially unnamed; they are identified only by their address family.*

#### 💡 Return Value 💡
**Upon successful completion, bind() shall return 0; otherwise, -1 shall be returned and errno set to indicate the error.**


### 🟢 Connect (Client Side)🟢

`int connect(int socket, const struct sockaddr *address,socklen_t address_len);`

*The connect() function shall attempt to make a connection on a socket.*

#### 💡 Return Value 💡
**Upon successful completion, connect() shall return 0; otherwise, -1 shall be returned and errno set to indicate the error.**

### 🟢 Listen (Server Side)🟢

`int listen(int socket, int backlog);`

*The listen() function shall mark a connection-mode socket, specified by the socket argument, as accepting connections.*

#### 💡 Return Value 💡
**Upon successful completions, listen() shall return 0; otherwise, -1 shall be returned and errno set to indicate the error.**

### 🟢 Accept (Server Side)🟢

`int accept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len);`

*The accept() function shall extract the first connection on the queue of pending connections, create a new socket with the same socket type protocol and address family as the specified socket, and allocate a new file descriptor for that socket.*

#### 💡 Return Value 💡
**Upon successful completion, accept() shall return the non-negative file descriptor of the accepted socket. Otherwise, -1 shall be returned and errno set to indicate the error.**

## 📌 Sockaddr_in struct (Defining Server Address)📌

`_address.sin_addr`
`_address.sin_family`
`_address.sin_port`
`_address.sin_zero`


## 📌 example of http request 📌

*GET / HTTP/1.1*  
*Host: localhost:8081*  
*Connection: keep-alive*  
*sec-ch-ua: "Chromium";v="136", "Google Chrome";v="136", "Not.A/Brand";v="99"*  
*sec-ch-ua-mobile: ?0*  
*sec-ch-ua-platform: "Linux"*  
*Upgrade-Insecure-Requests: 1*  
*User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36*  
*Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7*  
*Sec-Fetch-Site: none*  
*Sec-Fetch-Mode: navigate*  
*Sec-Fetch-User: ?1*  
*Sec-Fetch-Dest: document*  
*Accept-Encoding: gzip, deflate, br, zstd*  
*Accept-Language: en-US,en;q=0.9*  