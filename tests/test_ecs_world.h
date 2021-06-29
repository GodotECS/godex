#ifndef TEST_ECS_WORLD_H
#define TEST_ECS_WORLD_H

#include "tests/test_macros.h"

#include "../ecs.h"
#include "../modules/godot/components/transform_component.h"
#include "../modules/godot/databags/scene_tree_databag.h"
#include "../modules/godot/nodes/ecs_world.h"
#include "../modules/godot/nodes/entity.h"
#include "../world/world.h"

struct Test1Databag : public godex::Databag {
	DATABAG(Test1Databag)

public:
	int a = 10;
};

namespace godex_tests_world {

TEST_CASE("[Modules][ECS] Test world has self databag.") {
	World world;
	godex::Databag *commands_ptr = world.get_databag(WorldCommands::get_databag_id());
	godex::Databag *world_ptr = world.get_databag(World::get_databag_id());

	// Make sure `World` contains a pointer of its commands as `Databag`.
	CHECK(&world.get_commands() == commands_ptr);
	CHECK(&world == world_ptr);
}

TEST_CASE("[Modules][ECS] Test world") {
	World world;

	TransformComponent entity_1_transform_component(Transform3D(Basis(), Vector3(10.0, 10.0, 10.0)));

	EntityID entity_1 = world.create_entity();
	world.add_component(
			entity_1,
			entity_1_transform_component);

	const Storage<const TransformComponent> *storage = world.get_storage<const TransformComponent>();
	const TransformComponent *transform_from_storage = storage->get(entity_1);

	// Check the add component has the exact same data as the stored one.
	CHECK((entity_1_transform_component.origin - transform_from_storage->origin).length() < CMP_EPSILON);
}

TEST_CASE("[Modules][ECS] Test storage script component") {
	LocalVector<ScriptProperty> props;
	props.push_back({ PropertyInfo(Variant::INT, "variable_1"), 1 });
	props.push_back({ PropertyInfo(Variant::BOOL, "variable_2"), false });
	props.push_back({ PropertyInfo(Variant::TRANSFORM3D, "variable_3"), Transform3D() });

	const uint32_t test_world_component_id = ECS::register_or_update_script_component(
			"TestWorldComponent1.gd",
			props,
			StorageType::DENSE_VECTOR,
			Vector<StringName>());

	World world;

	EntityID entity_1 = world.create_entity();
	world.add_component(
			entity_1,
			test_world_component_id,
			Dictionary());

	// ~~ Test the component is initialized with defaults. ~~
	{
		const StorageBase *storage = world.get_storage(test_world_component_id);
		const void *test_component = storage->get_ptr(entity_1);

		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_1") == Variant(1));
		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_2") == Variant(false));
		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_3") == Variant(Transform3D()));
	}

	// ~~ Test change component values ~~
	{
		StorageBase *storage = world.get_storage(test_world_component_id);

		void *test_component = storage->get_ptr(entity_1);
		ECS::unsafe_component_set_by_name(test_world_component_id, test_component, "variable_1", 2);
		ECS::unsafe_component_set_by_name(test_world_component_id, test_component, "variable_2", true);
		ECS::unsafe_component_set_by_name(test_world_component_id, test_component, "variable_3", Transform3D(Basis(), Vector3(10., 10., 10.)));

		// Take it again, to confirm we are operating on the stored object.
		test_component = storage->get_ptr(entity_1);
		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_1") == Variant(2));
		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_2") == Variant(true));
		CHECK(ABS(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_3").operator Transform3D().origin.x - 10.) <= CMP_EPSILON);
	}

	// ~~ Test change value with a wrong type ~~
	{
		StorageBase *storage = world.get_storage(test_world_component_id);

		void *test_component = storage->get_ptr(entity_1);
		// Set the `variable_1` with a floating point variable.
		ECS::unsafe_component_set_by_name(test_world_component_id, test_component, "variable_1", 0.0);

		// Make sure the value is not changed, since it's integer and not a float.
		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_1") != Variant(0.0));
		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_1") == Variant(2));
	}

	// ~~ Test custom initialization ~~
	{
		EntityID entity_2 = world.create_entity();
		Dictionary entity_2_data;
		entity_2_data["variable_1"] = 100;
		entity_2_data["variable_2"] = true;
		entity_2_data["variable_3"] = Transform3D(Basis(), Vector3(-10., 0., 0.));

		world.add_component(
				entity_2,
				test_world_component_id,
				entity_2_data);

		const StorageBase *storage = world.get_storage(test_world_component_id);
		const void *test_component = storage->get_ptr(entity_2);

		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_1") == Variant(100));
		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_2") == Variant(true));
		CHECK(ABS(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_3").operator Transform3D().origin.x - (-10.)) <= CMP_EPSILON);
	}

	// ~~ Test partial custom initialization ~~
	{
		EntityID entity_3 = world.create_entity();
		Dictionary entity_3_data;
		entity_3_data["variable_1"] = 100;
		entity_3_data["variable_3"] = Transform3D(Basis(), Vector3(-10., 0., 0.));

		world.add_component(
				entity_3,
				test_world_component_id,
				entity_3_data);

		const StorageBase *storage = world.get_storage(test_world_component_id);
		const void *test_component = storage->get_ptr(entity_3);

		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_1") == Variant(100));
		// Check default.
		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_2") == Variant(false));
		CHECK(ABS(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_3").operator Transform3D().origin.x - (-10.)) <= CMP_EPSILON);
	}

	// ~~ Test custom initialization with wrong value type ~~
	{
		EntityID entity_4 = world.create_entity();
		Dictionary entity_4_data;
		entity_4_data["variable_1"] = 100;
		entity_4_data["variable_2"] = Vector3(); // Wrong value type `variable_2` is a bool.
		entity_4_data["variable_3"] = Transform3D(Basis(), Vector3(-10., 0., 0.));

		world.add_component(
				entity_4,
				test_world_component_id,
				entity_4_data);

		const StorageBase *storage = world.get_storage(test_world_component_id);
		const void *test_component = storage->get_ptr(entity_4);

		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_1") == Variant(100));
		// Make sure the default is set, and not the `Vector3()`.
		CHECK(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_2") == Variant(false));
		CHECK(ABS(ECS::unsafe_component_get_by_name(test_world_component_id, test_component, "variable_3").operator Transform3D().origin.x - (-10.)) <= CMP_EPSILON);
	}

	// ~~ Test databag initialization ~~
	{
		ECS::register_databag<Test1Databag>();

		// Make sure the databag is null.
		CHECK(world.get_databag<Test1Databag>() == nullptr);
		CHECK(world.get_databag(Test1Databag::get_databag_id()) == nullptr);

		// Add the databag
		world.create_databag<Test1Databag>();

		// Make sure we can retrieve the databag.
		CHECK(world.get_databag<Test1Databag>() != nullptr);
		CHECK(world.get_databag(Test1Databag::get_databag_id()) != nullptr);
		CHECK(world.get_databag<Test1Databag>() == world.get_databag(Test1Databag::get_databag_id()));

		// Now remove it.
		world.remove_databag<Test1Databag>();

		// Make sure the databag is now nullptr.
		CHECK(world.get_databag<Test1Databag>() == nullptr);
		CHECK(world.get_databag(Test1Databag::get_databag_id()) == nullptr);
	}
}

TEST_CASE("[Modules][ECS] Test World NodePath.") {
	World world;
	EntityID entity_1 = world.create_entity();
	EntityID entity_2 = world.create_entity();
	EntityID entity_3 = world.create_entity();

	NodePath node_1("/root/node1");
	NodePath node_2("/root/node2");
	NodePath node_3("/root/node3");

	world.assign_nodepath_to_entity(entity_1, node_1);
	world.assign_nodepath_to_entity(entity_2, node_2);
	world.assign_nodepath_to_entity(entity_3, node_3);

	CHECK(world.get_entity_from_path(node_1) == entity_1);
	CHECK(world.get_entity_from_path(node_2) == entity_2);
	CHECK(world.get_entity_from_path(node_3) == entity_3);

	CHECK(world.get_entity_path(entity_1) == node_1);
	CHECK(world.get_entity_path(entity_2) == node_2);
	CHECK(world.get_entity_path(entity_3) == node_3);
}

TEST_CASE("[Modules][ECS] Test WorldECS runtime API create entity from prefab.") {
	WorldECS world;

	Entity3D entity_prefab;

	Dictionary defaults;
	defaults["origin"] = Vector3(10.0, 0.0, 0.0);
	entity_prefab.add_component("TransformComponent", defaults);

	const uint32_t entity_id = world.create_entity_from_prefab(&entity_prefab);

	// Make sure something is created.
	CHECK(entity_id != UINT32_MAX);

	// Make sure the component is created

	Object *comp = world.get_entity_component_by_name(
			entity_id,
			"TransformComponent");

	CHECK(comp != nullptr);

	TransformComponent *transf = godex::unwrap_component<TransformComponent>(comp);

	// Make sure the default is also set.
	CHECK(ABS(transf->origin.x - 10) <= CMP_EPSILON);
}

TEST_CASE("[Modules][ECS] Test WorldECS runtime API fetch databags.") {
	WorldECS world;

	Object *world_res_raw = world.get_databag_by_name("World");

	CHECK(world_res_raw != nullptr);

	World *world_ptr = godex::unwrap_databag<World>(world_res_raw);
	CHECK(world.get_world() == world_ptr);
}

TEST_CASE("[Modules][ECS] Test WorldECS runtime API fetch databags.") {
	Node root;
	root.set_name("root");

	WorldECS world_ecs;
	world_ecs.set_name("world_ecs");

	Node child_1;
	child_1.set_name("child_1");

	Node child_2;
	child_2.set_name("child_2");

	root.add_child(&world_ecs);
	root.add_child(&child_1);
	child_1.add_child(&child_2);

	World *world = world_ecs.get_world();

	SceneTreeDatabag *scene_tree_db = world->get_databag<SceneTreeDatabag>();
	CHECK(scene_tree_db != nullptr);
	CHECK(scene_tree_db->get_world_ecs() == &world_ecs);

	CHECK(&child_1 == scene_tree_db->get_node(NodePath("../child_1")));
	CHECK(&child_1 == scene_tree_db->get_node_or_null(NodePath("../child_1")));
	CHECK(&child_2 == scene_tree_db->get_node(NodePath("../child_1/child_2")));
	CHECK(&child_2 == scene_tree_db->get_node_or_null(NodePath("../child_1/child_2")));
	CHECK(&world_ecs == scene_tree_db->get_node(NodePath("./")));
	CHECK(&world_ecs == scene_tree_db->get_node_or_null(NodePath("./")));

	// Can't test Absolute path because it need a SceneTree that I can't create
	// inside the tests.
}
} // namespace godex_tests_world

#endif // TEST_ECS_WORLD_H
