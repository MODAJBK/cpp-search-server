#include "test_example_functions.h"

using namespace std::string_literals;

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
    if (!value) {
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

std::vector<std::string_view> VectStringToVectStringView(const std::vector<std::string>& source) {
    std::vector<std::string_view> result;
    result.reserve(source.size());
    for (const auto& s : source) {
        result.emplace_back(std::string_view(s));
    }
    return result;
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Error in FindTopDocuments."s);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.id, doc_id, "Error in FindTopDocuments.");
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestExcludeDocumentsWithMinusWords() {
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };

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
    ASSERT_EQUAL_HINT(found_docs[0].rating, (1 + 2 + 3) / 3, "Error in avarage rating computation."s);
}

void TestRelevanceComputation() {
    SearchServer server;
    server.AddDocument(30, "black dog has found"s, DocumentStatus::ACTUAL, { 3, 5, 7 });
    server.AddDocument(31, "big curly capibara wiht brown fure"s, DocumentStatus::ACTUAL, { 2, 7, 1 });
    server.AddDocument(32, "small grey cat"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto found_docs = server.FindTopDocuments("big black dog"s);
    ASSERT_EQUAL_HINT(found_docs[0].relevance, log(server.GetDocumentCount() * 1.0 / 1) * (2.0 / 4), "Error in relevance computation");
}

void TextRelevantSort() {
    SearchServer server;
    server.AddDocument(30, "black dog has found"s, DocumentStatus::ACTUAL, { 3, 5, 7 });
    server.AddDocument(31, "big curly dog wiht white fure"s, DocumentStatus::ACTUAL, { 2, 7, 1 });
    server.AddDocument(32, "small black cat"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto found_docs = server.FindTopDocuments("big black dog"s);
    ASSERT_HINT(found_docs[0].relevance >= found_docs[1].relevance && found_docs[1].relevance >= found_docs[2].relevance, "Error in sorting of found docks"s);
}

void TestStatusSearch() {
    SearchServer server;
    server.AddDocument(30, "Big black dog has found"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(31, "Black cat"s, DocumentStatus::BANNED, { 0 });
    {
        const auto [matched_words, status] = server.MatchDocument("big black dog"s, 30);
        ASSERT_EQUAL_HINT(static_cast<int>(status), static_cast<int>(DocumentStatus::ACTUAL), "Error in document status appropriation"s);
    }
    {
        const auto [matched_words, status] = server.MatchDocument("big black dog"s, 31);
        ASSERT_EQUAL_HINT(static_cast<int>(status), static_cast<int>(DocumentStatus::BANNED), "Error in document status appropriation"s);
    }
}

void TestWordsMatching() {
    SearchServer search_server("and with to"s);
    const std::vector<int> ratings1 = { 1, 2, 3, 4, 5 };
    const std::vector<int> ratings2 = { -1, -2, 30, -3, 44, 5 };
    const std::vector<int> ratings3 = { 12, -20, 80, 0, 8, 0, 0, 9, 67 };
    search_server.AddDocument(1, "white cat and fashion collar"s, DocumentStatus::ACTUAL, ratings1);
    search_server.AddDocument(2, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, ratings2);
    search_server.AddDocument(3, "wellgroomed dog expressive eyes"s, DocumentStatus::ACTUAL,ratings3);
    search_server.AddDocument(4, "white fashion cat"s, DocumentStatus::IRRELEVANT, ratings1);
    search_server.AddDocument(5, "fluffy cat dog", DocumentStatus::IRRELEVANT, ratings2);
    search_server.AddDocument(6, "wellgroomed collar expressive eyes"s, DocumentStatus::IRRELEVANT, ratings3);
    const std::string query = "fluffy wellgroomed cat -collar"s;
    std::vector<std::vector<std::string>> ref = {
        {},
        {"cat"s, "fluffy"s},
        {"wellgroomed"s},
        {"cat"s},
        {"cat"s, "fluffy"s},
        {}
    };
    std::vector<std::vector<std::string_view>> match_ref(ref.size());
    for (int i = 0; i < ref.size(); ++i) {
        match_ref[i] = VectStringToVectStringView(ref[i]);
    }
    auto [words1, doc1] = search_server.MatchDocument(std::execution::seq, query, 1);
    auto [words2, doc2] = search_server.MatchDocument(query, 2);
    auto [words3, doc3] = search_server.MatchDocument(std::execution::par, query, 3);
    auto [words4, doc4] = search_server.MatchDocument(std::execution::seq, query, 4);
    auto [words5, doc5] = search_server.MatchDocument(query, 5);
    auto [words6, doc6] = search_server.MatchDocument(std::execution::par, query, 6);
    ASSERT_EQUAL_HINT(words1, match_ref[0], "Error in searching matched words"s);
    ASSERT_EQUAL_HINT(words2, match_ref[1], "Error in searching matched words"s);
    ASSERT_EQUAL_HINT(words3, match_ref[2], "Error in searching matched words"s);
    ASSERT_EQUAL_HINT(words4, match_ref[3], "Error in searching matched words"s);
    ASSERT_EQUAL_HINT(words5, match_ref[4], "Error in searching matched words"s);
    ASSERT_EQUAL_HINT(words6, match_ref[5], "Error in searching matched words"s);
}

void TestPredicatFiltration() {
    SearchServer server;
    server.AddDocument(30, "Big black dog has found"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(31, "Small white curly dog"s, DocumentStatus::BANNED, { 0 });
    server.AddDocument(32, "Black cat"s, DocumentStatus::ACTUAL, { 0 });
    const auto found_docs = server.FindTopDocuments("big black dog"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 != 0; });
    ASSERT_EQUAL_HINT(found_docs[0].id, 31, "Error in predicate filtration."s);
    const auto found_docs2 = server.FindTopDocuments("big black dog"s, [](int document_id, DocumentStatus status, int rating) { return rating > 0; });
    ASSERT_EQUAL_HINT(found_docs2[0].id, 30, "Error in predicate filtartion"s);
    const auto found_docs3 = server.FindTopDocuments("big black dog"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
    ASSERT_EQUAL_HINT(found_docs3[0].id, 31, "Error in predicate filtration"s);
}

void TestPeginator() {
    SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
    ASSERT_EQUAL_HINT(pages.size(), page_size, "Error in page distribution"s);
}

void TestQueryQueue() {
    int resulted_empty_requests = 1437;
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    request_queue.AddFindRequest("curly dog"s);
    request_queue.AddFindRequest("big collar"s);
    request_queue.AddFindRequest("sparrow"s);
    ASSERT_EQUAL_HINT(request_queue.GetNoResultRequests(), resulted_empty_requests, "Error in building the request queue"s);
}

void TestRemoveDocument() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const std::string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    const std::string query = "curly and funny"s;
    search_server.RemoveDocument(5);
    ASSERT_EQUAL_HINT(search_server.GetDocumentCount(), 4, "Error in documents removement"s);
    search_server.RemoveDocument(std::execution::seq, 1);
    ASSERT_EQUAL_HINT(search_server.GetDocumentCount(), 3, "Error in documents removement"s);
    search_server.RemoveDocument(std::execution::par, 2);
    ASSERT_EQUAL_HINT(search_server.GetDocumentCount(), 2, "Error in documents removement"s);
}

void TestRemoveDuplicates() {
    SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });
    search_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });
    search_server.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });
    search_server.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });
    search_server.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });
    search_server.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });
    search_server.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });
    //Need to remove 4 documents.
    auto documents_before_removing = search_server.GetDocumentCount();
    RemoveDuplicates(search_server);
    ASSERT_EQUAL_HINT(search_server.GetDocumentCount(), documents_before_removing - 4, "Error in function RemoveDuplicates"s);
}

void TestProcessQueries() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const std::string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    const std::vector<std::string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    const auto result = ProcessQueries(search_server, queries);
    ASSERT_EQUAL_HINT(result[0].size(), search_server.FindTopDocuments(queries[0]).size(), "Error in function ProcessQueries"s);
    ASSERT_EQUAL_HINT(result[1].size(), search_server.FindTopDocuments(queries[1]).size(), "Error in function ProcessQueries"s);
    ASSERT_EQUAL_HINT(result[2].size(), search_server.FindTopDocuments(queries[2]).size(), "Error in function ProcessQueries"s);
}

void TestFindingDocumentsWithPolicy() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const std::string& text : {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            "nasty pigeon john"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    const auto result1 = search_server.FindTopDocuments("curly nasty cat"s);
    ASSERT_EQUAL_HINT(result1.size(), id, "Error in Finding Documents without policy"s);
    const auto result2 = search_server.FindTopDocuments(std::execution::seq, "curly nasty cat"s, DocumentStatus::BANNED);
    ASSERT_EQUAL_HINT(result2.size(), 0, "Error in Finding Documents with seq policy"s);
    const auto result3 = search_server.FindTopDocuments(std::execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
    ASSERT_EQUAL_HINT(result3.size(), id / 2, "Error in Finding Documents without policy"s);
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
    RUN_TEST(TestPeginator);
    RUN_TEST(TestQueryQueue);
    RUN_TEST(TestRemoveDocument);
    RUN_TEST(TestRemoveDuplicates);
    RUN_TEST(TestProcessQueries);
    RUN_TEST(TestFindingDocumentsWithPolicy);
}