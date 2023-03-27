#include "document.h"

using namespace std::string_literals;

std::ostream& operator<<(std::ostream& os, const Document document) {
    return os << "{ "s << "document_id = "s << document.id
        << ", relevance = "s << document.relevance
        << ", rating = "s << document.rating << " }"s;
}