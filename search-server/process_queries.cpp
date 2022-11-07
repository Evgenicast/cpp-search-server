#include "process_queries.h"
#include <list>
std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> doc_to_return(queries.size()) ;
    std::transform(std::execution::par, (queries.begin()), queries.end(), doc_to_return.begin(), [&search_server](const std::string & str)
   {
       return search_server.FindTopDocuments(str);
   });

   return doc_to_return;
}

std::list<Document> ProcessQueriesJoined(const SearchServer &search_server, const std::vector<std::string> &queries)
{
    std::list<Document> doc_to_return;

    for (auto & documents : ProcessQueries(search_server, queries))
    {
        for (auto & document : documents)
        {
            doc_to_return.push_back(std::move(document));
        }
    }
    return doc_to_return;
}
