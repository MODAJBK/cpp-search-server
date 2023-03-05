#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <numeric>
#include <sstream>
#include <utility>
#include <vector>
#include <cassert>

using namespace std;

template<typename Element1, typename Element2>
ostream& operator<<(ostream& out, const pair<Element1, Element2>& container) {
    return out << container.first << ": "s << container.second;
}

template<typename Container>
void Print(ostream& out, const Container& container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) out << ", "s;
        out << element;
        is_first = false;
    }
}

template<typename Element>
ostream& operator<<(ostream& out, const vector<Element>& container) {
    out << "["s;
    Print(out, container);
    out << "]"s;
    return out;
}

template<typename Element>
ostream& operator<<(ostream& out, const set<Element>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template<typename Element1, typename Element2>
ostream& operator<<(ostream& out, const map<Element1, Element2>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename FuncTest>
void RunTestImpl(FuncTest& func_test, const string& func_name) {
    func_test();
    cerr << func_name << " completed." << endl;
}

#define RUN_TEST(func)  RunTestImpl((func), #func)

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result >> ws;
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

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
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
            DocumentData{
                ComputeAverageRating(ratings),
                status
            });
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const double eps = 1e-6;
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [eps](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < eps) {
                    return lhs.rating > rhs.rating;
                }
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        const int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
                });
        }
        return matched_documents;
    }
};


// -------- Ќачало модульных тестов поисковой системы ----------

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Eror in FindTopDocuments."s);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.id, doc_id, "Eror in FindTopDocuments.");
    }
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestExcludeDocumentsWithMinusWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(30, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(31, "big black dog"s, DocumentStatus::ACTUAL, { 0, 1, 4 });
    const auto found_docs = server.FindTopDocuments("big black animal -cat"s);
    ASSERT_EQUAL_HINT(found_docs[0].id, 31, "Documents with minus words should be exclude from search"s);
}

void TextAvRatingComputation() {
    SearchServer server;
    server.AddDocument(30, "Big black dog has found"s, DocumentStatus::ACTUAL, { 1, 2 ,3 });
    const auto found_docs = server.FindTopDocuments("big black dog");
    ASSERT_EQUAL_HINT(found_docs[0].rating, (1 + 2 + 3) / 3, "Eror in avarage rating computation."s);
}

void TestRelevanceComputation() {
    SearchServer server;
    server.AddDocument(30, "black dog has found"s, DocumentStatus::ACTUAL, { 3, 5, 7 });
    server.AddDocument(31, "big curly capibara wiht brown fure"s, DocumentStatus::ACTUAL, { 2, 7, 1 });
    server.AddDocument(32, "small grey cat"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto found_docs = server.FindTopDocuments("big black dog"s);
    ASSERT_EQUAL_HINT(found_docs[0].relevance, log(server.GetDocumentCount() * 1.0 / 1) * (2.0 / 4), "Eror in relevance computation");
}

void TextRelevantSort() {
    SearchServer server;
    server.AddDocument(30, "black dog has found"s, DocumentStatus::ACTUAL, { 3, 5, 7 });
    server.AddDocument(31, "big curly dog wiht white fure"s, DocumentStatus::ACTUAL, { 2, 7, 1 });
    server.AddDocument(32, "small black cat"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto found_docs = server.FindTopDocuments("big black dog"s);
    ASSERT_HINT(found_docs[0].relevance >= found_docs[1].relevance && found_docs[1].relevance >= found_docs[2].relevance, "Eror in sorting of found docks"s);
}

void TestStatusSearch() {
    SearchServer server;
    server.AddDocument(30, "Big black dog has found"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(31, "Black cat"s, DocumentStatus::BANNED, { 0 });
    {
        const auto [matched_words, status] = server.MatchDocument("big black dog"s, 30);
        ASSERT_EQUAL_HINT(static_cast<int>(status), static_cast<int>(DocumentStatus::ACTUAL), "Eror in document status appropriation"s);
    }
    {
        const auto [matched_words, status] = server.MatchDocument("big black dog"s, 31);
        ASSERT_EQUAL_HINT(static_cast<int>(status), static_cast<int>(DocumentStatus::BANNED), "Eror in document status appropriation"s);
    }
}

void TestWordsMatching() {
    SearchServer server;
    server.AddDocument(30, "Big black dog has found"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(31, "Black cat"s, DocumentStatus::ACTUAL, { 0 });
    {
        const vector<string> matched_ref = { "black"s, "dog"s };
        const auto [matched_words, status] = server.MatchDocument("big black dog"s, 30);
        ASSERT_EQUAL_HINT(matched_words, matched_ref, "Eror in matching words detection"s);
    }
    {
        const auto [matched_words, status] = server.MatchDocument("big black dog -cat", 31);
        ASSERT_HINT(matched_words.empty(), "Eror in minus words filtration"s);
    }
}

void TestPredicatFiltration() {
    SearchServer server;
    server.AddDocument(30, "Big black dog has found"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(31, "Small white curly dog"s, DocumentStatus::BANNED, { 0 });
    server.AddDocument(32, "Black cat"s, DocumentStatus::ACTUAL, { 0 });
    const auto found_docs = server.FindTopDocuments("big black dog"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 != 0; });
    ASSERT_EQUAL_HINT(found_docs[0].id, 31, "Eror in predicate filtration."s);
    const auto found_docs2 = server.FindTopDocuments("big black dog"s, [](int document_id, DocumentStatus status, int rating) { return rating > 0; });
    ASSERT_EQUAL_HINT(found_docs2[0].id, 30, "Eror in predicate filtartion"s);
    const auto found_docs3 = server.FindTopDocuments("big black dog"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
    ASSERT_EQUAL_HINT(found_docs3[0].id, 31, "Eror in predicate filtration"s);
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocumentsWithMinusWords);
    RUN_TEST(TextAvRatingComputation);
    RUN_TEST(TestRelevanceComputation);
    RUN_TEST(TextRelevantSort);
    RUN_TEST(TestStatusSearch);
    RUN_TEST(TestWordsMatching);
    RUN_TEST(TestPredicatFiltration);
}

// --------- ќкончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
}
