#include "remove_duplicates.h"

using std::string_literals::operator""s;

void RemoveDuplicates(SearchServer &search_server)
{
    std::map<std::set<std::string>, std::vector<int>> no_duplicates;
    std::set<int> duplicates;
    for (const int document_id : search_server)
    {
        std::set<std::string> words;
        std::map<std::string, double> word_with_freq = search_server.GetWordFrequencies(document_id);
        for (auto &[word, freq] : word_with_freq)
        {
            words.insert(word);
        }

        if (no_duplicates.count(words) == 0)
        {
            no_duplicates[words].push_back(document_id);
        }
        else
        {
            duplicates.insert(document_id);
            std::cout << "Found duplicates removed " << document_id << std::endl;
        }
    }

    for (const int & doc : duplicates)
    {
        search_server.RemoveDocument(doc);
    }
}
