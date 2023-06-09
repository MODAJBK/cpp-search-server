#pragma once

#include <algorithm>
#include <cstdlib>
#include <map>
#include <vector>
#include <mutex>
#include <cstdint>

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Bucket {
        std::mutex m;
        std::map<Key, Value> single_bucket;
    };

    struct Access {
        Access(const Key& key, Bucket& bucket)
            : guard(bucket.m)
            , ref_to_value(bucket.single_bucket[key])
        {}

        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
        : buckets_(bucket_count)
        , size_(bucket_count)
    {}

    Access operator[](const Key& key) {
        size_t index = (static_cast<uint64_t>(key) % size_);
        return { key, buckets_[index] };
    }

    void Erase(const Key& key) {
        size_t index = (static_cast<uint64_t>(key) % size_);
        buckets_[index].single_bucket.erase(key);
        return;
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (size_t i = 0; i < size_; ++i) {
            std::lock_guard<std::mutex> guard(buckets_[i].m);
            result.insert(buckets_[i].single_bucket.begin(), buckets_[i].single_bucket.end());
        }
        return result;
    }

private:
    std::vector<Bucket> buckets_;
    size_t size_ = 0;
};
