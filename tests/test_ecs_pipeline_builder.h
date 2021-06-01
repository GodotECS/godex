#ifndef TEST_ECS_PIPELINE_BUILDER_H
#define TEST_ECS_PIPELINE_BUILDER_H

#include "tests/test_macros.h"

#include "../modules/godot/nodes/ecs_utilities.h"
#include "../pipeline/pipeline.h"
#include "../pipeline/pipeline_builder.h"
#include "test_utilities.h"

namespace godex_tests {

TEST_CASE("[Modules][ECS] Test Pipeline builder.") {
	initialize_script_ecs();

	// The idea to verify that the pipeline is correctly constructed we need to
	// verify the following:
	// 1. The SystemBundles are correctly expanded.
	// 2. The Systems with implicit dependencies are correctly taken into account.
	// 3. The Systems with explicit dependencies are correctly taken into account.
	// 4. The Systems implicit priority, where the system registered before has
	//		the priority is correctly resolved.
	// 5. The Systems explicit priority, build with (`after` and `before`) is
	//		correctly resolved.
	// 6. All the above must be valid for C++ and Scripted systems.
	//
	// To verify the above we are going the build the following pipeline:
	//

	StringName system_1_name = "PipeBuilderSystem1.gd";

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.TransformComponent, MUTABLE)\n";
		code += "\n";
		code += "func _for_each(transform_com):\n";
		code += "	if get_current_entity_id() == 2:\n";
		code += "		transform_com.transform.origin.x = 10.0\n";
		code += "\n";

		CHECK(build_and_register_ecs_script(system_1_name, code));
	}

	Pipeline pipeline;
	{
		Vector<StringName> system_bundles;
		system_bundles.push_back(StringName(""));

		Vector<StringName> systems;
		systems.push_back(StringName("System1.gd"));

		PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);
	}

	finalize_script_ecs();
}
} // namespace godex_tests

#endif // TEST_ECS_PIPELINE_BUILDER_H
