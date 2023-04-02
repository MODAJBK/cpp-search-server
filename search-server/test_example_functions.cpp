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
    SearchServer server;
    server.AddDocument(30, "Big black dog has found"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(31, "Black cat"s, DocumentStatus::ACTUAL, { 0 });
    {
        const std::vector<std::string> matched_ref = { "black"s, "dog"s };
        const auto [matched_words, status] = server.MatchDocument("big black dog"s, 30);
        ASSERT_EQUAL_HINT(matched_words, matched_ref, "Error in matching words detection"s);
    }
    {
        const auto [matched_words, status] = server.MatchDocument("big black dog -cat", 31);
        ASSERT_HINT(matched_words.empty(), "Error in minus words filtration"s);
    }
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
    SearchServer search_server("and in at"s);
    search_server.AddDocument(1, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(2, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(3, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.RemoveDocument(2);
    auto found_docs = search_server.FindTopDocuments("a cat"s);
    ASSERT_HINT(found_docs.empty(), "Error in documents removement"s);
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
}