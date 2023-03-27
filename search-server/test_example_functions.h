#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "search_server.h"
#include "document.h"
#include "paginator.h"
#include "request_queue.h"

using namespace std::string_literals;

template <typename T, typename U>
void AssertEqualImpl(const T&, const U&, const std::string&, const std::string&, const std::string&, const std::string&, unsigned, const std::string&);


#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool, const std::string&, const std::string&, const std::string&, unsigned, const std::string&);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename FuncTest>
void RunTestImpl(FuncTest&, const std::string&);

#define RUN_TEST(func)  RunTestImpl((func), #func)

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        std::abort();
    }
}

template <typename FuncTest>
void RunTestImpl(FuncTest& func_test, const std::string& func_name) {
    func_test();
    std::cerr << func_name << " completed." << std::endl;
}

void TestExcludeStopWordsFromAddedDocumentContent();
void TestExcludeDocumentsWithMinusWords();
void TextAvRatingComputation();
void TestRelevanceComputation();
void TextRelevantSort();
void TestStatusSearch();
void TestWordsMatching();
void TestPredicatFiltration();
void TestPeginator();
void TestQueryQueue();
void TestSearchServer();