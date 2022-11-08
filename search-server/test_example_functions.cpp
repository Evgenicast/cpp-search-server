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
void TestMatchingDocuments(void)
{
    SearchServer server;

    server.AddDocument(0, "Hello world"s, DocumentStatus::ACTUAL, { 5, 3, 10 }); // Avarage rating = 6
    server.AddDocument(1, "The second world"s, DocumentStatus::ACTUAL, { -10, -15, -20 }); // Avarage rating = -15
    server.AddDocument(2, "Third doc in server"s, DocumentStatus::BANNED, { 1, 0, 0 }); // Avarage rating = 0
    server.AddDocument(3, "Another document in server"s, DocumentStatus::IRRELEVANT, { 1, 2, 3 }); // Avarage rating = 2
    server.AddDocument(4, "Removed doc in server"s, DocumentStatus::REMOVED, { 3, 10, 20 }); // Avarage rating = 11

    std::tuple<std::vector<std::string_view>, DocumentStatus> test_tuple;
    std::tuple<std::vector<std::string_view>, DocumentStatus> testMatching = server.MatchDocument(""s, 0);

    ASSERT(testMatching == test_tuple);
    std::vector<std::string_view> blankvectorOfWords;

    testMatching = server.MatchDocument("Test query"s, 0);
    ASSERT(testMatching == test_tuple);

    {// Тест пустого возврата при нахождении минус слова и верного статуса
        testMatching = server.MatchDocument("second the -world"s, 1);
        test_tuple = { blankvectorOfWords, DocumentStatus::ACTUAL };
        ASSERT(testMatching == test_tuple);

        testMatching = server.MatchDocument("third -in doc"s, 2);
        test_tuple = { blankvectorOfWords, DocumentStatus::BANNED };
        ASSERT(testMatching == test_tuple);

        testMatching = server.MatchDocument("-Another document"s, 3);
        test_tuple = { blankvectorOfWords, DocumentStatus::IRRELEVANT };
        ASSERT(testMatching == test_tuple);

        testMatching = server.MatchDocument("server -Removed"s, 4);
        test_tuple = { blankvectorOfWords, DocumentStatus::REMOVED };
        ASSERT(testMatching == test_tuple);
    }

    {// Тест пустого возврата при нахождении минус слова и верного статуса
        testMatching = server.MatchDocument(std::execution::par, "second the -world"s, 1);
        test_tuple = { blankvectorOfWords, DocumentStatus::ACTUAL };
        ASSERT(testMatching == test_tuple);

        testMatching = server.MatchDocument(std::execution::par, "third -in doc"s, 2);
        test_tuple = { blankvectorOfWords, DocumentStatus::BANNED };
        ASSERT(testMatching == test_tuple);

        testMatching = server.MatchDocument(std::execution::par, "-Another document"s, 3);
        test_tuple = { blankvectorOfWords, DocumentStatus::IRRELEVANT };
        ASSERT(testMatching == test_tuple);

        testMatching = server.MatchDocument(std::execution::par, "server -Removed"s, 4);
        test_tuple = { blankvectorOfWords, DocumentStatus::REMOVED };
        ASSERT(testMatching == test_tuple);
    }

    {// Тест возврата слов и статусов
        std::vector<std::string> vec_words{ {"The"s, "second"s} };
        std::vector<std::string_view> temp(vec_words.begin(), vec_words.end());
        std::string source{ "second The"s };
        testMatching = server.MatchDocument(source, 1);
        test_tuple = { temp, DocumentStatus::ACTUAL };
        ASSERT(std::get<0>(testMatching) == std::get<0>(test_tuple));
        ASSERT(std::get<1>(testMatching) == std::get<1>(test_tuple));
        ASSERT(testMatching == test_tuple);
    }

    {// Тест возврата слов и статусов
        std::vector<std::string> vec_words{ {"The"s, "second"s} };
        std::vector<std::string_view> temp(vec_words.begin(), vec_words.end());
        std::string source{ "second The"s };
        testMatching = server.MatchDocument(std::execution::par, source, 1);
        test_tuple = { temp, DocumentStatus::ACTUAL };
        ASSERT(std::get<0>(testMatching) == std::get<0>(test_tuple));
        ASSERT(std::get<1>(testMatching) == std::get<1>(test_tuple));
        ASSERT(testMatching == test_tuple);
    }

    {
        std::vector<std::string> vec_words{ {"Third"s, "doc"s, "in"s} };
        std::vector<std::string_view> temp(vec_words.begin(), vec_words.end());
        std::string source{ "Third in doc"s };
        testMatching = server.MatchDocument(source, 2);
        test_tuple = { temp, DocumentStatus::BANNED };
        ASSERT(testMatching == test_tuple);
    }

    {
        std::vector<std::string> vec_words{ {"Another"s, "document"s} };
        std::vector<std::string_view> temp(vec_words.begin(), vec_words.end());
        std::string source{ "Another document"s };
        testMatching = server.MatchDocument(source, 3);
        test_tuple = { temp, DocumentStatus::IRRELEVANT };
        ASSERT(testMatching == test_tuple);
    }

    {
        std::vector<std::string> vec_words{ {"Removed"s, "server"s} };
        std::vector<std::string_view> temp(vec_words.begin(), vec_words.end());
        std::string source{ "server Removed"s };
        testMatching = server.MatchDocument(source, 4);
        test_tuple = { temp , DocumentStatus::REMOVED };
        ASSERT(testMatching == test_tuple);
    }

    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    try
    {
        testMatching = server.MatchDocument("Another incorrect query in server -"s, doc_id);
        ASSERT_HINT(false, "Должно было сработать исключение при сравнении документа с некорректным запросом"s);
    }
    catch (const std::exception&)
    {
    }

    try
    {
        testMatching = server.MatchDocument("Another \x1incorrect query in server"s, doc_id);
        ASSERT_HINT(false, "Должно было сработать исключение при сравнении документа с некорректным запросом"s);
    }
    catch (const std::exception&)
    {
    }

    try
    {
        testMatching = server.MatchDocument("Another incorrect query in --server"s, doc_id);
        ASSERT_HINT(false, "Должно было сработать исключение при сравнении документа с некорректным запросом"s);
    }
    catch (const std::exception&)
    {
    }

    try
    {
        testMatching = server.MatchDocument("Another correct document-- in server"s, doc_id);
    }
    catch (const std::exception&)
    {
        ASSERT_HINT(false, "Не должно было сработать исключение при сравнении документа с корректным запросом"s);
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
