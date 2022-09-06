#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer &search_server)
    : search_(search_server)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentStatus status)
{
    const auto doc = search_.FindTopDocuments(raw_query);
    if ((int)requests_.size() == min_in_day_)
    {
        requests_.pop_front();
    }
    requests_.push_back(QueryResult({doc.size()}));
    return doc;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query)
{
    const auto doc = search_.FindTopDocuments(raw_query);
    if ((int)requests_.size() == min_in_day_)
    {
        requests_.pop_front();
    }
    requests_.push_back(QueryResult({doc.size()}));
    return doc;
}

int RequestQueue::GetNoResultRequests() const
{
    int time = 0;
    for (const auto & request : requests_)
    {
        if (request.requests_count == 0)
        {
            ++time;
        }
    }
    return time;
}
