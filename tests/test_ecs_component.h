#ifndef TEST_ECS_COMPONENT_H
#define TEST_ECS_COMPONENT_H

#include "tests/test_macros.h"

#include "../ecs.h"
#include "../modules/godot/components/transform_component.h"

namespace godex_component_test {

TEST_CASE("[Modules][ECS] Test component create pointer.") {
	void *transform = ECS::new_component(TransformComponent::get_component_id());
	CHECK(transform != nullptr);

	ECS::unsafe_component_set_by_name(TransformComponent::get_component_id(), transform, "origin", Vector3(1, 2, 3));
	const Vector3 vec = ECS::unsafe_component_get_by_name(TransformComponent::get_component_id(), transform, "origin");

	CHECK(vec.distance_to(Vector3(1, 2, 3)) <= CMP_EPSILON);

	ECS::free_component(TransformComponent::get_component_id(), transform);
}
} // namespace godex_component_test

#endif // TEST_ECS_COMOPNENT_H
