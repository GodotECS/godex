#ifndef TEST_ECS_STORAGE_DENSE_VECTOR_H
#define TEST_ECS_STORAGE_DENSE_VECTOR_H

#include "tests/test_macros.h"

#include "../components/component.h"
#include "../storages/dense_vector.h"

namespace godex_storage_dense_vector_tests {

class TestInt : public godex::Component {
	COMPONENT(TestInt, DenseVectorStorage)

public:
	int number;

	TestInt() {}
	TestInt(int i) :
			number(i) {}
};

TEST_CASE("[Modules][ECS] Test dense storage insert and remove.") {
	DenseVectorStorage<TestInt> storage;

	// Stress test the storage. Each time the element are removed the
	// internal indices changes. Doing it 500 times should test all possible
	// cases.
	for (uint32_t i = 0; i < 500; i += 1) {
		if (i % 2 == 0) {
			storage.insert(0, 0);
			storage.insert(4, 4);
			storage.insert(1, 1);
			storage.insert(3, 3);
			storage.insert(2, 2);
		} else {
			storage.insert(3, 3);
			storage.insert(1, 1);
			storage.insert(0, 0);
			storage.insert(4, 4);
			storage.insert(2, 2);
		}

		CHECK(storage.get(0)->number == 0);
		CHECK(storage.get(1)->number == 1);
		CHECK(storage.get(2)->number == 2);
		CHECK(storage.get(3)->number == 3);
		CHECK(storage.get(4)->number == 4);

		storage.remove(4);

		CHECK(storage.get(0)->number == 0);
		CHECK(storage.get(1)->number == 1);
		CHECK(storage.get(2)->number == 2);
		CHECK(storage.get(3)->number == 3);
		CHECK(storage.has(4) == false);

		storage.remove(0);

		CHECK(storage.has(0) == false);
		CHECK(storage.get(1)->number == 1);
		CHECK(storage.get(2)->number == 2);
		CHECK(storage.get(3)->number == 3);
		CHECK(storage.has(4) == false);

		storage.remove(1);

		CHECK(storage.has(0) == false);
		CHECK(storage.has(1) == false);
		CHECK(storage.get(2)->number == 2);
		CHECK(storage.get(3)->number == 3);
		CHECK(storage.has(4) == false);

		storage.remove(3);

		CHECK(storage.has(0) == false);
		CHECK(storage.has(1) == false);
		CHECK(storage.get(2)->number == 2);
		CHECK(storage.has(3) == false);
		CHECK(storage.has(4) == false);

		storage.remove(2);

		CHECK(storage.has(0) == false);
		CHECK(storage.has(1) == false);
		CHECK(storage.has(2) == false);
		CHECK(storage.has(3) == false);
		CHECK(storage.has(4) == false);

		storage.insert(4, 10);

		CHECK(storage.has(4));
		CHECK(storage.get(4)->number == 10);
	}
}

} // namespace godex_storage_dense_vector_tests

#endif
