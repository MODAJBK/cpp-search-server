#pragma once

#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <set>
#include <map>
#include <deque>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <execution>
#include <cassert>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"
#include "log_duration.h"

class SearchServer {
public:

    SearchServer() = default;
    template <typename StringContainer>
    explicit SearchServer(const StringContainer&);
    explicit SearchServer(const std::string_view);
    explicit SearchServer(const std::string&);
    void AddDocument(int, std::string_view, DocumentStatus, const std::vector<int>&);
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view, DocumentPredicate) const;
    std::vector<Document> FindTopDocuments(std::string_view, DocumentStatus) const;
    std::vector<Document> FindTopDocuments(std::string_view) const;
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view, DocumentPredicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view, DocumentStatus) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view) const;
    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;
    size_t GetDocumentCount() const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view, int) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, std::string_view, int) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, std::string_view, int) const;
    const std::map<std::string_view, double>& GetWordFrequencies(int) const;
    void RemoveDocument(int);
    void RemoveDocument(const std::execution::sequenced_policy&, int);
    void RemoveDocument(const std::execution::parallel_policy&, int);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::deque<std::string> storage_;

    bool IsStopWord(std::string_view) const;
    static bool IsValidWord(std::string_view);
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view) const;
    static int ComputeAverageRating(const std::vector<int>&);
    QueryWord ParseQueryWord(std::string_view) const;
    Query ParseQuery(std::string_view, bool = true) const;
    double ComputeWordInverseDocumentFreq(std::string_view) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query&, DocumentPredicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query&, DocumentPredicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query&, DocumentPredicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const {
    const size_t MAX_RESULT_DOCUMENT_COUNT = 5;
    const double EPSILON = 1e-6;
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);
    std::sort(matched_documents.begin(), matched_documents.end(),
              [EPSILON](const Document& lhs, const Document& rhs) {
                  if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                      return lhs.rating > rhs.rating;
                  }
                  return lhs.relevance > rhs.relevance; });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, 
                                                     std::string_view raw_query, 
                                                     DocumentPredicate document_predicate) const {
    const size_t MAX_RESULT_DOCUMENT_COUNT = 5;
    const double EPSILON = 1e-6;
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);
    std::sort(policy, matched_documents.begin(), matched_documents.end(),
        [EPSILON](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            return lhs.relevance > rhs.relevance; });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, 
                                                     std::string_view raw_query,
                                                     DocumentStatus status) const {
    return SearchServer::FindTopDocuments(policy, raw_query,
        [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status; });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, 
                                                     std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
                                                     DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&,
                                                     const Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(100);
    std::for_each(std::execution::par,
                  query.plus_words.begin(), query.plus_words.end(),
                  [this, &document_to_relevance, document_predicate](const std::string_view word) {
                       if (word_to_document_freqs_.count(word) == 0) return;
                       const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                       for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                           const auto& document_data = documents_.at(document_id);
                           if (document_predicate(document_id, document_data.status, document_data.rating)) {
                               document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                           }
                       }
                  });
    std::for_each(std::execution::par,
                  query.minus_words.begin(), query.minus_words.end(),
                  [this, &document_to_relevance](const std::string_view word) {
                      for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                          document_to_relevance.Erase(document_id);
                      }
                  });
    auto result = document_to_relevance.BuildOrdinaryMap();
    std::vector<Document> matched_documents(result.size());
    std::transform(std::execution::par, 
                   result.begin(), result.end(), 
                   matched_documents.begin(), 
                   [this](const auto& doc) {
                       return Document{ doc.first, doc.second, documents_.at(doc.first).rating };
                   });
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&, 
                                                     const Query& query, DocumentPredicate document_predicate) const {
    return SearchServer::FindAllDocuments(query, document_predicate);
}