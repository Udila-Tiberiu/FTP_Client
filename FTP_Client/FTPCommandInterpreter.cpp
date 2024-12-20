#include "FTPCommandInterpreter.h"

#include <functional>

// Macro to bind commands to specific FTP methods via lambda functions.
#define LAMBDA(ci, ftp, fname) ((std::function<void(const Parameter*)>)std::bind(fname, ci, ftp, std::placeholders::_1))

namespace
{
	// Command implementation for 'login' command: logs in the FTP client with a username and password
	void cmd_login(CommandInterpreter* ci, FTPClient* ftp, const Parameter* pms)
	{
		const char* user = pms[0].get_value_str();  // Get the username parameter
		const char* pass = pms[1].get_value_str();  // Get the password parameter
		ftp->login(user, pass);  // Perform FTP login
	}

	// Command implementation for 'logout' command: logs out the FTP client
	void cmd_logout(CommandInterpreter* ci, FTPClient* ftp, const Parameter* pms)
	{
		ftp->logout();  // Perform FTP logout
	}

	// Command implementation for 'help' command: displays available commands
	void cmd_help(CommandInterpreter* ci, FTPClient* ftp, const Parameter* pms)
	{
		ci->print_commands(std::cout);  // Print the available commands
	}

	// Command implementation for 'list1' command: lists files at a specific path
	void cmd_list1(CommandInterpreter* ci, FTPClient* ftp, const Parameter* pms)
	{
		const char* path = pms[0].get_value_str();  // Get the path parameter
		ftp->pasv();  // Enable passive mode for FTP
		ftp->list(path);  // List files in the specified path
	}

	// Command implementation for 'list0' command: lists files with no specific path (default)
	void cmd_list0(CommandInterpreter* ci, FTPClient* ftp, const Parameter* pms)
	{
		ftp->pasv();  // Enable passive mode for FTP
		ftp->list(nullptr);  // List files in the current directory
	}

	// Command implementation for 'pasv' command: enables passive mode for FTP
	void cmd_pasv(CommandInterpreter* ci, FTPClient* ftp, const Parameter* pms)
	{
		ftp->pasv();  // Enable passive mode for FTP
	}

	// Command implementation for 'put' command: uploads a file to the FTP server
	void cmd_put(CommandInterpreter* ci, FTPClient* ftp, const Parameter* pms)
	{
		const char* path = pms[0].get_value_str();  // Get the path parameter (file to upload)
		ftp->pasv();  // Enable passive mode for FTP
		ftp->stor(path);  // Upload the specified file
	}

	// Command implementation for 'retr' command: downloads a file from the FTP server
	void cmd_retr(CommandInterpreter* ci, FTPClient* ftp, const Parameter* pms)
	{
		const char* path = pms[0].get_value_str();  // Get the path parameter (file to download)
		ftp->pasv();  // Enable passive mode for FTP
		ftp->retr(path);  // Download the specified file
	}

	// Command implementation for 'binary' command: sets the FTP mode to binary
	void cmd_binary(CommandInterpreter* ci, FTPClient* ftp, const Parameter* pms)
	{
		ftp->mode_binary();  // Switch to binary transfer mode
	}

	// Command implementation for 'ascii' command: sets the FTP mode to ASCII
	void cmd_ascii(CommandInterpreter* ci, FTPClient* ftp, const Parameter* pms)
	{
		ftp->mode_ascii();  // Switch to ASCII transfer mode
	}

}

FTPCommandInterpreter::FTPCommandInterpreter(FTPClient* ftp) : ftp{ ftp }
{
	// Register each FTP command with the associated handler function and parameters
	// Register 'login' command with user and pass parameters
	register_command(LAMBDA(this, ftp, cmd_login), "login", Param(0, "user", ParameterType::STRING), Param(1, "pass", ParameterType::STRING));
	// Register 'help' command
	register_command(LAMBDA(this, ftp, cmd_help), "help");
	// Register 'logout' command
	register_command(LAMBDA(this, ftp, cmd_logout), "logout");
	// Register 'list' command with a specific path parameter
	register_command(LAMBDA(this, ftp, cmd_list1), "list", Param(0, "path", ParameterType::PATH));
	// Register 'list' command with no path (current directory)
	register_command(LAMBDA(this, ftp, cmd_list0), "list");
	// Register 'put' command for file upload with a path parameter
	register_command(LAMBDA(this, ftp, cmd_put), "put", Param(0, "path", ParameterType::PATH));
	// Register 'get' command for file download with a path parameter
	register_command(LAMBDA(this, ftp, cmd_retr), "get", Param(0, "path", ParameterType::PATH));
	// Register 'ascii' command to switch to ASCII mode
	register_command(LAMBDA(this, ftp, cmd_ascii), "ascii");
	// Register 'binary' command to switch to binary mode
	register_command(LAMBDA(this, ftp, cmd_binary), "binary");
}
