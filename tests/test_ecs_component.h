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

struct TestDynamicSetGetComponent {
	COMPONENT(TestDynamicSetGetComponent, DenseVectorStorage);

	static void _bind_methods() {
		ECS_BIND_PROPERTY(TestDynamicSetGetComponent, PropertyInfo(Variant::BOOL, "enable_color"), enable_color);
	}

	void _get_property_list(List<PropertyInfo> *r_list) {
		if (enable_color) {
			r_list->push_back(PropertyInfo(Variant::COLOR, "color"));
		}
	}

	bool _set(const StringName &p_name, const Variant &p_value) {
		if (enable_color) {
			if (p_name == SNAME("color")) {
				meta[SNAME("color")] = p_value;
				return true;
			}
		}
		return false;
	}

	bool _get(const StringName &p_name, Variant &r_value) const {
		if (enable_color) {
			if (p_name == SNAME("color")) {
				if (meta.has(SNAME("color"))) {
					r_value = meta[SNAME("color")];
				} else {
					// Default color.
					r_value = Color(1.0, 0.0, 0.0);
				}
				return true;
			}
		}
		return false;
	}

	bool enable_color = false;
	Dictionary meta;
};

namespace godex_component_test {
TEST_CASE("[Modules][ECS] Test component dynamic set and get.") {
	ECS::register_component<TestDynamicSetGetComponent>();

	// Create the component using the `unsafe_` functions, so we can also test those.
	void *component = ECS::new_component(TestDynamicSetGetComponent::get_component_id());

	{
		List<PropertyInfo> properties;
		ECS::unsafe_component_get_property_list(
				TestDynamicSetGetComponent::get_component_id(),
				component,
				&properties);

		// By default `enable_color` is set to `false` so we have only one property.
		CHECK(properties.size() == 1);
		CHECK(properties[0].name == "enable_color");
	}

	// Now set `enable_color` to `true`.
	ECS::unsafe_component_set_by_name(TestDynamicSetGetComponent::get_component_id(), component, SNAME("enable_color"), true);

	{
		List<PropertyInfo> properties;
		ECS::unsafe_component_get_property_list(
				TestDynamicSetGetComponent::get_component_id(),
				component,
				&properties);

		// Now `enable_color` is set to `true` so we have two property.
		CHECK(properties.size() == 2);
		// And one of those is `color` which is dynamic.
		CHECK((properties[0].name == "color" || properties[1].name == "color"));
	}

	{
		// Get the default color:
		const Color color = ECS::unsafe_component_get_by_name(
				TestDynamicSetGetComponent::get_component_id(),
				component,
				SNAME("color"));

		CHECK(Math::is_equal_approx(color.r, real_t(1.0)));
		CHECK(Math::is_equal_approx(color.g, real_t(0.0)));
		CHECK(Math::is_equal_approx(color.b, real_t(0.0)));
	}

	{
		// Try to set `color`
		ECS::unsafe_component_set_by_name(TestDynamicSetGetComponent::get_component_id(), component, SNAME("color"), Color(0.0, 1.0, 0.0));

		const Color color = ECS::unsafe_component_get_by_name(
				TestDynamicSetGetComponent::get_component_id(),
				component,
				SNAME("color"));

		CHECK(Math::is_equal_approx(color.r, real_t(0.0)));
		CHECK(Math::is_equal_approx(color.g, real_t(1.0)));
		CHECK(Math::is_equal_approx(color.b, real_t(0.0)));
	}

	// Set color enabled to false
	ECS::unsafe_component_set_by_name(TestDynamicSetGetComponent::get_component_id(), component, SNAME("enable_color"), false);

	// Make sure we can't fetch color:
	Variant color;
	CHECK(!ECS::unsafe_component_get_by_name(
			TestDynamicSetGetComponent::get_component_id(),
			component,
			SNAME("color"),
			color));

	// Make sure we can't set
	CHECK(!ECS::unsafe_component_set_by_name(
			TestDynamicSetGetComponent::get_component_id(),
			component,
			SNAME("color"),
			Color(0.0, 1.0, 0.0)));
}
} // namespace godex_component_test
#endif // TEST_ECS_COMOPNENT_H
