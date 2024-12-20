#include <iostream>
#include <exception>

#include "FTPClient.h"
#include "FTPCommandInterpreter.h"

#include <string>
#include "tcp_exception.h"
#include "utils.h"
#include "ArgsParser.h"

using namespace std;

// Function that runs the FTP client and handles user input for commands
void run_client(const char* ip, int port)
{
    // Create an instance of FTPClient and initialize with IP and port
    FTPClient ftp_client(ip, port, printf);

    // Create an instance of FTPCommandInterpreter to process FTP commands
    FTPCommandInterpreter ci(&ftp_client);

    std::string cmd;
    while (1)  // Infinite loop to continuously accept user input
    {
        std::cout << ">> ";  // Prompt for user input
        std::getline(cin, cmd);  // Read a line of input from the user

        try
        {
            // Attempt to execute the entered command using the command interpreter
            ci.execute(cmd.c_str());
        }
        catch (exception& e)
        {
            // Catch exceptions thrown during command execution and display the error message
            cout << Utils::Color::Red() << e.what() << Utils::Color::White() << "\n";
        }
    }

    return;  // End of client loop (though it will never be reached due to infinite loop)
}

// Main function where the program starts
int main(int argc, const char** argv)
{
    try
    {
        // Parse command-line arguments using ArgsParser
        ArgsParser args(argc, argv);

        // Get the IP address and port from the command-line arguments (default to "127.0.0.1" and port 21)
        const char* ip = args.get_arg<const char*>(1, "127.0.0.1");
        int port = args.get_arg(2, 21);

        // Validate the IP address (ensure it is a valid length)
        if (Utils::get_str_bound(ip, 20) < 0)
            throw std::exception("Invalid IP");  // Throw exception if the IP is invalid

        // Call run_client to start the FTP client with the specified IP and port
        run_client(ip, port);
    }
    catch (exception& e)
    {
        // Catch and display any exceptions that occur during setup or execution
        cout << e.what() << "\n";
    }
}
