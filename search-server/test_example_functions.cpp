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
        abort();
    }
}

// -------- Начало модульных тестов поисковой системы ----------
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent()
{
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

// Тест проверяет что поисковая система исключает из выдачи документы со стоп словами
void TestMinusWordsExcludeDocumentFromQuery(void)
{
    SearchServer server;

    server.AddDocument(0, "Hello world"s, DocumentStatus::ACTUAL, { 5, 3, 10 }); // Avarage rating = 6
    server.AddDocument(1, "The second world"s, DocumentStatus::ACTUAL, { -10, -15, -20 }); // Avarage rating = -15
    server.AddDocument(2, "Third doc in server"s, DocumentStatus::ACTUAL, { 1, 0, 0 }); // Avarage rating = 0
    server.AddDocument(3, "Another document in server"s, DocumentStatus::ACTUAL, { 1, 2, 3 }); // Avarage rating = 2

    std::vector<struct Document> document = server.FindTopDocuments("-Hello world"s);
    int i = 0;
    ASSERT_EQUAL(document[i].id, 1);
    ASSERT_EQUAL(document[i].rating, -15);
    ASSERT_EQUAL(document[i].relevance, 0.23104906018664842);
    ASSERT_EQUAL_HINT(document.size(), 1u, "Только 1 документ должен быть найден"s);
    document.clear();

    document = server.FindTopDocuments("Hello -world"s);
    ASSERT_HINT(document.empty(), "Не должно быть найденных документов"s);
    document.clear();

    try
    {
        document = server.FindTopDocuments("Hello - world"s);
        ASSERT_HINT(false, "Должно было сработать исключение"s);
    }
    catch (const std::exception&)
    {
    }

    try
    {
        document = server.FindTopDocuments("Hello world -"s);
        ASSERT_HINT(false, "Должно было сработать исключение"s);
    }
    catch (const std::exception&)
    {
    }
}

// Тест проверки метода Matching
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

// Тест выдачи релевантности запроса
void TestSortingDocumentsWithRelevance(void)
{
    SearchServer server;
    std::vector<std::string> documents = { "first document at server excel"s, "second document in system word"s, "third document of database adobe"s, "fourth document on configuration photoshop"s, "fifth document at test windows"s, "sixth document in server office"s, "seventh document of system linux"s, "eighth document on database plm"s, "ninth document at configuration cad"s, "tenth document in test solidworks"s, "eleventh document of server compas"s, "twelfth document on system excel"s, "thirteenth document at database word"s, "fourteenth document in configuration adobe"s, "fifteenth document of test photoshop"s, "sixteenth document on server windows"s };

    for (size_t i = 0; i < 16; i++)
    {
        server.AddDocument(static_cast<int>(i), documents[i], DocumentStatus::ACTUAL, { 5, 7, 9 });
    }

    std::vector<Document> result = server.FindTopDocuments("excel word document in windows at execute some programms"s);
    int doc_size = static_cast<int>(result.size()) - 1;
    for (int i = 0; i < doc_size - 1; ++i)
    {
        ASSERT_HINT(result[i].relevance >= result[i + 1].relevance, "Неверная сортировка реливантности документа, i документ не может иметь релевантность меньше нижестоящего"s);
    }
}

// Тест проверки добавления документа
void TestAddDocument(void)
{
    SearchServer server;

    server.AddDocument(0, "Hello world"s, DocumentStatus::ACTUAL, { 5, 3, 10 }); // Avarage rating = 6
    server.AddDocument(1, "The second world"s, DocumentStatus::ACTUAL, { -10, -15, -20 }); // Avarage rating = -15
    server.AddDocument(2, "Third doc in server"s, DocumentStatus::ACTUAL, { 1, 0, 0 }); // Avarage rating = 0
    server.AddDocument(3, "Another document in server"s, DocumentStatus::ACTUAL, { 1, 2, 3 }); // Avarage rating = 2

    {
        std::vector<Document> result = server.FindTopDocuments("Hello"s);
        std::vector<Document> testingRes;
        testingRes.push_back({ 0, 0.69314718055994529, 6 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        std::vector<Document> result = server.FindTopDocuments("second"s);
        std::vector<Document> testingRes;
        testingRes.push_back({ 1, 0.46209812037329684, -15 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        std::vector<Document> result = server.FindTopDocuments("doc"s);
        std::vector<Document> testingRes;
        testingRes.push_back({ 2, 0.34657359027997264, 0 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        std::vector<Document> result = server.FindTopDocuments("Another document"s);
        std::vector<Document> testingRes;
        testingRes.push_back({ 3, 0.69314718055994529, 2 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }
    server.AddDocument(55, "Another correct document in server"s, DocumentStatus::ACTUAL, { 1, 2, 3 }); // Avarage rating = 2

    try
    {// Тест добавления документа с уже имеющимся id
        server.AddDocument(55, "Another incorrect document in server"s, DocumentStatus::ACTUAL, { 1, 2, 3 }); // Avarage rating = 2
        ASSERT_HINT(false, "Должно было сработать исключение при добавлении документа с дублирующимся id"s);
    }
    catch (const std::exception&)
    {
    }

    try
    {
        server.AddDocument(-42, "Another incorrect document in server"s, DocumentStatus::ACTUAL, { 1, 2, 3 }); // Avarage rating = 2
        ASSERT_HINT(false, "Должно было сработать исключение при добавлении документа с отрицательным id"s);
    }
    catch (const std::exception&)
    {
    }

    try
    {
        server.AddDocument(42, "Another \x0incorrect document in server"s, DocumentStatus::ACTUAL, { 1, 2, 3 }); // Avarage rating = 2
        ASSERT_HINT(false, "Должно было сработать исключение при добавлении документа с некорректным стоп-словом"s);
    }
    catch (const std::exception&)
    {
    }
}

// Проверка возврата правильного статуса документа
void TestDocumentStatus(void)
{
    SearchServer server;

    server.AddDocument(0, "Hello world"s, DocumentStatus::ACTUAL, { 5, 3, 10 }); // Avarage rating = 6
    server.AddDocument(1, "The second world"s, DocumentStatus::BANNED, { -10, -15, -20 }); // Avarage rating = -15
    server.AddDocument(2, "Third doc in server"s, DocumentStatus::IRRELEVANT, { 1, 0, 0 }); // Avarage rating = 0
    server.AddDocument(3, "Another document in server"s, DocumentStatus::REMOVED, { 1, 2, 3 }); // Avarage rating = 2

    {
        std::vector<Document> result = server.FindTopDocuments("Hello"s, DocumentStatus::ACTUAL);
        std::vector<Document> testingRes;
        testingRes.push_back({ 0, 0.69314718055994529, 6 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        std::vector<Document> result = server.FindTopDocuments("second"s, DocumentStatus::BANNED);
        std::vector<Document> testingRes;
        testingRes.push_back({ 1, 0.46209812037329684, -15 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        std::vector<Document> result = server.FindTopDocuments("doc"s, DocumentStatus::IRRELEVANT);
        std::vector<Document> testingRes;
        testingRes.push_back({ 2, 0.34657359027997264, 0 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        std::vector<Document> result = server.FindTopDocuments("Another document"s, DocumentStatus::REMOVED);
        std::vector<Document> testingRes;
        testingRes.push_back({ 3, 0.69314718055994529, 2 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }
}

void TestRatingCalculation(void)
{
    SearchServer server;
    std::vector<int> test1 = { 1, 2, 3, 4, 5, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3 };
    std::vector<int> test2 = { 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178 };
    std::vector<int> test3 = { -1, -3, -5, -7, -9, -11, -13, -15, -17, -19, -21, -23, -25, -27, -29, -31, -33, -35, -37, -39, -41, -43, -45, -47, -49, -51, -53, -55, -57, -59, -61, -63, -65, -67, -69, -71, -73, -75, -77, -79, -81, -83, -85, -87, -89, -91, -93, -95, -97, -99, -101, -103, -105, -107, -109, -111, -113, -115, -117, -119, -121, -123, -125, -127, -129, -131, -133, -135, -137, -139, -141, -143, -145, -147, -149, -151, -153, -155, -157, -159, -161, -163, -165, -167, -169, -171, -173, -175, -177 };
    std::vector<int> test4 = { 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 1, 2, 3, 4, 5, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, -21, -23, -25, -27, -29, -31, -33, -35, -37, -39, -41, -43, -45, -47, -49, -51, -53, -55, -57, -59, -61, -63, -65, -67, -69, 4, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3 };

    server.AddDocument(0, "Hello world"s, DocumentStatus::ACTUAL, test1);
    server.AddDocument(1, "The second world"s, DocumentStatus::BANNED, test2);
    server.AddDocument(2, "Third doc in server"s, DocumentStatus::IRRELEVANT, test3);
    server.AddDocument(3, "Another document in server"s, DocumentStatus::REMOVED, test4);

    {
        std::vector<Document> result = server.FindTopDocuments("Hello"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(result.front().rating, 2);
    }

    {
        std::vector<Document> result = server.FindTopDocuments("second"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(result.front().rating, 90);
    }

    {
        std::vector<Document> result = server.FindTopDocuments("doc"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(result.front().rating, -89);
    }

    {
        std::vector<Document> result = server.FindTopDocuments("Another document"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(result.front().rating, -4);
    }
}

// Тест корректного вычисления релевантности найденных документов.
void TestCorrectRelevance(void)
{
    SearchServer server;
    std::vector<std::string> documents = { "first document at server excel"s, "second document in system word"s, "third document of database adobe"s, "fourth document on configuration photoshop"s, "fifth document at test windows"s, "sixth document in server office"s, "seventh document of system linux"s, "eighth document on database plm"s, "ninth document at configuration cad"s, "tenth document in test solidworks"s, "eleventh document of server compas"s, "twelfth document on system excel"s, "thirteenth document at database word"s, "fourteenth document in configuration adobe"s, "fifteenth document of test photoshop"s, "sixteenth document on server windows"s };

    for (size_t i = 0; i < 16; i++)
    {
        std::vector<int> rating_vector = { 10 };
        for (size_t j = 0; j < i; j++)
        {
            rating_vector.push_back(((j + 1) * (i + 3) % 100));
        }
        server.AddDocument(static_cast<int>(i), documents[i], DocumentStatus::ACTUAL, rating_vector);
    }
    std::vector<Document> result = server.FindTopDocuments("excel word document in"s, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(result[0].relevance, 0.69314718055994529);
    ASSERT_EQUAL(result[1].relevance, 0.41588830833596718);
    ASSERT_EQUAL(result[2].relevance, 0.41588830833596718);
    ASSERT_EQUAL(result[3].relevance, 0.41588830833596718);
    ASSERT_EQUAL(result[4].relevance, 0.27725887222397810);
}

// Тест Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания
void TestCorrectOrderRelevance(void)
{
    SearchServer server;
    std::vector<std::string> documents = { "first document at server excel with some programms"s, "second document in system word and some execute files"s, "third document of database adobe"s, "fourth document on configuration photoshop"s, "fifth document at test windows"s, "sixth document in server office"s, "seventh document of system linux"s, "eighth document on database plm"s, "ninth document at configuration cad"s, "tenth document in test solidworks"s, "eleventh document of server compas"s, "twelfth document on system excel"s, "thirteenth document at database word"s, "fourteenth document in configuration adobe"s, "fifteenth document of test photoshop"s, "sixteenth document on server windows"s };

    for (size_t i = 0; i < 16; i++)
    {
        std::vector<int> rating_vector = { 10 };
        for (size_t j = 0; j < i; j++)
        {
            rating_vector.push_back(((j + 1) * (i + 3) % 100));
        }
        server.AddDocument(static_cast<int>(i), documents[i], DocumentStatus::ACTUAL, rating_vector);
    }

    std::vector<Document> result = server.FindTopDocuments("excel word document in windows at execute some programms", DocumentStatus::ACTUAL);

    int doc_size = static_cast<int>(result.size()) - 1;
    for (int i = 0; i < doc_size - 1; ++i)
    {
        ASSERT_HINT(result[i].relevance >= result[i + 1].relevance, "Неверная сортировка реливантности документа, i документ не может иметь релевантность меньше нижестоящего"s);
    }
}

// Тест Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestPredacateWorks(void)
{
    SearchServer server;
    std::vector<std::string> documents = { "first document at server excel with some programms"s, "second document in system word and some execute files"s, "third document of database adobe"s, "fourth document on configuration photoshop"s, "fifth document at test windows"s, "sixth document in server office"s, "seventh document of system linux"s, "eighth document on database plm"s, "ninth document at configuration cad"s, "tenth document in test solidworks"s, "eleventh document of server compas"s, "twelfth document on system excel"s, "thirteenth document at database word"s, "fourteenth document in configuration adobe"s, "fifteenth document of test photoshop"s, "sixteenth document on server windows"s };

    for (size_t i = 0; i < 16; i++)
    {
        std::vector<int> rating_vector = { 10 };
        for (size_t j = 0; j < i; j++)
        {
            rating_vector.push_back(((j + 1) * (i + 3) % 100));
        }
        server.AddDocument(static_cast<int>(i), documents[i], DocumentStatus::ACTUAL, rating_vector);
    }

    std::vector<Document> result;
    {
        result = server.FindTopDocuments("excel word document in windows at execute some programms"s,
            [](int document_id, DocumentStatus status, int rating)
            {
                return document_id % 2 == 0;
            });

        int arrId[] = { 0, 12, 4, 8, 14 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].id, arrId[i], "Неверный ID документа при предикатном поиске"s);
        }

        int arrRating[] = { 10, 44, 16, 45, 39 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].rating, arrRating[i], "Неверный рейтинг при предикатном поиске"s);
        }

        double arrRelevance[] = { 1.0397207708399179, 0.69314718055994529, 0.69314718055994529, 0.27725887222397810, 0.0000000000000000 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].relevance, arrRelevance[i], "Неверная релевантность при предикатном поиске"s);
        }
    }

    {
        result = server.FindTopDocuments("excel word document in windows at execute some programms",
            [](int document_id, DocumentStatus status, int rating)
            {
                return rating > 35;
            });

        int arrId[] = { 12, 15, 11, 13, 8 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].id, arrId[i], "Неверный ID документа при предикатном поиске"s);
        }

        int arrRating[] = { 44, 48, 44, 47, 45 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].rating, arrRating[i], "Неверный рейтинг при предикатном поиске"s);
        }

        double arrRelevance[] = { 0.69314718055994529, 0.41588830833596718, 0.41588830833596718, 0.27725887222397810, 0.27725887222397810 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].relevance, arrRelevance[i], "Неверная релевантность при предикатном поиске"s);
        }
    }
}

void TestCreateServerWithStopWords(void)
{
    {// минус в конце
        try
        {
            SearchServer server("Bad minus words in the end -"s);
            ASSERT_HINT(false, "Должно было сработать исключение"s);
        }
        catch (const std::exception&)
        {
        }
    }

    {// минус в середине
        try
        {
            SearchServer server("Bad minus - words in the middle"s);
            ASSERT_HINT(false, "Должно было сработать исключение"s);
        }
        catch (const std::exception&)
        {
        }
    }

    {// минус в начале
        try
        {
            SearchServer server("- Bad minus words at the start"s);
            ASSERT_HINT(false, "Должно было сработать исключение"s);
        }
        catch (const std::exception&)
        {
        }
    }

    {// минус c двойным --
        try
        {
            SearchServer server("Bad minus with double --word"s);
            ASSERT_HINT(false, "Должно было сработать исключение"s);
        }
        catch (const std::exception&)
        {
        }
    }

    {// минус c двойным -- корректным
        try
        {
            SearchServer server("Good minus with double-- word"s);
        }
        catch (const std::exception&)
        {
            ASSERT_HINT(false, "Не должно было сработать исключение"s);
        }
    }

    {// минус c двойным -- корректным
        try
        {
            SearchServer server("Good minus with dou-ble word"s);
        }
        catch (const std::exception&)
        {
            ASSERT_HINT(false, "Не должно было сработать исключение"s);
        }
    }
}

void TestThrowExeptionWithIncorrectQueryWithInFindTopDocuments(void)
{
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    try
    {
        const auto found_docs = server.FindTopDocuments("Another incorrect query in server -"s);
        ASSERT_HINT(false, "Должно было сработать исключение при поиске документа с некорректным запросом"s);
    }
    catch (const std::exception&)
    {
    }

    try
    {
        const auto found_docs = server.FindTopDocuments("Another \x1incorrect query in server"s);
        ASSERT_HINT(false, "Должно было сработать исключение при поиске документа с некорректным запросом"s);
    }
    catch (const std::exception&)
    {
    }

    try
    {
        const auto found_docs = server.FindTopDocuments("Another incorrect query in --server"s);
        ASSERT_HINT(false, "Должно было сработать исключение при поиске документа с некорректным запросом"s);
    }
    catch (const std::exception&)
    {
    }

    try
    {
        const auto found_docs = server.FindTopDocuments("Another correct document-- in server"s);
    }
    catch (const std::exception&)
    {
        ASSERT_HINT(false, "Не должно было сработать исключение при поиске документа с корректным запросом"s);
    }
}

void TestBeginEndIterators(void)
{
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    SearchServer search_server;
    search_server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    for (const int document_id : search_server)
    {
        ASSERT(document_id == doc_id);
    }

    std::set<int>::const_iterator begin_iter = search_server.begin();
    std::set<int>::const_iterator end_iter = search_server.end();

    ASSERT(std::next(begin_iter) == end_iter);
    ASSERT(std::prev(end_iter) == begin_iter);
}

void TestGetWordFrequencies(void)
{
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    SearchServer search_server;
    search_server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    std::map<std::string, double> to_compare;

    {
        auto res = search_server.GetWordFrequencies(1);
        ASSERT(res == to_compare);
    }

    {
        auto res = search_server.GetWordFrequencies(42);
        to_compare.insert({ {"cat", 0.25f},
                            {"city", 0.25f},
                            {"in", 0.25f},
                            {"the", 0.25f} });
        ASSERT(res == to_compare);
    }
}

void TestRemoveDocument(void)
{
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    SearchServer search_server;
    search_server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    ASSERT_EQUAL(search_server.GetDocumentCount(), 1);
    search_server.RemoveDocument(0);
    ASSERT_EQUAL(search_server.GetDocumentCount(), 1);
    search_server.RemoveDocument(42);
    ASSERT_EQUAL(search_server.GetDocumentCount(), 0);
}

void TestRemoveDuplicates(void)
{
    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // дубликат документа 2, будет удалён
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // добавились новые слова, дубликатом не является
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

    // есть не все слова, не является дубликатом
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // слова из разных документов, не является дубликатом
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    RemoveDuplicates(search_server);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer()
{
    RUN_TEST(TestCreateServerWithStopWords);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWordsExcludeDocumentFromQuery);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestSortingDocumentsWithRelevance);
    RUN_TEST(TestDocumentStatus);
    RUN_TEST(TestRatingCalculation);
    RUN_TEST(TestCorrectRelevance);
    RUN_TEST(TestCorrectOrderRelevance);
    RUN_TEST(TestPredacateWorks);
    RUN_TEST(TestThrowExeptionWithIncorrectQueryWithInFindTopDocuments);
    RUN_TEST(TestBeginEndIterators);
    RUN_TEST(TestGetWordFrequencies);
    RUN_TEST(TestRemoveDocument);
    RUN_TEST(TestRemoveDuplicates);
}
//#include "test_example_functions.h"

//using std::string_literals::operator""s;

//void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
//    const std::string& hint)
//{
//    if (!value)
//    {
//        std::cerr << file << "("s << line << "s): "s << func << ": "s;
//        std::cerr << "ASSERT("s << expr_str << "s) failed."s;
//        if (!hint.empty())
//        {
//            std::cerr << " Hint: "s << hint;
//        }
//        std::cerr << std::endl;
//    }
//}

//void TestExcludeStopWordsFromAddedDocumentContent() {

//    const int doc_id = 42;
//    const std::string content = "cat in the city"s;
//    const std::vector<int> ratings = { 1, 2, 3 };

//    {
//        SearchServer server;
//        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
//        const auto found_docs = server.FindTopDocuments("in"s);
//        ASSERT_EQUAL(found_docs.size(), 1u);
//        const Document& doc0 = found_docs[0];
//        ASSERT_EQUAL(doc0.id, doc_id);
//    }

//    {
//        SearchServer server("in the"s);
//        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
//        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
//    }
//}
//void TestMatchingDocuments(void)
//{
//    SearchServer server;

//    server.AddDocument(0, "Hello world"s, DocumentStatus::ACTUAL, { 5, 3, 10 }); // Avarage rating = 6
//    server.AddDocument(1, "The second world"s, DocumentStatus::ACTUAL, { -10, -15, -20 }); // Avarage rating = -15
//    server.AddDocument(2, "Third doc in server"s, DocumentStatus::BANNED, { 1, 0, 0 }); // Avarage rating = 0
//    server.AddDocument(3, "Another document in server"s, DocumentStatus::IRRELEVANT, { 1, 2, 3 }); // Avarage rating = 2
//    server.AddDocument(4, "Removed doc in server"s, DocumentStatus::REMOVED, { 3, 10, 20 }); // Avarage rating = 11

//    std::tuple<std::vector<std::string_view>, DocumentStatus> test_tuple;
//    std::tuple<std::vector<std::string_view>, DocumentStatus> testMatching = server.MatchDocument(""s, 0);

//    ASSERT(testMatching == test_tuple);
//    std::vector<std::string_view> blankvectorOfWords;

//    testMatching = server.MatchDocument("Test query"s, 0);
//    ASSERT(testMatching == test_tuple);

//    {// Тест пустого возврата при нахождении минус слова и верного статуса
//        testMatching = server.MatchDocument("second the -world"s, 1);
//        test_tuple = { blankvectorOfWords, DocumentStatus::ACTUAL };
//        ASSERT(testMatching == test_tuple);

//        testMatching = server.MatchDocument("third -in doc"s, 2);
//        test_tuple = { blankvectorOfWords, DocumentStatus::BANNED };
//        ASSERT(testMatching == test_tuple);

//        testMatching = server.MatchDocument("-Another document"s, 3);
//        test_tuple = { blankvectorOfWords, DocumentStatus::IRRELEVANT };
//        ASSERT(testMatching == test_tuple);

//        testMatching = server.MatchDocument("server -Removed"s, 4);
//        test_tuple = { blankvectorOfWords, DocumentStatus::REMOVED };
//        ASSERT(testMatching == test_tuple);
//    }

//    {// Тест пустого возврата при нахождении минус слова и верного статуса
//        testMatching = server.MatchDocument(std::execution::par, "second the -world"s, 1);
//        test_tuple = { blankvectorOfWords, DocumentStatus::ACTUAL };
//        ASSERT(testMatching == test_tuple);

//        testMatching = server.MatchDocument(std::execution::par, "third -in doc"s, 2);
//        test_tuple = { blankvectorOfWords, DocumentStatus::BANNED };
//        ASSERT(testMatching == test_tuple);

//        testMatching = server.MatchDocument(std::execution::par, "-Another document"s, 3);
//        test_tuple = { blankvectorOfWords, DocumentStatus::IRRELEVANT };
//        ASSERT(testMatching == test_tuple);

//        testMatching = server.MatchDocument(std::execution::par, "server -Removed"s, 4);
//        test_tuple = { blankvectorOfWords, DocumentStatus::REMOVED };
//        ASSERT(testMatching == test_tuple);
//    }

//    {// Тест возврата слов и статусов
//        std::vector<std::string> vec_words{ {"The"s, "second"s} };
//        std::vector<std::string_view> temp(vec_words.begin(), vec_words.end());
//        std::string source{ "second The"s };
//        testMatching = server.MatchDocument(source, 1);
//        test_tuple = { temp, DocumentStatus::ACTUAL };
//        ASSERT(std::get<0>(testMatching) == std::get<0>(test_tuple));
//        ASSERT(std::get<1>(testMatching) == std::get<1>(test_tuple));
//        ASSERT(testMatching == test_tuple);
//    }

//    {// Тест возврата слов и статусов
//        std::vector<std::string> vec_words{ {"The"s, "second"s} };
//        std::vector<std::string_view> temp(vec_words.begin(), vec_words.end());
//        std::string source{ "second The"s };
//        testMatching = server.MatchDocument(std::execution::par, source, 1);
//        test_tuple = { temp, DocumentStatus::ACTUAL };
//        ASSERT(std::get<0>(testMatching) == std::get<0>(test_tuple));
//        ASSERT(std::get<1>(testMatching) == std::get<1>(test_tuple));
//        ASSERT(testMatching == test_tuple);
//    }

//    {
//        std::vector<std::string> vec_words{ {"Third"s, "doc"s, "in"s} };
//        std::vector<std::string_view> temp(vec_words.begin(), vec_words.end());
//        std::string source{ "Third in doc"s };
//        testMatching = server.MatchDocument(source, 2);
//        test_tuple = { temp, DocumentStatus::BANNED };
//        ASSERT(testMatching == test_tuple);
//    }

//    {
//        std::vector<std::string> vec_words{ {"Another"s, "document"s} };
//        std::vector<std::string_view> temp(vec_words.begin(), vec_words.end());
//        std::string source{ "Another document"s };
//        testMatching = server.MatchDocument(source, 3);
//        test_tuple = { temp, DocumentStatus::IRRELEVANT };
//        ASSERT(testMatching == test_tuple);
//    }

//    {
//        std::vector<std::string> vec_words{ {"Removed"s, "server"s} };
//        std::vector<std::string_view> temp(vec_words.begin(), vec_words.end());
//        std::string source{ "server Removed"s };
//        testMatching = server.MatchDocument(source, 4);
//        test_tuple = { temp , DocumentStatus::REMOVED };
//        ASSERT(testMatching == test_tuple);
//    }

//    const int doc_id = 42;
//    const std::string content = "cat in the city"s;
//    const std::vector<int> ratings = { 1, 2, 3 };
//    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
//    try
//    {
//        testMatching = server.MatchDocument("Another incorrect query in server -"s, doc_id);
//        ASSERT_HINT(false, "Должно было сработать исключение при сравнении документа с некорректным запросом"s);
//    }
//    catch (const std::exception&)
//    {
//    }

//    try
//    {
//        testMatching = server.MatchDocument("Another \x1incorrect query in server"s, doc_id);
//        ASSERT_HINT(false, "Должно было сработать исключение при сравнении документа с некорректным запросом"s);
//    }
//    catch (const std::exception&)
//    {
//    }

//    try
//    {
//        testMatching = server.MatchDocument("Another incorrect query in --server"s, doc_id);
//        ASSERT_HINT(false, "Должно было сработать исключение при сравнении документа с некорректным запросом"s);
//    }
//    catch (const std::exception&)
//    {
//    }

//    try
//    {
//        testMatching = server.MatchDocument("Another correct document-- in server"s, doc_id);
//    }
//    catch (const std::exception&)
//    {
//        ASSERT_HINT(false, "Не должно было сработать исключение при сравнении документа с корректным запросом"s);
//    }
//}


//void TestAverageRating()
//{
//    SearchServer server("и в на"s);

//    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
//    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
//    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
//    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

//    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);

//    {
//        const Document& doc0 = found_docs[0];
//        ASSERT_EQUAL(doc0.rating, 5);
//        ASSERT_EQUAL(doc0.id, 1);
//        const Document& doc2 = found_docs[2];
//        ASSERT_EQUAL(doc2.rating, -1);
//        ASSERT_EQUAL(doc2.id, 2);
//    }
//}

//void TestPredicateFilter()
//{
//    const int doc_id = 42;
//    const std::string content = "cat in the city"s;
//    const std::vector<int> ratings = {1, 2, 3};

//    {
//        SearchServer server;
//        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
//        const auto found_docs = server.FindTopDocuments("in"s, DocumentStatus::BANNED );
//        ASSERT(found_docs.empty());
//        const auto found_docs_1 = server.FindTopDocuments("in"s, [](int document_id, [[maybe_unused]] DocumentStatus status, [[maybe_unused]] int rating) { return document_id % 2 == 0; });
//        ASSERT_EQUAL(found_docs_1.size(), 1u);
//    }
//}

//void TestCorrectRelevanceCount() {

//    SearchServer server("и в на"s);

//    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
//    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
//    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
//    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

//    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
//    const Document& doc0 = found_docs.at(0);
//    const Document& doc1 = found_docs.at(1);

//    if (std::abs(found_docs.at(0).relevance - found_docs.at(1).relevance) < 1e-6)
//    {
//        ASSERT (doc0.relevance > doc1.relevance || doc0.rating > doc1.rating);
//    }
//}
