#ifndef TEST_ECS_SYSTEM_H
#define TEST_ECS_SYSTEM_H

#include "tests/test_macros.h"

#include "../components/dynamic_component.h"
#include "../ecs.h"
#include "../events/events.h"
#include "../modules/godot/components/transform_component.h"
#include "../modules/godot/nodes/ecs_utilities.h"
#include "../pipeline/pipeline.h"
#include "../pipeline/pipeline_builder.h"
#include "../storage/dense_vector_storage.h"
#include "../systems/dynamic_system.h"
#include "../world/world.h"
#include "test_utilities.h"

struct TagTestComponent {
	COMPONENT(TagTestComponent, DenseVectorStorage)
	static void _bind_methods() {}
};

struct Test1Component {
	COMPONENT(Test1Component, DenseVectorStorage)

	static void _bind_methods() {
		ECS_BIND_PROPERTY(Test1Component, PropertyInfo(Variant::INT, "a"), a);
	}

	int a = 30;

	Test1Component(int p_a) :
			a(p_a) {}
};

struct TestSystem1Databag : public godex::Databag {
	DATABAG(TestSystem1Databag)

	int a = 10;
};

struct BatchableComponent1 {
	COMPONENT_BATCH(BatchableComponent1, DenseVector, 2)

	static void _bind_methods() {
		ECS_BIND_PROPERTY(BatchableComponent1, PropertyInfo(Variant::INT, "a"), a);
	}

	int a = 0;

	BatchableComponent1(int p_a) :
			a(p_a) {}
};

class TestSystemSubPipeDatabag : public godex::Databag {
	DATABAG(TestSystemSubPipeDatabag)

	static void _bind_methods() {
		ECS_BIND_PROPERTY(TestSystemSubPipeDatabag, PropertyInfo(Variant::INT, "exe_count"), exe_count);
		ECS_BIND_PROPERTY(TestSystemSubPipeDatabag, PropertyInfo(Variant::INT, "iterations"), iterations);
		ECS_BIND_PROPERTY(TestSystemSubPipeDatabag, PropertyInfo(Variant::INT, "sub_iterations"), sub_iterations);
	}

public:
	int exe_count = 0;

	int iterations = 0;
	int sub_iterations = 0;

	void set_exe_count(int p_i) {
		exe_count = p_i;
	}

	int get_exe_count() const {
		return exe_count;
	}
};

namespace godex_tests_system {

void test_system_tag(Query<TransformComponent, const TagTestComponent> &p_query) {
	for (auto [transform, component] : p_query) {
		transform->origin.x += 100.0;
	}
}

void test_system_with_databag(TestSystem1Databag *test_res, Query<TransformComponent, const TagTestComponent> &p_query) {
	test_res->a += 10;
}

void test_system_check_databag(TestSystem1Databag *test_res) {
	CHECK(test_res != nullptr);
}

void test_system_generate_batch(Query<EntityID, TransformComponent> &p_query, Storage<BatchableComponent1> *p_events) {
	CRASH_COND_MSG(p_events == nullptr, "When taken mutable it's never supposed to be nullptr.");

	for (auto [entity, _t] : p_query) {
		p_events->insert(entity, BatchableComponent1(123));
		p_events->insert(entity, BatchableComponent1(456));
		p_events->insert(entity, BatchableComponent1(12));
		p_events->insert(entity, BatchableComponent1(33));
	}
}

void test_system_check_events(Query<Batch<const BatchableComponent1>> &p_query) {
	uint32_t entities_with_events = 0;

	for (auto [event] : p_query) {
		CHECK(event.get_size() == 2);
		CHECK(event[0]->a == 123);
		CHECK(event[1]->a == 456);

		entities_with_events += 1;
	}

	CHECK(entities_with_events == 1);
}

void test_add_entity_system(WorldCommands *p_commands, Storage<TransformComponent> *p_events) {
	for (uint32_t i = 0; i < 3; i += 1) {
		const EntityID id = p_commands->create_entity();
		p_events->insert(id, TransformComponent());
	}
}

void test_remove_entity_system(WorldCommands *p_command, Query<EntityID, const TransformComponent> &p_query) {
	uint32_t count = 0;

	for (auto [entity, _t] : p_query) {
		count += 1;
		p_command->destroy_deferred(entity);
	}

	// Make sure the `test_add_entity_system` added exactly `3` `Entities` with
	// `TransformComponent` component.
	CHECK(count == 3);
}

TEST_CASE("[Modules][ECS] Test system and query") {
	ECS::register_component<TagTestComponent>();
	godex::system_id system_id = ECS::register_system(test_system_tag, "test_system_tag").get_id();

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

	PipelineBuilder pipeline_builder;
	pipeline_builder.add_system(system_id);
	Pipeline pipeline;
	pipeline_builder.build(pipeline);
	Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);

	for (uint32_t i = 0; i < 3; i += 1) {
		pipeline.dispatch(token);
	}

	const Storage<const TransformComponent> *storage = world.get_storage<const TransformComponent>();

	const Vector3 entity_1_origin = storage->get(entity_1)->origin;
	const Vector3 entity_2_origin = storage->get(entity_2)->origin;
	const Vector3 entity_3_origin = storage->get(entity_3)->origin;

	// This entity is expected to change.
	CHECK(ABS(entity_1_origin.x - 300.0) <= CMP_EPSILON);

	// This entity doesn't have a `TagTestComponent` so the systems should not
	// change it.
	CHECK(entity_2_origin.x <= CMP_EPSILON);

	// This entity is expected to change.
	CHECK(ABS(entity_3_origin.x - 300.0) <= CMP_EPSILON);
}

TEST_CASE("[Modules][ECS] Test dynamic system using a script.") {
	initialize_script_ecs();

	{
		// Create the script.
		String code;
		code += "extends Component\n";
		code += "var variable_1: int = 1\n";
		code += "var variable_2: bool = false\n";
		code += "\n";

		CHECK(register_ecs_script("TestDynamicSystemComponent1.gd", code));
	}

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	var query = DynamicQuery.new()\n";
		code += "	query.with_component(ECS.TransformComponent, MUTABLE)\n";
		code += "	query.maybe_component(ECS.TestDynamicSystemComponent1_gd, MUTABLE)\n";
		code += "	with_query(query)\n";
		code += "\n";
		code += "func _execute(q):\n";
		code += "	while q.next():\n";
		code += "		assert(q[0].is_valid())\n";
		code += "		assert(q[0].is_mutable())\n";
		code += "		q[0].origin.x += 100.0\n";
		code += "		if q[1].is_valid():\n";
		code += "			q[1].variable_1 += 1\n";
		code += "			# Try to set a different type.\n";
		code += "			q[1].set(\"variable_2\", Transform3D())\n";
		code += "\n";

		CHECK(register_ecs_script("TestDynamicSystem1.gd", code));
	}

	build_scripts();
	flush_ecs_script_preparation();

	World world;

	EntityID entity_1 = world
								.create_entity()
								.with(TransformComponent())
								.with(ECS::get_component_id("TestDynamicSystemComponent1.gd"), Dictionary());

	EntityID entity_2 = world
								.create_entity()
								.with(TransformComponent());

	EntityID entity_3 = world
								.create_entity()
								.with(TransformComponent())
								.with(ECS::get_component_id("TestDynamicSystemComponent1.gd"), Dictionary());

	PipelineBuilder pipeline_builder;
	// Add the system to the pipeline.
	pipeline_builder.add_system(ECS::get_system_id("TestDynamicSystem1.gd"));

	// Create the pipeline.
	Pipeline pipeline;
	pipeline_builder.build(pipeline);
	Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);

	// Dispatch
	for (uint32_t i = 0; i < 3; i += 1) {
		pipeline.dispatch(token);
	}

	// Validate the C++ component.
	{
		const Storage<const TransformComponent> *storage = world.get_storage<const TransformComponent>();

		const Vector3 entity_1_origin = storage->get(entity_1)->origin;
		const Vector3 entity_2_origin = storage->get(entity_2)->origin;
		const Vector3 entity_3_origin = storage->get(entity_3)->origin;

		// This entity is expected to change.
		CHECK(ABS(entity_1_origin.x - 300.0) <= CMP_EPSILON);

		// This entity is expected to change.
		CHECK(ABS(entity_2_origin.x - 300.0) <= CMP_EPSILON);

		// This entity is expected to change.
		CHECK(ABS(entity_3_origin.x - 300.0) <= CMP_EPSILON);
	}

	// Validate the dynamic component.
	{
		godex::component_id test_dyn_component_id = ECS::get_component_id("TestDynamicSystemComponent1.gd");
		const StorageBase *storage = world.get_storage(test_dyn_component_id);
		CHECK(ECS::unsafe_component_get_by_name(test_dyn_component_id, storage->get_ptr(entity_1), "variable_1") == Variant(4));
		// Make sure this doesn't changed.
		CHECK(ECS::unsafe_component_get_by_name(test_dyn_component_id, storage->get_ptr(entity_1), "variable_2") == Variant(false));

		CHECK(storage->has(entity_2) == false);

		CHECK(ECS::unsafe_component_get_by_name(test_dyn_component_id, storage->get_ptr(entity_3), "variable_1") == Variant(4));
		// Make sure this doesn't changed.
		CHECK(ECS::unsafe_component_get_by_name(test_dyn_component_id, storage->get_ptr(entity_3), "variable_2") == Variant(false));
	}

	finalize_script_ecs();
}

uint32_t test_sub_pipeline_execute(TestSystemSubPipeDatabag *bag) {
	return bag->exe_count;
}

void test_system_transform_add_x(Query<TransformComponent> &p_query) {
	for (auto [transform] : p_query) {
		transform->origin.x += 100.0;
	}
}

TEST_CASE("[Modules][ECS] Test dynamic system with sub pipeline C++.") {
	ECS::register_databag<TestSystemSubPipeDatabag>();

	ECS::register_system_dispatcher(test_sub_pipeline_execute, "test_sub_pipeline_execute");

	ECS::register_system(test_system_transform_add_x, "test_system_transform_add_x")
			.execute_in(PHASE_PROCESS, "test_sub_pipeline_execute");

	initialize_script_ecs();

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_databag(ECS.TestSystemSubPipeDatabag, MUTABLE)\n";
		code += "\n";
		code += "func _execute(test_databag):\n";
		code += "	test_databag.iterations += 1\n";
		code += "\n";

		CHECK(register_ecs_script("TestSubPip_System.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_in(ECS.PHASE_PROCESS, ECS.test_sub_pipeline_execute)\n";
		code += "	with_databag(ECS.TestSystemSubPipeDatabag, MUTABLE)\n";
		code += "\n";
		code += "func _execute(test_databag):\n";
		code += "	test_databag.sub_iterations += 1\n";
		code += "\n";

		CHECK(register_ecs_script("TestSubPip_SubSystem.gd", code));
	}

	build_scripts();
	flush_ecs_script_preparation();

	// ~~ Main pipeline ~~
	Vector<StringName> system_bundles;

	Vector<StringName> systems;
	systems.push_back(StringName("test_sub_pipeline_execute"));
	systems.push_back(StringName("test_system_transform_add_x"));
	systems.push_back(StringName("TestSubPip_System.gd"));
	systems.push_back(StringName("TestSubPip_SubSystem.gd"));

	Pipeline main_pipeline;
	PipelineBuilder::build_pipeline(system_bundles, systems, &main_pipeline);

	World world;
	Token token = main_pipeline.prepare_world(&world);
	main_pipeline.set_active(token, true);

	// ~~ Create world ~~
	world.create_databag<TestSystemSubPipeDatabag>();
	world.get_databag<TestSystemSubPipeDatabag>()->exe_count = 2;

	EntityID entity_1 = world
								.create_entity()
								.with(TransformComponent());

	// ~~ Test ~~
	{
		main_pipeline.dispatch(token);
		main_pipeline.dispatch(token);

		// The subsystem is dispatched 2 times per main pipeline dispatch,
		// since we are dispatching the main pipeline 2 times the
		// `test_system_transform_add_x` add 100, four times.
		const TransformComponent *comp = world.get_storage<TransformComponent>()->get(entity_1);
		CHECK(ABS(comp->origin.x - 400.0) <= CMP_EPSILON);

		const TestSystemSubPipeDatabag *bag = world.get_databag<TestSystemSubPipeDatabag>();
		CHECK(bag->iterations == 2);
		CHECK(bag->sub_iterations == 4);
	}

	{
		// Now change the sub execution to 6
		world.get_databag<TestSystemSubPipeDatabag>()->exe_count = 6;

		// Dispatch the main pipeline
		main_pipeline.dispatch(token);

		// Verify the execution is properly done by making sure the value is 1000.
		const TransformComponent *comp = world.get_storage<TransformComponent>()->get(entity_1);
		CHECK(ABS(comp->origin.x - 1000.0) <= CMP_EPSILON);

		const TestSystemSubPipeDatabag *bag = world.get_databag<TestSystemSubPipeDatabag>();
		CHECK(bag->iterations == 3);
		CHECK(bag->sub_iterations == 10);
	}

	finalize_script_ecs();
}

TEST_CASE("[Modules][ECS] Test system and databag") {
	ECS::register_databag<TestSystem1Databag>();

	World world;

	world.create_databag<TestSystem1Databag>();

	// Test with databag
	{
		godex::system_id system_id = ECS::register_system(test_system_with_databag, "test_system_with_databag").get_id();

		// Confirm the databag is initialized.
		CHECK(world.get_databag<TestSystem1Databag>()->a == 10);

		PipelineBuilder pipeline_builder;
		// Add the system to the pipeline.
		pipeline_builder.add_system(system_id);

		// Create the pipeline.
		Pipeline pipeline;
		pipeline_builder.build(pipeline);
		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		// Dispatch
		for (uint32_t i = 0; i < 3; i += 1) {
			pipeline.dispatch(token);
		}

		CHECK(world.get_databag<TestSystem1Databag>()->a == 40);
	}

	// Test without databag
	{
		godex::system_id system_id = ECS::register_system(test_system_check_databag, "test_system_check_databag").get_id();
		world.remove_databag<TestSystem1Databag>();

		// Confirm the databag doesn't exists.
		CHECK(world.get_databag<TestSystem1Databag>() == nullptr);

		PipelineBuilder pipeline_builder;
		pipeline_builder.add_system(system_id);

		// Create the pipeline.
		Pipeline pipeline;
		pipeline_builder.build(pipeline);

		// This make sure to create the `Databag`
		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		// Dispatch
		for (uint32_t i = 0; i < 3; i += 1) {
			pipeline.dispatch(token);
		}

		// Make sure the databag is not nullptr.
		CHECK(world.get_databag<TestSystem1Databag>() != nullptr);
	}
}

TEST_CASE("[Modules][ECS] Test system databag fetch with dynamic query.") {
	initialize_script_ecs();

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_databag(ECS.TestSystemSubPipeDatabag, MUTABLE)\n";
		code += "\n";
		code += "func _execute(test_databag):\n";
		code += "	assert(test_databag.is_valid())\n";
		code += "	assert(test_databag.is_mutable())\n";
		code += "	test_databag.exe_count = 10\n";
		code += "\n";

		CHECK(register_ecs_script("TestDatabagDynamicSystem.gd", code));
	}

	build_scripts();
	flush_ecs_script_preparation();

	World world;
	world.create_databag<TestSystemSubPipeDatabag>();
	world.get_databag<TestSystemSubPipeDatabag>()->exe_count = 20;

	world
			.create_entity()
			.with(TransformComponent());

	PipelineBuilder pipeline_builder;
	// Add the system to the pipeline.
	pipeline_builder.add_system(ECS::get_system_id("TestDatabagDynamicSystem.gd"));

	Pipeline pipeline;
	// Create the pipeline.
	pipeline_builder.build(pipeline);
	Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);

	// Dispatch 1 time.
	pipeline.dispatch(token);

	// Make sure the `exe_count` is changed to 10 by the script.
	CHECK(world.get_databag<TestSystemSubPipeDatabag>()->exe_count == 10);

	finalize_script_ecs();
}

void test_move_root(Query<EntityID, TransformComponent> &p_query) {
	for (auto [entity, transform] : p_query) {
		if (entity == EntityID(0)) {
			transform->origin.x += 1.0;
			return;
		}
	}
}

void test_move_1_global(Query<EntityID, TransformComponent> &p_query) {
	for (auto [entity, transform] : p_query.space(Space::GLOBAL)) {
		if (entity == EntityID(1)) {
			transform->origin.x = 6.0;
			return;
		}
	}
}

void test_make_entity_1_root(Storage<Child> *p_hierarchy) {
	p_hierarchy->insert(1, Child());
}

TEST_CASE("[Modules][ECS] Test event mechanism.") {
	godex::system_id generate_event_system_id = ECS::register_system(test_system_generate_batch, "test_system_generate_batch").get_id();
	godex::system_id check_event_system_id = ECS::register_system(test_system_check_events, "test_system_check_events").get_id();
	ECS::register_component<BatchableComponent1>();

	World world;

	EntityID entity = world
							  .create_entity()
							  .with(TransformComponent());

	PipelineBuilder pipeline_builder;
	// Dispatch the pipeline.
	pipeline_builder.add_system(generate_event_system_id);
	pipeline_builder.add_system(check_event_system_id);

	Pipeline pipeline;
	pipeline_builder.build(pipeline);
	Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);

	// Make sure no component event is left at the end of each cycle.
	for (uint32_t i = 0; i < 5; i += 1) {
		pipeline.dispatch(token);

		// Make sure we don't add more than 2 components.
		CHECK(world.get_storage<BatchableComponent1>()->get_batch_size(entity) == 2);
	}
}

TEST_CASE("[Modules][ECS] Test create and remove Entity from Systems.") {
	godex::system_id add_entity_system_id = ECS::register_system(test_add_entity_system, "test_add_entity_system").get_id();
	godex::system_id remove_entity_system_id = ECS::register_system(test_remove_entity_system, "test_remove_entity_system").get_id();

	World world;

	PipelineBuilder pipeline_builder;
	// Create the pipeline.
	// Add the system to the pipeline.
	pipeline_builder.add_system(add_entity_system_id);
	pipeline_builder.add_system(remove_entity_system_id);

	Pipeline pipeline;
	pipeline_builder.build(pipeline);
	Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);

	for (uint32_t i = 0; i < 5; i += 1) {
		pipeline.dispatch(token);

		{
			// Count the `Entities` at this point.
			Query<const TransformComponent> query(&world);
			query.initiate_process(&world);

			// The `System` removes the `Entities` defferred, so at this point
			// the `Entities` still exists.
			const uint32_t count = query.count();
			CHECK(count == 3);
		}

		world.flush();

		{
			// Count the `Entities` at this point.
			Query<const TransformComponent> query(&world);
			query.initiate_process(&world);

			// Now the `Entities` are removed.
			const uint32_t count = query.count();
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
								.with(TransformComponent(Transform3D(Basis(), Vector3(1, 0, 0))));

	EntityID entity_1 = world
								.create_entity()
								.with(Child(entity_0))
								.with(TransformComponent(Transform3D(Basis(), Vector3(1, 0, 0))));

	EntityID entity_2 = world
								.create_entity()
								.with(Child(entity_1))
								.with(TransformComponent(Transform3D(Basis(), Vector3(1, 0, 0))));

	// Try move `Entity_0` using `LOCAL`.
	{
		godex::system_id system_id = ECS::register_system(test_move_root, "test_move_root").get_id();

		PipelineBuilder pipeline_builder;
		// Create the pipeline.
		pipeline_builder.add_system(system_id);
		Pipeline pipeline;
		pipeline_builder.build(pipeline);
		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		// Check local transform.
		{
			const TransformComponent *entity_2_transform = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform->origin.x - 1.0) <= CMP_EPSILON);
		}

		// Check global transform
		{
			const TransformComponent *entity_2_transform = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform->origin.x - 3.0) <= CMP_EPSILON);
		}

		pipeline.dispatch(token);

		// Check local transform after root motion.
		{
			const TransformComponent *entity_2_transform = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform->origin.x - 1.0) <= CMP_EPSILON);
		}

		// Check global transform after root motion.
		{
			const TransformComponent *entity_2_transform = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform->origin.x - 4.0) <= CMP_EPSILON);
		}
	}

	// Now move `Entity_1` but using `GLOBAL`.
	{
		godex::system_id system_id = ECS::register_system(test_move_1_global, "test_move_1_global").get_id();

		PipelineBuilder pipeline_builder;
		// Create the pipeline.
		pipeline_builder.add_system(system_id);
		Pipeline pipeline;
		pipeline_builder.build(pipeline);
		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		// Hierarchy is:
		// Entity 0           2 Local | 2 Global
		//  |- Entity 1       1 Local | 3 Global
		//  |   |- Entity 2   1 Local | 4 Global

		// Check local transform before motion.
		{
			const TransformComponent *entity_1_transform = world.get_storage<TransformComponent>()->get(entity_1);
			CHECK(ABS(entity_1_transform->origin.x - 1.0) <= CMP_EPSILON);
		}

		// Dispatch the pipeline, so to move the `Entity_1` globally.
		pipeline.dispatch(token);

		// Hierarchy is:
		// Entity 0           2 Local | 2 Global
		//  |- Entity 1       4 Local | 6 Global
		//  |   |- Entity 2   1 Local | 7 Global

		// `Entity 0` din't move.
		{
			const TransformComponent *entity_0_transform = world.get_storage<TransformComponent>()->get(entity_0);
			CHECK(ABS(entity_0_transform->origin.x - 2.0) <= CMP_EPSILON);
		}

		// `Entity 1` moved globally to 6, so check local and global position:
		{
			const TransformComponent *entity_1_transform_l = world.get_storage<TransformComponent>()->get(entity_1);
			CHECK(ABS(entity_1_transform_l->origin.x - 4.0) <= CMP_EPSILON);

			const TransformComponent *entity_1_transform_g = world.get_storage<TransformComponent>()->get(entity_1, Space::GLOBAL);
			CHECK(ABS(entity_1_transform_g->origin.x - 6.0) <= CMP_EPSILON);
		}

		// `Entity 2` moved cause of the parent motion.
		{
			const TransformComponent *entity_2_transform_l = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform_l->origin.x - 1.0) <= CMP_EPSILON);

			const TransformComponent *entity_2_transform_g = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform_g->origin.x - 7.0) <= CMP_EPSILON);
		}
	}

	// Now change hierarchy
	{
		godex::system_id system_id = ECS::register_system(test_make_entity_1_root, "test_make_entity_1_root").get_id();

		PipelineBuilder pipeline_builder;
		// Create the pipeline.
		pipeline_builder.add_system(system_id);
		Pipeline pipeline;
		pipeline_builder.build(pipeline);
		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		// Dispatch the pipeline, so to move the `Entity_1` globally.
		pipeline.dispatch(token);

		// Hierarchy is:
		// Entity 1       4 Local | 4 Global
		//  |- Entity 2   1 Local | 5 Global

		CHECK(world.get_storage<Child>()->has(0) == false); // This doesn't exist anymore.
		CHECK(world.get_storage<Child>()->has(1)); // This is Root
		CHECK(world.get_storage<Child>()->has(2)); // This is child of `Entity 1`

		// `Entity 0` din't move.
		{
			const TransformComponent *entity_0_transform = world.get_storage<TransformComponent>()->get(entity_0);
			CHECK(ABS(entity_0_transform->origin.x - 2.0) <= CMP_EPSILON);
		}

		// `Entity 1` it's not root, and it's relative to itself: local and
		// global are equals.
		{
			const TransformComponent *entity_1_transform_l = world.get_storage<TransformComponent>()->get(entity_1);
			CHECK(ABS(entity_1_transform_l->origin.x - 4.0) <= CMP_EPSILON);

			const TransformComponent *entity_1_transform_g = world.get_storage<TransformComponent>()->get(entity_1, Space::GLOBAL);
			CHECK(ABS(entity_1_transform_g->origin.x - 4.0) <= CMP_EPSILON);
		}

		// `Entity 2` moved cause of the parent hierarchy change.
		{
			const TransformComponent *entity_2_transform_l = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform_l->origin.x - 1.0) <= CMP_EPSILON);

			const TransformComponent *entity_2_transform_g = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform_g->origin.x - 5.0) <= CMP_EPSILON);
		}
	}

	// Try move `Entity_0` using `LOCAL`, and make sure now one else move.
	{
		godex::system_id system_id = ECS::get_system_id("test_move_root"); // System already registered.

		PipelineBuilder pipeline_builder;
		// Create the pipeline.
		pipeline_builder.add_system(system_id);
		Pipeline pipeline;
		pipeline_builder.build(pipeline);
		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		// Check `Entity 0` transform.
		{
			const TransformComponent *entity_0_transform_l = world.get_storage<TransformComponent>()->get(entity_0);
			CHECK(ABS(entity_0_transform_l->origin.x - 2.0) <= CMP_EPSILON);

			const TransformComponent *entity_0_transform_g = world.get_storage<TransformComponent>()->get(entity_0, Space::GLOBAL);
			CHECK(ABS(entity_0_transform_g->origin.x - 2.0) <= CMP_EPSILON);
		}

		// Check `Entity 1` transform.
		{
			const TransformComponent *entity_1_transform = world.get_storage<TransformComponent>()->get(entity_1, Space::GLOBAL);
			CHECK(ABS(entity_1_transform->origin.x - 4.0) <= CMP_EPSILON);
		}

		pipeline.dispatch(token);

		// Hierarchy is:
		// Entity 1       4 Local | 4 Global
		//  |- Entity 2   1 Local | 5 Global

		// Make sure `Entity 0` moved.
		{
			const TransformComponent *entity_0_transform_l = world.get_storage<TransformComponent>()->get(entity_0);
			CHECK(ABS(entity_0_transform_l->origin.x - 3.0) <= CMP_EPSILON);

			const TransformComponent *entity_0_transform_g = world.get_storage<TransformComponent>()->get(entity_0, Space::GLOBAL);
			CHECK(ABS(entity_0_transform_g->origin.x - 3.0) <= CMP_EPSILON);
		}

		// Make sure `Entity 1` din't move.
		{
			const TransformComponent *entity_1_transform = world.get_storage<TransformComponent>()->get(entity_1, Space::GLOBAL);
			CHECK(ABS(entity_1_transform->origin.x - 4.0) <= CMP_EPSILON);
		}
	}

	// Try move `Entity_2` using a GDScript system.
	{
		initialize_script_ecs();
		{
			// Create the script.
			String code;
			code += "extends System\n";
			code += "\n";
			code += "func _prepare():\n";
			code += "	var query = DynamicQuery.new()\n";
			code += "	query.set_space(ECS.GLOBAL)\n";
			code += "	query.with_component(ECS.TransformComponent, MUTABLE)\n";
			code += "	with_query(query)\n";
			code += "\n";
			code += "func _execute(query):\n";
			code += "	while query.next():\n";
			code += "		if query.get_current_entity_id() == 2:\n";
			code += "			query[\"TransformComponent\"].origin.x = 10.0\n";
			code += "\n";

			CHECK(register_ecs_script("TestMoveHierarchySystem.gd", code));
		}

		build_scripts();
		flush_ecs_script_preparation();

		PipelineBuilder pipeline_builder;
		// Create the pipeline.
		pipeline_builder.add_system(ECS::get_system_id("TestMoveHierarchySystem.gd"));
		Pipeline pipeline;
		pipeline_builder.build(pipeline);
		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		// Hierarchy is:
		// Entity 1       4 Local | 4 Global
		//  |- Entity 2   1 Local | 5 Global

		// Make sure `Entity 2` initial position.
		{
			const TransformComponent *entity_2_transform_l = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform_l->origin.x - 1.0) <= CMP_EPSILON);

			const TransformComponent *entity_2_transform_g = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform_g->origin.x - 5.0) <= CMP_EPSILON);
		}

		pipeline.dispatch(token);

		// Hierarchy is:
		// Entity 1       4 Local | 4 Global
		//  |- Entity 2   6 Local | 10 Global

		// Make sure `Entity 1` didn't move.
		{
			const TransformComponent *entity_1_transform_l = world.get_storage<TransformComponent>()->get(entity_1);
			CHECK(ABS(entity_1_transform_l->origin.x - 4.0) <= CMP_EPSILON);
		}

		// Make sure `Entity 2` moved.
		{
			const TransformComponent *entity_2_transform_l = world.get_storage<TransformComponent>()->get(entity_2);
			CHECK(ABS(entity_2_transform_l->origin.x - 6.0) <= CMP_EPSILON);

			const TransformComponent *entity_2_transform_g = world.get_storage<TransformComponent>()->get(entity_2, Space::GLOBAL);
			CHECK(ABS(entity_2_transform_g->origin.x - 10.0) <= CMP_EPSILON);
		}
		finalize_script_ecs();
	}
}

TEST_CASE("[Modules][ECS] Test Add/remove from dynamic system.") {
	initialize_script_ecs();
	ECS::register_component<Test1Component>();

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_databag(ECS.WorldCommands, MUTABLE)\n";
		code += "	with_storage(ECS.Test1Component)\n";
		code += "\n";
		code += "func _execute(world_commands, comp_storage):\n";
		code += "	var entity_2 = world_commands.create_entity()\n";
		code += "	comp_storage.insert(entity_2, {\"a\": 975})\n";
		code += "	var entity_3 = world_commands.create_entity()\n";
		code += "	comp_storage.insert(entity_3)\n";
		code += "\n";

		CHECK(register_ecs_script("TestSpawnDynamicSystem.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_databag(ECS.WorldCommands, MUTABLE)\n";
		code += "	with_storage(ECS.Test1Component)\n";
		code += "\n";
		code += "func _execute(world_commands, comp_storage):\n";
		code += "	comp_storage.remove(2)\n";
		code += "	comp_storage.remove(1)\n";
		code += "\n";

		CHECK(register_ecs_script("TestRemoveDynamicSystem.gd", code));
	}

	build_scripts();
	flush_ecs_script_preparation();

	World world;

	const EntityID entity_1 = world
									  .create_entity()
									  .with(TransformComponent());

	// This entity got created by the script.
	const EntityID entity_2(1);
	const EntityID entity_3(2);

	// Test add
	{
		PipelineBuilder pipeline_builder;
		// Add the system to the pipeline.
		pipeline_builder.add_system(ECS::get_system_id("TestSpawnDynamicSystem.gd"));
		// Create the pipeline.
		Pipeline pipeline;
		pipeline_builder.build(pipeline);
		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		// Dispatch 1 time.
		pipeline.dispatch(token);

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
		PipelineBuilder pipeline_builder;
		// Add the system to the pipeline.
		pipeline_builder.add_system(ECS::get_system_id("TestRemoveDynamicSystem.gd"));

		// Create the pipeline.
		Pipeline pipeline;
		pipeline_builder.build(pipeline);
		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		// Dispatch 1 time.
		pipeline.dispatch(token);

		// Make sure the Test1Component is correctly removed
		CHECK(world.get_storage<Test1Component>()->has(entity_2) == false);
		CHECK(world.get_storage<Test1Component>()->has(entity_3) == false);
	}

	finalize_script_ecs();
}

TEST_CASE("[Modules][ECS] Test fetch changed from dynamic system.") {
	initialize_script_ecs();
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	var query = DynamicQuery.new()\n";
		code += "	query.changed_component(ECS.TransformComponent, MUTABLE)\n";
		code += "	with_query(query)";
		code += "\n";
		code += "func _execute(query):\n";
		code += "	while query.next():\n";
		code += "		query[0].transform.origin.x = 100.0\n";
		code += "\n";

		CHECK(register_ecs_script("TestChangedDynamicSystem.gd", code));
	}

	build_scripts();
	flush_ecs_script_preparation();

	World world;

	PipelineBuilder pipeline_builder;
	// Add the system to the pipeline.
	pipeline_builder.add_system(ECS::get_system_id("TestChangedDynamicSystem.gd"));

	// Create the pipeline.
	Pipeline pipeline;
	pipeline_builder.build(pipeline);
	Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);

	// Insert here, so the change is triggered.
	const EntityID entity_1 = world
									  .create_entity()
									  .with(TransformComponent());

	// Dispatch 1 time.
	pipeline.dispatch(token);

	CHECK(ABS(world.get_storage<TransformComponent>()->get(entity_1)->origin.x - 100.0) <= CMP_EPSILON);

	finalize_script_ecs();
}
} // namespace godex_tests_system

struct ChangeTracer {
	COMPONENT(ChangeTracer, DenseVectorStorage)
	static void _bind_methods() {}

	int trace = 0;
};

void test_changed(Query<Changed<const TransformComponent>, ChangeTracer> &p_query) {
	for (auto [t, c] : p_query) {
		c->trace += 1;
	}
}

namespace godex_tests_system {
TEST_CASE("[Modules][ECS] Test system changed query filter.") {
	ECS::register_component<ChangeTracer>();
	godex::system_id system_id = ECS::register_system(test_changed, "test_changed").get_id();

	World world;

	PipelineBuilder pipeline_builder;
	pipeline_builder.add_system(system_id);

	Pipeline pipeline;
	pipeline_builder.build(pipeline);
	Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);

	// Insert at this point, so we the `Changed` event is captured.
	EntityID entity_1 = world
								.create_entity()
								.with(ChangeTracer())
								.with(TransformComponent());

	EntityID entity_2 = world
								.create_entity()
								.with(ChangeTracer())
								.with(TransformComponent());

	EntityID entity_3 = world
								.create_entity()
								.with(ChangeTracer())
								.with(TransformComponent());

	for (uint32_t i = 0; i < 3; i += 1) {
		// Leave entity_1 untouched.
		{}

		// Trigger the entity_2 change.
		{
			Storage<TransformComponent> *storage = world.get_storage<TransformComponent>();
			storage->get(entity_2);
		}

		// Touch the entity_3 immutable, this doesn't trigger the change.
		{
			const Storage<const TransformComponent> *storage = world.get_storage<const TransformComponent>();
			storage->get(entity_3);
		}

		pipeline.dispatch(token);
	}

	const Storage<const ChangeTracer> *storage = world.get_storage<const ChangeTracer>();

	const ChangeTracer *entity_1_tracer = storage->get(entity_1);
	const ChangeTracer *entity_2_tracer = storage->get(entity_2);
	const ChangeTracer *entity_3_tracer = storage->get(entity_3);

	// Only one trace because it's trigger by the initial insert.
	CHECK(entity_1_tracer->trace == 1);
	// Triggered each frame, because taken mutably.
	CHECK(entity_2_tracer->trace == 3);
	// Taken immutably, so never changed.
	CHECK(entity_3_tracer->trace == 1);
}

TEST_CASE("[Modules][ECS] Test fetch entity from nodepath, using a dynamic system.") {
	initialize_script_ecs();
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_databag(ECS.World, IMMUTABLE)\n";
		code += "	var query = DynamicQuery.new()\n";
		code += "	query.with_component(ECS.Test1Component, MUTABLE)\n";
		code += "	with_query(query)\n";
		code += "\n";
		code += "func _execute(world, query):\n";
		code += "	var entity_1 = world.get_entity_from_path(\"/root/node_1\")\n";
		code += "	if query.has(entity_1):\n";
		code += "		query.fetch(entity_1)\n";
		code += "		query[0].a = 1000\n";
		code += "\n";

		CHECK(register_ecs_script("TestFetchEntityFromNodePath.gd", code));
	}

	build_scripts();
	flush_ecs_script_preparation();

	World world;

	const EntityID entity_1 = world
									  .create_entity()
									  .with(Test1Component());

	world.assign_nodepath_to_entity(entity_1, NodePath("/root/node_1"));

	// Test add
	{
		PipelineBuilder pipeline_builder;

		// Add the system to the pipeline.
		pipeline_builder.add_system(ECS::get_system_id("TestFetchEntityFromNodePath.gd"));

		// Create the pipeline.
		Pipeline pipeline;
		pipeline_builder.build(pipeline);

		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		// Dispatch 1 time.
		pipeline.dispatch(token);

		// Make sure the entity 0 has the `TransformComponent`
		CHECK(world.get_storage<Test1Component>()->has(entity_1));

		// Make sure the value is correctly changed.
		CHECK(world.get_storage<Test1Component>()->get(entity_1)->a == 1000);
	}

	finalize_script_ecs();
}
} // namespace godex_tests_system

struct MyEvent1Test {
	EVENT(MyEvent1Test)

	static void _bind_methods() {
		ECS_BIND_PROPERTY(MyEvent1Test, PropertyInfo(Variant::INT, "a"), a);
	}

	int a = 0;
};

void test_emit_event(EventsEmitter<MyEvent1Test> &p_emitter) {
	MyEvent1Test e;
	e.a = 10;
	p_emitter.emit("Test1", e);
}

void test_fetch_event(EventsReceiver<MyEvent1Test, EMITTER(Test1)> &p_events) {
	int c = 0;
	for (const MyEvent1Test *e : p_events) {
		c += 1;
		CHECK(e->a == 10);
	}
	CHECK(c == 1);
}

namespace godex_tests {
TEST_CASE("[Modules][ECS] Test `Events` class is able to fetch the emitter name.") {
	ECS::register_event<MyEvent1Test>();
	{
		// Make sure the Events is correctly reporting the EmitterName set at compile
		// time.
		EventsReceiver<MyEvent1Test, EMITTER(Test11)> events;
		CHECK(events.get_emitter_name() == String("Test11"));
	}
}

TEST_CASE("[Modules][ECS] Make sure the events storages are automatically created.") {
	ECS::register_system(test_emit_event, "test_emit_event");
	ECS::register_system(test_fetch_event, "test_fetch_event")
			.after("test_emit_event");

	// Make sure the world creates the event storage when the `EventEmitter` is encountered.
	{
		Pipeline pipeline;
		{
			Vector<StringName> system_bundles;

			Vector<StringName> systems;
			systems.push_back("test_emit_event");

			PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);
		}

		World world;
		pipeline.prepare_world(&world);

		EventStorage<MyEvent1Test> *storage = world.get_events_storage<MyEvent1Test>();
		// Make sure the storage is been created at this point, since `prepare` does it.
		CHECK(storage != nullptr);
	}

	// Make sure the world creates the event storage when the `Events` is encountered.
	{
		Pipeline pipeline;
		{
			Vector<StringName> system_bundles;

			Vector<StringName> systems;
			systems.push_back("test_fetch_event");

			PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);
		}

		World world;
		pipeline.prepare_world(&world);

		EventStorage<MyEvent1Test> *storage = world.get_events_storage<MyEvent1Test>();
		// Make sure the storage is been created at this point, since `prepare` does it.
		CHECK(storage != nullptr);
		// Make sure the emitter has been created.
		CHECK(storage->has_emitter("Test1"));
	}

	// Now test using GDScript
	initialize_script_ecs();
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_events_emitter(ECS.MyEvent1Test)\n";
		code += "\n";
		code += "func _execute(event_emitter):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(register_ecs_script("TestAEventEmitterSystem.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_events_receiver(ECS.MyEvent1Test, \"EmitterTest\")\n";
		code += "\n";
		code += "func _execute(event_receiver):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(register_ecs_script("TestAEventReceiverSystem.gd", code));
	}

	build_scripts();
	flush_ecs_script_preparation();

	{
		Pipeline pipeline;
		{
			Vector<StringName> system_bundles;

			Vector<StringName> systems;
			systems.push_back("TestAEventEmitterSystem.gd");

			PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);
		}

		World world;
		pipeline.prepare_world(&world);

		EventStorage<MyEvent1Test> *storage = world.get_events_storage<MyEvent1Test>();
		// Make sure the storage is been created at this point, since `prepare` does it.
		CHECK(storage != nullptr);
	}
	{
		Pipeline pipeline;
		{
			Vector<StringName> system_bundles;

			Vector<StringName> systems;
			systems.push_back("TestAEventReceiverSystem.gd");

			PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);
		}

		World world;
		pipeline.prepare_world(&world);

		EventStorage<MyEvent1Test> *storage = world.get_events_storage<MyEvent1Test>();
		// Make sure the storage is been created at this point, since `prepare` does it.
		CHECK(storage != nullptr);
		// Make sure the emitter has been created.
		CHECK(storage->has_emitter("EmitterTest"));
	}
	finalize_script_ecs();
}

TEST_CASE("[Modules][ECS] Test EventEmitter and EventReceiver") {
	{
		Pipeline pipeline;
		{
			Vector<StringName> system_bundles;

			Vector<StringName> systems;
			systems.push_back("test_emit_event");
			systems.push_back("test_fetch_event");

			PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);
		}

		World world;
		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		const EventStorage<MyEvent1Test> *storage = world.get_events_storage<MyEvent1Test>();

		pipeline.dispatch(token);
		CHECK(storage->get_events("Test1")->size() == 1);

		pipeline.dispatch(token);
		CHECK(storage->get_events("Test1")->size() == 1);

		pipeline.dispatch(token);
		CHECK(storage->get_events("Test1")->size() == 1);
	}

	// Now test using GDScript
	initialize_script_ecs();
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_events_emitter(ECS.MyEvent1Test)\n";
		code += "\n";
		code += "func _execute(event_emitter):\n";
		code += "	event_emitter.emit(\"Test1\", {'a': 10})\n";
		code += "\n";

		CHECK(register_ecs_script("TestBEventEmitterSystem.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_after(ECS.TestBEventEmitterSystem_gd)\n";
		code += "	with_events_receiver(ECS.MyEvent1Test, \"Test1\")\n";
		code += "\n";
		code += "func _execute(event_receiver):\n";
		code += "	var count: int = 0\n";
		code += "	for event in event_receiver.fetch():\n";
		code += "		assert(event.a == 10)\n";
		code += "		count += 1\n";
		code += "	assert(count == 1)\n";
		code += "\n";

		CHECK(register_ecs_script("TestBEventReceiverSystem.gd", code));
	}

	build_scripts();
	flush_ecs_script_preparation();

	{
		Pipeline pipeline;
		{
			Vector<StringName> system_bundles;

			Vector<StringName> systems;
			systems.push_back("TestBEventEmitterSystem.gd");
			systems.push_back("TestBEventReceiverSystem.gd");

			PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);
		}

		World world;
		Token token = pipeline.prepare_world(&world);
		pipeline.set_active(token, true);

		const EventStorage<MyEvent1Test> *storage = world.get_events_storage<MyEvent1Test>();

		pipeline.dispatch(token);
		CHECK(storage->get_events("Test1")->size() == 1);

		pipeline.dispatch(token);
		CHECK(storage->get_events("Test1")->size() == 1);

		pipeline.dispatch(token);
		CHECK(storage->get_events("Test1")->size() == 1);
	}
	finalize_script_ecs();
}
} // namespace godex_tests

struct MyEvent2Test {
	EVENT(MyEvent2Test)
};

void test2_emit_event(EventsEmitter<MyEvent2Test> &p_emitter) {}
void test2_fetch1_event(EventsReceiver<MyEvent2Test, EMITTER(Test1)> &p_events) {}
void test2_fetch2_event(EventsReceiver<MyEvent2Test, EMITTER(Test1)> &p_events) {}
void test2_fetch3_event(EventsReceiver<MyEvent2Test, EMITTER(Test2)> &p_events) {}

void test_fetch_databag_mut(TestSystem1Databag *) {}
void test_fetch_databag_const(TestSystem1Databag *) {}

namespace godex_tests {
TEST_CASE("[Modules][ECS] Make sure the function `can_systems_run_in_parallel` works as expected.") {
	ECS::register_system(test2_emit_event, "test2_emit_event");
	ECS::register_system(test2_fetch1_event, "test2_fetch1_event");
	ECS::register_system(test2_fetch2_event, "test2_fetch2_event");
	ECS::register_system(test2_fetch3_event, "test2_fetch3_event");
	ECS::register_system(test_fetch_databag_mut, "test_fetch_databag_mut");
	ECS::register_system(test_fetch_databag_const, "test_fetch_databag_const");
	ECS::register_event<MyEvent2Test>();

	// Make sure that two systems that emits two different events can run in parallel.
	CHECK(ECS::can_systems_run_in_parallel(ECS::get_system_id("test2_emit_event"), ECS::get_system_id("test_emit_event")));

	// Make sure the system that emit the event can't run in parallel with the
	// systems that fetch it.
	CHECK(!ECS::can_systems_run_in_parallel(ECS::get_system_id("test_emit_event"), ECS::get_system_id("test_fetch_event")));

	// Check the above on the other set of systems.
	CHECK(!ECS::can_systems_run_in_parallel(ECS::get_system_id("test2_emit_event"), ECS::get_system_id("test2_fetch1_event")));
	CHECK(!ECS::can_systems_run_in_parallel(ECS::get_system_id("test2_emit_event"), ECS::get_system_id("test2_fetch2_event")));
	CHECK(!ECS::can_systems_run_in_parallel(ECS::get_system_id("test2_emit_event"), ECS::get_system_id("test2_fetch3_event")));

	// Make sure the fetch events can run in parallel no matter what.
	CHECK(ECS::can_systems_run_in_parallel(ECS::get_system_id("test2_fetch1_event"), ECS::get_system_id("test2_fetch2_event")));
	CHECK(ECS::can_systems_run_in_parallel(ECS::get_system_id("test2_fetch2_event"), ECS::get_system_id("test2_fetch3_event")));
	CHECK(ECS::can_systems_run_in_parallel(ECS::get_system_id("test2_fetch3_event"), ECS::get_system_id("test_fetch_event")));

	// Make sure the systems fetching the databags can't run in parallel.
	CHECK(!ECS::can_systems_run_in_parallel(ECS::get_system_id("test_fetch_databag_mut"), ECS::get_system_id("test_fetch_databag_const")));

	// But can run with the events one.
	CHECK(ECS::can_systems_run_in_parallel(ECS::get_system_id("test_fetch_databag_mut"), ECS::get_system_id("test2_emit_event")));
	CHECK(ECS::can_systems_run_in_parallel(ECS::get_system_id("test_fetch_databag_mut"), ECS::get_system_id("test2_fetch3_event")));
	CHECK(ECS::can_systems_run_in_parallel(ECS::get_system_id("test_fetch_databag_const"), ECS::get_system_id("test2_emit_event")));
	CHECK(ECS::can_systems_run_in_parallel(ECS::get_system_id("test_fetch_databag_const"), ECS::get_system_id("test2_fetch3_event")));

	// TODO add other checks, like query, etc??
}
} // namespace godex_tests

#endif // TEST_ECS_SYSTEM_H
