#ifndef TEST_STATIC_VECTOR_H
#define TEST_STATIC_VECTOR_H

#include "../storage/static_vector.h"

#include "tests/test_macros.h"

namespace godex_ecs_static_vector_tests {

TEST_CASE("[StaticVector] Push Back.") {
	StaticVector<int, 5> vector;

	CHECK(vector.capacity() == 5);
	CHECK(vector.size() == 0);

	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	CHECK(vector.capacity() == 5);
	CHECK(vector.size() == 5);

	CHECK(vector[0] == 0);
	CHECK(vector[1] == 1);
	CHECK(vector[2] == 2);
	CHECK(vector[3] == 3);
	CHECK(vector[4] == 4);
}

TEST_CASE("[StaticVector] Find.") {
	StaticVector<int, 5> vector;
	vector.push_back(3);
	vector.push_back(1);
	vector.push_back(4);
	vector.push_back(0);
	vector.push_back(2);

	CHECK(vector[0] == 3);
	CHECK(vector[1] == 1);
	CHECK(vector[2] == 4);
	CHECK(vector[3] == 0);
	CHECK(vector[4] == 2);

	CHECK(vector.find(0) == 3);
	CHECK(vector.find(1) == 1);
	CHECK(vector.find(2) == 4);
	CHECK(vector.find(3) == 0);
	CHECK(vector.find(4) == 2);

	CHECK(vector.find(-1) == -1);
	CHECK(vector.find(5) == -1);
}

TEST_CASE("[StaticVector] Remove.") {
	StaticVector<int, 5> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	vector.remove(0);

	CHECK(vector[0] == 1);
	CHECK(vector[1] == 2);
	CHECK(vector[2] == 3);
	CHECK(vector[3] == 4);

	vector.remove(2);

	CHECK(vector[0] == 1);
	CHECK(vector[1] == 2);
	CHECK(vector[2] == 4);

	vector.remove(1);

	CHECK(vector[0] == 1);
	CHECK(vector[1] == 4);

	vector.remove(0);

	CHECK(vector[0] == 4);
}

TEST_CASE("[StaticVector] Remove Unordered.") {
	StaticVector<int, 5> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	CHECK(vector.size() == 5);

	vector.remove_at_unordered(0);

	CHECK(vector.size() == 4);

	CHECK(vector.find(0) == -1);
	CHECK(vector.find(1) != -1);
	CHECK(vector.find(2) != -1);
	CHECK(vector.find(3) != -1);
	CHECK(vector.find(4) != -1);

	// Now the vector is no more ordered.
	vector.remove_at_unordered(vector.find(3));

	CHECK(vector.size() == 3);

	CHECK(vector.find(3) == -1);
	CHECK(vector.find(1) != -1);
	CHECK(vector.find(2) != -1);
	CHECK(vector.find(4) != -1);

	vector.remove_at_unordered(vector.find(2));

	CHECK(vector.size() == 2);

	CHECK(vector.find(2) == -1);
	CHECK(vector.find(1) != -1);
	CHECK(vector.find(4) != -1);

	vector.remove_at_unordered(vector.find(4));

	CHECK(vector.size() == 1);

	CHECK(vector.find(4) == -1);
	CHECK(vector.find(1) != -1);

	// Remove the last one.
	vector.remove_at_unordered(0);

	CHECK(vector.is_empty());
	CHECK(vector.size() == 0);
}

TEST_CASE("[StaticVector] Erase.") {
	StaticVector<int, 5> vector;
	vector.push_back(1);
	vector.push_back(3);
	vector.push_back(0);
	vector.push_back(2);
	vector.push_back(4);

	CHECK(vector.find(2) == 3);

	vector.erase(2);

	CHECK(vector.find(2) == -1);
	CHECK(vector.size() == 4);
}
} // namespace godex_ecs_static_vector_tests

#endif // TEST_STATIC_VECTOR_H
