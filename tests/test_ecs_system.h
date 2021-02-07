#ifndef TEST_ECS_SYSTEM_H
#define TEST_ECS_SYSTEM_H

#include "tests/test_macros.h"

#include "../components/dynamic_component.h"
#include "../ecs.h"
#include "../godot/components/transform_component.h"
#include "../pipeline/pipeline.h"
#include "../systems/dynamic_system.h"
#include "../world/world.h"
#include "test_utilities.h"

class TagTestComponent : public godex::Component {
	COMPONENT(TagTestComponent, DenseVectorStorage)
};

struct Test1Component : public godex::Component {
	COMPONENT(Test1Component, DenseVectorStorage)

public:
	static void _bind_methods() {
		ECS_BIND_PROPERTY(Test1Component, PropertyInfo(Variant::INT, "a"), a);
	}

	int a = 30;

	Test1Component() {}
	Test1Component(int p_a) :
			a(p_a) {}
};

class TestSystem1Databag : public godex::Databag {
	DATABAG(TestSystem1Databag)

public:
	int a = 10;
};

struct Event1Component : public godex::Component {
	COMPONENT_BATCH(Event1Component, DenseVector, 2)

public:
	static void _bind_methods() {
		ECS_BIND_PROPERTY(Event1Component, PropertyInfo(Variant::INT, "a"), a);
	}

	int a = 0;

	Event1Component() {}
	Event1Component(int p_a) :
			a(p_a) {}
};

class TestSystemSubPipeDatabag : public godex::Databag {
	DATABAG(TestSystemSubPipeDatabag)

	static void _bind_methods() {
		ECS_BIND_PROPERTY(TestSystemSubPipeDatabag, PropertyInfo(Variant::INT, "exe_count"), exe_count);
	}

public:
	int exe_count = 0;

	void set_exe_count(int p_i) {
		exe_count = p_i;
	}

	int get_exe_count() const {
		return exe_count;
	}
};

namespace godex_tests_system {

void test_system_tag(Query<TransformComponent, const TagTestComponent> &p_query) {
	while (p_query.is_done() == false) {
		auto [transform, component] = p_query.get();
		transform->get_transform_mut().origin.x += 100.0;
		p_query.next();
	}
}

void test_system_with_databag(TestSystem1Databag *test_res, Query<TransformComponent, const TagTestComponent> &p_query) {
	test_res->a += 10;
}

void test_system_check_databag(TestSystem1Databag *test_res) {
	CHECK(test_res != nullptr);
}

void test_system_generate_events(Query<TransformComponent> &p_query, Storage<Event1Component> *p_events) {
	CRASH_COND_MSG(p_events == nullptr, "When taken mutable it's never supposed to be nullptr.");

	while (p_query.is_done() == false) {
		p_events->insert(p_query.get_current_entity(), Event1Component(123));
		p_events->insert(p_query.get_current_entity(), Event1Component(456));
		p_events->insert(p_query.get_current_entity(), Event1Component(12));
		p_events->insert(p_query.get_current_entity(), Event1Component(33));
		p_query.next();
	}
}

void test_system_check_events(Query<const Event1Component> &p_query) {
	uint32_t entities_with_events = 0;

	while (p_query.is_done() == false) {
		auto [event] = p_query.get();

		CHECK(event.get_size() == 2);
		CHECK(event[0]->a == 123);
		CHECK(event[1]->a == 456);

		entities_with_events += 1;

		p_query.next();
	}

	CHECK(entities_with_events == 1);
}

void test_add_entity_system(WorldCommands *p_commands, Storage<TransformComponent> *p_events) {
	for (uint32_t i = 0; i < 3; i += 1) {
		const EntityID id = p_commands->create_entity();
		p_events->insert(id, TransformComponent());
	}
}

void test_remove_entity_system(WorldCommands *p_command, Query<const TransformComponent> &p_query) {
	uint32_t count = 0;

	while (p_query.is_done() == false) {
		count += 1;
		p_command->destroy_deferred(p_query.get_current_entity());
		p_query.next();
	}

	// Make sure the `test_add_entity_system` added exactly `3` `Entities` with
	// `TransformComponent` component.
	CHECK(count == 3);
}

TEST_CASE("[Modules][ECS] Test system and query") {
	ECS::register_component<TagTestComponent>();

	World world;

	EntityID entity_1 = world
								.create_entity()
								.with(TransformComponent())
								.with(TagTestComponent());

	EntityID entity_2 = world
								.create_entity()
								.with(TransformComponent());

	EntityID entity_3 = world
								.create_entity()
								.with(TransformComponent())
								.with(TagTestComponent());

	Pipeline pipeline;
	pipeline.add_system(test_system_tag);
	pipeline.build();
	pipeline.prepare(&world);

	for (uint32_t i = 0; i < 3; i += 1) {
		pipeline.dispatch(&world);
	}

	const Storage<const TransformComponent> *storage = world.get_storage<const TransformComponent>();

	const Vector3 entity_1_origin = storage->get(entity_1)->get_transform().origin;
	const Vector3 entity_2_origin = storage->get(entity_2)->get_transform().origin;
	const Vector3 entity_3_origin = storage->get(entity_3)->get_transform().origin;

	// This entity is expected to change.
	CHECK(ABS(entity_1_origin.x - 300.0) <= CMP_EPSILON);

	// This entity doesn't have a `TagTestComponent` so the systems should not
	// change it.
	CHECK(entity_2_origin.x <= CMP_EPSILON);

	// This entity is expected to change.
	CHECK(ABS(entity_3_origin.x - 300.0) <= CMP_EPSILON);
}

TEST_CASE("[Modules][ECS] Test dynamic system using a script.") {
	LocalVector<ScriptProperty> props;
	props.push_back({ PropertyInfo(Variant::INT, "variable_1"), 1 });
	props.push_back({ PropertyInfo(Variant::BOOL, "variable_2"), false });

	const uint32_t test_dyn_component_id = ECS::register_script_component(
			"TestDynamicSystemComponent1.gd",
			props,
			StorageType::DENSE_VECTOR);

	World world;

	EntityID entity_1 = world
								.create_entity()
								.with(TransformComponent())
								.with(test_dyn_component_id, Dictionary());

	EntityID entity_2 = world
								.create_entity()
								.with(TransformComponent());

	EntityID entity_3 = world
								.create_entity()
								.with(TransformComponent())
								.with(test_dyn_component_id, Dictionary());

	Object target_obj;
	{
		// Create the script.
		String code;
		code += "extends Object\n";
		code += "\n";
		code += "func _for_each(transform_com, test_comp):\n";
		code += "	assert(transform_com.is_valid())\n";
		code += "	assert(transform_com.is_mutable())\n";
		code += "	transform_com.transform.origin.x += 100.0\n";
		code += "	if test_comp != null:\n";
		code += "		test_comp.variable_1 += 1\n";
		code += "		test_comp.variable_2 = Transform()\n";
		code += "\n";

		CHECK(build_and_assign_script(&target_obj, code));
	}

	// Build dynamic query.
	const uint32_t system_id = ECS::register_dynamic_system("TestDynamicSystem.gd");
	godex::DynamicSystemInfo *dynamic_system_info = ECS::get_dynamic_system_info(system_id);
	dynamic_system_info->with_component(TransformComponent::get_component_id(), true);
	dynamic_system_info->maybe_component(test_dyn_component_id, true);
	dynamic_system_info->set_target(target_obj.get_script_instance());
	dynamic_system_info->build();

	// Create the pipeline.
	Pipeline pipeline;
	// Add the system to the pipeline.
	pipeline.add_registered_system(system_id);
	pipeline.build();
	pipeline.prepare(&world);

	// Dispatch
	for (uint32_t i = 0; i < 3; i += 1) {
		pipeline.dispatch(&world);
	}

	// Validate the C++ component.
	{
		const Storage<const TransformComponent> *storage = world.get_storage<const TransformComponent>();

		const Vector3 entity_1_origin = storage->get(entity_1)->get_transform().origin;
		const Vector3 entity_2_origin = storage->get(entity_2)->get_transform().origin;
		const Vector3 entity_3_origin = storage->get(entity_3)->get_transform().origin;

		// This entity is expected to change.
		CHECK(ABS(entity_1_origin.x - 300.0) <= CMP_EPSILON);

		// This entity is expected to change.
		CHECK(ABS(entity_2_origin.x - 300.0) <= CMP_EPSILON);

		// This entity is expected to change.
		CHECK(ABS(entity_3_origin.x - 300.0) <= CMP_EPSILON);
	}

	// Validate the dynamic component.
	{
		const StorageBase *storage = world.get_storage(test_dyn_component_id);
		CHECK(storage->get_ptr(entity_1)->get("variable_1") == Variant(4));
		// Make sure this doesn't changed.
		CHECK(storage->get_ptr(entity_1)->get("variable_2") == Variant(false));

		CHECK(storage->has(entity_2) == false);

		CHECK(storage->get_ptr(entity_3)->get("variable_1") == Variant(4));
		// Make sure this doesn't changed.
		CHECK(storage->get_ptr(entity_3)->get("variable_2") == Variant(false));
	}
}

void test_sub_pipeline_execute(World *p_world, Pipeline *p_pipeline) {
	CRASH_COND_MSG(p_world == nullptr, "The world is never nullptr in this test.");
	CRASH_COND_MSG(p_pipeline == nullptr, "The pipeline is never nullptr in this test.");
	CRASH_COND_MSG(p_pipeline->is_ready() == false, "The pipeline is not supposed to be not ready at this point.");

	uint32_t exe_count = 0;
	// Extract the info and forget about the databag, so the pipeline can
	// access it safely.
	{
		const TestSystemSubPipeDatabag *bag = p_world->get_databag<TestSystemSubPipeDatabag>();
		exe_count = bag->exe_count;
	}

	for (uint32_t i = 0; i < exe_count; i += 1) {
		p_pipeline->dispatch(p_world);
	}
}

void test_system_transform_add_x(Query<TransformComponent> &p_query) {
	while (p_query.is_done() == false) {
		auto [transform] = p_query.get();
		transform->get_transform_mut().origin.x += 100.0;
		p_query.next();
	}
}

void test_move_root(Query<TransformComponent> &p_query) {
	while (p_query.is_done() == false) {
		if (p_query.get_current_entity() == EntityID(0)) {
			auto [transform] = p_query.get();
			transform->get_transform_mut().origin.x += 1.0;
			return;
		}
		p_query.next();
	}
}

void test_move_1_global(Query<TransformComponent> &p_query) {
	while (p_query.is_done() == false) {
		if (p_query.get_current_entity() == EntityID(1)) {
			auto [transform] = p_query.get(Space::GLOBAL);
			transform->get_transform_mut().origin.x = 6.0;
			return;
		}
		p_query.next();
	}
}

void test_make_entity_1_root(Storage<Child> *p_hierarchy) {
	p_hierarchy->insert(1, Child());
}

TEST_CASE("[Modules][ECS] Test dynamic system with sub pipeline C++.") {
	ECS::register_databag<TestSystemSubPipeDatabag>();

	// ~~ Sub pipeline ~~
	Pipeline sub_pipeline;
	sub_pipeline.add_system(test_system_transform_add_x);
	sub_pipeline.build();

	const uint32_t sub_pipeline_system_id = ECS::register_dynamic_system("TestSubPipelineExecute");
	godex::DynamicSystemInfo *sub_pipeline_system = ECS::get_dynamic_system_info(sub_pipeline_system_id);
	sub_pipeline_system->set_target(test_sub_pipeline_execute);
	// Used internally by the `test_sub_pipeline_execute`.
	sub_pipeline_system->with_databag(TestSystemSubPipeDatabag::get_databag_id(), false);
	ECS::set_system_pipeline(sub_pipeline_system_id, &sub_pipeline);
	sub_pipeline_system->build();

	World world;

	// ~~ Main pipeline ~~
	Pipeline main_pipeline;
	main_pipeline.add_registered_system(sub_pipeline_system_id);
	main_pipeline.build();
	main_pipeline.prepare(&world);

	// ~~ Create world ~~
	world.create_databag<TestSystemSubPipeDatabag>();
	world.get_databag<TestSystemSubPipeDatabag>()->exe_count = 2;

	EntityID entity_1 = world
								.create_entity()
								.with(TransformComponent());

	// ~~ Test ~~
	{
		main_pipeline.dispatch(&world);
		main_pipeline.dispatch(&world);

		// The subsystem is dispatched 2 times per main pipeline dispatch,
		// since we are dispatching the main pipeline 2 times the
		// `test_system_transform_add_x` add 100, four times.
		const TransformComponent *comp = world.get_storage<TransformComponent>()->get(entity_1);
		CHECK(ABS(comp->get_transform().origin.x - 400.0) <= CMP_EPSILON);
	}

	{
		// Now change the sub execution to 6
		world.get_databag<TestSystemSubPipeDatabag>()->exe_count = 6;

		// Dispatch the main pipeline
		main_pipeline.dispatch(&world);

		// Verify the execution is properly done by making sure the value is 1000.
		const TransformComponent *comp = world.get_storage<TransformComponent>()->get(entity_1);
		CHECK(ABS(comp->get_transform().origin.x - 1000.0) <= CMP_EPSILON);
	}
}

TEST_CASE("[Modules][ECS] Test system and databag") {
	ECS::register_databag<TestSystem1Databag>();

	World world;

	world.create_databag<TestSystem1Databag>();

	// Test with databag
	{
		// Confirm the databag is initialized.
		CHECK(world.get_databag<TestSystem1Databag>()->a == 10);

		// Create the pipeline.
		Pipeline pipeline;
		// Add the system to the pipeline.
		pipeline.add_system(test_system_with_databag);
		pipeline.build();
		pipeline.prepare(&world);

		// Dispatch
		for (uint32_t i = 0; i < 3; i += 1) {
			pipeline.dispatch(&world);
		}

		CHECK(world.get_databag<TestSystem1Databag>()->a == 40);
	}

	// Test without databag
	{
		world.remove_databag<TestSystem1Databag>();

		// Confirm the databag doesn't exists.
		CHECK(world.get_databag<TestSystem1Databag>() == nullptr);

		// Create the pipeline.
		Pipeline pipeline;
		pipeline.add_system(test_system_check_databag);
		pipeline.build();

		// This make sure to create the `Databag`
		pipeline.prepare(&world);

		// Dispatch
		for (uint32_t i = 0; i < 3; i += 1) {
			pipeline.dispatch(&world);
		}

		// Make sure the databag is not nullptr.
		CHECK(world.get_databag<TestSystem1Databag>() != nullptr);
	}
}

TEST_CASE("[Modules][ECS] Test system databag fetch with dynamic query.") {
	World world;
	world.create_databag<TestSystemSubPipeDatabag>();
	world.get_databag<TestSystemSubPipeDatabag>()->exe_count = 20;

	world
			.create_entity()
			.with(TransformComponent());

	Object target_obj;
	{
		// Create the script.
		String code;
		code += "extends Object\n";
		code += "\n";
		code += "func _for_each(test_databag, transform_com):\n";
		code += "	assert(test_databag.is_valid())\n";
		code += "	assert(test_databag.is_mutable())\n";
		code += "	test_databag.exe_count = 10\n";
		code += "\n";

		CHECK(build_and_assign_script(&target_obj, code));
	}

	// Build dynamic query.
	const uint32_t system_id = ECS::register_dynamic_system("TestDatabagDynamicSystem.gd");
	godex::DynamicSystemInfo *dynamic_system_info = ECS::get_dynamic_system_info(system_id);
	dynamic_system_info->with_databag(TestSystemSubPipeDatabag::get_databag_id(), true);
	dynamic_system_info->with_component(TransformComponent::get_component_id(), false);
	dynamic_system_info->set_target(target_obj.get_script_instance());
	dynamic_system_info->build();

	// Create the pipeline.
	Pipeline pipeline;
	// Add the system to the pipeline.
	pipeline.add_registered_system(system_id);
	pipeline.build();
	pipeline.prepare(&world);

	// Dispatch 1 time.
	pipeline.dispatch(&world);

	// Make sure the `exe_count` is changed to 10 by the script.
	CHECK(world.get_databag<TestSystemSubPipeDatabag>()->exe_count == 10);
}

TEST_CASE("[Modules][ECS] Test event mechanism.") {
	ECS::register_component_event<Event1Component>();

	World world;

	EntityID entity = world
							  .create_entity()
							  .with(TransformComponent());

	// Dispatch the pipeline.
	Pipeline pipeline;
	pipeline.add_system(test_system_generate_events);
	pipeline.add_system(test_system_check_events);
	pipeline.build();
	pipeline.prepare(&world);

	// Make sure no component event is left at the end of each cycle.
	for (uint32_t i = 0; i < 5; i += 1) {
		if (world.get_storage(Event1Component::get_component_id())) {
			// The first check the storages is `nullptr`, so this check.
			CHECK(world.get_storage(Event1Component::get_component_id())->has(entity) == false);
		}

		pipeline.dispatch(&world);

		// At the end of each dispatch the events are dropped.
		CHECK(world.get_storage(Event1Component::get_component_id())->has(entity) == false);
	}
}

TEST_CASE("[Modules][ECS] Test create and remove Entity from Systems.") {
	World world;

	// Create the pipeline.
	Pipeline pipeline;
	// Add the system to the pipeline.
	pipeline.add_system(test_add_entity_system);
	pipeline.add_system(test_remove_entity_system);
	pipeline.build();
	pipeline.prepare(&world);

	for (uint32_t i = 0; i < 5; i += 1) {
		pipeline.dispatch(&world);

		{
			// Count the `Entities` at this point.
			Query<const TransformComponent> query(&world);

			uint32_t count = 0;
			while (query.is_done() == false) {
				count += 1;
				query.next();
			}

			// The `System` removes the `Entities` defferred, so at this point
			// the `Entities` still exists.
			CHECK(count == 3);
		}

		world.flush();

		{
			// Count the `Entities` at this point.
			Query<const TransformComponent> query(&world);

			uint32_t count = 0;
			while (query.is_done() == false) {
				count += 1;
				query.next();
			}

			// Now the `Entities` are removed.
			CHECK(count == 0);
		}
	}
}

TEST_CASE("[Modules][ECS] Test system and hierarchy.") {
	World world;

	// Create a hierarchy like this:
	// Entity 0           1 Local | 1 Global
	//  |- Entity 1       1 Local | 2 Global
	//  |   |- Entity 2   1 Local | 3 Global
	EntityID entity_0 = world
								.create_entity()
								.with(TransformComponent(Transform(Basis(), Vector3(1, 0, 0))));

	EntityID entity_1 = world
								.create_entity()
								.with(Child(entity_0))
								.with(TransformComponent(Transform(Basis(), Vector3(1, 0, 0))));

	EntityID entity_2 = world
								.create_entity()
								.with(Child(entity_1))
								.with(TransformComponent(Transform(Basis(), Vector3(1, 0, 0))));

	// Try move `Entity_0` using `LOCAL`.
	{
		// Create the pipeline.
		Pipeline pipeline;
		pipeline.add_system(test_move_root);
		pipeline.build();
		pipeline.prepare(&world);

		// Check local transform.
		{
			const TransformComponent *entity_2_transform = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform->get_transform().origin.x - 1.0) <= CMP_EPSILON);
		}

		// Check global transform
		{
			const TransformComponent *entity_2_transform = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform->get_transform().origin.x - 3.0) <= CMP_EPSILON);
		}

		pipeline.dispatch(&world);

		// Check local transform after root motion.
		{
			const TransformComponent *entity_2_transform = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform->get_transform().origin.x - 1.0) <= CMP_EPSILON);
		}

		// Check global transform after root motion.
		{
			const TransformComponent *entity_2_transform = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform->get_transform().origin.x - 4.0) <= CMP_EPSILON);
		}
	}

	// Now move `Entity_1` but using `GLOBAL`.
	{
		// Create the pipeline.
		Pipeline pipeline;
		pipeline.add_system(test_move_1_global);
		pipeline.build();
		pipeline.prepare(&world);

		// Hierarchy is:
		// Entity 0           2 Local | 2 Global
		//  |- Entity 1       1 Local | 3 Global
		//  |   |- Entity 2   1 Local | 4 Global

		// Check local transform before motion.
		{
			const TransformComponent *entity_1_transform = world.get_storage<TransformComponent>()->get(entity_1);
			CHECK(ABS(entity_1_transform->get_transform().origin.x - 1.0) <= CMP_EPSILON);
		}

		// Dispatch the pipeline, so to move the `Entity_1` globally.
		pipeline.dispatch(&world);

		// Hierarchy is:
		// Entity 0           2 Local | 2 Global
		//  |- Entity 1       4 Local | 6 Global
		//  |   |- Entity 2   1 Local | 7 Global

		// `Entity 0` din't move.
		{
			const TransformComponent *entity_0_transform = world.get_storage<TransformComponent>()->get(entity_0);
			CHECK(ABS(entity_0_transform->get_transform().origin.x - 2.0) <= CMP_EPSILON);
		}

		// `Entity 1` moved globally to 6, so check local and global position:
		{
			const TransformComponent *entity_1_transform_l = world.get_storage<TransformComponent>()->get(entity_1);
			CHECK(ABS(entity_1_transform_l->get_transform().origin.x - 4.0) <= CMP_EPSILON);

			const TransformComponent *entity_1_transform_g = world.get_storage<TransformComponent>()->get(entity_1, Space::GLOBAL);
			CHECK(ABS(entity_1_transform_g->get_transform().origin.x - 6.0) <= CMP_EPSILON);
		}

		// `Entity 2` moved cause of the parent motion.
		{
			const TransformComponent *entity_2_transform_l = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform_l->get_transform().origin.x - 1.0) <= CMP_EPSILON);

			const TransformComponent *entity_2_transform_g = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform_g->get_transform().origin.x - 7.0) <= CMP_EPSILON);
		}
	}

	// Now change hierarchy
	{
		// Create the pipeline.
		Pipeline pipeline;
		pipeline.add_system(test_make_entity_1_root);
		pipeline.build();
		pipeline.prepare(&world);

		// Dispatch the pipeline, so to move the `Entity_1` globally.
		pipeline.dispatch(&world);

		// Hierarchy is:
		// Entity 1       4 Local | 4 Global
		//  |- Entity 2   1 Local | 5 Global

		CHECK(world.get_storage<Child>()->has(0) == false); // This doesn't exist anymore.
		CHECK(world.get_storage<Child>()->has(1)); // This is Root
		CHECK(world.get_storage<Child>()->has(2)); // This is child of `Entity 1`

		// `Entity 0` din't move.
		{
			const TransformComponent *entity_0_transform = world.get_storage<TransformComponent>()->get(entity_0);
			CHECK(ABS(entity_0_transform->get_transform().origin.x - 2.0) <= CMP_EPSILON);
		}

		// `Entity 1` it's not root, and it's relative to itself: local and
		// global are equals.
		{
			const TransformComponent *entity_1_transform_l = world.get_storage<TransformComponent>()->get(entity_1);
			CHECK(ABS(entity_1_transform_l->get_transform().origin.x - 4.0) <= CMP_EPSILON);

			const TransformComponent *entity_1_transform_g = world.get_storage<TransformComponent>()->get(entity_1, Space::GLOBAL);
			CHECK(ABS(entity_1_transform_g->get_transform().origin.x - 4.0) <= CMP_EPSILON);
		}

		// `Entity 2` moved cause of the parent hierarchy change.
		{
			const TransformComponent *entity_2_transform_l = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform_l->get_transform().origin.x - 1.0) <= CMP_EPSILON);

			const TransformComponent *entity_2_transform_g = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform_g->get_transform().origin.x - 5.0) <= CMP_EPSILON);
		}
	}

	// Try move `Entity_0` using `LOCAL`, and make sure now one else move.
	{
		// Create the pipeline.
		Pipeline pipeline;
		pipeline.add_system(test_move_root);
		pipeline.build();
		pipeline.prepare(&world);

		// Check `Entity 0` transform.
		{
			const TransformComponent *entity_0_transform_l = world.get_storage<TransformComponent>()->get(entity_0);
			CHECK(ABS(entity_0_transform_l->get_transform().origin.x - 2.0) <= CMP_EPSILON);

			const TransformComponent *entity_0_transform_g = world.get_storage<TransformComponent>()->get(entity_0, Space::GLOBAL);
			CHECK(ABS(entity_0_transform_g->get_transform().origin.x - 2.0) <= CMP_EPSILON);
		}

		// Check `Entity 1` transform.
		{
			const TransformComponent *entity_1_transform = world.get_storage<TransformComponent>()->get(entity_1, Space::GLOBAL);
			CHECK(ABS(entity_1_transform->get_transform().origin.x - 4.0) <= CMP_EPSILON);
		}

		pipeline.dispatch(&world);

		// Hierarchy is:
		// Entity 1       4 Local | 4 Global
		//  |- Entity 2   1 Local | 5 Global

		// Make sure `Entity 0` moved.
		{
			const TransformComponent *entity_0_transform_l = world.get_storage<TransformComponent>()->get(entity_0);
			CHECK(ABS(entity_0_transform_l->get_transform().origin.x - 3.0) <= CMP_EPSILON);

			const TransformComponent *entity_0_transform_g = world.get_storage<TransformComponent>()->get(entity_0, Space::GLOBAL);
			CHECK(ABS(entity_0_transform_g->get_transform().origin.x - 3.0) <= CMP_EPSILON);
		}

		// Make sure `Entity 1` din't move.
		{
			const TransformComponent *entity_1_transform = world.get_storage<TransformComponent>()->get(entity_1, Space::GLOBAL);
			CHECK(ABS(entity_1_transform->get_transform().origin.x - 4.0) <= CMP_EPSILON);
		}
	}

	// Try move `Entity_2` using a GDScript system.
	{
		Ref<System> target_obj;
		target_obj.instance();
		{
			// Create the script.
			String code;
			code += "extends System\n";
			code += "\n";
			code += "func _for_each(transform_com):\n";
			code += "	if get_current_entity_id() == 2:\n";
			code += "		transform_com.transform.origin.x = 10.0\n";
			code += "\n";

			CHECK(build_and_assign_script(target_obj.ptr(), code));
		}

		// Build dynamic query.
		const uint32_t system_id = ECS::register_dynamic_system("TestMoveHierarchySystem.gd");
		godex::DynamicSystemInfo *dynamic_system_info = ECS::get_dynamic_system_info(system_id);
		dynamic_system_info->set_space(Space::GLOBAL);
		dynamic_system_info->with_component(TransformComponent::get_component_id(), true);
		target_obj->__force_set_system_info(dynamic_system_info, system_id);
		dynamic_system_info->set_target(target_obj->get_script_instance());
		dynamic_system_info->build();

		// Create the pipeline.
		Pipeline pipeline;
		pipeline.add_registered_system(system_id);
		pipeline.build();
		pipeline.prepare(&world);

		// Hierarchy is:
		// Entity 1       4 Local | 4 Global
		//  |- Entity 2   1 Local | 5 Global

		// Make sure `Entity 2` initial position.
		{
			const TransformComponent *entity_2_transform_l = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform_l->get_transform().origin.x - 1.0) <= CMP_EPSILON);

			const TransformComponent *entity_2_transform_g = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform_g->get_transform().origin.x - 5.0) <= CMP_EPSILON);
		}

		pipeline.dispatch(&world);

		// Hierarchy is:
		// Entity 1       4 Local | 4 Global
		//  |- Entity 2   6 Local | 10 Global

		// Make sure `Entity 1` didn't move.
		{
			const TransformComponent *entity_1_transform_l = world.get_storage<TransformComponent>()->get(entity_1);
			CHECK(ABS(entity_1_transform_l->get_transform().origin.x - 4.0) <= CMP_EPSILON);
		}

		// Make sure `Entity 2` moved.
		{
			const TransformComponent *entity_2_transform_l = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform_l->get_transform().origin.x - 6.0) <= CMP_EPSILON);

			const TransformComponent *entity_2_transform_g = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform_g->get_transform().origin.x - 10.0) <= CMP_EPSILON);
		}
	}
}

TEST_CASE("[Modules][ECS] Test Add/remove from dynamic query.") {
	ECS::register_component<Test1Component>();

	World world;

	const EntityID entity_1 = world
									  .create_entity()
									  .with(TransformComponent());

	// This entity got created by the script.
	const EntityID entity_2(1);
	const EntityID entity_3(2);

	// Test add
	{
		Object target_obj;
		{
			// Create the script.
			String code;
			code += "extends Object\n";
			code += "\n";
			code += "func _for_each(world_commands, comp_storage, transform_com):\n";
			code += "	var entity_2 = world_commands.create_entity()\n";
			code += "	comp_storage.insert(entity_2, {\"a\": 975})\n";
			code += "	var entity_3 = world_commands.create_entity()\n";
			code += "	comp_storage.insert(entity_3)\n";
			code += "\n";

			CHECK(build_and_assign_script(&target_obj, code));
		}

		// Build dynamic query.
		const uint32_t system_id = ECS::register_dynamic_system("TestSpawnDynamicSystem.gd");
		godex::DynamicSystemInfo *dynamic_system_info = ECS::get_dynamic_system_info(system_id);
		dynamic_system_info->with_databag(WorldCommands::get_databag_id(), true);
		dynamic_system_info->with_storage(Test1Component::get_component_id());
		dynamic_system_info->with_component(TransformComponent::get_component_id(), false);
		dynamic_system_info->set_target(target_obj.get_script_instance());
		dynamic_system_info->build();

		// Create the pipeline.
		Pipeline pipeline;
		// Add the system to the pipeline.
		pipeline.add_registered_system(system_id);
		pipeline.build();
		pipeline.prepare(&world);

		// Dispatch 1 time.
		pipeline.dispatch(&world);

		// Make sure a new entity got created.
		CHECK(world.get_biggest_entity_id() == entity_3);

		// Make sure the entity 0 has the `TransformComponent`
		CHECK(world.get_storage<TransformComponent>()->has(entity_1));

		// but also the runtime created one (entity 2) has the `Test1Component`.
		CHECK(world.get_storage<Test1Component>()->has(entity_2));
		CHECK(world.get_storage<Test1Component>()->has(entity_3));

		// Make sure the value is correctly set.
		CHECK(world.get_storage<Test1Component>()->get(entity_2)->a == 975);

		// Make sure the default value is set.
		CHECK(world.get_storage<Test1Component>()->get(entity_3)->a == Test1Component().a);
	}

	// Test remove
	{
		Object target_obj;
		{
			// Create the script.
			String code;
			code += "extends Object\n";
			code += "\n";
			code += "func _for_each(world_commands, comp_storage, transform_com):\n";
			code += "	comp_storage.remove(2)\n";
			code += "	comp_storage.remove(1)\n";
			code += "\n";

			CHECK(build_and_assign_script(&target_obj, code));
		}

		// Build dynamic query.
		const uint32_t system_id = ECS::register_dynamic_system("TestRemoveDynamicSystem.gd");
		godex::DynamicSystemInfo *dynamic_system_info = ECS::get_dynamic_system_info(system_id);
		dynamic_system_info->with_databag(WorldCommands::get_databag_id(), true);
		dynamic_system_info->with_storage(Test1Component::get_component_id());
		dynamic_system_info->with_component(TransformComponent::get_component_id(), false);
		dynamic_system_info->set_target(target_obj.get_script_instance());
		dynamic_system_info->build();

		// Create the pipeline.
		Pipeline pipeline;
		// Add the system to the pipeline.
		pipeline.add_registered_system(system_id);
		pipeline.build();
		pipeline.prepare(&world);

		// Dispatch 1 time.
		pipeline.dispatch(&world);

		// Make sure the Test1Component is correctly removed
		CHECK(world.get_storage<Test1Component>()->has(entity_2) == false);
		CHECK(world.get_storage<Test1Component>()->has(entity_3) == false);
	}
}
} // namespace godex_tests_system

#endif // TEST_ECS_SYSTEM_H
