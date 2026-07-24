#include <vector>
#include <optional>
#include <iostream>

template <typename T>
class RingBuffer {
private:
    std::vector<T> buffer;
    size_t write_idx = 0; // Often called rear, tail, or real
    size_t read_idx  = 0; // Often called head or front
    size_t count     = 0; // Number of elements currently stored
    size_t capacity  = 0;

public:
    explicit RingBuffer(size_t size) : buffer(size), capacity(size) {}

    bool enqueue(const T& item) {
        if (count == capacity) {
            return false; // Buffer is full!
        }

        // 1. Store the item at the write position
        buffer[write_idx] = item;

        // 2. Advance the write index with wrap-around
        write_idx = (write_idx + 1) % capacity;

        // 3. Increment the current element count
        count++;

        return true;
    }

    // Pulls the oldest element out of the ring buffer
    std::optional<T> dequeue() {
        if (count == 0) {
            return std::nullopt; // Buffer is empty!
        }

        // 1. Fetch the item at the read position
        T item = buffer[read_idx];

        // 2. Advance the read index with wrap-around
        read_idx = (read_idx + 1) % capacity;

        // 3. Decrement the element count
        count--;

        return item;
    }

    // Utility getters
    [[nodiscard]] size_t size() const { return count; }
    [[nodiscard]] bool empty() const { return count == 0; }
    [[nodiscard]] bool full() const { return count == capacity; }
};

int main() {
    // Create a RingBuffer of integers with a max capacity of 5 slots
    RingBuffer<int> rb(5);

    std::cout << "--- Enqueueing Items ---" << std::endl;
    for (int i = 10; i <= 50; i += 10) {
        if (rb.enqueue(i)) {
            std::cout << "Enqueued: " << i << std::endl;
        }
    }

    // Try enqueueing to a full buffer to verify bounds check
    if (!rb.enqueue(60)) {
        std::cout << "Enqueue 60 failed: Buffer is FULL!" << std::endl;
    }

    std::cout << "\n--- Dequeueing Items ---" << std::endl;
    while (!rb.empty()) {
        auto val = rb.dequeue();
        if (val.has_value()) {
            std::cout << "Dequeued: " << val.value() << std::endl;
        }
    }

    // Try dequeueing from an empty buffer
    auto empty_val = rb.dequeue();
    if (!empty_val.has_value()) {
        std::cout << "Dequeue failed: Buffer is EMPTY!" << std::endl;
    }

    return 0;
}
