#ifndef TEST_ECS_SYSTEM_H
#define TEST_ECS_SYSTEM_H

#include "tests/test_macros.h"

#include "../components/dynamic_component.h"
#include "../components/transform_component.h"
#include "../ecs.h"
#include "../pipeline/pipeline.h"
#include "../systems/dynamic_system.h"
#include "../world/world.h"
#include "test_utilities.h"

class TagTestComponent : public godex::Component {
	COMPONENT(TagTestComponent, DenseVector)
};

class TestSystem1Resource : public godex::Resource {
	RESOURCE(TestSystem1Resource)

public:
	int a = 10;
};

class TestSystemSubPipeResource : public godex::Resource {
	RESOURCE(TestSystemSubPipeResource)

public:
	int exe_count = 0;
};

namespace godex_tests_system {

void test_system_tag(Query<TransformComponent, const TagTestComponent> &p_query) {
	while (p_query.is_done() == false) {
		auto [transform, component] = p_query.get();
		transform.transform.origin.x += 100.0;
		p_query.next_entity();
	}
}

void test_system_with_resource(TestSystem1Resource *test_res, Query<TransformComponent, const TagTestComponent> &p_query) {
	test_res->a += 10;
}

void test_system_with_null_resource(TestSystem1Resource *test_res) {
	CRASH_COND(test_res != nullptr);
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

	for (uint32_t i = 0; i < 3; i += 1) {
		pipeline.dispatch(&world);
	}

	const TypedStorage<const TransformComponent> *storage = world.get_storage<const TransformComponent>();

	const Vector3 entity_1_origin = storage->get(entity_1).get_transform().origin;
	const Vector3 entity_2_origin = storage->get(entity_2).get_transform().origin;
	const Vector3 entity_3_origin = storage->get(entity_3).get_transform().origin;

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
		code += "	transform_com.transform.origin.x += 100.0\n";
		code += "	test_comp.variable_1 += 1\n";
		code += "	test_comp.variable_2 = Transform()\n";
		code += "\n";

		ERR_FAIL_COND(build_and_assign_script(&target_obj, code) == false);
	}

	// Build dynami component.
	godex::DynamicSystemInfo dynamic_system_info;
	dynamic_system_info.with_component(TransformComponent::get_component_id(), true);
	dynamic_system_info.with_component(test_dyn_component_id, true);
	dynamic_system_info.set_target(&target_obj);
	uint32_t system_id = ECS::register_dynamic_system("TestDynamicSystem.gd", &dynamic_system_info);

	// Create the pipeline.
	Pipeline pipeline;
	// Add the system to the pipeline.
	pipeline.add_registered_system(system_id);
	pipeline.build();

	// Dispatch
	for (uint32_t i = 0; i < 3; i += 1) {
		pipeline.dispatch(&world);
	}

	// Validate the C++ component.
	{
		const TypedStorage<const TransformComponent> *storage = world.get_storage<const TransformComponent>();

		const Vector3 entity_1_origin = storage->get(entity_1).get_transform().origin;
		const Vector3 entity_2_origin = storage->get(entity_2).get_transform().origin;
		const Vector3 entity_3_origin = storage->get(entity_3).get_transform().origin;

		// This entity is expected to change.
		CHECK(ABS(entity_1_origin.x - 300.0) <= CMP_EPSILON);

		// This entity doesn't have a `TagTestComponent` so the systems should not
		// change it.
		CHECK(entity_2_origin.x <= CMP_EPSILON);

		// This entity is expected to change.
		CHECK(ABS(entity_3_origin.x - 300.0) <= CMP_EPSILON);
	}

	// Validate the dynamic component.
	{
		const Storage *storage = world.get_storage(test_dyn_component_id);
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
	// Extract the info and forget about the resource, so the pipeline can
	// access it safely.
	{
		const TestSystemSubPipeResource *res = p_world->get_resource<TestSystemSubPipeResource>();
		exe_count = res->exe_count;
	}

	for (uint32_t i = 0; i < exe_count; i += 1) {
		p_pipeline->dispatch(p_world);
	}
}

void test_system_transform_add_x(Query<TransformComponent> &p_query) {
	while (p_query.is_done() == false) {
		auto [transform] = p_query.get();
		transform.transform.origin.x += 100.0;
		p_query.next_entity();
	}
}

TEST_CASE("[Modules][ECS] Test dynamic system with sub pipeline C++.") {
	ECS::register_resource<TestSystemSubPipeResource>();

	// ~~ Sub pipeline ~~
	Pipeline sub_pipeline;
	sub_pipeline.add_system(test_system_transform_add_x);
	sub_pipeline.build();

	godex::DynamicSystemInfo sub_pipeline_system;
	sub_pipeline_system.set_target(test_sub_pipeline_execute);
	// Used internally by the `test_sub_pipeline_execute`.
	sub_pipeline_system.with_resource(TestSystemSubPipeResource::get_resource_id(), false);

	const uint32_t sub_pipeline_system_id = ECS::register_dynamic_system("TestSubPipelineExecute", &sub_pipeline_system);
	ECS::set_dynamic_system_pipeline(sub_pipeline_system_id, &sub_pipeline);

	// ~~ Main pipeline ~~
	Pipeline main_pipeline;
	main_pipeline.add_registered_system(sub_pipeline_system_id);
	main_pipeline.build();

	// ~~ Create world ~~
	World world;
	world.add_resource<TestSystemSubPipeResource>();
	world.get_resource<TestSystemSubPipeResource>()->exe_count = 2;

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
		const TransformComponent &comp = world.get_storage<TransformComponent>()->get(entity_1);
		CHECK(ABS(comp.transform.origin.x - 400.0) <= CMP_EPSILON);
	}

	{
		// Now change the sub execution to 6
		world.get_resource<TestSystemSubPipeResource>()->exe_count = 6;

		// Dispatch the main pipeline
		main_pipeline.dispatch(&world);

		// Verify the execution is properly done by making sure the value is 1000.
		const TransformComponent &comp = world.get_storage<TransformComponent>()->get(entity_1);
		CHECK(ABS(comp.transform.origin.x - 1000.0) <= CMP_EPSILON);
	}
}

TEST_CASE("[Modules][ECS] Test system and resource") {
	ECS::register_resource<TestSystem1Resource>();

	World world;

	world.add_resource<TestSystem1Resource>();

	// Test with resource
	{
		// Confirm the resource is initialized.
		CHECK(world.get_resource<TestSystem1Resource>()->a == 10);

		// Create the pipeline.
		Pipeline pipeline;
		// Add the system to the pipeline.
		pipeline.add_system(test_system_with_resource);
		pipeline.build();

		// Dispatch
		for (uint32_t i = 0; i < 3; i += 1) {
			pipeline.dispatch(&world);
		}

		CHECK(world.get_resource<TestSystem1Resource>()->a == 40);
	}

	// Test without resource
	{
		world.remove_resource<TestSystem1Resource>();

		// Confirm the resource doesn't exists.
		CHECK(world.get_resource<TestSystem1Resource>() == nullptr);

		// Create the pipeline.
		Pipeline pipeline;
		// Add the system to the pipeline.
		pipeline.add_system(test_system_with_null_resource);
		pipeline.build();

		// Dispatch
		for (uint32_t i = 0; i < 3; i += 1) {
			pipeline.dispatch(&world);
		}

		// Make sure the resource is still nullptr.
		CHECK(world.get_resource<TestSystem1Resource>() == nullptr);
	}
}

TEST_CASE("[Modules][ECS] Test system and script resource") {
	// TODO
}

// TODO test resources with C++ and Scripts systems.

} // namespace godex_tests_system

#endif // TEST_ECS_SYSTEM_H
