#pragma once
#include <cassert>
#include <corecrt_malloc.h>


template <typename T>
class UtlVector {
public:
    constexpr T& operator[](int i) noexcept { return memory[i]; }
    constexpr const T& operator[](int i) const noexcept { return memory[i]; }

    T* memory;
    int allocationCount;
    int growSize;
    int size;
    T* elements;

	template< class T >
	void grow(int num) {
		if (size == -1) {
			assert(0);
			return;
		}

		int allocationRequested = allocationCount + num;
		while (allocationCount < allocationRequested) {
			if (allocationCount != 0) {
				if (growSize) {
					allocationCount += growSize;
				}
				else {
					allocationCount += allocationCount;
				}
			}
			else {
				// Compute an allocation which is at least as big as a cache line...
				allocationCount = (31 + sizeof(T)) / sizeof(T);
				assert(allocationCount != 0);
			}
		}

		if (memory) {
			memory = (T*)realloc(memory, allocationCount * sizeof(T));
		}
		else {
			memory = (T*)malloc(allocationCount * sizeof(T));
		}
	}

    template< class T >
    void growVector(int num = 1) {
        if (size + num - 1 >= allocationCount) {
            grow<T>(size + num - allocationCount);
        }

        size += num;
    }

	template< class T >
	inline bool isValidIndex(int i) const {
		return (i >= 0) && (i < size);
	}

	template< class T >
	inline T& element(int i) {
		assert(isValidIndex<T>(i));
		return memory[i];
	}


	template< class T >
	void shiftElementsRight(int elem, int num = 1) {
		assert(isValidIndex<T>(elem) || (size == 0) || (num == 0));
		int numToMove = size - elem - num;
		if ((numToMove > 0) && (num > 0))
			memmove(&element<T>(elem + num), &element<T>(elem), numToMove * sizeof(T));
	}

	template <class T>
	inline void construct(T* pMemory) {
		::new(pMemory) T;
	}

	template <class T>
	inline void destruct(T* pMemory) {
		pMemory->~T();
	}

	template <class T>
	inline void copyConstruct(T* pMemory, T const& src) {
		::new(pMemory) T(src);
	}

	template< class T >
	int insertBefore(int elem, T const& src) {
		assert((elem == size) || isValidIndex<T>(elem));

		growVector<T>();
		shiftElementsRight<T>(elem);
		copyConstruct(&element<T>(elem), src);
		return elem;
	}

	template< class T >
	inline int addToTail(T const& src) {
		return insertBefore<T>(size, src);
	}
	
	template< class T >
	void removeAll() {
		for (int i = size; --i >= 0;)
			destruct<T>(&element<T>(i));

		size = 0;
	}
};