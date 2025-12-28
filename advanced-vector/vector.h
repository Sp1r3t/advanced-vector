#pragma once

#include <cassert>
#include <cstdlib>
#include <new>
#include <memory>
#include <utility>
#include <algorithm>
#include <type_traits>
#include <stdexcept>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept
        : buffer_(other.buffer_), capacity_(other.capacity_) {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:

    Vector() = default;

    Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }

    Vector(const Vector& vector)
        : data_(vector.size_)
        , size_(vector.size_)
    {
        std::uninitialized_copy_n(vector.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_(other.size_)
    {
        other.size_ = 0;
    }

    Vector& operator=(const Vector& rhs) {
        if (this == &rhs) return *this;

        if (rhs.size_ > data_.Capacity()) {
            Vector temp(rhs);
            Swap(temp);
        } else {
            AssignFromSmaller(rhs);
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return (data_.GetAddress() + size_);
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return (data_.GetAddress() + size_);
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return (data_.GetAddress() + size_);
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    void Reserve(size_t capacity) {
        if (capacity <= data_.Capacity()) return;

        RawMemory<T> newData(capacity);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, newData.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, newData.GetAddress());
        }

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(newData);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
            size_ = new_size;
            return;
        }

        if (new_size <= Capacity()) {
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
            size_ = new_size;
            return;
        }

        size_t new_capacity = std::max(new_size, Capacity() == 0 ? size_t(1) : Capacity() * 2);
        RawMemory<T> newData(new_capacity);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, newData.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, newData.GetAddress());
        }

        std::uninitialized_value_construct_n(newData.GetAddress() + size_, new_size - size_);

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(newData);
        size_ = new_size;
    }

    void PushBack(const T& value) {
        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    void PopBack() noexcept {
        if (size_ == 0) return;

        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        T* ptr = nullptr;

        if (size_ < Capacity()) {
            ptr = new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
            ++size_;
            return *ptr;
        }

        size_t new_capacity = Capacity() == 0 ? 1 : Capacity() * 2;
        RawMemory<T> newData(new_capacity);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, newData.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, newData.GetAddress());
        }

        try {
            ptr = new (newData.GetAddress() + size_) T(std::forward<Args>(args)...);
        } catch (...) {
            std::destroy_n(newData.GetAddress(), size_);
            throw;
        }

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(newData);
        ++size_;
        return *ptr;
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        if (pos < begin() || pos > end()) {
            throw std::out_of_range("Invalid position");
        }

        size_t index = pos - begin();

        if (size_ == Capacity()) {
            size_t new_capacity = (size_ == 0) ? 1 : size_ * 2;
            RawMemory<T> new_data(new_capacity);

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), index, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), index, new_data.GetAddress());
            }

            T* new_item_ptr = nullptr;
            try {
                new_item_ptr = new (new_data.GetAddress() + index) T(std::forward<Args>(args)...);
            } catch (...) {
                std::destroy_n(new_data.GetAddress(), index);
                throw;
            }

            try {
                if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                    std::uninitialized_move_n(
                        data_.GetAddress() + index,
                        size_ - index,
                        new_data.GetAddress() + index + 1
                    );
                } else {
                    std::uninitialized_copy_n(
                        data_.GetAddress() + index,
                        size_ - index,
                        new_data.GetAddress() + index + 1
                    );
                }
            } catch (...) {
                std::destroy_at(new_item_ptr);
                std::destroy_n(new_data.GetAddress(), index);
                throw;
            }

            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            ++size_;
            return begin() + index;

        } else {
            if (index == size_) {
                new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
            } else {
                T temp(std::forward<Args>(args)...);

                new (data_.GetAddress() + size_) T(std::move(data_[size_ - 1]));

                try {
                    std::move_backward(
                        data_.GetAddress() + index,
                        data_.GetAddress() + size_ - 1,
                        data_.GetAddress() + size_
                    );
                    data_[index] = std::move(temp);
                } catch (...) {
                    std::destroy_at(data_.GetAddress() + size_);
                    throw;
                }
            }
            ++size_;
            return begin() + index;
        }
    }

    iterator Erase(const_iterator pos) {
        if (pos < cbegin() || pos >= cend()) {
            throw std::out_of_range("Invalid position");
        }

        size_t index = pos - cbegin();

        for (size_t i = index; i + 1 < size_; ++i) {
            data_[i] = std::move(data_[i + 1]);
        }

        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;

        return begin() + index;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void DestroyN(T* ptr, size_t size) {
        std::destroy_n(ptr, size);
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

private:
    void AssignFromSmaller(const Vector& rhs) {
        size_t common = std::min(size_, rhs.size_);
        std::copy_n(rhs.data_.GetAddress(), common, data_.GetAddress());

        if (rhs.size_ > size_) {
            std::uninitialized_copy_n(
                rhs.data_.GetAddress() + common,
                rhs.size_ - common,
                data_.GetAddress() + common
            );
        } else {
            std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
        }
        size_ = rhs.size_;
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};