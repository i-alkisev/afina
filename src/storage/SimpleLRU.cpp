#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

bool SimpleLRU::_UpdateNode(SimpleLRU::lru_node_iterator &node_it, const std::string& value) {
    lru_node &curr_node = node_it->second;
    if (curr_node.key.size() + value.size() > _max_size) return false;
    _MoveToHead(curr_node);
    /*
    Here _DeleteTail(value.size() - curr_node.value.size()) always returns True because
    value.size() - curr_node.value.size() < value.size() < value.size() + curr_node.key.size() <= _max_size
    and curr_node will remain in the head of the list
    */
    if (value.size() > curr_node.value.size()) {
        _DeleteTail(value.size() - curr_node.value.size());
    }
    _curr_size += value.size() - curr_node.value.size();
    curr_node.value = value;
    return true;
}

bool SimpleLRU::_InsertNode(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size) return false;
    _DeleteTail(key.size() + value.size());
    std::unique_ptr<lru_node> tmp(std::move(_lru_head));
    _lru_head = std::unique_ptr<lru_node>(new lru_node(key, value, nullptr, std::move(tmp)));
    if (_lru_head->next){
        _lru_head->next->prev = _lru_head.get();
    }
    _lru_index.insert({_lru_head->key, *_lru_head});
    _curr_size += key.size() + value.size();
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
    if (it != _lru_index.end()) {
        return _UpdateNode(it, value);
    }
    return _InsertNode(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (_lru_index.find(key) != _lru_index.end()) return false;
    return _InsertNode(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) return false;
    return _UpdateNode(it, value);
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
    value = it->second.get().value;
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

void SimpleLRU::_DeleteTail(std::size_t new_size) {
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
}

} // namespace Backend
} // namespace Afina
