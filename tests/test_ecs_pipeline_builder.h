#ifndef TEST_ECS_PIPELINE_BUILDER_H
#define TEST_ECS_PIPELINE_BUILDER_H

#include "tests/test_macros.h"

#include "../modules/godot/nodes/ecs_utilities.h"
#include "test_utilities.h"

namespace godex_tests {

TEST_CASE("[Modules][ECS] Test Pipeline builder.") {
	initialize_script_ecs();

	StringName system_1_name = "System1.gd";

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	pass";
		code += "\n";
		code += "func _for_each(transform_com):\n";
		code += "	if get_current_entity_id() == 2:\n";
		code += "		transform_com.transform.origin.x = 10.0\n";
		code += "\n";

		CHECK(build_and_register_ecs_script(system_1_name, code));
	}

	Ref<System> system_1 = ScriptEcs::get_singleton()->get_script_system(system_1_name);
	CHECK(system_1.is_valid());

	finalize_script_ecs();
}
} // namespace godex_tests

#endif // TEST_ECS_PIPELINE_BUILDER_H
