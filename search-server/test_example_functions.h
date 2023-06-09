#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "search_server.h"
#include "document.h"
#include "paginator.h"
#include "request_queue.h"
#include "remove_duplicates.h"
#include "process_queries.h"

template<typename Element1, typename Element2>
std::ostream& operator<<(std::ostream& out, const std::pair<Element1, Element2>& container);

template<typename Container>
void Print(std::ostream& out, const Container& container);

template<typename Element>
std::ostream& operator<<(std::ostream& out, const std::vector<Element>& container);

template<typename Element>
std::ostream& operator<<(std::ostream& out, const std::set<Element>& container);

template<typename Element1, typename Element2>
std::ostream& operator<<(std::ostream& out, const std::map<Element1, Element2>& container);

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
    using namespace std::string_literals;
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
    using namespace std::string_literals;
    func_test();
    std::cerr << func_name << " completed."s << std::endl;
}

template<typename Element1, typename Element2>
std::ostream& operator<<(std::ostream& out, const std::pair<Element1, Element2>& container) {
    using namespace std::string_literals;
    return out << container.first << ": "s << container.second;
}

template<typename Container>
void Print(std::ostream& out, const Container& container) {
    using namespace std::string_literals;
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) out << ", "s;
        out << element;
        is_first = false;
    }
}

template<typename Element>
std::ostream& operator<<(std::ostream& out, const std::vector<Element>& container) {
    using namespace std::string_literals;
    out << "["s;
    Print(out, container);
    out << "]"s;
    return out;
}

template<typename Element>
std::ostream& operator<<(std::ostream& out, const std::set<Element>& container) {
    using namespace std::string_literals;
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template<typename Element1, typename Element2>
std::ostream& operator<<(std::ostream& out, const std::map<Element1, Element2>& container) {
    using namespace std::string_literals;
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

std::vector<std::string_view> VectStringToVectStringView(const std::vector<std::string>&);

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
void TestRemoveDocument();
void TestRemoveDuplicates();
void TestProcessQueries();
void TestFindingDocumentsWithPolicy();