#include "search_server.h"

using std::string_literals::operator""s;

//------------------constructors-----------------------//

SearchServer::SearchServer(const std::string& stop_words)
{
    SetStopWords(SplitIntoWords(stop_words));
}

SearchServer::SearchServer(std::string_view stop_words)
{
    SetStopWords(SplitIntoWords(stop_words));
}

//--------------------private methods------------------//

bool SearchServer::IsStopWord(const std::string_view& word) const
{
    return stop_words_.count(static_cast<std::string>(word)) > 0;
}

bool SearchServer::IsValidWord(const std::string_view& word) const
{
    return std::none_of(word.begin(), word.end(), [](char c)
    {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const
{
    std::vector<std::string_view> words;
    for (const std::string_view& word : SplitIntoWords(text))
    {
        if (!IsValidWord(word))
        {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }

        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
    if (ratings.empty())
    {
        return 0;
    }

    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const
{
    if (text.empty())
    {
        throw std::invalid_argument("Query word is empty"s);
    }

    std::string_view word = text;
    bool is_minus = false;

    if (word[0] == '-')
    {
        is_minus = true;
        word = word.substr(1);
    }

    if (word.empty() || word[0] == '-' || !IsValidWord(word))
    {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid"s);
    }
    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const
{
    return ParseQuery(std::execution::seq, text, false);
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const
{
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void SearchServer::SortAndRemoveDublicates(std::vector<std::string_view>& dummy) const
{
    if (dummy.size() > 1)
    {
        const auto& begin_it = dummy.begin();
        const auto& end_it = dummy.end();
        std::sort(begin_it, end_it);
        auto it = std::unique(begin_it, end_it);
        dummy.resize(it - begin_it);
    }
}

//--------------------public methods------------------//

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings)
{
    using namespace std::literals::string_literals;
    if (document_id < 0)
    {
        throw std::invalid_argument( "Document id "s + std::to_string(document_id) + " is invalid (is negative)" );
    }
    if (documents_.count(document_id) > 0)
    {
       throw std::invalid_argument( "Document with such ID"s + std::to_string(document_id)  + "already exists" );
    }

    const auto words = SplitIntoWordsNoStop(document);
    std::vector<std::string> words_(words.begin(), words.end());
    const double inv_word_count = 1.0 / words.size();

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, words_ });

    for (const std::string& word : documents_.at(document_id).words_list)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        freqs_by_id_[document_id][word] += inv_word_count;
    }
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const
{
    return FindTopDocuments(raw_query, [&status]([[__maybe_unused__]]int document_id, DocumentStatus document_status,[[__maybe_unused__]] int rating)
    {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const
{    
    const auto query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;

    if (documents_.count(document_id) == 0)
    {
        throw std::out_of_range("Document out of range");
    }


    for (const std::string_view& word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) > 0 && word_to_document_freqs_.at(word).count(document_id) > 0)
        {
            matched_words.clear();
            continue;
        }
        else
        {
            for (const std::string_view& word : query.plus_words)
            {
                if (word_to_document_freqs_.count(word) > 0 && word_to_document_freqs_.at(word).count(document_id) > 0)
                {
                    matched_words.push_back(word);
                }
            }
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy, std::string_view raw_query, int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, std::string_view raw_query, int document_id) const
{
    if (documents_.count(document_id) == 0)
    {
        throw std::out_of_range("Wrong document id");
    }

    auto query = ParseQuery(policy, raw_query);

    const auto l = [this, document_id](std::string_view word)
    {
        const auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end() && it->second.count(document_id);
    };

    const auto& status = documents_.at(document_id).status;

    if (std::any_of(policy, query.minus_words.begin(), query.minus_words.end(), l))
    {
        return { std::vector<std::string_view>{}, status };
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());

    auto it = std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), l);

    matched_words.erase(it, matched_words.end());
    SortAndRemoveDublicates(matched_words);

    return { matched_words, status };
}

void SearchServer::RemoveDocument(int document_id)
{
    return RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id)
{
    const auto& doc_to_freq = freqs_by_id_.find(document_id);

    if (doc_to_freq == freqs_by_id_.end())
    {
        return;
    }

    const auto& words_freq = doc_to_freq->second;
    std::vector<const std::string*> words_to_remove(words_freq.size());

    std::transform(std::execution::par, words_freq.begin(), words_freq.end(), words_to_remove.begin(),
    [](const auto& word_to_freq)
    {
        return &word_to_freq.first;
    });

    std::for_each(std::execution::par, words_to_remove.begin(), words_to_remove.end(),
    [this, document_id](const auto& word_to_remove)
    {
        this->word_to_document_freqs_.at(*word_to_remove).erase(document_id);
    });

    freqs_by_id_.erase(doc_to_freq);
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy &, int document_id)
{
    if (document_ids_.count(document_id) == 0)
    {
        const auto & word_freq = GetWordFrequencies(document_id);
        for_each(std::execution::seq, word_freq.begin(), word_freq.end(), [&document_id, this](const auto& item)
        {
            word_to_document_freqs_.at(item.first).erase(document_id);
        ;});
    }

    document_ids_.erase(document_id);
    documents_.erase(document_id);
    freqs_by_id_.erase(document_id);

    return;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string_view& document, DocumentStatus status,
    const std::vector<int>& ratings)
{
    try
    {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::invalid_argument& e)
    {
        std::cout << "Error! Invalid document "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string_view& raw_query)
{
    std::cout << "Query results: "s << raw_query << std::endl;
    try
    {
        for (const Document& document : search_server.FindTopDocuments(raw_query))
        {
            PrintDocument(document);
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cout << "Search error has occured: "s << e.what() << std::endl;
    }
}

size_t SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    const auto it_to_doc = freqs_by_id_.find(document_id);

    const static std::map<std::string, double> map_to_return;

    if (it_to_doc == freqs_by_id_.end())
    {
        return map_to_return;
    }
    return it_to_doc->second;
}
