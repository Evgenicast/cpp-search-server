#pragma once
#include "document.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "string_processing.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <algorithm>


const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

class SearchServer
{
private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string, double>> freqs_by_id_;   //2 done
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    template <typename StringCollection>
    void SetStopWords(const StringCollection& stop_words);

    bool IsStopWord(const std::string& word) const;
    bool IsValidSymbol(const std::string& text) const;
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord
    {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string text) const;

    struct Query
    {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string& text) const;
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

public:
    SearchServer(){};
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    int GetDocumentCount() const;
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    auto begin() const   //1 done
    {
        return document_ids_.begin();
    }

    auto end() const
    {
        return document_ids_.end();
    }

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;   //2 done
    void RemoveDocument(int document_id);   // 3 done

};
//author's "MakeUnique" was removed and my old method SetStopWords was reinstated
template <typename StringCollection>
void SearchServer::SetStopWords(const StringCollection& stop_words)
{
    using std::string_literals::operator""s;

    for (const std::string& word : stop_words)
    {
        if (word != ""s)
        {
            if (!IsValidSymbol(word))
            {
                throw std::invalid_argument( "Invalid word"s + word + "has been added"s );
            }
            if (word == "-" || word[1] == '-')
            {
                throw std::invalid_argument( "Invalid stop words"s  + word + "found"s );
            }
            stop_words_.insert(std::string(word));
        }
        else
        {
            throw std::invalid_argument( "Empty string was passed to vector"s );
        }
    }
}

template <typename StringCollection>
SearchServer::SearchServer(const StringCollection& stop_words)
{
    SetStopWords(stop_words);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const
{
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
        {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating))
            {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word))
        {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance)
    {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const
{
    const Query query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);
    std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs)
    {
        if (std::abs(lhs.relevance - rhs.relevance) < EPSILON)
        {
            return lhs.rating > rhs.rating;
        }
        else
        {
            return lhs.relevance > rhs.relevance;
        }
    });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

