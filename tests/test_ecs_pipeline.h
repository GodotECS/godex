#ifndef TEST_ECS_PIPELINE_H
#define TEST_ECS_PIPELINE_H

#include "tests/test_macros.h"

#include "../ecs.h"
#include "../modules/godot/components/transform_component.h"
#include "../pipeline/pipeline.h"
#include "../pipeline/pipeline_builder.h"
#include "../storage/dense_vector_storage.h"
#include "../systems/dynamic_system.h"

class PipelineTestDatabag1 : public godex::Databag {
	DATABAG(PipelineTestDatabag1)

public:
	int a = 10;
};

class PipelineTestDatabag2 : public godex::Databag {
	DATABAG(PipelineTestDatabag2)

public:
	int a = 10;
};

struct PipelineTestComponent1 {
	COMPONENT(PipelineTestComponent1, DenseVectorStorage)
	static void _bind_methods() {}
};

namespace godex_tests_pipeline {

void system_with_databag(PipelineTestDatabag1 *test_res) {
}

void system_with_immutable_databag(const PipelineTestDatabag2 *test_res) {
}

void system_with_component(Query<PipelineTestComponent1> &p_query) {
}

void system_with_immutable_component(Query<const TransformComponent> &p_query) {
}

void system_with_storage(Storage<TransformComponent> *p_storage) {
}

void sub_dispatcher_system_test(World *p_world, Pipeline *p_pipeline) {
}

TEST_CASE("[Modules][ECS] Test pipeline build.") {
	godex::system_id system_id = ECS::register_system(system_with_databag, "system_with_databag").get_id();
	ECS::register_databag<PipelineTestDatabag1>();
	ECS::register_databag<PipelineTestDatabag2>();
	ECS::register_component<PipelineTestComponent1>();

	PipelineBuilder pipeline_builder;
	pipeline_builder.add_system(system_id);

	Pipeline pipeline;

	// Make sure the pipeline is not yet ready.
	CHECK(pipeline.is_ready() == false);

	pipeline_builder.build(pipeline);

	// Make sure the pipeline is now ready.
	CHECK(pipeline.is_ready());

	pipeline.reset();

	// Make sure the pipeline is not ready again.
	CHECK(pipeline.is_ready() == false);
}

TEST_CASE("[Modules][ECS] Test pipeline get_systems_dependencies") {
	godex::system_id system_with_databag_system_id = ECS::get_system_id("system_with_databag"); // This sytem is already registered.
	godex::system_id system_with_immutable_databag_system_id = ECS::register_system(system_with_immutable_databag, "system_with_immutable_databag").get_id();
	godex::system_id system_with_component_system_id = ECS::register_system(system_with_component, "system_with_component").get_id();
	godex::system_id system_with_immutable_component_system_id = ECS::register_system(system_with_immutable_component, "system_with_immutable_component").get_id();
	godex::system_id system_with_storage_system_id = ECS::register_system(system_with_storage, "system_with_storage").get_id();

	PipelineBuilder pipeline_builder;

	// Add system with MUTABLE `PipelineTestDatabag1` databag.
	pipeline_builder.add_system(system_with_databag_system_id);

	// Add system with IMMUTABLE `PipelineTestDatabag2` databag.
	pipeline_builder.add_system(system_with_immutable_databag_system_id);

	// Add system with MUTABLE `PipelineTestComponent1` component.
	pipeline_builder.add_system(system_with_component_system_id);

	// Add system with MUTABLE `TransformComponent` component.
	pipeline_builder.add_system(system_with_immutable_component_system_id);

	// Add system with a storage.
	pipeline_builder.add_system(system_with_storage_system_id);

	Pipeline pipeline;
	pipeline_builder.build(pipeline);

	SystemExeInfo info;
	pipeline.get_systems_dependencies(info);

	// Make sure a MUTABLE `PipelineTestDatabag1` databag is found.
	CHECK(info.mutable_databags.find(PipelineTestDatabag1::get_databag_id()) != nullptr);

	// Make sure an IMMUTABLE `PipelineTestDatabag2` databag is found.
	CHECK(info.immutable_databags.find(PipelineTestDatabag2::get_databag_id()) != nullptr);

	// Make sure an MUTABLE `PipelineTestComponent1` component is found.
	CHECK(info.mutable_components.find(PipelineTestComponent1::get_component_id()) != nullptr);

	// Make sure an IMMUTABLE `TransformComponent` component is found.
	CHECK(info.immutable_components.find(TransformComponent::get_component_id()) != nullptr);

	// Make sure the component storage is correctly fetched from the world.
	CHECK(info.mutable_components_storage.find(TransformComponent::get_component_id()) != nullptr);
}

TEST_CASE("[Modules][ECS] Test pipeline get_systems_dependencies from a dispatcher.") {
	godex::system_id system_with_databag_system_id = ECS::get_system_id("system_with_databag"); // This sytem is already registered.
	const godex::system_id system_with_immutable_databag_system_id = ECS::get_system_id("system_with_immutable_databag");
	const godex::system_id system_with_component_system_id = ECS::get_system_id("system_with_component");
	const godex::system_id system_with_immutable_component_system_id = ECS::get_system_id("system_with_immutable_component");
	const godex::system_id system_with_storage_system_id = ECS::get_system_id("system_with_storage");
	const godex::system_id sub_dispatcher_system_id = ECS::register_dynamic_system("SubDispatcherSystemTest")
															  .set_description("Unit tests system.")
															  .get_id();

	Pipeline sub_pipeline;
	{
		PipelineBuilder sub_pipeline_builder;

		// Add system with MUTABLE `PipelineTestDatabag1` databag.
		sub_pipeline_builder.add_system(system_with_databag_system_id);

		// Add system with IMMUTABLE `PipelineTestDatabag2` databag.
		sub_pipeline_builder.add_system(system_with_immutable_databag_system_id);

		// Add system with MUTABLE `PipelineTestComponent1` component.
		sub_pipeline_builder.add_system(system_with_component_system_id);

		// Add system with MUTABLE `TransformComponent` component.
		sub_pipeline_builder.add_system(system_with_immutable_component_system_id);

		// Add system with a storage.
		sub_pipeline_builder.add_system(system_with_storage_system_id);

		sub_pipeline_builder.build(sub_pipeline);

		// Build the system that dispatches the sub pipeline.
		godex::DynamicSystemInfo *info = ECS::get_dynamic_system_info(sub_dispatcher_system_id);
		info->set_target(sub_dispatcher_system_test);
		info->set_pipeline(&sub_pipeline);
		info->build();
	}

	PipelineBuilder pipeline_builder;

	pipeline_builder.add_system(sub_dispatcher_system_id);

	Pipeline pipeline;
	pipeline_builder.build(pipeline);

	SystemExeInfo info;
	pipeline.get_systems_dependencies(info);

	// Make sure a MUTABLE `PipelineTestDatabag1` databag is found.
	CHECK(info.mutable_databags.find(PipelineTestDatabag1::get_databag_id()) != nullptr);

	// Make sure an IMMUTABLE `PipelineTestDatabag2` databag is found.
	CHECK(info.immutable_databags.find(PipelineTestDatabag2::get_databag_id()) != nullptr);

	// Make sure an MUTABLE `PipelineTestComponent1` component is found.
	CHECK(info.mutable_components.find(PipelineTestComponent1::get_component_id()) != nullptr);

	// Make sure an IMMUTABLE `TransformComponent` component is found.
	CHECK(info.immutable_components.find(TransformComponent::get_component_id()) != nullptr);

	// Make sure the component storage is correctly fetched from the world.
	CHECK(info.mutable_components_storage.find(TransformComponent::get_component_id()) != nullptr);
}
} // namespace godex_tests_pipeline

#endif // TEST_ECS_PIPELINE_H
