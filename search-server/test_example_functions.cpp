#include "test_example_functions.h"

using std::string_literals::operator""s;

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint)
{
    if (!value)
    {
        std::cerr << file << "("s << line << "s): "s << func << ": "s;
        std::cerr << "ASSERT("s << expr_str << "s) failed."s;
        if (!hint.empty())
        {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
    }
}

void TestExcludeStopWordsFromAddedDocumentContent() {

    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestMatchedDocuments()
{
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};

    std::tuple<std::vector<std::string>, DocumentStatus> matched_documents;

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        matched_documents = server.MatchDocument("cat in"s, doc_id);
        std::vector<std::string> s;
        s = std::get<0>(matched_documents);
        ASSERT_EQUAL(s.size(), 2u);
        ASSERT_HINT(server.FindTopDocuments("-the"s).empty(), "Minus-words should be excluded from documents");
        DocumentStatus status;
        status = std::get<1>(matched_documents);
        ASSERT(status == DocumentStatus::ACTUAL);
    }
}

void TestAverageRating()
{
    SearchServer server("и в на"s);

    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);

    {
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.rating, 5);
        ASSERT_EQUAL(doc0.id, 1);
        const Document& doc2 = found_docs[2];
        ASSERT_EQUAL(doc2.rating, -1);
        ASSERT_EQUAL(doc2.id, 2);
    }
}

void TestStatus()
{
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};

    std::tuple<std::vector<std::string>, DocumentStatus> matched_documents;

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        matched_documents = server.MatchDocument("cat in"s, doc_id);
        DocumentStatus status;
        status = std::get<1>(matched_documents);
        ASSERT(status == DocumentStatus::ACTUAL);
    }
}
void TestPredicateFilter()
{
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s, DocumentStatus::BANNED );
        ASSERT(found_docs.empty());
        const auto found_docs_1 = server.FindTopDocuments("in"s, [](int document_id, [[maybe_unused]] DocumentStatus status, [[maybe_unused]] int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(found_docs_1.size(), 1u);
    }
}

void TestCorrectRelevanceCount() {

    SearchServer server("и в на"s);

    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    const Document& doc0 = found_docs.at(0);
    const Document& doc1 = found_docs.at(1);

    if (std::abs(found_docs.at(0).relevance - found_docs.at(1).relevance) < 1e-6)
    {
        ASSERT (doc0.relevance > doc1.relevance || doc0.rating > doc1.rating);
    }
}
