#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (_lru_index.find(key) != _lru_index.end()) {
        return Set(key, value);
    }
    return PutIfAbsent(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (_lru_index.find(key) != _lru_index.end() ||
        !_DeleteTail(key.size() + value.size())) return false;
    std::unique_ptr<lru_node> tmp(std::move(_lru_head));
    _lru_head = std::make_unique<lru_node>(key, value, nullptr, std::move(tmp));
    if (_lru_head->next){
        _lru_head->next->prev = _lru_head.get();
    }
    _lru_index.insert({_lru_head->key, *_lru_head});
    _curr_size += key.size() + value.size();
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end() || key.size() + value.size() > _max_size) return false;
    lru_node &curr_node = it->second;
    _MoveToHead(curr_node);
    _DeleteTail(key.size() + value.size() - curr_node.value.size());
    _curr_size += key.size() + value.size() - curr_node.value.size();
    curr_node.value = value;
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) return false;
    lru_node &curr_node = it->second;
    std::unique_ptr<lru_node> curr_ptr;
    if (!curr_node.prev) {
        curr_ptr = std::move(_lru_head);
        _lru_head = std::move(curr_node.next);
        _lru_head->prev = nullptr;
    }
    else {
        curr_ptr = std::move(curr_node.prev->next);
        curr_node.prev->next = std::move(curr_node.next);
        if (curr_node.next){
            curr_node.next->prev = curr_node.prev;
        }
    }
    _curr_size -= curr_node.key.size() + curr_node.value.size();
    _lru_index.erase(it);
    curr_ptr.reset();
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) return false;
    size_t len = it->second.get().value.size();
    value.resize(len);
    it->second.get().value.copy(value.data(), len);
    return _MoveToHead(it->second);
}

bool SimpleLRU::_MoveToHead(lru_node &node) {
    if (!node.prev) return true;
    std::unique_ptr<lru_node> curr_ptr(std::move(node.prev->next));
    node.prev->next = std::move(node.next);
    if (node.next) {
        node.next->prev = node.prev;
    }
    node.prev = nullptr;
    _lru_head->prev = curr_ptr.get();
    node.next = std::move(_lru_head);
    _lru_head = std::move(curr_ptr);
    return true;
}

bool SimpleLRU::_DeleteTail(std::size_t new_size) {
    if (new_size > _max_size) return false;
    if (new_size + _curr_size > _max_size){
        lru_node *ptr = _lru_head.get();
        while (ptr->next) ptr = ptr->next.get();
        while (ptr && new_size + _curr_size > _max_size) {
            _curr_size -= ptr->key.size() + ptr->value.size();
            _lru_index.erase(_lru_index.find(ptr->key));
            ptr = ptr->prev;
            if (ptr) {
                ptr->next.reset();
            }
            else {
                _lru_head.reset();
            }
        }
    }
    return true;
}

} // namespace Backend
} // namespace Afina
