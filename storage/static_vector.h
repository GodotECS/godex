/*************************************************************************/
/*  static_vector.h                                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef STATIC_VECTOR_H
#define STATIC_VECTOR_H

#include "core/error/error_macros.h"
#include "core/os/copymem.h"
#include "core/os/memory.h"
#include "core/templates/sort_array.h"
#include "core/templates/vector.h"

template <class T, uint32_t SIZE, class U = uint32_t, bool force_trivial = false>
class StaticVector {
private:
	U count = 0;
	T data[SIZE];

public:
	T *ptr() {
		return data;
	}

	const T *ptr() const {
		return data;
	}

	_FORCE_INLINE_ void push_back(T p_elem) {
		CRASH_COND_MSG(count >= SIZE, "Out of memory");

		if (!__has_trivial_constructor(T) && !force_trivial) {
			memnew_placement(&data[count++], T(p_elem));
		} else {
			data[count++] = p_elem;
		}
	}

	void remove(U p_index) {
		ERR_FAIL_UNSIGNED_INDEX(p_index, count);
		count--;
		for (U i = p_index; i < count; i++) {
			data[i] = data[i + 1];
		}
		if (!__has_trivial_destructor(T) && !force_trivial) {
			data[count].~T();
		}
	}

	// Removes the item copying the last value into the position of the one to
	// remove. It's generally faster than `remove`.
	void remove_unordered(U p_index) {
		ERR_FAIL_UNSIGNED_INDEX(p_index, count);
		count--;
		if (count > p_index) {
			data[p_index] = data[count];
		}
		if (!__has_trivial_destructor(T) && !force_trivial) {
			data[count].~T();
		}
	}

	void erase(const T &p_val) {
		const int64_t idx = find(p_val);
		if (idx >= 0) {
			remove(idx);
		}
	}

	void invert() {
		for (U i = 0; i < count / 2; i++) {
			SWAP(data[i], data[count - i - 1]);
		}
	}

	_FORCE_INLINE_ void clear() { count = 0; }

	_FORCE_INLINE_ bool is_empty() const { return count == 0; }

	_FORCE_INLINE_ U size() const { return count; }
	_FORCE_INLINE_ U capacity() const { return SIZE; }

	_FORCE_INLINE_ const T &operator[](U p_index) const {
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		return data[p_index];
	}
	_FORCE_INLINE_ T &operator[](U p_index) {
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		return data[p_index];
	}

	void insert(U p_pos, T p_val) {
		ERR_FAIL_COND(count >= SIZE);
		ERR_FAIL_UNSIGNED_INDEX(p_pos, count + 1);
		if (p_pos == count) {
			push_back(p_val);
		} else {
			count += 1;
			for (U i = count; i > p_pos; i--) {
				data[i] = data[i - 1];
			}
			data[p_pos] = p_val;
		}
	}

	int64_t find(const T &p_val, U p_from = 0) const {
		for (U i = 0; i < count; i++) {
			if (data[i] == p_val) {
				return int64_t(i);
			}
		}
		return -1;
	}

	template <class C>
	void sort_custom() {
		U len = count;
		if (len == 0) {
			return;
		}

		SortArray<T, C> sorter;
		sorter.sort(data, len);
	}

	void sort() {
		sort_custom<_DefaultComparator<T>>();
	}

	void ordered_insert(T p_val) {
		U i;
		for (i = 0; i < count; i++) {
			if (p_val < data[i]) {
				break;
			}
		}
		insert(i, p_val);
	}

	_FORCE_INLINE_ StaticVector() {}
	_FORCE_INLINE_ StaticVector(const StaticVector &p_from) {
		for (U i = 0; i < p_from.count; i++) {
			data[i] = p_from.data[i];
		}
		count = p_from.count;
	}
	inline StaticVector &operator=(const StaticVector &p_from) {
		for (U i = 0; i < p_from.count; i++) {
			data[i] = p_from.data[i];
		}
		count = p_from.count;
		return *this;
	}
};

#endif // LOCAL_VECTOR_H
