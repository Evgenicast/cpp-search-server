#include "request_queue.h"
#include "process_queries.h"
#include "paginator.h"
#include "remove_duplicates.h"
#include "log_duration.h"
#include <random>
#include "search_server.h"
#include <execution>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}
vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}
string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}
vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}
template <typename ExecutionPolicy>
void Test(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << endl;
}
#define TEST(policy) Test(#policy, search_server, queries, execution::policy)
void PrintDocument2(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const string& text : {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            "nasty pigeon john"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }
    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument2(document);
    }
    cout << "BANNED:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument2(document);
    }
    cout << "Even ids:"s << endl;
    // параллельная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument2(document);
    }

    mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
    SearchServer search_server2(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server2.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
    }
    const auto queries = GenerateQueries(generator, dictionary, 100, 70);
    TEST(seq);
    TEST(par);

    return 0;
}
//int main() {
//    SearchServer search_server("and with"s);
//    int id = 0;
//     for (
//         const string& text : {
//             "funny pet and nasty rat"s,
//             "funny pet with curly hair"s,
//             "funny pet and not very nasty rat"s,
//             "pet with rat and rat and rat"s,
//             "nasty rat with curly hair"s,
//         }
//     ) {
//         search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
//     }

//     const string query = "curly and funny -not"s;

//     {
//         const auto [words, status] = search_server.MatchDocument(query, 1);
//         cout << words.size() << " words for document 1"s << endl;
//         // 1 words for document 1
//     }

//     {
//         const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
//         cout << words.size() << " words for document 2"s << endl;
//         // 2 words for document 2
//     }

//     {
//         const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
//         cout << words.size() << " words for document 3"s << endl;
//         // 0 words for document 3
//     }
//    return 0;
//}
//5 documents total, 4 documents for query [curly and funny]
//4 documents total, 3 documents for query [curly and funny]
//3 documents total, 2 documents for query [curly and funny]
//2 documents total, 1 documents for query [curly and funny]

//Вывод:
//3 documents for query [nasty rat -not]
//5 documents for query [not very funny nasty pet]
//2 documents for query [curly hair]

//void AddDocument(SearchServer & server, int document_id, const string & document, DocumentStatus status, const vector<int> & ratings)
//{
//    try
//    {
//        server.AddDocument(document_id, document, status, ratings);
//    }
//    catch ( const std::exception& e )
//    {
//        std::cout << "This shouldn't be called " << document_id << ": " << e.what() << std::endl;
//    }
//};

//int main() {
//    SearchServer search_server("and with"s);

//    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
//    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

//    // дубликат документа 2, будет удалён
//    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

//    // отличие только в стоп-словах, считаем дубликатом
//    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

//    // множество слов такое же, считаем дубликатом документа 1
//    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

//    // добавились новые слова, дубликатом не является
//    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

//    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
//    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

//    // есть не все слова, не является дубликатом
//    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

//    // слова из разных документов, не является дубликатом
//    AddDocument(search_server, 50, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

//    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
//    RemoveDuplicates(search_server);
//    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;

//    search_server.GetWordFrequencies(2);
//    search_server.RemoveDocument(50);

//    for (const int document_id : search_server) {
//       // ...
//       cout << document_id << endl;
//    }

//   Ожидаемый вывод этой программы:
//   Before duplicates removed: 9
//   Found duplicate document id 3
//   Found duplicate document id 4
//   Found duplicate document id 5
//   Found duplicate document id 7
//   After duplicates removed: 5

//    return 0;
//}
