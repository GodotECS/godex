#ifndef TEST_STEADY_STORAGE_H
#define TEST_STEADY_STORAGE_H

#include "../components/component.h"
#include "../storage/steady_storage.h"
#include "core/math/math_funcs.h"

#include "tests/test_macros.h"

namespace godex_ecs_steady_storage_tests {

struct SteadyComponentTest {
	COMPONENT_CUSTOM_STORAGE(SteadyComponentTest)

	int number = 0;

	SteadyComponentTest(int i) :
			number(i) {}
};

TEST_CASE("[SteadyStorage] Insert and iteration check.") {
	// Create a storage with pages of 5 elements.
	SteadyStorage<SteadyComponentTest> storage;
	Dictionary config;
	config["page_size"] = 5;
	storage.configure(config);

	const uint32_t count = 100;
	bool entities[count];

	// Insert `count` elements and init the `entities` array with false.
	for (uint32_t i = 0; i < count; i += 1) {
		storage.insert(i, i);
		entities[i] = false;
	}

	// For each stored element in the array, set true on the `entities` array.
	const EntitiesBuffer buf = storage.get_stored_entities();
	for (uint32_t i = 0; i < buf.count; i += 1) {
		SteadyComponentTest *data = storage.get(buf.entities[i]);
		entities[data->number] = true;
	}

	// Make sure each entity is set to true, confirming all got correctly stored.
	for (uint32_t i = 0; i < count; i += 1) {
		CHECK(entities[i]);
	}
}

TEST_CASE("[SteadyStorage] Push and remove memory check.") {
	SteadyStorage<SteadyComponentTest> storage;
	Dictionary config;
	config["page_size"] = 5;
	storage.configure(config);

	const uint32_t count = 100;
	SteadyComponentTest *pointers[count];

	// Insert `count` elements and store the pointer.
	for (uint32_t i = 0; i < count; i += 1) {
		storage.insert(i, i);
		pointers[i] = storage.get(i);
	}

	// For each pointer, make sure the memory is still valid after all the
	// insert.
	for (uint32_t i = 0; i < count; i += 1) {
		CHECK(pointers[i] == storage.get(i));
	}

	// Remove some random pointers.
	const uint32_t remove_count = count / 4;
	for (uint32_t i = 0; i < remove_count; i += 1) {
		const uint32_t entity_to_remove = Math::random(0, count - 1);
		if (pointers[entity_to_remove]) {
			// This pointer is still valid.
			storage.remove(entity_to_remove);
			pointers[entity_to_remove] = nullptr;
		}
	}

	// Check the validity of remaining one.
	for (uint32_t i = 0; i < count; i += 1) {
		if (pointers[i]) {
			CHECK(pointers[i] == storage.get(i));
		}
	}

	// Validate the internal data.
	for (uint32_t i = 0; i < count; i += 1) {
		if (pointers[i]) {
			CHECK(pointers[i]->number == i);
		}
	}

	// Alter the internal data, using the pointers.
	for (uint32_t i = 0; i < count; i += 1) {
		if (pointers[i]) {
			pointers[i]->number = -1;
		}
	}

	// Make sure the storage data is also changed, and change the data taking
	// it from the storage.
	for (uint32_t i = 0; i < count; i += 1) {
		if (pointers[i]) {
			CHECK(storage.get(i)->number == -1);
			storage.get(i)->number = i;
		}
	}

	// Make sure pointers are still pointing to the same data.
	for (uint32_t i = 0; i < count; i += 1) {
		if (pointers[i]) {
			CHECK(pointers[i]->number == i);
		}
	}

	// At this point it's safe to assume that the data created is steady inplace
	// until explictly removed, even if other entities are added or removed.
	// All the pointers remain valid until explicitly removed.
}
} // namespace godex_ecs_steady_storage_tests

#endif // TEST_STEADY_STORAGE_H
