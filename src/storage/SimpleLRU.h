#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size), _curr_size(0) {}

    ~SimpleLRU() {
        _lru_index.clear();

        // TODO: Here is stack overflow
        // _lru_head.reset();

        std::unique_ptr<lru_node> ptr(std::move(_lru_head));
        std::unique_ptr<lru_node> next;
        while(ptr){
            next = std::move(ptr->next);
            ptr.reset();
            ptr = std::move(next);
        }
    }

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
    // LRU cache node
    using lru_node = struct lru_node {
        lru_node(const std::string &key_, const std::string &value_,
                 lru_node *prev_, std::unique_ptr<lru_node>&& next_):
            key(key_), value(value_), prev(prev_), next(std::move(next_)) {}
        const std::string key;
        std::string value;
        lru_node *prev;
        std::unique_ptr<lru_node> next;
    };

    using lru_node_iterator = std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>>::iterator;

    bool _MoveToHead(lru_node &node);

    void _DeleteTail(std::size_t new_size);

    bool _UpdateNode(lru_node_iterator &node_it, const std::string& value);

    bool _InsertNode(const std::string &key, const std::string &value);

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be not greater than the _max_size
    std::size_t _max_size;
    std::size_t _curr_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>> _lru_index;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
