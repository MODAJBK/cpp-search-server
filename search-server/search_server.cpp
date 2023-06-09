#include "search_server.h"

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer::SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const std::string& stop_words_text) 
    : SearchServer::SearchServer(std::string_view(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,
                               const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }
    const auto words = SearchServer::SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view& word : words) {
        auto& word_to_insert = storage_.emplace_back(std::string(word));
        word_to_document_freqs_[word_to_insert][document_id] += inv_word_count;
        document_to_word_[document_id][word_to_insert] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ SearchServer::ComputeAverageRating(ratings), status });
    document_ids_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(raw_query, 
            [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;});
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    if (document_ids_.count(document_id) == 0) throw std::out_of_range("Invalid document id");
    const auto query = SearchServer::ParseQuery(raw_query);
    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { std::vector<std::string_view>{}, documents_.at(document_id).status };
        }
    }
    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, 
                                                                                      std::string_view raw_query, int document_id) const {
    return SearchServer::MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&,
                                                                                      std::string_view raw_query, int document_id) const {
    if (document_ids_.count(document_id) == 0) throw std::out_of_range("Invalid document id");
    auto query = SearchServer::ParseQuery(raw_query, false);
    if (std::any_of(std::execution::par,
                    query.minus_words.begin(), query.minus_words.end(),
                    [this, document_id](std::string_view word) {return document_to_word_.at(document_id).count(word); })) {
        return { std::vector<std::string_view>{}, documents_.at(document_id).status};
    }
    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto it = std::copy_if(std::execution::par,
                           query.plus_words.begin(), query.plus_words.end(),
                           matched_words.begin(),
                           [this, document_id](std::string_view word) {return document_to_word_.at(document_id).count(word); });
    std::sort(matched_words.begin(), it);
    matched_words.erase(std::unique(matched_words.begin(), it), matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> empty_map;
    if (document_to_word_.count(document_id) == 0) {
        return empty_map;
    }
    return document_to_word_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    if (document_ids_.count(document_id) == 0) throw std::out_of_range("Invalid document id");
    for (const auto& [word, freq] : document_to_word_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
        if (word_to_document_freqs_.at(word).empty()) word_to_document_freqs_.erase(word);
    }
    document_to_word_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    SearchServer::RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if (document_ids_.count(document_id) == 0) throw std::out_of_range("Invalid document id");
    auto& word_to_freq = GetWordFrequencies(document_id);
    std::vector<const std::string_view*> words_to_delete(word_to_freq.size());
    std::transform(std::execution::par, 
                   word_to_freq.begin(), word_to_freq.end(), 
                   words_to_delete.begin(),
                   [](const auto& key_value) {return &key_value.first; });
    std::for_each(std::execution::par, 
                  words_to_delete.begin(), words_to_delete.end(), 
                  [this, document_id](const std::string_view* word) { 
                      word_to_document_freqs_.at(*word).erase(document_id);});
    document_to_word_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    using namespace std::string_literals;
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("Word: "s + std::string(word) + " is invalid"s);
        }
        if (!SearchServer::IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) return 0;
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    using namespace std::string_literals;
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !SearchServer::IsValidWord(word)) {
        throw std::invalid_argument("Query word: "s + std::string(text) + " is invalid");
    }
    return { word, is_minus, SearchServer::IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text, bool is_sort_need) const {
    SearchServer::Query result;
    for (std::string_view word : SplitIntoWords(text)) {
        const auto query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.emplace_back(query_word.data);
            }
            else {
                result.plus_words.emplace_back(query_word.data);
            }
        }
    }
    if (is_sort_need) {
        std::sort(result.plus_words.begin(), result.plus_words.end());
        std::sort(result.minus_words.begin(), result.minus_words.end());
        result.plus_words.erase(std::unique(result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());
        result.minus_words.erase(std::unique(result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}