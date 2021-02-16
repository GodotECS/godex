#ifndef TEST_ECS_ENTITY_LIST_H
#define TEST_ECS_ENTITY_LIST_H

#include "tests/test_macros.h"

#include "../storage/storage.h"
#include "core/os/os.h"

namespace godex_entity_list_tests {

TEST_CASE("[Modules][ECS] Test ECS EntityList.") {
	EntityList changed;

	{
		// Make sure only one time the entity is stored, even if notified multiple
		// times.
		changed.insert(1);
		changed.insert(1);
		changed.insert(1);
		changed.insert(1);

		uint32_t count = 0;
		changed.for_each([&](EntityID p_entity) {
			count += 1;
		});
		CHECK(count == 1);

		// Make sure on updated the entity is removed.
		changed.remove(1);
		count = 0;
		changed.for_each([&](EntityID p_entity) {
			count += 1;
		});
		CHECK(count == 0);
	}

	// Test clear.
	{
		changed.remove(1);
		changed.remove(2);
		changed.clear();
		uint32_t count = 0;
		changed.for_each([&](EntityID p_entity) {
			count += 1;
		});
		CHECK(count == 0);
		CHECK(changed.is_empty());
	}

	changed.clear();
	CHECK(changed.is_empty());

	// Remove the `EntityID` 1 as soon as possible.
	{
		changed.insert(0);
		changed.insert(1);
		changed.insert(2);
		changed.insert(3);

		// Using a vector because the order is not guaranteed by this container.
		LocalVector<bool> checked;
		checked.resize(4);
		checked[0] = false;
		checked[1] = true; // This get removed
		checked[2] = false;
		checked[3] = false;

		changed.for_each([&](EntityID p_entity) {
			checked[p_entity] = true;
			changed.remove(1);
		});

		CHECK(checked[0]);
		CHECK(checked[1]);
		CHECK(checked[2]);
		CHECK(checked[3]);
	}

	changed.clear();
	CHECK(changed.is_empty());

	// Remove the `EntityID` 1, while processing.
	{
		changed.insert(0);
		changed.insert(1);
		changed.insert(2);
		changed.insert(3);

		LocalVector<bool> checked;
		checked.resize(4);
		checked[0] = false;
		checked[1] = true; // This get removed
		checked[2] = false;
		checked[3] = false;

		changed.for_each([&](EntityID p_entity) {
			checked[p_entity] = true;
			if (p_entity == EntityID(1)) {
				changed.remove(1);
			}
		});

		CHECK(checked[0]);
		CHECK(checked[1]);
		CHECK(checked[2]);
		CHECK(checked[3]);
	}

	changed.clear();
	CHECK(changed.is_empty());

	// Remove the `EntityID` 1, when already processed.
	{
		changed.insert(0);
		changed.insert(1);
		changed.insert(2);
		changed.insert(3);
		changed.insert(4);

		LocalVector<bool> checked;
		checked.resize(5);
		checked[0] = false;
		checked[1] = true; // This get removed
		checked[2] = false;
		checked[3] = false;
		checked[4] = false;

		changed.for_each([&](EntityID p_entity) {
			checked[p_entity] = true;
			if (p_entity == EntityID(2)) {
				changed.remove(1);
			}
		});

		CHECK(checked[0]);
		CHECK(checked[1]);
		CHECK(checked[2]);
		CHECK(checked[3]);
		CHECK(checked[4]);
	}

	changed.clear();
	CHECK(changed.is_empty());

	// Test self consuming for_each.
	{
		changed.insert(4);
		changed.insert(2);
		changed.insert(3);
		changed.insert(1);
		changed.insert(0);

		changed.for_each([&](EntityID p_entity) {
			changed.remove(p_entity);
		});

		CHECK(changed.is_empty());
	}
}
} // namespace godex_entity_list_tests

#endif
