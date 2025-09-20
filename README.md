# Webserv - HTTP Server Implementation

A high-performance, non-blocking HTTP server written in C++ that implements the HTTP/1.1 protocol with CGI support, file operations, and comprehensive error handling.

---

## Overview

Webserv is a complete HTTP server implementation that handles multiple concurrent connections using Linux `epoll` for efficient I/O multiplexing. The server supports static file serving, CGI script execution, file uploads, and comprehensive error handling with custom error pages.

### Key Characteristics

* **Non-blocking I/O**: The server uses `epoll` for an event-driven architecture.
* **Asynchronous CGI**: It features non-blocking CGI execution with proper process management.
* **Multi-server Support**: The server can handle multiple server blocks with different configurations.
* **Timeout Handling**: It includes client and CGI timeouts with proper HTTP responses.
* **Error Management**: The server supports custom error pages and correct HTTP status codes.

---

## Core Components

The server's architecture is built around several key components:

* **`ConfParser`**: Parses the configuration file, validates syntax, and populates the server data structures.
* **`Server42` & `SingleServer`**: These classes represent the overall server configuration and individual server blocks.
* **`Router`**: Receives a client request and determines the appropriate action, such as serving a static file, running a CGI script, or handling a redirect.
* **`Webserv`**: The core of the server's event loop, it uses `epoll` to monitor sockets for events.
* **`Request` & `Response`**: These classes parse and build HTTP requests and responses, handling headers and bodies.
* **`Cgi`**: Manages the non-blocking execution of CGI scripts by creating pipes for communication.

---

## Building and Running

### Prerequisites

* A Linux operating system.
* A GCC compiler with C++11 support.
* The `make` build system.

### Build Instructions


# Clone the repository
git clone <repository-url>
cd webserv

# Build the project
make

### Running the Server

To start the server, provide a configuration file.


./webserv webserv.conf

-----

### Testing

#### Simple `curl` Test

Test the server's basic functionality with this command:

curl -v http://localhost:8042/
curl -X POST -d "name=test&phone=32143&email=test@test.test" http://localhost:8087/cgi-bin/form.py
curl -X POST -F "file=@cat.jpeg" http://localhost:8087/upload/upload.py
curl -X GET "http://localhost:8087/cgi-bin/get.py?search_term=test@test.test"
curl -X DELETE "http://localhost:8087/cgi-bin/delete.py?file=cat_1.jpeg"


#### `siege` Test

For load testing, use `siege` to simulate concurrent users:


siege -c 30 -t 60s http://localhost:8042/


#### Manual Connection Tests

Use `nc` (netcat) to send raw HTTP requests and check how the server responds.

**Valid request**


printf "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8042


**Invalid request**


printf "HELLO / HTTP/1.1\r\n\r\n" | nc localhost 8042


#### CGI Test Cases

These `curl` commands test specific CGI behaviors and error conditions.

Don't forget to add permission to execute:
chmod +x www/cgi-bin/...

  * `curl http://localhost:8087/cgi-bin/infinite.py`: Tests the server's timeout handling for a long-running CGI script.

    python
    #### infinite.py
    #!/usr/bin/env python3
    print("Content-Type: text/plain\n")
    while True:
        pass  # Infinite loop
    

  * `curl http://localhost:8087/cgi-bin/error.py`: Tests how the server handles an exception thrown by a CGI script.

    python
    #### error.py
    #!/usr/bin/env python3
    print("Content-Type: text/plain\n")
    raise Exception("Intentional error")
    

  * `curl http://localhost:8087/cgi-bin/bad_header.sh`: Tests the server's response when a CGI script violates the protocol by missing a required header.

    
    #### bad_header.py
    #!/usr/bin/env python3
    # Intentionally missing "Content-Type" header
    # This is a "bad header" CGI script
    print("This script is missing the Content-Type header")
    raise Exception("Intentional error after bad header")
    

  * `curl http://localhost:8087/cgi-bin/huge_payload.py`: Tests the server's ability to handle a very large output from a CGI script.

    python
    #### huge_payload.py
    #!/usr/bin/env python3
    print("Content-Type: text/plain\n")
    print("A" * 30000000)
    

-----

### Project Structure

webserv/
├── src/                    # Source files
│   ├── main.cpp           # Entry point
│   ├── Webserv.cpp        # Main server class
│   ├── Cgi.cpp            # CGI implementation
│   ├── Request.cpp        # HTTP request parsing
│   ├── Response.cpp       # HTTP response generation
│   ├── Router.cpp         # Request routing logic
│   └── ...                # Other source files
├── incl/                  # Header files
│   ├── Webserv.hpp        # Main server header
│   ├── Cgi.hpp            # CGI header
│   ├── Request.hpp        # Request header
│   └── ...                # Other headers
├── www/                   # Web root directory
│   ├── index.html         # Homepage
│   ├── cgi-bin/           # CGI scripts
│   │   ├── form.py        # Form handler
│   │   ├── upload.py      # Upload handler
│   │   └── remove.py      # Delete handler
│   └── upload/            # Upload directory
├── error/                 # Custom error pages
│   ├── 400.html           # Bad Request
│   ├── 404.html           # Not Found
│   ├── 408.html           # Request Timeout
│   └── ...                # Other error pages
├── webserv.conf           # Configuration file
└── Makefile              # Build configuration
