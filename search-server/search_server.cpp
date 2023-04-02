#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer::SearchServer(SplitIntoWords(stop_words_text))  
{
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    using namespace std::string_literals;
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SearchServer::SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ SearchServer::ComputeAverageRating(ratings), status });
    document_ids_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::vector<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::vector<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
    int document_id) const {
    using namespace std::string_literals;
    if (document_to_word_freqs_.count(document_id) == 0) {
        throw std::invalid_argument("Document with such id haven't found"s);
    }
    const auto query = SearchServer::ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (document_to_word_freqs_.at(document_id).count(word) == 0) {
            continue;
        }
        matched_words.push_back(word);
    }
    for (const std::string& word : query.minus_words) {
        if (document_to_word_freqs_.at(document_id).count(word) == 0) {
            continue;
        }
        matched_words.clear();
        break;
    }
    return { matched_words, documents_.at(document_id).status };
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const { 
    if (document_to_word_freqs_.count(document_id) == 0) {
        const auto& empty_link = empty_map_;
        return empty_link;
    }
    const auto& word_to_tf = document_to_word_freqs_.at(document_id);
    return word_to_tf;
}

void SearchServer::RemoveDocument(int document_id) {
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(find(document_ids_.begin(), document_ids_.end(), document_id));
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    using namespace std::string_literals;
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("Word: "s + word + " is invalid"s);
        }
        if (!SearchServer::IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    using namespace std::string_literals;
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !SearchServer::IsValidWord(word)) {
        throw std::invalid_argument("Query word: "s + text + " is invalid");
    }
    return { word, is_minus, SearchServer::IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    SearchServer::Query result;
    for (const std::string& word : SplitIntoWords(text)) {
        const auto query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    double word_count = 1.0 * std::count_if(document_to_word_freqs_.begin(), document_to_word_freqs_.end(), [word](const auto& word_tf){
        return word_tf.second.count(word);
    });
    return std::log(GetDocumentCount() * 1.0 / word_count);
}