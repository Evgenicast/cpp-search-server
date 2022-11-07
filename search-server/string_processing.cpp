#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view text)
{
    std::vector<std::string_view> words;

    auto word_begin_iter = text.begin();
    auto word_end_iter = text.end();


    for (auto i = text.begin(); i < text.end(); ++i)
    {
        if (*i == ' ')
        {
            word_end_iter = i;
            if (*word_begin_iter != ' ' && word_begin_iter != word_end_iter)
            {
                words.push_back(text.substr(
                    std::distance(text.begin(), word_begin_iter),
                    std::distance(word_begin_iter, word_end_iter)));
            }
            word_begin_iter = std::next(word_end_iter);
        }
    }
    if (word_begin_iter != word_end_iter)
    {
        words.push_back(text.substr(word_begin_iter - text.begin(),
            std::distance(word_begin_iter, word_end_iter)));
    }
    return words;
}
