#include "TCPResult.h"
#include <winsock.h>
#include "tcp_exception.h"
#include <bout.h>

// Method to retrieve the error message for a socket error, using the error code from the system
const char* TCPResult::get_error_message()
{
    wchar_t msgbuf[256]{};  // Buffer to hold the error message (wide characters)
    msgbuf[0] = '\0';  // Initialize the buffer to empty string

    // Format the error message from the system using the error code and store it in msgbuf
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        msgbuf,
        sizeof(msgbuf),
        NULL);

    // Return the formatted error message, appending the error code to the message
    return bout() << "Socket error " << error_code << ": " << msgbuf << bfin;
}

// Method to validate the result of a send operation
void TCPResult::validate_send(size_t desired_size)
{
    if (!ok)
        // If the operation was not successful, throw an exception with the error message
        throw tcp_exception(get_error_message());

    if (bytes_count != desired_size)
        // If the number of bytes sent doesn't match the desired size, throw an exception
        throw tcp_exception(bout() << "Not all bytes were sent (" << bytes_count << "/" << desired_size << ")" << bfin);
}

// Method to validate the result of a receive operation
void TCPResult::validate_recv(size_t desired_size)
{
    if (!ok)
        // If the operation was not successful, throw an exception with the error message
        throw tcp_exception(get_error_message());

    if (bytes_count == 0)
        // If no bytes were received, throw an exception indicating that the connection was interrupted
        throw tcp_exception("Connection interrupted during recv");

    if (bytes_count != desired_size)
        // If the number of bytes received doesn't match the desired size, throw an exception
        throw tcp_exception(bout() << "Not all bytes were sent (" << bytes_count << "/" << desired_size << ")" << bfin);
}
