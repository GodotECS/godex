#ifndef TEST_ECS_WORLD_H
#define TEST_ECS_WORLD_H

#include "tests/test_macros.h"

#include "../components/transform_component.h"
#include "../ecs.h"
#include "../nodes/ecs_world.h"
#include "../nodes/entity.h"
#include "../world/world.h"

struct Test1Resource : public godex::Resource {
	RESOURCE(Test1Resource)

public:
	int a = 10;
};

namespace godex_tests_world {

TEST_CASE("[Modules][ECS] Test world has self resource.") {
	World world;
	godex::Resource *world_ptr = world.get_resource(World::get_resource_id());

	// Make sure `World` contains a pointer of itself as resource.
	CHECK(&world == world_ptr);
}

TEST_CASE("[Modules][ECS] Test world") {
	World world;

	TransformComponent entity_1_transform_component;
	entity_1_transform_component.set_transform(Transform(Basis(), Vector3(10.0, 10.0, 10.0)));

	EntityID entity_1 = world.create_entity();
	world.add_component(
			entity_1,
			entity_1_transform_component);

	const TypedStorage<const TransformComponent> *storage = world.get_storage<const TransformComponent>();
	const TransformComponent &transform_from_storage = storage->get(entity_1);

	// Check the add component has the exact same data as the stored one.
	CHECK((entity_1_transform_component.get_transform().origin - transform_from_storage.get_transform().origin).length() < CMP_EPSILON);
}

TEST_CASE("[Modules][ECS] Test storage script component") {
	LocalVector<ScriptProperty> props;
	props.push_back({ PropertyInfo(Variant::INT, "variable_1"), 1 });
	props.push_back({ PropertyInfo(Variant::BOOL, "variable_2"), false });
	props.push_back({ PropertyInfo(Variant::TRANSFORM, "variable_3"), Transform() });

	const uint32_t test_world_component_id = ECS::register_script_component(
			"TestWorldComponent1.gd",
			props,
			StorageType::DENSE_VECTOR);

	World world;

	EntityID entity_1 = world.create_entity();
	world.add_component(
			entity_1,
			test_world_component_id,
			Dictionary());

	// ~~ Test the component is initialized with defaults. ~~
	{
		const Storage *storage = world.get_storage(test_world_component_id);
		const godex::Component *test_component = storage->get_ptr(entity_1);

		CHECK(test_component->get("variable_1") == Variant(1));
		CHECK(test_component->get("variable_2") == Variant(false));
		CHECK(test_component->get("variable_3") == Variant(Transform()));
	}

	// ~~ Test change component values ~~
	{
		Storage *storage = world.get_storage(test_world_component_id);

		godex::Component *test_component = storage->get_ptr(entity_1);
		test_component->set("variable_1", 2);
		test_component->set("variable_2", true);
		test_component->set("variable_3", Transform(Basis(), Vector3(10., 10., 10.)));

		// Take it again, to confirm we are operating on the stored object.
		test_component = storage->get_ptr(entity_1);
		CHECK(test_component->get("variable_1") == Variant(2));
		CHECK(test_component->get("variable_2") == Variant(true));
		CHECK(ABS(test_component->get("variable_3").operator Transform().origin.x - 10.) <= CMP_EPSILON);
	}

	// ~~ Test change value with a wrong type ~~
	{
		Storage *storage = world.get_storage(test_world_component_id);

		godex::Component *test_component = storage->get_ptr(entity_1);
		// Set the `variable_1` with a floating point variable.
		test_component->set("variable_1", 0.0);

		// Make sure the value is not changed, since it's integer and not a float.
		CHECK(test_component->get("variable_1") != Variant(0.0));
		CHECK(test_component->get("variable_1") == Variant(2));
	}

	// ~~ Test custom initialization ~~
	{
		EntityID entity_2 = world.create_entity();
		Dictionary entity_2_data;
		entity_2_data["variable_1"] = 100;
		entity_2_data["variable_2"] = true;
		entity_2_data["variable_3"] = Transform(Basis(), Vector3(-10., 0., 0.));

		world.add_component(
				entity_2,
				test_world_component_id,
				entity_2_data);

		const Storage *storage = world.get_storage(test_world_component_id);
		const godex::Component *test_component = storage->get_ptr(entity_2);

		CHECK(test_component->get("variable_1") == Variant(100));
		CHECK(test_component->get("variable_2") == Variant(true));
		CHECK(ABS(test_component->get("variable_3").operator Transform().origin.x - (-10.)) <= CMP_EPSILON);
	}

	// ~~ Test partial custom initialization ~~
	{
		EntityID entity_3 = world.create_entity();
		Dictionary entity_3_data;
		entity_3_data["variable_1"] = 100;
		entity_3_data["variable_3"] = Transform(Basis(), Vector3(-10., 0., 0.));

		world.add_component(
				entity_3,
				test_world_component_id,
				entity_3_data);

		const Storage *storage = world.get_storage(test_world_component_id);
		const godex::Component *test_component = storage->get_ptr(entity_3);

		CHECK(test_component->get("variable_1") == Variant(100));
		// Check default.
		CHECK(test_component->get("variable_2") == Variant(false));
		CHECK(ABS(test_component->get("variable_3").operator Transform().origin.x - (-10.)) <= CMP_EPSILON);
	}

	// ~~ Test custom initialization with wrong value type ~~
	{
		EntityID entity_4 = world.create_entity();
		Dictionary entity_4_data;
		entity_4_data["variable_1"] = 100;
		entity_4_data["variable_2"] = Vector3(); // Wrong value type `variable_2` is a bool.
		entity_4_data["variable_3"] = Transform(Basis(), Vector3(-10., 0., 0.));

		world.add_component(
				entity_4,
				test_world_component_id,
				entity_4_data);

		const Storage *storage = world.get_storage(test_world_component_id);
		const godex::Component *test_component = storage->get_ptr(entity_4);

		CHECK(test_component->get("variable_1") == Variant(100));
		// Make sure the default is set, and not the `Vector3()`.
		CHECK(test_component->get("variable_2") == Variant(false));
		CHECK(ABS(test_component->get("variable_3").operator Transform().origin.x - (-10.)) <= CMP_EPSILON);
	}

	// ~~ Test resource initialization ~~
	{
		ECS::register_resource<Test1Resource>();

		// Make sure the resource is null.
		CHECK(world.get_resource<Test1Resource>() == nullptr);
		CHECK(world.get_resource(Test1Resource::get_resource_id()) == nullptr);

		// Add the resource
		world.add_resource<Test1Resource>();

		// Make sure we can retrieve the resource.
		CHECK(world.get_resource<Test1Resource>() != nullptr);
		CHECK(world.get_resource(Test1Resource::get_resource_id()) != nullptr);
		CHECK(world.get_resource<Test1Resource>() == world.get_resource(Test1Resource::get_resource_id()));

		// Now remove it.
		world.remove_resource<Test1Resource>();

		// Make sure the resource is now nullptr.
		CHECK(world.get_resource<Test1Resource>() == nullptr);
		CHECK(world.get_resource(Test1Resource::get_resource_id()) == nullptr);
	}
}

TEST_CASE("[Modules][ECS] Test WorldECSCommands create entity from prefab.") {
	World world;

	AccessResource world_res_access;
	world_res_access.__resource = &world;
	world_res_access.__mut = true;

	Entity entity_prefab;

	Dictionary defaults;
	defaults["transform"] = Transform(Basis(), Vector3(10.0, 0.0, 0.0));
	entity_prefab.add_component("TransformComponent", defaults);

	const uint32_t entity_id = WorldECSCommands::get_singleton()->create_entity_from_prefab(
			&world_res_access,
			&entity_prefab);

	// Make sure something is created.
	CHECK(entity_id != UINT32_MAX);

	// Make sure the component is created
	CHECK(world.get_storage<TransformComponent>()->has(0));

	// Make sure the default is also set.
	CHECK(ABS(world.get_storage<TransformComponent>()->get(0).transform.origin.x - 10) <= CMP_EPSILON);
}

} // namespace godex_tests_world

#endif // TEST_ECS_WORLD_H
