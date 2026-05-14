#pragma once

#include "base.h"

template <typename T>
struct SPSCQueue
{
  usize head{};
  usize tail{};
  usize capacity{};
  T* buffer{};
};

template <typename T>
SPSCQueue<T> spsc_queue_init(usize capacity);
template <typename T>
void spsc_queue_uninit(SPSCQueue<T>& queue);

template <typename T>
bool spsc_queue_push(SPSCQueue<T>& queue, const T& item);
template <typename T>
bool spsc_queue_consume_one(SPSCQueue<T>& queue, T& out);

// NOTE: implementation

template <typename T>
SPSCQueue<T> spsc_queue_init(usize capacity)
{
  SPSCQueue<T> queue{};
  queue.buffer = (T*) malloc(capacity * sizeof(T));
  ASSERT(queue.buffer, "Failed to alloc memory for SPSC queue.");
  queue.capacity = capacity;
  return queue;
}

template <typename T>
void spsc_queue_uninit(SPSCQueue<T>& queue)
{
  free(queue.buffer);
}

template <typename T>
bool spsc_queue_push(SPSCQueue<T>& queue, const T& item)
{
  usize curr_tail = atomic_load_usize(&queue.tail, ATOMIC_MEMORY_ORDER_RELAXED);
  usize next_tail = (curr_tail + 1) % queue.capacity;
  if (next_tail == atomic_load_usize(&queue.head, ATOMIC_MEMORY_ORDER_ACQUIRE))
  {
    return false;
  }
  queue.buffer[curr_tail] = item;
  atomic_store_usize(&queue.tail, next_tail, ATOMIC_MEMORY_ORDER_RELEASE);
  return true;
}

template <typename T>
bool spsc_queue_consume_one(SPSCQueue<T>& queue, T& out)
{
  usize curr_head = atomic_load_usize(&queue.head, ATOMIC_MEMORY_ORDER_RELAXED);
  if (curr_head == atomic_load_usize(&queue.tail, ATOMIC_MEMORY_ORDER_ACQUIRE))
  {
    return false;
  }
  out = queue.buffer[curr_head];
  atomic_store_usize(&queue.head, (curr_head + 1) % queue.capacity, ATOMIC_MEMORY_ORDER_RELEASE);
  return true;
}
