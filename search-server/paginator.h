#pragma once

#include <iterator>
#include <iostream>
#include <vector>
#include <algorithm>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange() = default;

    IteratorRange(const Iterator begin, const Iterator end)
        : begin_(begin)
        , end_(end) {
    }

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    size_t size() const {
        return distance(begin_, end_);
    }

private:
    Iterator begin_;
    Iterator end_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator() = default;

    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (size_t left = std::distance(begin, end); left > 0;) {
            const size_t current_page_size = std::min(page_size, left);
            const Iterator current_page_end = std::next(begin, current_page_size);
            paginator_.push_back({ begin, current_page_end });
            left -= current_page_size;
            begin = current_page_end;
        }
    }

    auto begin() const {
        return paginator_.begin();
    }

    auto end() const {
        return paginator_.end();
    }

    auto size() const {
        return paginator_.size();
    }
private:
    std::vector<IteratorRange<Iterator>> paginator_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, const IteratorRange<Iterator>& iterator_range) {
    for (auto it = iterator_range.begin(); it != iterator_range.end(); ++it) {
        output << *it;
    }
    return output;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(std::begin(c), std::end(c), page_size);
}