#ifndef TEST_ECS_PIPELINE_H
#define TEST_ECS_PIPELINE_H

#include "tests/test_macros.h"

#include "../ecs.h"
#include "../godot/components/transform_component.h"
#include "../pipeline/pipeline.h"
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
	ECS::register_databag<PipelineTestDatabag1>();
	ECS::register_databag<PipelineTestDatabag2>();
	ECS::register_component<PipelineTestComponent1>();

	Pipeline pipeline;

	// Make sure the pipeline is not yet ready.
	CHECK(pipeline.is_ready() == false);

	pipeline.add_system(system_with_databag);

	// Make sure the pipeline is not yet ready.
	CHECK(pipeline.is_ready() == false);

	pipeline.build();

	// Make sure the pipeline is now ready.
	CHECK(pipeline.is_ready());

	pipeline.reset();

	// Make sure the pipeline is not ready again.
	CHECK(pipeline.is_ready() == false);
}

TEST_CASE("[Modules][ECS] Test pipeline get_systems_dependencies") {
	Pipeline pipeline;

	// Add system with MUTABLE `PipelineTestDatabag1` databag.
	pipeline.add_system(system_with_databag);

	// Add system with IMMUTABLE `PipelineTestDatabag2` databag.
	pipeline.add_system(system_with_immutable_databag);

	// Add system with MUTABLE `PipelineTestComponent1` component.
	pipeline.add_system(system_with_component);

	// Add system with MUTABLE `TransformComponent` component.
	pipeline.add_system(system_with_immutable_component);

	// Add system with a storage.
	pipeline.add_system(system_with_storage);

	pipeline.build();

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
	const godex::system_id sub_dispatcher_system_id = ECS::register_dynamic_system("SubDispatcherSystemTest", "Unit tests system.");

	Pipeline pipeline;
	Pipeline sub_pipeline;
	{
		// Add system with MUTABLE `PipelineTestDatabag1` databag.
		sub_pipeline.add_system(system_with_databag);

		// Add system with IMMUTABLE `PipelineTestDatabag2` databag.
		sub_pipeline.add_system(system_with_immutable_databag);

		// Add system with MUTABLE `PipelineTestComponent1` component.
		sub_pipeline.add_system(system_with_component);

		// Add system with MUTABLE `TransformComponent` component.
		sub_pipeline.add_system(system_with_immutable_component);

		// Add system with a storage.
		sub_pipeline.add_system(system_with_storage);

		sub_pipeline.build();

		godex::DynamicSystemInfo *info = ECS::get_dynamic_system_info(sub_dispatcher_system_id);
		info->set_target(sub_dispatcher_system_test);
		info->set_pipeline(&sub_pipeline);
		info->build();
	}

	pipeline.add_registered_system(sub_dispatcher_system_id);
	pipeline.build();

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
