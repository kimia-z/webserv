# WebServ - HTTP Server Implementation

A high-performance, non-blocking HTTP server written in C++ that implements the HTTP/1.1 protocol with CGI support, file operations, and comprehensive error handling.

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Features](#features)
4. [Project Structure](#project-structure)
5. [Configuration](#configuration)
6. [HTTP Methods](#http-methods)
7. [CGI Implementation](#cgi-implementation)
8. [Error Handling](#error-handling)
9. [Timeout Management](#timeout-management)
10. [Building and Running](#building-and-running)
11. [Testing Guide](#testing-guide)

## Overview

WebServ is a complete HTTP server implementation that handles multiple concurrent connections using Linux epoll for efficient I/O multiplexing. The server supports static file serving, CGI script execution, file uploads, and comprehensive error handling with custom error pages.

### Key Characteristics
- **Non-blocking I/O**: Uses epoll for event-driven architecture
- **Asynchronous CGI**: Non-blocking CGI execution with proper process management
- **Multi-server support**: Can handle multiple server blocks with different configurations
- **Timeout handling**: Client and CGI timeouts with proper HTTP responses
- **Error management**: Custom error pages and proper HTTP status codes

## Architecture

### Core Components

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Client        │    │   WebServ       │    │   CGI Process   │
│   (Browser)     │◄──►│   (Main Server) │◄──►│   (Python)      │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                              │
                              ▼
                       ┌─────────────────┐
                       │   File System   │
                       │   (Static Files)│
                       └─────────────────┘
```

### Event Loop Flow

1. **Initialization**: Create epoll instance and bind to configured ports
2. **Event Loop**: 
   - Check timeouts (clients and CGI processes)
   - Wait for events using `epoll_wait()`
   - Process events based on file descriptor type
3. **Request Processing**:
   - Parse HTTP request
   - Route to appropriate handler
   - Generate response
4. **Response Delivery**: Send response back to client

## Features

### HTTP Protocol Support
- **Methods**: GET, POST, DELETE
- **Headers**: Full HTTP/1.1 header parsing and validation
- **Status Codes**: 200, 301, 400, 403, 404, 405, 408, 409, 413, 415, 500, 501, 502, 504
- **Content Types**: Automatic MIME type detection
- **Chunked Transfer**: Support for chunked encoding

### File Operations
- **Static File Serving**: Serve HTML, CSS, JS, images, etc.
- **File Upload**: POST requests with multipart/form-data
- **File Deletion**: DELETE requests for file removal
- **Directory Listing**: Autoindex functionality
- **Path Traversal Protection**: Security against directory traversal attacks

### CGI Support
- **Asynchronous Execution**: Non-blocking CGI process management
- **Python Support**: Execute Python scripts (.py files)
- **Environment Variables**: Proper CGI environment setup
- **Process Management**: Fork/exec with proper cleanup
- **Timeout Handling**: CGI process timeouts (30 seconds)

### Error Handling
- **Custom Error Pages**: HTML error pages for different status codes
- **Graceful Degradation**: Fallback responses when custom pages unavailable
- **Proper HTTP Responses**: Correct status codes and headers

## Project Structure

```
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
```

## Configuration

The server uses a custom configuration file format similar to nginx. Here's an example:

```nginx
server {
    listen 8087;
    server_name localhost;
    root www;
    index index.html;
    client_max_body_size 10M;
    
    location / {
        allow_methods GET POST DELETE;
        autoindex on;
    }
    
    location /cgi-bin/ {
        allow_methods GET POST;
        cgi_extension .py;
    }
    
    location /upload/ {
        allow_methods POST;
        upload_path www/upload;
    }
    
    error_page 404 error/404.html;
    error_page 408 error/408.html;
}
```

### Configuration Options
- **listen**: Port number to listen on
- **server_name**: Server name for virtual hosting
- **root**: Document root directory
- **index**: Default file to serve for directory requests
- **client_max_body_size**: Maximum request body size
- **allow_methods**: Allowed HTTP methods for location
- **autoindex**: Enable/disable directory listing
- **cgi_extension**: File extensions that trigger CGI execution
- **upload_path**: Directory for file uploads
- **error_page**: Custom error pages

## HTTP Methods

### GET Requests
- **Static Files**: Serve files from document root
- **Directory Listing**: Show directory contents when autoindex enabled
- **CGI Scripts**: Execute CGI scripts and return output

### POST Requests
- **File Upload**: Handle multipart/form-data uploads
- **CGI Processing**: Send data to CGI scripts for processing
- **Form Submission**: Process form data through CGI

### DELETE Requests
- **File Deletion**: Remove files from server
- **Directory Deletion**: Remove empty directories
- **CGI Processing**: Use CGI scripts for complex deletion logic

## CGI Implementation

### Architecture
The CGI implementation is fully asynchronous and non-blocking:

1. **Process Creation**: Fork a new process for each CGI request
2. **Pipe Communication**: Use pipes for stdin/stdout communication
3. **Event-Driven I/O**: Monitor pipes using epoll events
4. **Process Management**: Proper cleanup and timeout handling

### CGI Lifecycle

```
Request → CGI Detection → Process Fork → Pipe Setup → Data Transfer → Response Generation → Cleanup
```

### Key Features
- **Non-blocking**: Server continues handling other requests during CGI execution
- **Timeout Protection**: CGI processes are killed after 30 seconds
- **Proper Cleanup**: All pipes and processes are cleaned up properly
- **Error Handling**: CGI errors are converted to proper HTTP responses

### Environment Variables
CGI scripts receive standard environment variables:
- `REQUEST_METHOD`: HTTP method (GET, POST, DELETE)
- `CONTENT_LENGTH`: Size of request body
- `CONTENT_TYPE`: MIME type of request body
- `QUERY_STRING`: URL query parameters
- `SCRIPT_NAME`: Path to CGI script
- `SERVER_PROTOCOL`: HTTP version (HTTP/1.1)

## Error Handling

### Error Types
1. **Client Errors (4xx)**: Bad requests, not found, method not allowed
2. **Server Errors (5xx)**: Internal server errors, CGI failures
3. **Timeout Errors (408)**: Request timeout after 5 minutes

### Custom Error Pages
The server supports custom HTML error pages:
- `error/400.html` - Bad Request
- `error/404.html` - Not Found
- `error/408.html` - Request Timeout
- `error/500.html` - Internal Server Error

### Error Response Format
```http
HTTP/1.1 404 Not Found
Content-Type: text/html
Content-Length: 1234
Connection: close

<html>
<head><title>404 Not Found</title></head>
<body>
    <h1>404 Not Found</h1>
    <p>The requested resource was not found.</p>
</body>
</html>
```

## Timeout Management

### Client Timeouts
- **Timeout Duration**: 5 minutes (300 seconds)
- **Reset on Activity**: Timeout resets on any client activity
- **Response**: Sends HTTP 408 Request Timeout with custom page

### CGI Timeouts
- **Timeout Duration**: 30 seconds
- **Process Termination**: Kills hanging CGI processes
- **Response**: Returns HTTP 500 Internal Server Error

### Implementation Details
```cpp
// Client timeout check
void checkClientTimeouts() {
    time_t currentTime = time(NULL);
    const int TIMEOUT_SECONDS = 300;
    
    for (auto it = clientTimeouts_.begin(); it != clientTimeouts_.end();) {
        if (currentTime - it->second > TIMEOUT_SECONDS) {
            sendTimeoutResponse(it->first);
            closeClientConnection(it->first);
            it = clientTimeouts_.erase(it);
        } else {
            ++it;
        }
    }
}
```

## Building and Running

### Prerequisites
- Linux operating system
- GCC compiler with C++11 support
- Make build system

### Build Instructions
```bash
# Clone the repository
git clone <repository-url>
cd webserv

# Build the project
make

# Run the server
./webserv webserv.conf
```

### Makefile Targets
- `make` - Build the project
- `make clean` - Remove object files
- `make fclean` - Remove all generated files
- `make re` - Rebuild the project

## Testing Guide

### 1. Basic Functionality Tests

#### Test Static File Serving
```bash
# Test homepage
curl http://localhost:8087/

# Test specific file
curl http://localhost:8087/index.html

# Test non-existent file (should return 404)
curl http://localhost:8087/nonexistent.html
```

#### Test Directory Listing
```bash
# Enable autoindex in config and test
curl http://localhost:8087/www/
```

### 2. HTTP Method Tests

#### GET Requests
```bash
# Basic GET
curl -X GET http://localhost:8087/

# GET with query parameters
curl "http://localhost:8087/cgi-bin/form.py?name=test&email=test@example.com"
```

#### POST Requests
```bash
# Simple POST
curl -X POST -d "name=test&email=test@example.com" http://localhost:8087/cgi-bin/form.py

# File upload
curl -X POST -F "file=@test.txt" http://localhost:8087/upload/upload.py
```

#### DELETE Requests
```bash
# Delete file
curl -X DELETE http://localhost:8087/upload/test.txt

# Delete through CGI
curl -X DELETE http://localhost:8087/cgi-bin/remove.py?file=test.txt
```

### 3. CGI Testing

#### Test Form Processing
```bash
# Create test form data
curl -X POST -d "name=John&email=john@example.com&message=Hello" \
     -H "Content-Type: application/x-www-form-urlencoded" \
     http://localhost:8087/cgi-bin/form.py
```

#### Test File Upload
```bash
# Create test file
echo "This is a test file" > test.txt

# Upload file
curl -X POST -F "file=@test.txt" \
     -H "Content-Type: multipart/form-data" \
     http://localhost:8087/upload/upload.py
```

#### Test File Deletion
```bash
# Delete uploaded file
curl -X DELETE http://localhost:8087/cgi-bin/remove.py?file=test.txt
```

### 4. Error Handling Tests

#### Test 404 Error
```bash
curl http://localhost:8087/nonexistent.html
# Should return 404 with custom error page
```

#### Test 405 Method Not Allowed
```bash
curl -X POST http://localhost:8087/index.html
# Should return 405 if POST not allowed for this location
```

#### Test 413 Payload Too Large
```bash
# Create large file
dd if=/dev/zero of=large.txt bs=1M count=20

# Try to upload (should fail if exceeds client_max_body_size)
curl -X POST -F "file=@large.txt" http://localhost:8087/upload/upload.py
```

### 5. Timeout Tests

#### Test Client Timeout
```bash
# Start a request but don't complete it
telnet localhost 8087
# Type: GET / HTTP/1.1
# Don't press Enter, wait 5+ minutes
# Should receive 408 timeout response
```

#### Test CGI Timeout
```bash
# Create a slow CGI script
echo '#!/usr/bin/python3
import time
time.sleep(35)  # Sleep longer than CGI timeout
print("Content-Type: text/html")
print("")
print("<h1>This should not appear</h1>")' > www/cgi-bin/slow.py

chmod +x www/cgi-bin/slow.py

# Test the slow script
curl http://localhost:8087/cgi-bin/slow.py
# Should timeout after 30 seconds and return 500 error
```

### 6. Concurrent Connection Tests

#### Test Multiple Connections
```bash
# Test multiple simultaneous connections
for i in {1..10}; do
    curl http://localhost:8087/ &
done
wait
```

#### Test CGI Concurrency
```bash
# Test multiple CGI requests simultaneously
for i in {1..5}; do
    curl -X POST -d "test=$i" http://localhost:8087/cgi-bin/form.py &
done
wait
```

### 7. Performance Tests

#### Test with Apache Bench (if available)
```bash
# Basic load test
ab -n 1000 -c 10 http://localhost:8087/

# Test CGI performance
ab -n 100 -c 5 -p testdata.txt -T application/x-www-form-urlencoded \
   http://localhost:8087/cgi-bin/form.py
```

#### Test Large File Handling
```bash
# Create large file
dd if=/dev/zero of=large.bin bs=1M count=100

# Test serving large file
curl -o downloaded.bin http://localhost:8087/large.bin

# Test uploading large file
curl -X POST -F "file=@large.bin" http://localhost:8087/upload/upload.py
```

### 8. Browser Testing

#### Manual Browser Tests
1. Open browser and navigate to `http://localhost:8087/`
2. Test form submission on `/form.html`
3. Test file upload on `/upload.html`
4. Test file deletion functionality
5. Test error pages by accessing non-existent URLs

#### Test Different Browsers
- Chrome/Chromium
- Firefox
- Safari (if on macOS)
- Edge

### 9. Configuration Testing

#### Test Different Configurations
```bash
# Test with different port
# Modify webserv.conf to use port 8088
./webserv webserv.conf

# Test with different root directory
# Modify root in config and test
```

#### Test Error Page Customization
```bash
# Modify error/404.html and test
curl http://localhost:8087/nonexistent.html
# Should show custom 404 page
```

### 10. Stress Testing

#### Test High Load
```bash
# Use siege if available
siege -c 50 -t 60s http://localhost:8087/

# Or use curl in a loop
for i in {1..1000}; do
    curl -s http://localhost:8087/ > /dev/null &
    if [ $((i % 50)) -eq 0 ]; then
        wait  # Wait for batch to complete
    fi
done
wait
```

### 11. Memory and Resource Testing

#### Monitor Resource Usage
```bash
# Monitor memory usage
while true; do
    ps aux | grep webserv
    sleep 1
done

# Monitor file descriptors
lsof -p $(pgrep webserv) | wc -l
```

### 12. Log Analysis

#### Check Server Output
```bash
# Run server in foreground to see logs
./webserv webserv.conf

# Look for:
# - Connection logs
# - Error messages
# - CGI execution logs
# - Timeout messages
```

### Test Results Validation

For each test, verify:
1. **Correct HTTP Status Codes**: 200, 404, 405, 408, 500, etc.
2. **Proper Headers**: Content-Type, Content-Length, Connection
3. **Response Content**: Correct HTML, error pages, CGI output
4. **No Memory Leaks**: Server should handle many requests without memory growth
5. **No Hanging Processes**: CGI processes should be cleaned up properly
6. **Timeout Behavior**: Proper 408 responses for timeouts

### Troubleshooting

#### Common Issues
1. **Permission Denied**: Check file permissions for CGI scripts
2. **Port Already in Use**: Kill existing processes or change port
3. **CGI Not Working**: Verify Python path and script permissions
4. **Files Not Found**: Check document root configuration
5. **Upload Fails**: Verify upload directory exists and is writable

#### Debug Mode
```bash
# Run with debug output
gdb ./webserv
(gdb) run webserv.conf
# Set breakpoints and step through code
```

This comprehensive testing guide ensures your webserv implementation works correctly under various conditions and handles edge cases properly.
