#include "TelNetClient.h"
#include <exception>
#include <bout.h>

// Constructor to initialize the TelNetClient with the server IP, port, and a callback function for line reception
TelNetClient::TelNetClient(const char* ip, int port, std::function<void(char*)> line_received_callback)
    : line_received_callback{ line_received_callback }, ip{ ip }, port{ port }
{
    try
    {
        // Establish TCP connection with the provided IP and port
        tcp.connect(ip, port);
        tcp.set_timeout(3);  // Set timeout for the connection

        // Print the connection details
        printf("Client: %s:%i\n", tcp.get_ip(), tcp.get_port());

        // Receive the server greeting response
        recv_response();
    }
    catch (std::exception& e)
    {
        // If an exception occurs during connection, propagate the exception
        throw e;
    }
}

// Reconnect method to handle reconnection to the server
void TelNetClient::reconnect()
{
    printf("Reconnecting...\n");

    // If already connected, close the current connection before reconnecting
    if (is_connected)
        close();

    // Reconnect to the server
    tcp.connect(ip, port);

    // Receive the server greeting again after reconnection
    recv_response();
    is_connected = true;
}

// Close the connection by setting is_connected to false and closing the TCP connection
void TelNetClient::close()
{
    is_connected = false;
    tcp.close();
}

// Send a command to the server and receive the response
int TelNetClient::send_command(const char* command)
{
    // Format the command by appending carriage return and newline
    command = bout() << command << "\r\n" << bfin;

    size_t len = strlen(command);  // Get the length of the command string

    // Send the command to the server over TCP
    tcp.send(command, len);

    // Receive and return the server's response code
    return recv_response();
}

namespace
{
    // Helper function to read a line from the TCP connection
    void read_line(TCP& tcp, char* buffer)
    {
        char* c = buffer;
        // Read characters from the TCP connection until a newline character is encountered
        for (; (*c = tcp.recv_i8()) != 0x0A; c++);
        *(++c) = '\0';  // Null-terminate the string after the newline
    }
}

namespace
{
    // Helper function to convert a 3-digit response code to an integer
    int response_code_to_int(const char* code)
    {
        return (code[0] - '0') * 100 + (code[1] - '0') * 10 + (code[2] - '0');
    }
}

// Method to receive a response from the server, including handling multi-line responses
int TelNetClient::recv_response()
{
    char first_line[2048] = { 0 };  // Buffer to store the first line of the response
    char buffer[2048] = { 0 };      // Buffer to store subsequent lines

    // Read the first line of the response from the server
    read_line(tcp, first_line);

    // Invoke the callback function with the first line
    line_received_callback(first_line);

    // If the first line contains a response code (3 digits followed by a space), return the response code
    if (first_line[3] == ' ') return response_code_to_int(first_line);

    // If the first line doesn't contain a response code, keep reading lines until we find one
    while (buffer[0] != first_line[0] || buffer[1] != first_line[1] || buffer[2] != first_line[2] || buffer[3] != ' ')
    {
        // Read the next line of the response
        read_line(tcp, buffer);

        // Invoke the callback function with the new line
        line_received_callback(buffer);
    }

    // Return the response code from the first line
    return response_code_to_int(first_line);
}
