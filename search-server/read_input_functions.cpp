#include "read_input_functions.h"

using namespace std::literals::string_literals;

std::string ReadLine()
{
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}
