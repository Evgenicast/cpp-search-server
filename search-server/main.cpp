#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"
#include "remove_duplicates.h"
#include <iostream>

using namespace std;
void AddDocument(SearchServer & server, int document_id, const string & document, DocumentStatus status, const vector<int> & ratings)
{
    try
    {
        server.AddDocument(document_id, document, status, ratings);
    }
    catch ( const std::exception& e )
    {
        std::cout << "This shouldn't be called " << document_id << ": " << e.what() << std::endl;
    }
};

int main() {
    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // дубликат документа 2, будет удалён
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // добавились новые слова, дубликатом не является
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

    // есть не все слова, не является дубликатом
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

    // слова из разных документов, не является дубликатом
    AddDocument(search_server, 50, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    RemoveDuplicates(search_server);
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;

    search_server.GetWordFrequencies(2);
    search_server.RemoveDocument(50);

    for (const int document_id : search_server) {
       // ...
       cout << document_id << endl;
    }

//   Ожидаемый вывод этой программы:
//   Before duplicates removed: 9
//   Found duplicate document id 3
//   Found duplicate document id 4
//   Found duplicate document id 5
//   Found duplicate document id 7
//   After duplicates removed: 5

    return 0;
}
