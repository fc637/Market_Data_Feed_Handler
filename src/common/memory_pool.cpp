#include "protocol.h"

MemoryPool::MemoryPool(size_t block_size, size_t block_count)
    : block_size_(block_size),
      storage_(block_size * block_count) {

    free_list_.reserve(block_count);

    for (size_t i = 0; i < block_count; ++i) {
        free_list_.push_back(storage_.data() + i * block_size_);
    }
}

void* MemoryPool::allocate() {
    if (free_list_.empty())
        return nullptr;
    void* ptr = free_list_.back();
    free_list_.pop_back();
    return ptr;
}

void MemoryPool::deallocate(void* ptr) {
    free_list_.push_back(ptr);
}