#include "search_server.h"
#include <algorithm>
#include <numeric>
#include <cmath>

//------------------constructors--------------//
SearchServer::SearchServer(const std::string &stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)){}

//--------------------private methods------------------//

bool SearchServer::IsStopWord(const std::string &word) const
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsSpaceAfterMinus(const std::string &text) const
{
    bool minus = false;
    for (char c : text)
    {
        if (c == '-')
        {
            minus = true;
        }
        else
        {
            if ((c == ' ') && (minus == true))
            {
                return false;
            }
            minus = false;
        }
    }
    return true;
}

bool SearchServer::IsValidSymbol(const std::string &text)
{
    return (std::none_of(text.begin(), text.end(), [text](char x)
    {
        return ((x < 31) && (x > 0) ? true : false);
    }));
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string &text) const
{
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text))
    {
        if (!IsValidSymbol(word))
        {
            throw std::invalid_argument( "Invalid word format or forbidden symbols"s + word + "have been added"s );
        }
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int> &ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string &word) const
{
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

SearchServer::Query SearchServer::ParseQuery(const std::string &text) const
{
    Query query;
    for (const std::string& word : SplitIntoWords(text))
    {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                query.minus_words.insert(query_word.data);
            }
            else
            {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const
{
    if (!IsSpaceAfterMinus(text))
    {
        throw std::invalid_argument( "Extra space after minus is not allowed" );
    }
    bool is_minus = false;
    if (text[0] == '-')
    {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-')
    {
        throw std::invalid_argument( "Parsing query "s + text + " is invalid (extra minus in front)"s );
    }
    if (!IsValidSymbol(text))
    {
        throw std::invalid_argument( "Query "s + text + " is invalid (contains forbidden symbols)"s );
    }
    return QueryWord{text, is_minus, IsStopWord(text)};
}

//---------------------public methods----------------------------//

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings)
{
    if (document_id < 0)
    {
        throw std::invalid_argument( "Document id "s + std::to_string(document_id) + " is invalid (is negative)" );
    }

    if (documents_.count(document_id) > 0)
    {
        throw std::invalid_argument( "Document with such ID"s + std::to_string(document_id)  + "already exists" );
    }

    std::vector<std::string> words =  SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        freqs_by_id_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string &raw_query, DocumentStatus status) const
{
    return FindTopDocuments(raw_query, [&status]([[maybe_unused]]int document_id, DocumentStatus document_status, [[maybe_unused]]int rating)
    {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string &raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string &raw_query, int document_id) const
{
    const Query query = ParseQuery((raw_query));
    std::vector<std::string> matched_words;

    for (const std::string& word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.push_back(word);
        }
    }

    for (const std::string& word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

const std::map<std::string, double> &SearchServer::GetWordFrequencies(int document_id) const
{
    static std::map<std::string, double> tmp_;
    const auto iter = freqs_by_id_.find(document_id);

    if (iter == freqs_by_id_.end())
    {
        return tmp_;
    }
    return iter->second;
}

void SearchServer::RemoveDocument(int document_id)
{
    for (auto & word : word_to_document_freqs_)
    {
        word.second.erase(document_id);
    }

    document_ids_.erase(document_id);
    freqs_by_id_.erase(document_id);
    documents_.erase(document_id);
}
