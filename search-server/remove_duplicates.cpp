#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    using namespace std::string_literals;
    std::set<int> documents_for_delete;
    size_t count = 0, doc_numb = search_server.GetDocumentCount();
    for (const int document_id : search_server) {
        const auto word_tf1 = search_server.GetWordFrequencies(document_id);
        const auto word_numb = word_tf1.size();
        for (int i = 1; i <= doc_numb - count; ++i) {
            bool is_words_equal = true;
            const auto word_tf2 = search_server.GetWordFrequencies(document_id + i);
            auto it = word_tf1.begin();
            if (word_numb != word_tf2.size()) is_words_equal = false;
            while (is_words_equal && it != word_tf1.end()) {
                if (word_tf2.count(it->first) == 0) is_words_equal = false;
                ++it;
            }
            if (is_words_equal && documents_for_delete.count(document_id + i) == 0) {
                documents_for_delete.emplace(document_id + i);
            }
        }
        ++count;
    }
    for (auto document_id : documents_for_delete) {
        std::cout << "Found duplicate document id "s << document_id << "\n";
        search_server.RemoveDocument(document_id);
    }
}