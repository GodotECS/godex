#ifndef TEST_ECS_BASE_H
#define TEST_ECS_BASE_H

#include "tests/test_macros.h"

#include "modules/ecs/components/mesh_component.h"
#include "modules/ecs/components/transform_component.h"
#include "modules/ecs/ecs.h"

namespace godex_tests {

TEST_CASE("[Modules][ECS] Test ECS singleton validity.") {
	CHECK(ECS::get_singleton() != nullptr);
}

TEST_CASE("[Modules][ECS] Test ECS Component ID validity.") {
	// Make sure the component IDs are properly registerd.
	CHECK(String(ECS::get_component_name(TransformComponent::get_component_id())) == TransformComponent::get_class_static());
	CHECK(String(ECS::get_component_name(MeshComponent::get_component_id())) == MeshComponent::get_class_static());
}

TEST_CASE("[Modules][ECS] Test ECS dynamic component.") {
	LocalVector<ScriptProperty> props;
	props.push_back({ PropertyInfo(Variant::INT, "variable_1"), 2 });

	const uint32_t test_dyn_component_id = ECS::register_script_component(
			"TestDynamicBaseComponent1.gd",
			props,
			StorageType::DENSE_VECTOR);

	// Make sure this component is created.
	CHECK(test_dyn_component_id != UINT32_MAX);
	// with correct default values.
	CHECK(ECS::get_component_property_default(test_dyn_component_id, "variable_1") == Variant(2));
}

TEST_CASE("[Modules][ECS] Test ECS dynamic component double registration.") {
	LocalVector<ScriptProperty> props;
	props.push_back({ PropertyInfo(Variant::INT, "variable_1"), 2 });

	const uint32_t second_test_dyn_component_id = ECS::register_script_component(
			"TestDynamicBaseComponent1.gd",
			props,
			StorageType::DENSE_VECTOR);

	// Make sure this component was not created since it already exists.
	CHECK(second_test_dyn_component_id == UINT32_MAX);
}

TEST_CASE("[Modules][ECS] Test ECS dynamic component with wrong default type.") {
	LocalVector<ScriptProperty> props;
	props.push_back({ PropertyInfo(Variant::INT, "variable_1"), false });

	print_line("Test ECS dynamic component with wrong default type, the followint error is legit:");
	const uint32_t test_dyn_component_id = ECS::register_script_component(
			"TestDynamicBaseComponent2.gd",
			props,
			StorageType::DENSE_VECTOR);

	// Make sure this component was not created.
	CHECK(test_dyn_component_id == UINT32_MAX);
}

} // namespace godex_tests

#endif // TEST_ECS_BASE_H
