#ifndef TEST_ECS_STORAGE_HIERARCHICAL_H
#define TEST_ECS_STORAGE_HIERARCHICAL_H

#include "tests/test_macros.h"

#include "../components/child.h"
#include "../godot/components/transform_component.h"
#include "../storage/hierarchical_storage.h"

namespace godex_storage_hierarchical_tests {

TEST_CASE("[Modules][ECS] Test HierarchicalStorage.") {
	Hierarchy hierarchy;

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

	// Test update transform
	//TODO
}

// TODO test hierarchy sorting?
} // namespace godex_storage_hierarchical_tests

#endif
