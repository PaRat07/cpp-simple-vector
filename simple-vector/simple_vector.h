#pragma once

#include <cassert>
#include <initializer_list>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <utility>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t size)
            :size_(size)
    {

    }
    size_t size_;
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept
            : size_(0)
            , capacity_(0)
    {

    }

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
            : items_(new Type[size])
            , size_(size)
            , capacity_(size)
    {
        std::fill(items_.Get(), items_.Get() + size, Type());
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
            : items_(new Type[size])
            , size_(size)
            , capacity_(size)
    {
        std::fill(items_.Get(), items_.Get() + capacity_, value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
            : items_(new Type[init.size()])
            , size_(init.size())
            , capacity_(init.size())
    {
        std::copy(init.begin(), init.end(), items_.Get());
    }

    SimpleVector(const SimpleVector& other) {
        ArrayPtr<Type> buffer(new Type[other.capacity_]);
        std::copy(other.begin(), other.end(), &buffer[0]);
        items_.swap(buffer);
        size_ = other.size_;
        capacity_ = other.capacity_;
    }

    SimpleVector(SimpleVector&& other) noexcept
            : items_(new Type[other.capacity_])
            , size_(std::move(other.size_))
            , capacity_(std::move(other.capacity_))
    {
        std::copy(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()), begin());
        other.Clear();
    }

    SimpleVector(const ReserveProxyObj& x)
            : size_(0)
            , capacity_(0)
    {
        Reserve(x.size_);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        ArrayPtr<Type> buffer(new Type[rhs.capacity_]);
        std::copy(rhs.begin(), rhs.end(), &buffer[0]);
        items_.swap(buffer);
        size_ = rhs.size_;
        capacity_ = rhs.capacity_;
        return *this;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index >= 0 && index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("index is bigger then size");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("index is bigger then size");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            const int real_new_size = capacity_ * 2 > new_size ? capacity_ * 2 : new_size;
            ArrayPtr<Type> buffer(new Type[real_new_size]);
            for (auto it = buffer.Get() + size_; it != buffer.Get() + real_new_size; ++it) {
                *it = std::move(Type());
            }
            std::copy(std::make_move_iterator(items_.Get()), std::make_move_iterator(items_.Get() + size_), buffer.Get());
            items_.swap(buffer);
            size_ = real_new_size;
            capacity_ = real_new_size;
        }
        else {
            if (new_size > size_) {
                for (size_t i = size_; i <= new_size; ++i) {
                    items_[i] = std::move(Type());
                }
            }
            size_ = new_size;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(Type&& item) {
        if (capacity_ == size_) {
            ArrayPtr<Type> buffer(capacity_ == 0 ? 1 : capacity_ * 2);
            std::copy(begin(), end(), &buffer[0]);
            items_.swap(buffer);
            capacity_ = capacity_ == 0 ? 1 : capacity_ * 2;
        }
        items_[size_++] = std::exchange(item, Type());
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (capacity_ == size_) {
            ArrayPtr<Type> buffer(capacity_ == 0 ? 1 : capacity_ * 2);
            std::copy(begin(), end(), &buffer[0]);
            items_.swap(buffer);
            capacity_ = capacity_ == 0 ? 1 : capacity_ * 2;
        }
        items_[size_++] = item;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos < end());
        if (size_ == capacity_) {
            const int num_of_inserted_elem = std::distance(&items_[0], const_cast<Iterator>(pos));
            ArrayPtr<Type> buffer(new Type[capacity_ == 0 ? 1 : capacity_ * 2]);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(const_cast<Iterator>(pos)), &buffer[0]);
            buffer[num_of_inserted_elem] = std::move(value);
            std::copy(std::make_move_iterator(const_cast<Iterator>(pos)), std::make_move_iterator(end()), &buffer[num_of_inserted_elem + 1]);
            items_.swap(buffer);
            capacity_ = capacity_ == 0 ? 1 : capacity_ * 2;
            ++size_;
            return &items_[num_of_inserted_elem];
        }
        else {
            std::copy(std::make_move_iterator(const_cast<Iterator>(pos)), std::make_move_iterator(end()), const_cast<Iterator>(pos) + 1);
            *const_cast<Iterator>(pos) = std::move(value);
            ++size_;
            return const_cast<Iterator>(pos);
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos < end());
        if (size_ == capacity_) {
            const int num_of_inserted_elem = std::distance(&items_[0], const_cast<Iterator>(pos));
            ArrayPtr<Type> buffer(new Type[capacity_ == 0 ? 1 : capacity_ * 2]);
            std::copy(begin(), const_cast<Iterator>(pos), buffer.Get());
            buffer[num_of_inserted_elem] = value;
            std::copy(const_cast<Iterator>(pos)), end(), buffer.Get()[num_of_inserted_elem + 1];
            items_.swap(buffer);
            capacity_ = capacity_ == 0 ? 1 : capacity_ * 2;
            ++size_;
            return &items_[num_of_inserted_elem];
        }
        else {
            std::copy(const_cast<Iterator>(pos), end(), const_cast<Iterator>(pos) + 1);
            *const_cast<Iterator>(pos) = value;
            ++size_;
            return const_cast<Iterator>(pos);
        }
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(size_ > 0);
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(size_ > 0);
        assert(pos >= begin() && pos < end());
        std::copy(std::make_move_iterator(const_cast<Iterator>(pos) + 1), std::make_move_iterator(end()), const_cast<Iterator>(pos));
        --size_;
        return const_cast<Iterator>(pos);
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> buffer(new Type[new_capacity]);
            std::copy(begin(), end(), buffer.Get());
            items_.swap(buffer);
            capacity_ = new_capacity;
        }
    }
private:
    ArrayPtr<Type> items_;
    size_t size_, capacity_;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs.GetSize() == rhs.GetSize() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs < rhs || lhs == rhs;
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs <= rhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}


ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}