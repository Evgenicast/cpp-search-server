#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <numeric>
#include <cassert>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            word = "";
        } else {
            word += c;
        }
    }
    words.push_back(word);

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text)
    {
        for (const string& word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words)
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData {ComputeAverageRating(ratings), status});
    }

    template <typename T>
    vector<Document> FindTopDocuments(const string& raw_query, T predicate) const
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs)
            {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON)
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const
    {
        return FindTopDocuments(raw_query, [&status]([[maybe_unused]] int document_id, DocumentStatus status_, [[maybe_unused]] int rating)
        {
            return status_ == status;
        });
    }
    vector<Document> FindTopDocuments(const string& raw_query) const
    {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
    {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings)
    {
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord
    {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const
    {
        bool is_minus = false;
        if (text[0] == '-')
        {
            is_minus = true;
            text = text.substr(1);
        }
        return
        {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const
    {
        Query query;
        for (const string& word : SplitIntoWords(text))
        {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop)
            {
                if (query_word.is_minus)
                {
                    query.minus_words.insert(query_word.data);
                }
                else
                {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const
    {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
//        size_t size = word_to_document_freqs_.at(word).size();
//        auto del1 = 1.0 / word_to_document_freqs_.at(word).size();
//        auto doccnt = GetDocumentCount();
//        auto logg = log(4*1);
//        auto res = log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
//        return res;
    }

    template <typename T>
    vector<Document> FindAllDocuments(const Query& query, T predicate) const
    {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                const auto& document_data = documents_.at(document_id);
                if (predicate(document_id, document_data.status, document_data.rating))
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename foo>
void RunTestImpl(const foo f, const string& f_name)
{
    f();
    if (f)
    cerr << f_name << " OK" << endl;
}
#define RUN_TEST(func) RunTestImpl((func), #func)

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint)
{
    if (!value)
    {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty())
        {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u)
    {
        //cout << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

// -------- Начало модульных тестов поисковой системы ----------

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestMatchedDocuments()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    tuple<vector<string>, DocumentStatus> matched_documents;
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        matched_documents = server.MatchDocument("cat in"s, doc_id);
        vector<string> s;
        s = get<0>(matched_documents);
        ASSERT_EQUAL(s.size(), 2u);
        //auto check = server.FindTopDocuments("the"s);
        ASSERT_HINT(server.FindTopDocuments("-the"s).empty(), "Minus-words should be excluded from documents");
        DocumentStatus status;
        status = get<1>(matched_documents);
        ASSERT(status == DocumentStatus::ACTUAL);
    }
}

void TestAverageRating()
{
    SearchServer server;
    server.SetStopWords("и в на"s);

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
    SearchServer server;
    server.SetStopWords("и в на"s);

    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::REMOVED, {8, -3});
    server.AddDocument(1, "пушистый  кот пушистый хвост"s,       DocumentStatus::IRRELEVANT, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    const auto found_docs_BANNED = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT(found_docs_BANNED.size() == 1);
    ASSERT(found_docs_BANNED[0].id == 3);
    ASSERT(found_docs_BANNED[0].rating == 9);
    const auto found_docs_IRRELEVANT = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::IRRELEVANT);
    ASSERT(found_docs_IRRELEVANT.size() == 1);
    ASSERT(found_docs_IRRELEVANT[0].id == 1);
    ASSERT(found_docs_IRRELEVANT[0].rating == 5);
    const auto found_docs_REMOVED = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::REMOVED);
    ASSERT(found_docs_REMOVED.size() == 1);
    ASSERT(found_docs_REMOVED[0].id == 0);
    ASSERT(found_docs_REMOVED[0].rating == 2);
    const auto found_docs_ACTUAL = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::ACTUAL);
    ASSERT(found_docs_ACTUAL.size() == 1);
    ASSERT(found_docs_ACTUAL[0].id == 2);
    ASSERT(found_docs_ACTUAL[0].rating == -1);

}
void TestPredicateFilter()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s, DocumentStatus::BANNED );
        ASSERT(found_docs.empty());
        const auto found_docs_1 = server.FindTopDocuments("in"s, [](int document_id, [[maybe_unused]] DocumentStatus status, [[maybe_unused]] int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(found_docs_1.size(), 1u);
    }
}

void TestCorrectRelevanceCount()
{
    SearchServer server;
    server.SetStopWords("и в на"s);

    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    const Document& doc0 = found_docs.at(0);
    const Document& doc1 = found_docs.at(1);
    const Document& doc2 = found_docs.at(2);
    ASSERT_EQUAL(found_docs.size(), 3u);
    int furry_cnt = 1;
    double TF_furry = 0.5;
    double IDF_furry = log(server.GetDocumentCount() * 1.0 / furry_cnt);
    double relevevance_for_furry = (IDF_furry * TF_furry) + doc1.relevance; //doc1.relevance проверяется ниже, так что допустимо, на мой взгляд,
    ASSERT_EQUAL(doc0.relevance, relevevance_for_furry);                    //для симуляции теста.
    ASSERT(doc0.id == 1);
    ASSERT(doc0.rating == 5);

    if ( doc0.relevance < EPSILON )
    {
        ASSERT(doc0.rating >= doc1.rating);
    }
    else
    {
        ASSERT(doc0.relevance >= doc1.relevance);
        //ASSERT(doc0.relevance <= doc1.relevance);  ASSERT выдаст ошибку, т.к. сортировка идет по убыванию.
    }

    int neat_cnt = 2;
    double TF_neat = 0.25;
    double IDF_neat = log(server.GetDocumentCount() * 1.0 / neat_cnt);
    double relevevance_for_neat = IDF_neat * TF_neat;
    ASSERT_EQUAL(doc1.relevance, relevevance_for_neat);
    ASSERT(doc1.id == 0);
    ASSERT(doc1.rating == 2);
    if ( doc1.relevance < EPSILON )
    {
        ASSERT(doc1.rating >= doc2.rating);
    }
    else
    {
        ASSERT(doc1.relevance >= doc2.relevance);
    }
    int cat_cnt = 2;
    double TF_cat = 0.25;
    double IDF_cat = log(server.GetDocumentCount() * 1.0 / cat_cnt);
    double relevevance_for_cat = IDF_cat * TF_cat;
    ASSERT_EQUAL(doc2.relevance, relevevance_for_cat);
    ASSERT(doc2.id == 2);
    ASSERT(doc2.rating == -1);

}

void TestSearchServer()
{
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMatchedDocuments);
    RUN_TEST(TestAverageRating);
    RUN_TEST(TestStatus);
    RUN_TEST(TestPredicateFilter);
    RUN_TEST(TestCorrectRelevanceCount);
}

int main()
{
    TestSearchServer();
    cout << "Search server testing finished"s << endl;
}
