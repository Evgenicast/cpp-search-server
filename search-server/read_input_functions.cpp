#include "read_input_functions.h"

std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string> &words, DocumentStatus status)
{
    using namespace std::literals::string_literals;
    std::cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const std::string& word : words)
    {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}
