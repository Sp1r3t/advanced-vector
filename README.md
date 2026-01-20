# Custom C++ Vector Implementation

This repository contains a custom implementation of a dynamic array (`Vector`) in C++, along with a low-level memory management helper class (`RawMemory`). The project demonstrates manual memory handling, placement new, and exception-safe operations in modern C++.

---

## Features

- **Dynamic resizing** with exponential growth.
- **Manual memory management** using `RawMemory`.
- **Move semantics and copy semantics** support.
- **Element insertion and deletion** at arbitrary positions.
- **Exception safety** for element construction and movement.
- **Iterators** for range-based loops.
- **Emplacement** of elements to construct them in-place.
- Compatible with any type `T`, including non-copyable or non-movable types.

---

## Classes

### `RawMemory<T>`

A low-level helper class that manages a raw memory buffer for objects of type `T`.

**Main functionalities:**
- Allocate and deallocate raw memory.
- Provide pointer arithmetic (`operator+`).
- Access elements safely (`operator[]`).
- Swap memory buffers between objects.
- Query buffer capacity.

**Example:**
```cpp
RawMemory<int> buffer(10); // allocate memory for 10 ints
buffer[0] = 42;            // assign value
