#pragma once

#include <string>
#include <vector>
#include <deque>
#include <cstdint>

#include "search_server.h"
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer&);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string&, DocumentPredicate);
    std::vector<Document> AddFindRequest(const std::string&, DocumentStatus);
    std::vector<Document> AddFindRequest(const std::string&);
    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        int64_t time_for_request;
        int result;
    };
    const SearchServer& server_;
    std::deque<QueryResult> requests_;
    int failed_requests_;
    int64_t time_;
    const static int min_in_day_ = 1440;

    void AddRequest(int);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    const auto result = server_.FindTopDocuments(raw_query, document_predicate);
    AddRequest(static_cast<int>(result.size()));
    return result;
}