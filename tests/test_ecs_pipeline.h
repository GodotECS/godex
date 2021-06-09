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

namespace godex_tests_pipeline {

void system_with_databag(PipelineTestDatabag1 *test_res) {}

TEST_CASE("[Modules][ECS] Test pipeline build.") {
	godex::system_id system_id = ECS::register_system(system_with_databag, "system_with_databag").get_id();
	ECS::register_databag<PipelineTestDatabag1>();

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
} // namespace godex_tests_pipeline
#endif // TEST_ECS_PIPELINE_H
