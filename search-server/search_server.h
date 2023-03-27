#pragma once

#include <cmath>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <algorithm>

#include "document.h"
#include "string_processing.h"

using namespace std::string_literals;

class SearchServer {
public:
    
    SearchServer() = default;
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string&);
    void AddDocument(int, const std::string&, DocumentStatus, const std::vector<int>&);
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string&, DocumentPredicate) const;
    std::vector<Document> FindTopDocuments(const std::string&, DocumentStatus) const;
    std::vector<Document> FindTopDocuments(const std::string&) const;
    size_t GetDocumentCount() const;
    int GetDocumentId(int) const;
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string&, int) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;

    bool IsStopWord(const std::string&) const;
    static bool IsValidWord(const std::string&);
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
    static int ComputeAverageRating(const std::vector<int>&);
    QueryWord ParseQueryWord(const std::string&) const;
    Query ParseQuery(const std::string& text) const;
    double ComputeWordInverseDocumentFreq(const std::string&) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query&, DocumentPredicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query,
    DocumentPredicate document_predicate) const {
    const int MAX_RESULT_DOCUMENT_COUNT = 5;
    const double EPSILON = 1e-6;
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);
    std::sort(matched_documents.begin(), matched_documents.end(),
        [&EPSILON](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
               return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
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
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}