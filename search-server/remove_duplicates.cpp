#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    using namespace std::string_literals;
    LOG_DURATION_STREAM("Duration: "s, std::cout);
    std::vector<int> documents_for_delete;
    std::set<std::set<std::string>> words;
    for (const int document_id : search_server) {
        const auto word_tf = search_server.GetWordFrequencies(document_id);
        std::set<std::string> doc_words;
        std::transform(word_tf.begin(), word_tf.end(), std::inserter(doc_words, doc_words.begin()), [](const auto& key_value) {
            return key_value.first;
            });
        if (words.count(doc_words)) {
            std::cout << "Found duplicate document id "s << document_id << "\n";
            documents_for_delete.emplace_back(document_id);
        }
        else {
            words.emplace(doc_words);
        }
    }
    for (auto document_id : documents_for_delete) {
        search_server.RemoveDocument(document_id);
    }
}