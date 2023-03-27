#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) 
     : server_(search_server)
     , failed_requests_(0)
     , time_(0)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    return RequestQueue::AddFindRequest(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    }

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return RequestQueue::AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
    return failed_requests_;
}

void RequestQueue::AddRequest(int result) {
    ++time_;
    while (!requests_.empty() && time_ - requests_.front().time_for_request >= min_in_day_){
        if (requests_.front().result == 0) --failed_requests_;
        requests_.pop_front();
    }
    requests_.push_back({time_, result});
    if (result == 0) ++failed_requests_;
}