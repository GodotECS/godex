#ifndef TEST_ECS_BASE_H
#define TEST_ECS_BASE_H

#include "tests/test_macros.h"

#include "../ecs.h"
#include "../godot/components/mesh_component.h"
#include "../godot/components/transform_component.h"

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
	}

	changed.clear();

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
}
} // namespace godex_tests

#endif // TEST_ECS_BASE_H
