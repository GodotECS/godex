#ifndef TEST_ECS_PIPELINE_BUILDER_H
#define TEST_ECS_PIPELINE_BUILDER_H

#include "tests/test_macros.h"

#include "../components/component.h"
#include "../databags/databag.h"
#include "../modules/godot/nodes/ecs_utilities.h"
#include "../pipeline/pipeline.h"
#include "../pipeline/pipeline_builder.h"
#include "../storage/dense_vector_storage.h"
#include "test_utilities.h"

// The idea to verify that the pipeline is correctly constructed we need to
// verify the following:
// 1. The Systems with implicit dependencies are correctly taken into account.
//      - The Systems implicit priority is used in this case, where the
//        priority is given by the registration order.
//      - The Systems implicit dependency, where two system fetch the same
//        Compoenents and DataBags mutable, are resolved.
//      - The systems that fetch the sane Components and DataBag immutable
//        can run in parallel.
// 2. The Systems with explicit dependencies are correctly taken into account.
//      - The Systems explicit priority, build with (`after` and `before`) is
//        correctly resolved.
// 3. The Systems phase is correctly used.
// 4. The SystemBundles are correctly expanded.
// 5. The SystemBundles explicit dependencies are take into account.
// 6. The Query filer `Not` can be used to specialize the fetched component
//    so two systems that fetch the same component mutable may run in parallel
//    thanks to the specialization given by `Not`.
// 7. All the above must be valid for C++ and Scripted systems.
// 8. Include sub pipeline composition
// 9. Make sure that the systems that fetch World, SceneTreeDatabag are always
//     are always executed in single thread.
// 10. Detect when an event isn't catched by any system, tell how to fix it.
// TODO 11. Test advice mechanism

struct PbComponentA {
	COMPONENT(PbComponentA, DenseVectorStorage)
};

struct PbComponentB {
	COMPONENT(PbComponentB, DenseVectorStorage)
};

struct PbDatabagA : public godex::Databag {
	DATABAG(PbDatabagA)
};

namespace godex_tests {

TEST_CASE("[Modules][ECS] Initialize PipelineBuilder tests.") {
	// Just initializes the common Components and Databags used by the following tests.
	ECS::register_component<PbComponentA>();
	ECS::register_component<PbComponentB>();
	ECS::register_databag<PbDatabagA>();
}
} // namespace godex_tests

void test_A_system_1(Query<PbComponentA, const PbComponentB> &p_query) {}
void test_A_system_3(Query<const PbComponentA, const PbComponentB> &p_query, PbDatabagA *p_db_a) {}
void test_A_system_5(Query<const PbComponentA, const PbComponentB> &p_query, const PbDatabagA *p_db_a) {}

namespace godex_tests {

TEST_CASE("[Modules][ECS] Verify the PipelineBuilder takes into account implicit dependencies and implicit order.") {
	initialize_script_ecs();

	ECS::register_system(test_A_system_1, "test_A_system_1");

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, MUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_2.gd", code));
	}

	ECS::register_system(test_A_system_3, "test_A_system_3");

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_4.gd", code));
	}

	ECS::register_system(test_A_system_5, "test_A_system_5");

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "	with_databag(ECS.PbDatabagA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b, c):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_6.gd", code));
	}

	flush_ecs_script_preparation();

	Pipeline pipeline;

	Vector<StringName> system_bundles;

	Vector<StringName> systems;
	// Insert the systems with random order.
	systems.push_back(StringName("test_A_system_6.gd"));
	systems.push_back(StringName("test_A_system_5"));
	systems.push_back(StringName("test_A_system_4.gd"));
	systems.push_back(StringName("test_A_system_3"));
	systems.push_back(StringName("test_A_system_2.gd"));
	systems.push_back(StringName("test_A_system_1"));

	PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);

	// Verify the test_A_system_1 is the first one being executed,
	// has its implicit registration order imply.
	// TODO

	// Verify the test_A_system_2.gd is the second one being executed,
	// has its implicit registration order imply, and not in parallel with
	// test_A_system_1.
	// TODO

	// Verify the test_A_system_3 and test_A_system_4.gd run in parallel
	// and after test_A_system_2.gd
	// TODO

	// Verify the test_A_system_5 and test_A_system_6.gd run in parallel,
	// but after test_A_system_3 and test_A_system_4.gd.
	// TODO

	finalize_script_ecs();
}
} // namespace godex_tests

#endif // TEST_ECS_PIPELINE_BUILDER_H
