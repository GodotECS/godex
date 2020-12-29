#ifndef TEST_ECS_PIPELINE_H
#define TEST_ECS_PIPELINE_H

#include "tests/test_macros.h"

#include "../ecs.h"
#include "../godot/components/transform_component.h"
#include "../pipeline/pipeline.h"

class PipelineTestResource1 : public godex::Resource {
	RESOURCE(PipelineTestResource1)

public:
	int a = 10;
};

class PipelineTestResource2 : public godex::Resource {
	RESOURCE(PipelineTestResource2)

public:
	int a = 10;
};

class PipelineTestComponent1 : public godex::Component {
	COMPONENT(PipelineTestComponent1, DenseVector)
};

namespace godex_tests_pipeline {

void system_with_resource(PipelineTestResource1 *test_res) {
}

void system_with_immutable_resource(const PipelineTestResource2 *test_res) {
}

void system_with_component(Query<PipelineTestComponent1> &p_query) {
}

void system_with_immutable_component(Query<const TransformComponent> &p_query) {
}

TEST_CASE("[Modules][ECS] Test pipeline build.") {
	ECS::register_resource<PipelineTestResource1>();
	ECS::register_resource<PipelineTestResource2>();
	ECS::register_component<PipelineTestComponent1>();

	Pipeline pipeline;

	// Make sure the pipeline is not yet ready.
	CHECK(pipeline.is_ready() == false);

	pipeline.add_system(system_with_resource);

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

	// Add system with MUTABLE `PipelineTestResource1` resource.
	pipeline.add_system(system_with_resource);

	// Add system with IMMUTABLE `PipelineTestResource2` resource.
	pipeline.add_system(system_with_immutable_resource);

	// Add system with MUTABLE `PipelineTestComponent1` component.
	pipeline.add_system(system_with_component);

	// Add system with MUTABLE `TransformComponent` component.
	pipeline.add_system(system_with_immutable_component);

	pipeline.build();

	SystemExeInfo info;
	pipeline.get_systems_dependencies(info);

	// Make sure a MUTABLE `PipelineTestResource1` resource is found.
	CHECK(info.mutable_resources.find(PipelineTestResource1::get_resource_id()) != -1);

	// Make sure an IMMUTABLE `PipelineTestResource2` resource is found.
	CHECK(info.immutable_resources.find(PipelineTestResource2::get_resource_id()) != -1);

	// Make sure an MUTABLE `PipelineTestComponent1` component is found.
	CHECK(info.mutable_components.find(PipelineTestComponent1::get_component_id()) != -1);

	// Make sure an IMMUTABLE `TransformComponent` component is found.
	CHECK(info.immutable_components.find(TransformComponent::get_component_id()) != -1);
}

} // namespace godex_tests_pipeline

#endif // TEST_ECS_PIPELINE_H
