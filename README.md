
Markdown
# C++ Templated Ring Buffer (Circular Buffer)

A lightweight, header-only, templated circular buffer implementation written in modern C++17. Designed for predictable FIFO streaming, low overhead, and safe memory management on embedded targets like the **Raspberry Pi Compute Module 4 (CM4)**.

---

## Key Features

* **C++17 Type Safety:** Uses `std::optional<T>` for error-free dequeueing without relying on exceptions or magic dummy values on empty buffer states.
* **Header-Only & Templated:** Easily supports arbitrary payload types (`int`, structs, raw byte buffers, floats) without heap allocation overhead after initialization.
* **Bounds-Checked Protection:**
  * Rejects enqueue operations when full (`enqueue` returns `false`).
  * Rejects dequeue operations when empty (`dequeue` returns `std::nullopt`).
* **Zero Dynamic Reallocation:** Allocates underlying vector storage once at construction time to ensure bounded memory usage and deterministic performance during runtime.

---

## API Overview

```cpp
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t size);           // Allocates buffer capacity
    bool enqueue(const T& item);                // Pushes item; returns false if full
    std::optional<T> dequeue();                 // Pops oldest item; returns std::nullopt if empty
    
    [[nodiscard]] size_t size() const;          // Returns current stored element count
    [[nodiscard]] bool empty() const;           // Returns true if count == 0
    [[nodiscard]] bool full() const;            // Returns true if count == capacity
};
Hardware Execution & Build Instructions
Tested and executed on Raspberry Pi CM4 running Linux.

1. Compile
Compile using g++ or clang++ with standard C++17 support enabled:

Bash
g++ -std=c++17 -O2 main.cpp -o ring_buffer
2. Run
Bash
./ring_buffer
Verification Output
Below is the verified execution trace confirming modulo wrap-around logic and strict overflow/underflow bounds handling:

Plaintext
--- Enqueueing Items ---
Enqueued: 10
Enqueued: 20
Enqueued: 30
Enqueued: 40
Enqueued: 50
Enqueue 60 failed: Buffer is FULL!

--- Dequeueing Items ---
Dequeued: 10
Dequeued: 20
Dequeued: 30
Dequeued: 40
Dequeued: 50
Dequeue failed: Buffer is EMPTY!
