#include "utils.h"
#include "bout.h"

// Overload of the << operator to handle color output in the console
std::ostream& Utils::operator << (std::ostream& o, const Utils::Color& color)
{
    // Check if the output stream is std::cout (for console output)
    if (o.rdbuf() == std::cout.rdbuf())
    {
        // Change the text color using Windows API based on color code
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color.code);
    }
    return o;
}

// Function to find the first occurrence of character 'c' in the string 'buff' with given length 'len'
// Throws exception if the character is not found
const char* Utils::my_strnchr(const char* buff, int len, char c)
{
    // Loop through the string to find the character
    for (int i = 0; i < len && *buff && *buff != c; i++, buff++);

    // If the character was found, return the pointer to it
    if (*buff == c) return buff;

    // If the character was not found, throw an exception
    throw std::exception(bout() << "Failed to find character: '" << c << "'" << bfin);
}

// Function to convert a string to an integer, with basic error checking
int Utils::my_atoi(const char* input)
{
    constexpr int MAX_INPUT_LEN = 10;  // Maximum length for the input string
    long long result = 0;              // Store the parsed integer (using long long to avoid overflow)
    int sgn = 1;                       // Sign of the integer, 1 for positive, -1 for negative

    // Loop through the string, parsing one character at a time
    for (int i = 0; i < MAX_INPUT_LEN && *input; i++, input++)
    {
        // If the first character is '-', set the sign to negative
        if (i == 0 && *input == '-')
        {
            sgn = -1;
            continue;  // Move to the next character
        }

        // If the character is a digit, update the result
        if ('0' <= *input && *input <= '9')
        {
            result = result * 10 + (*input - '0');
            continue;
        }

        // If an invalid character is encountered, throw an exception
        throw std::exception(bout() << "Failed to parse integer: invalid character '" << *input << "'" << bfin);
    }

    // If we encountered extra characters, input length exceeded
    if (*input)
        throw std::exception("Failed to parse integer: input length exceeded");

    // Apply the sign and check if the result is within the valid integer range
    result *= sgn;
    if (result >= INT_MAX || result <= INT_MIN)
        throw std::exception(bout() << "Argument out of range: " << result << bfin);

    // Return the final parsed integer
    return (int)result;
}
