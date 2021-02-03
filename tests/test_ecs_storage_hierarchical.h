#ifndef TEST_ECS_STORAGE_HIERARCHICAL_H
#define TEST_ECS_STORAGE_HIERARCHICAL_H

#include "tests/test_macros.h"

#include "../components/child.h"
#include "../godot/components/transform_component.h"
#include "../storage/hierarchical_storage.h"

namespace godex_storage_hierarchical_tests {

TEST_CASE("[Modules][ECS] Test Hierarchy.") {
	Hierarchy hierarchy;
	const Hierarchy &hierarchy_const = hierarchy;

	// This is used to make sure the hierarchy is properly created even when
	// bad data is used.
	Child malformed_child(3);
	malformed_child.first_child = 1;
	malformed_child.next = 1;

	// The hierarchy is as follows:
	// Entity 0
	//  |- Entity 1
	//  |   |- Entity 2
	//  |   |   |- Entity 3
	//  |   |   |   |- Entity 4
	//  |   |- Entity 5
	//
	hierarchy.insert(4, malformed_child);
	hierarchy.insert(5, Child(1));
	hierarchy.insert(1, Child(0));
	hierarchy.insert(3, Child(2));
	hierarchy.insert(2, Child(1));

	// Iterate over the root and validate the hierarchy.
	hierarchy.for_each_child(0, [](EntityID p_entity, const Child &p_child) -> bool {
		CHECK(p_entity == EntityID(1));
		return true;
	});
	hierarchy.for_each_child(1, [](EntityID p_entity, const Child &p_child) -> bool {
		bool valid = p_entity == EntityID(2) || p_entity == EntityID(5);
		CHECK(valid);
		return true;
	});
	hierarchy.for_each_child(2, [](EntityID p_entity, const Child &p_child) -> bool {
		CHECK(p_entity == EntityID(3));
		return true;
	});
	hierarchy.for_each_child(3, [](EntityID p_entity, const Child &p_child) -> bool {
		CHECK(p_entity == EntityID(4));
		return true;
	});

	// Check inverse navigation
	{
		bool visited[] = { false, false, false, false };
		hierarchy.for_each_parent(4, [&](EntityID p_entity, const Child &p_child) -> bool {
			visited[p_entity.operator uint32_t()] = true;
			return true;
		});
		CHECK(visited[0]);
		CHECK(visited[1]);
		CHECK(visited[2]);
		CHECK(visited[3]);
	}

	// Make sure the hierarchy is correctly constructed even if inserted with
	// random order.
	CHECK(hierarchy_const.get(4)->parent == EntityID(3));
	CHECK(hierarchy_const.get(3)->parent == EntityID(2));
	CHECK(hierarchy_const.get(2)->parent == EntityID(1));
	CHECK(hierarchy_const.get(1)->parent == EntityID(0));
	CHECK(hierarchy_const.get(5)->parent == EntityID(1));
	CHECK(hierarchy_const.get(0)->parent == EntityID()); // It's Root.

	// Alter the hierarchy as follows:
	// Entity 0
	//  |- Entity 1
	//  |   |- Entity 5
	// Entity 2
	//  |- Entity 3
	//  |   |- Entity 4
	//
	// Now `Entity 2` is root.
	hierarchy.insert(2, Child());

	CHECK(hierarchy_const.get(4)->parent == EntityID(3));
	CHECK(hierarchy_const.get(3)->parent == EntityID(2));
	CHECK(hierarchy_const.get(2)->parent == EntityID()); // It's Root.
	CHECK(hierarchy_const.get(5)->parent == EntityID(1));
	CHECK(hierarchy_const.get(1)->parent == EntityID(0));
	CHECK(hierarchy_const.get(0)->parent == EntityID()); // It's Root.

	// Add a new entity as child of 4.
	// Remove from the hierarchy 3, so to obtain:
	// Entity 0
	//  |- Entity 1
	//  |   |- Entity 5
	// Entity 2
	// Entity 4
	//  |- Entity 6
	//
	// Now `Entity 4` is also root and has the `Entity 6` as child.
	hierarchy.insert(6, Child(4));
	hierarchy.remove(3);

	CHECK(hierarchy_const.get(6)->parent == EntityID(4));
	CHECK(hierarchy_const.get(4)->parent == EntityID()); // It's Root.
	CHECK(hierarchy_const.get(2)->parent == EntityID()); // It's Root.
	CHECK(hierarchy_const.get(5)->parent == EntityID(1));
	CHECK(hierarchy_const.get(1)->parent == EntityID(0));
	CHECK(hierarchy_const.get(0)->parent == EntityID()); // It's Root.

	// Make `Entity 2` child of `Entity 6`
	// Entity 0
	//  |- Entity 1
	//  |   |- Entity 5
	// Entity 4
	//  |- Entity 6
	//  |   |- Entity 2
	hierarchy.insert(2, Child(6));

	CHECK(hierarchy_const.get(2)->parent == EntityID(6));
	CHECK(hierarchy_const.get(6)->parent == EntityID(4));
	CHECK(hierarchy_const.get(4)->parent == EntityID()); // It's Root.
	CHECK(hierarchy_const.get(5)->parent == EntityID(1));
	CHECK(hierarchy_const.get(1)->parent == EntityID(0));
	CHECK(hierarchy_const.get(0)->parent == EntityID()); // It's Root.

	// Iterate over the root and validate the hierarchy.
	hierarchy.for_each_child(0, [](EntityID p_entity, const Child &p_child) -> bool {
		CHECK(p_entity == EntityID(1));
		return true;
	});
	hierarchy.for_each_child(1, [](EntityID p_entity, const Child &p_child) -> bool {
		CHECK(p_entity == EntityID(5));
		return true;
	});
	hierarchy.for_each_child(4, [](EntityID p_entity, const Child &p_child) -> bool {
		CHECK(p_entity == EntityID(6));
		return true;
	});
	hierarchy.for_each_child(6, [](EntityID p_entity, const Child &p_child) -> bool {
		CHECK(p_entity == EntityID(2));
		return true;
	});

	// Check inverse for_each.
	{
		bool visited[] = { false, false, false, false, false, false, false };
		hierarchy.for_each_parent(5, [&](EntityID p_entity, const Child &p_child) -> bool {
			visited[p_entity.operator uint32_t()] = true;
			return true;
		});
		CHECK(visited[0]);
		CHECK(visited[1]);
		CHECK(visited[2] == false);
		CHECK(visited[3] == false);
		CHECK(visited[4] == false);
		CHECK(visited[5] == false);
		CHECK(visited[6] == false);
	}
	{
		bool visited[] = { false, false, false, false, false, false, false };
		hierarchy.for_each_parent(2, [&](EntityID p_entity, const Child &p_child) -> bool {
			visited[p_entity.operator uint32_t()] = true;
			return true;
		});
		CHECK(visited[0] == false);
		CHECK(visited[1] == false);
		CHECK(visited[2] == false);
		CHECK(visited[3] == false);
		CHECK(visited[4]);
		CHECK(visited[5] == false);
		CHECK(visited[6]);
	}
}

TEST_CASE("[Modules][ECS] Test HierarchicalStorage.") {
	Hierarchy hierarchy;
	return; // TODO

	HierarchicalStorage<TransformComponent> transform_storage;
	transform_storage.hierarchy = &hierarchy;

	// The hierarchy is as follows:
	// Entity 0
	// |- Entity 1
	// |   |- Entity 2
	//
	hierarchy.insert(1, Child(0));
	hierarchy.insert(2, Child(1));

	transform_storage.insert(0, TransformComponent(Transform(Basis(), Vector3(1, 0, 0))));
	transform_storage.insert(1, TransformComponent(Transform(Basis(), Vector3(1, 0, 0))));
	transform_storage.insert(2, TransformComponent(Transform(Basis(), Vector3(1, 0, 0))));

	{
		const TransformComponent *tc_entity_0 = transform_storage.get_local(0);
		const TransformComponent *tc_entity_1 = transform_storage.get_local(1);
		const TransformComponent *tc_entity_2 = transform_storage.get_local(2);

		// Test local world space.
		CHECK(ABS(tc_entity_0->get_transform().origin[0] - 1.) <= CMP_EPSILON);
		CHECK(ABS(tc_entity_1->get_transform().origin[0] - 1.) <= CMP_EPSILON);
		CHECK(ABS(tc_entity_2->get_transform().origin[0] - 1.) <= CMP_EPSILON);
	}

	{
		const TransformComponent *tc_entity_2 = transform_storage.get_global(2);
		const TransformComponent *tc_entity_1 = transform_storage.get_global(1);
		const TransformComponent *tc_entity_0 = transform_storage.get_global(0);

		// Test global world space.
		CHECK(ABS(tc_entity_0->get_transform().origin[0] - 1.) <= CMP_EPSILON);
		CHECK(ABS(tc_entity_1->get_transform().origin[0] - 2.) <= CMP_EPSILON);
		CHECK(ABS(tc_entity_2->get_transform().origin[0] - 3.) <= CMP_EPSILON);
	}

	// Test update local transform
	{
		{
			TransformComponent *tc_entity_0 = transform_storage.get(0);
			tc_entity_0->set_transform(Transform(Basis(), Vector3(3.0, 0., 0.)));
		}

		const TransformComponent *tc_entity_2 = transform_storage.get_global(2);
		const TransformComponent *tc_entity_1 = transform_storage.get_global(1);
		const TransformComponent *tc_entity_0 = transform_storage.get_global(0);

		// Test global world space.
		CHECK(ABS(tc_entity_0->get_transform().origin[0] - 3.) <= CMP_EPSILON);
		CHECK(ABS(tc_entity_1->get_transform().origin[0] - 4.) <= CMP_EPSILON);
		CHECK(ABS(tc_entity_2->get_transform().origin[0] - 5.) <= CMP_EPSILON);
	}

	// Test change hierarchy
	{
		{
			hierarchy.remove(1); // Make the `Entity1` root.
			hierarchy.insert(2, Child(0)); // Make the `Entity2` parent of `Entity0`.
		}

		const TransformComponent *tc_entity_2 = transform_storage.get_global(2);
		const TransformComponent *tc_entity_1 = transform_storage.get_global(1);
		const TransformComponent *tc_entity_0 = transform_storage.get_global(0);

		// Test global world space.
		CHECK(ABS(tc_entity_0->get_transform().origin[0] - 3.) <= CMP_EPSILON); // Root
		CHECK(ABS(tc_entity_1->get_transform().origin[0] - 1.) <= CMP_EPSILON); // Root
		CHECK(ABS(tc_entity_2->get_transform().origin[0] - 4.) <= CMP_EPSILON); // Child of `Entity0`.
	}
}

// TODO test hierarchy sorting?
} // namespace godex_storage_hierarchical_tests

#endif
