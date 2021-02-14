#ifndef TEST_ECS_CHANGE_LIST_H
#define TEST_ECS_CHANGE_LIST_H

#include "tests/test_macros.h"

#include "../storage/storage.h"
#include "core/os/os.h"

namespace godex_change_list_tests {

TEST_CASE("[Modules][ECS] Test ECS ChangeList.") {
	ChangeList changed;

	{
		// Make sure only one time the entity is stored, even if notified multiple
		// times.
		changed.notify_changed(1);
		changed.notify_changed(1);
		changed.notify_changed(1);
		changed.notify_changed(1);

		uint32_t count = 0;
		changed.for_each([&](EntityID p_entity) {
			count += 1;
		});
		CHECK(count == 1);

		// Make sure on updated the entity is removed.
		changed.notify_updated(1);
		count = 0;
		changed.for_each([&](EntityID p_entity) {
			count += 1;
		});
		CHECK(count == 0);
	}

	// Test clear.
	{
		changed.notify_updated(1);
		changed.notify_updated(2);
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
		changed.notify_changed(0);
		changed.notify_changed(1);
		changed.notify_changed(2);
		changed.notify_changed(3);

		// Using a vector because the order is not guaranteed by this container.
		LocalVector<bool> checked;
		checked.resize(4);
		checked[0] = false;
		checked[1] = true; // This get removed
		checked[2] = false;
		checked[3] = false;

		changed.for_each([&](EntityID p_entity) {
			checked[p_entity] = true;
			changed.notify_updated(1);
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
		changed.notify_changed(0);
		changed.notify_changed(1);
		changed.notify_changed(2);
		changed.notify_changed(3);

		LocalVector<bool> checked;
		checked.resize(4);
		checked[0] = false;
		checked[1] = true; // This get removed
		checked[2] = false;
		checked[3] = false;

		changed.for_each([&](EntityID p_entity) {
			checked[p_entity] = true;
			if (p_entity == EntityID(1)) {
				changed.notify_updated(1);
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
		changed.notify_changed(0);
		changed.notify_changed(1);
		changed.notify_changed(2);
		changed.notify_changed(3);
		changed.notify_changed(4);

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
				changed.notify_updated(1);
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
		changed.notify_changed(4);
		changed.notify_changed(2);
		changed.notify_changed(3);
		changed.notify_changed(1);
		changed.notify_changed(0);

		changed.for_each([&](EntityID p_entity) {
			changed.notify_updated(p_entity);
		});

		CHECK(changed.is_empty());
	}
}
} // namespace godex_change_list_tests

#endif
