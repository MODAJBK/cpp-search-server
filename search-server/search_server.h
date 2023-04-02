#pragma once

#include <cmath>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <iterator>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"

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
    std::vector<int>::const_iterator begin() const;
    std::vector<int>::const_iterator end() const;
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string&, int) const;
    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;
    void RemoveDocument(int document_id);

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
    std::map<int, std::map<std::string, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;
    std::map<std::string, double> empty_map_;

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
    using namespace std::string_literals;
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
    for(const auto [document_id, word_tf] : document_to_word_freqs_){
        bool minus_word_found = false;
        for (const std::string& word : query.minus_words){
            if (word_tf.count(word) == 0) continue;
            minus_word_found = true;
        } 
        if(!minus_word_found){
            const auto& document_data = documents_.at(document_id);
            for (const std::string& word : query.plus_words){
                if(word_tf.count(word) == 0) continue;
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += word_tf.at(word) * inverse_document_freq;
                }
            }
        }
    }
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}