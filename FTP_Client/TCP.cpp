// Include necessary headers for networking and system functionality
#include "TCP.h"

#define _WIN32_WINNT 0x601 // Define minimum Windows version (Windows 7 or later)
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/types.h>
#include <exception>
#include <string>

#pragma comment(lib, "Ws2_32.lib") // Link against Winsock library
#pragma comment(lib, "Mswsock.lib") // Link against Windows socket extensions
#pragma comment(lib, "AdvApi32.lib") // Link against Advanced Windows API library

#include "bufferf.h"
#include "tcp_exception.h"
#include <bout.h>

// Private class for handling the internal socket connection
class TCP::__privates__ {
private:
    SOCKET sockd = INVALID_SOCKET; // Socket descriptor (initialized to invalid)
    addrinfo* server = nullptr;    // Pointer to server address information
    int port = 0;                  // Port number
    char ip[100] = {};             // IP address as a string

public:
    // Constructor initializes Winsock
    __privates__() {
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // Initialize Winsock
        if (iResult != 0) {
            throw std::exception(bout() << "WSAStartup failed with error:" << iResult << bfin);
        }
    }

    // Set socket timeout in seconds
    void set_timeout(int seconds) {
        DWORD timeout = seconds * 1000; // Convert seconds to milliseconds
        setsockopt(sockd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)); // Set receive timeout
        setsockopt(sockd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)); // Set send timeout
    }

    // Establish connection to the given host and port
    void connect(const char* host, int port) {
        close(); // Close any existing connection
        sockd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create a TCP socket
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        char port_str[20] = { 0 };
        if (_itoa_s(port, port_str, 10) != 0) {
            throw std::exception(bout() << "Failed to convert port number to string: " << port << bfin);
        }

        addrinfo* result = nullptr;
        int iResult = getaddrinfo(host, port_str, &hints, &result); // Get address info for the host and port
        if (iResult != 0) {
            WSACleanup(); // Cleanup Winsock
            throw std::exception(bout() << "getaddrinfo failed with error:" << iResult << bfin);
        }

        // Iterate through possible results to find a valid connection
        for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) {
            printf("Trying..\n");
            sockd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); // Create socket
            if (sockd == INVALID_SOCKET) {
                WSACleanup(); // Cleanup Winsock
                throw std::exception("socket failed with error: %ld\n", WSAGetLastError());
            }

            // Attempt to connect to the server
            int iResult = ::connect(sockd, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (iResult == SOCKET_ERROR) {
                closesocket(sockd);
                sockd = INVALID_SOCKET;
                continue; // Try next address
            }

            server = ptr; // Save server info

            sockaddr_in struc_{};
            int struc_len = sizeof(struc_);
            if (getsockname(sockd, (sockaddr*)&struc_, &struc_len)) {
                printf("getname failed\n");
            }
            else {
                // Retrieve local IP address and port number
                strncpy_s(ip, inet_ntoa(((sockaddr_in*)&struc_)->sin_addr), sizeof(ip));
                port = ntohs(((sockaddr_in*)&struc_)->sin_port);
            }

            return; // Successfully connected
        }

        throw std::exception("Connection failed"); // Throw exception if connection attempt fails
    }

    // Getters for IP and port
    int get_port() const { return port; }
    const char* get_ip() const { return ip; }

    // Send data through the socket
    int send(const char* buffer, size_t size) {
        return ::send(sockd, buffer, (int)size, 0); // Send data to the server
    }

    // Receive data from the socket
    int recv(char* buffer, size_t size) {
        return ::recv(sockd, buffer, (int)size, 0); // Receive data from the server
    }

    // Close the socket connection
    void close() {
        if (sockd != INVALID_SOCKET) {
            closesocket(sockd); // Close socket
            sockd = INVALID_SOCKET; // Mark socket as invalid
        }
    }

    // Destructor ensures that the socket is closed when the object is destroyed
    ~__privates__() { close(); }
};

// TCP class constructor
TCP::TCP() { privates = new __privates__(); }

// Connect to a host and port
void TCP::connect(const char* host, int port) { privates->connect(host, port); }

// Send data and return a TCPResult object indicating success or failure
TCPResult TCP::send(const void* buffer, size_t size) {
    int sent_result = privates->send(static_cast<const char*>(buffer), size); // Send the data
    if (sent_result < 0) return TCPResult::fail(WSAGetLastError()); // Return failure if error
    return TCPResult::success(sent_result); // Return success with sent bytes count
}

// Receive data and return a TCPResult object indicating success or failure
TCPResult TCP::recv(void* buffer, size_t size) {
    int recv_result = privates->recv(static_cast<char*>(buffer), size); // Receive the data
    if (recv_result < 0) return TCPResult::fail(WSAGetLastError()); // Return failure if error
    return TCPResult::success(recv_result); // Return success with received bytes count
}

// Ensure the data is sent successfully
void TCP::ensure_send(void* buffer, size_t size) { send(buffer, size).validate_send(size); }

// Ensure the data is received successfully
void TCP::ensure_recv(void* buffer, size_t size) { recv(buffer, size).validate_recv(size); }

// Send an integer as a 32-bit value
TCPResult TCP::send_i32(int n) {
    int x = htonl(n); // Convert to network byte order (big-endian)
    return send(&x, sizeof(int)); // Send the data
}

// Receive a 32-bit integer and convert it from network byte order
TCPResponse<int> TCP::recv_i32() {
    int x;
    ensure_recv(&x, sizeof(int)); // Ensure the data is received
    return TCPResponse<int>::success(ntohl(x)); // Convert to host byte order and return
}

// Send an unsigned 8-bit integer
TCPResult TCP::send_u8(unsigned char n) { return send(&n, sizeof(unsigned char)); }

// Receive an unsigned 8-bit integer
TCPResponse<unsigned char> TCP::recv_u8() {
    unsigned char x;
    ensure_recv(&x, sizeof(unsigned char)); // Ensure the data is received
    return TCPResponse<unsigned char>::success(x); // Return received value
}

// Send a signed 8-bit integer
TCPResult TCP::send_i8(char n) { return send(&n, sizeof(char)); }

// Receive a signed 8-bit integer
TCPResponse<char> TCP::recv_i8() {
    char x;
    ensure_recv(&x, sizeof(char)); // Ensure the data is received
    return TCPResponse<char>::success(x); // Return received value
}

// Set the timeout for socket operations
void TCP::set_timeout(int seconds) { privates->set_timeout(seconds); }

// Get the port number
int TCP::get_port() const { return privates->get_port(); }

// Get the IP address
const char* TCP::get_ip() const { return privates->get_ip(); }

// Close the socket connection
void TCP::close() { privates->close(); }

// Destructor ensures that the private connection object is cleaned up
TCP::~TCP() { delete privates; }
