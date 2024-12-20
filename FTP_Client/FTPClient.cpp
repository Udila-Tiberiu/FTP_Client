#include "FTPClient.h"
#include <iostream>
#include "utils.h"
#include "bout.h"

// Constructor for FTPClient, initializes connection and filesystem
FTPClient::FTPClient(const char* ip, int port, std::function<void(const char*)> print_line)
{
    // Store the print_line callback
    this->print_line = print_line;

    // Create a callback function for line received from server
    std::function<void(const char*)> line_rec_cb = std::bind(&FTPClient::line_received_callback, this, std::placeholders::_1);

    // Initialize TelNetClient for communication with server
    telnet_client = new TelNetClient(ip, port, line_rec_cb);

    // Set initial connection state
    connected = true;

    // Create VirtualFS object for file system operations
    filesystem = new VirtualFS("vfs_root");
}

// Callback for processing received lines from the server
void FTPClient::line_received_callback(const char* line)
{
    // Store the received line in a buffer
    strncpy_s(line_buffer, line, sizeof(line_buffer));

    // Print the received line with special formatting
    std::cout << Utils::Color::Yellow();
    print_line(line);
    std::cout << Utils::Color::White();
}

// Wrapper function to send commands to the server and handle output
int FTPClient::send_command_wrapper(const char* cmd)
{
    // Print the command to be sent with blue formatting
    std::cout << Utils::Color::Blue() << cmd << Utils::Color::White() << "\n";

    // Send the command through the TelNet client
    return telnet_client->send_command(cmd);
}

// Function to log in to the FTP server using a given username and password
void FTPClient::login(const char* user, const char* pass)
{
    // Reconnect if not already connected
    if (!connected)
    {
        telnet_client->reconnect();
        connected = true;
    }

    // Send USER command and check for 331 response (username okay)
    if (send_command_wrapper(bout() << "USER " << user << bfin) != 331)
    {
        throw std::exception("login failed");
    }

    // Send PASS command and check for 230 response (logged in)
    if (send_command_wrapper(bout() << "PASS " << pass << bfin) != 230)
    {
        throw std::exception("login failed");
    }
}

// Function to log out from the FTP server
void FTPClient::logout()
{
    // Send QUIT command and check for 221 response (logged out)
    if (send_command_wrapper("QUIT") != 221)
    {
        throw std::exception("logout failed");
    }

    // Set connection state to false and close the TelNet client
    connected = false;
    telnet_client->close();
}

// Function to list files in the specified directory (or current directory if no path given)
void FTPClient::list(const char* path)
{
    // Send LIST command with the provided path (or just LIST for current directory)
    int resp = path == nullptr ? send_command_wrapper("LIST") : send_command_wrapper(bout() << "LIST " << path << bfin);

    // Check for 150 response (start of data transfer)
    if (resp != 150)
        throw std::exception("Failed");

    // Temporary buffer for receiving data
    std::vector<char> tmp_buffer(1024);
    std::vector<char> buffer;
    int tmp_effective_size = 0;

    // Receive data from the server and store it in the buffer
    while ((tmp_effective_size = data_port.recv(tmp_buffer.data(), tmp_buffer.size()).bytes_count) > 0)
    {
        buffer.insert(buffer.end(), tmp_buffer.begin(), tmp_buffer.begin() + tmp_effective_size);
    }

    // Close the data connection
    data_port.close();

    // Null-terminate and print the received data
    buffer.push_back('\0');
    printf(buffer.data());

    // Check for 226 response (successful transfer)
    if (telnet_client->recv_response() != 226)
        throw std::exception("Failed transfer");
}

// Function to set the transfer mode to binary
void FTPClient::mode_binary()
{
    // Send TYPE I command for binary mode
    if (send_command_wrapper("TYPE I") != 200)
        throw std::exception("Failed");
}

// Function to set the transfer mode to ASCII
void FTPClient::mode_ascii()
{
    // Send TYPE A command for ASCII mode
    if (send_command_wrapper("TYPE A") != 200)
        throw std::exception("Failed");
}

// Function to store (upload) a file to the server
void FTPClient::stor(const char* path)
{
    std::vector<char> buffer;

    try
    {
        // Read the file from the virtual file system
        buffer = filesystem->read(path);
    }
    catch (const std::exception& e)
    {
        data_port.close();
        throw e;
    }

    // Send STOR command to initiate file upload
    if (send_command_wrapper(bout() << "STOR " << path << bfin) != 150)
        throw std::exception("Failed");

    // Send the file data through the data connection
    data_port.send(buffer.data(), buffer.size());
    data_port.close();

    // Check for 226 response (successful transfer)
    if (telnet_client->recv_response() != 226)
        throw std::exception("Failed transfer");
}

// Function to retrieve (download) a file from the server
void FTPClient::retr(const char* path)
{
    // Send RETR command to retrieve the file
    if (send_command_wrapper(bout() << "RETR " << path << bfin) != 150)
        throw std::exception("Failed");

    // Temporary buffer for receiving data
    std::vector<char> tmp_buffer(1024);
    std::vector<char> buffer;
    int tmp_effective_size = 0;

    // Receive data from the server and store it in the buffer
    while ((tmp_effective_size = data_port.recv(tmp_buffer.data(), tmp_buffer.size()).bytes_count) > 0)
    {
        buffer.insert(buffer.end(), tmp_buffer.begin(), tmp_buffer.begin() + tmp_effective_size);
    }

    // Close the data connection
    data_port.close();

    // Write the received data to the virtual file system
    filesystem->write(path, buffer);

    // Check for 226 response (successful transfer)
    if (telnet_client->recv_response() != 226)
        throw std::exception("Failed transfer");
}

namespace
{
    // Helper function to parse PASV address response from server
    void parse_pasv_addr(const char* buff, int a[6])
    {
        constexpr int maxStrLen = 4 * 6;
        int i = 0, k = 0;

        // Parse the address string
        for (; buff[k] != ')' && k < maxStrLen; k++)
        {
            if ('0' <= buff[k] && buff[k] <= '9')
            {
                a[i] = a[i] * 10 + (buff[k] - '0');
                continue;
            }
            if (buff[k] == ',')
            {
                if (i >= 6)
                    throw std::exception("Failed to parse PASV address: too many numbers");
                i++;
                continue;
            }
            throw std::exception(bout() << "Failed to parse PASV address: invalid character '0x" << bhex << buff[i] << "'" << bfin);
        }

        // Ensure the correct number of values were parsed
        if (buff[k] != ')')
            throw std::exception("Failed to parse PASV address: input too long");
        else
        {
            if (i >= 6)
                throw std::exception("Failed to parse PASV address: too many numbers");
            i++;
        }

        if (i < 6)
            throw std::exception("Failed to parse PASV address: insufficient numbers");
    }
}

// Function to enter passive mode for data transfer
void FTPClient::pasv()
{
    // Send PASV command and check for 227 response
    if (send_command_wrapper("PASV") != 227)
        throw std::exception("Entering passive mode failed");

    // Extract the passive mode address from the response
    const char* line_pfx = "227 Entering Passive Mode (";
    if (strncmp(line_pfx, line_buffer, strlen(line_pfx)) != 0)
        throw std::exception("Invalid passive response message");
    char* buff = line_buffer + strlen(line_pfx);

    int a[6]{};
    parse_pasv_addr(buff, a);

    // Build the IP address and port from the parsed data
    const char* ip = bout() << a[0] << "." << a[1] << "." << a[2] << "." << a[3] << bfin;
    int port = a[4] * 256 + a[5];

    // Connect to the data port
    data_port.connect(ip, port);
    printf("Opened data port on %s:%i.\n", (const char*)ip, port);
}

// Destructor to clean up resources
FTPClient::~FTPClient()
{
    delete telnet_client;  // Delete the TelNet client
    delete filesystem;     // Delete the virtual file system
}
