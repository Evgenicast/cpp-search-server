#pragma once
#include "document.h"
#include "search_server.h"
#include <deque>
#include <vector>
#include <string>

class RequestQueue
{
private:
    struct QueryResult
    {
        size_t requests_count = 0;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer & search_;
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
};

template <typename DocumentPredicate>
std::vector<Document>  RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate)
{
    std::vector<Document> doc = search_.FindTopDocuments(raw_query, document_predicate);
    if ((int)requests_.size() == min_in_day_)
    {
        requests_.pop_front();
    }
    requests_.push_back(QueryResult({doc.size()}));
    return doc;
}
