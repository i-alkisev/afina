#ifndef AFINA_STORAGE_STRIPED_LRU_H
#define AFINA_STORAGE_STRIPED_LRU_H

#include <vector>

#include "afina/Storage.h"
#include "ThreadSafeSimpleLRU.h"

namespace Afina {
namespace Backend {

class StripedLRU: public Afina::Storage {
public:
    StripedLRU (size_t memory_limit = 4 * 1024, size_t stripe_count = 4);
    ~StripedLRU() {}

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;

private:

    size_t _GetShardNum(const std::string &key);

    std::vector<std::unique_ptr<ThreadSafeSimplLRU>> _shards;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_STRIPED_LRU_H