#pragma once

#include "base.h"

template <typename T>
struct SPSCQueue
{
  u32 head{};
  u32 tail{};
  u32 capacity{};
  T* buffer{};
};

template <typename T>
SPSCQueue<T> spsc_queue_init(u32 capacity);
template <typename T>
void spsc_queue_deinit(SPSCQueue<T>& queue);
template <typename T>
bool spsc_queue_push(SPSCQueue<T>& queue, const T& item);
template <typename T>
bool spsc_queue_consume_one(SPSCQueue<T>& queue, T& out);

// NOTE: implementation

template <typename T>
SPSCQueue<T> spsc_queue_init(u32 capacity)
{
  SPSCQueue<T> queue{};
  queue.buffer = (T*) malloc(capacity * sizeof(T));
  ASSERT(queue.buffer, "Failed to alloc memory for SPSC queue.");
  queue.capacity = capacity;
  return queue;
}

template <typename T>
void spsc_queue_deinit(SPSCQueue<T>& queue)
{
  free(queue.buffer);
}

template <typename T>
bool spsc_queue_push(SPSCQueue<T>& queue, const T& item)
{
  u32 curr_tail = atomic_load_32(&queue.tail, ATOMIC_MEMORY_ORDER_RELAXED);
  u32 next_tail = (curr_tail + 1) % queue.capacity;
  if (next_tail == atomic_load_32(&queue.head, ATOMIC_MEMORY_ORDER_ACQUIRE))
  {
    return false;
  }
  queue.buffer[curr_tail] = item;
  atomic_store_32(&queue.tail, next_tail, ATOMIC_MEMORY_ORDER_RELEASE);
  return true;
}

template <typename T>
bool spsc_queue_consume_one(SPSCQueue<T>& queue, T& out)
{
  u32 curr_head = atomic_load_32(&queue.head, ATOMIC_MEMORY_ORDER_RELAXED);
  if (curr_head == atomic_load_32(&queue.tail, ATOMIC_MEMORY_ORDER_ACQUIRE))
  {
    return false;
  }
  out = queue.buffer[curr_head];
  atomic_store_32(&queue.head, (curr_head + 1) % queue.capacity, ATOMIC_MEMORY_ORDER_RELEASE);
  return true;
}
