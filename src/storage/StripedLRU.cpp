#include <functional>
#include <stdexcept>

#include "StripedLRU.h"

namespace Afina {
namespace Backend {

StripedLRU StripedLRU::Create_StripedLRU(size_t memory_limit, size_t stripe_count) {
    size_t stripe_limit = memory_limit / stripe_count;
    if (stripe_limit < 1024) {
        throw std::runtime_error("Cache size of each shard is too small (less than 1Kb)");
    }
    return StripedLRU(stripe_limit, stripe_count);
}

StripedLRU::StripedLRU(StripedLRU&& other): _shards(std::move(other._shards)) {}


StripedLRU::StripedLRU (size_t stripe_limit, size_t stripe_count) {
    _shards.reserve(stripe_count);
    for (size_t i = 0; i < stripe_count; ++i){
        _shards.emplace_back(new ThreadSafeSimplLRU(stripe_limit));
    }
}

bool StripedLRU::Put(const std::string &key, const std::string &value) {
    return _shards[_GetShardNum(key)]->Put(key, value);
}

bool StripedLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    return _shards[_GetShardNum(key)]->PutIfAbsent(key, value);
}

bool StripedLRU::Set(const std::string &key, const std::string &value) {
    return _shards[_GetShardNum(key)]->Set(key, value);
}

bool StripedLRU::Delete(const std::string &key) {
    return _shards[_GetShardNum(key)]->Delete(key);
}

bool StripedLRU::Get(const std::string &key, std::string &value) {
    return _shards[_GetShardNum(key)]->Get(key, value);
}

size_t StripedLRU::_GetShardNum(const std::string &key) {
    return std::hash<std::string>{}(key) % _shards.size();
}

} // namespace Backend
} // namespace Afina