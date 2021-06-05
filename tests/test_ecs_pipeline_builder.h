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
// 11. Test the World and SceneTreeDatabag systems are always run in single thread.

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
void test_A_system_14(Query<const PbComponentA, const PbComponentB> &p_query) {}
void test_A_system_15(Query<PbComponentA, PbComponentB> &p_query) {}

namespace godex_tests {

TEST_CASE("[Modules][ECS] Verify the PipelineBuilder takes into account implicit and explicit dependencies.") {
	initialize_script_ecs();

	ECS::register_system(test_A_system_15, "test_A_system_15")
			.set_phase(PHASE_CONFIG);

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_in_phase(ECS.PHASE_POST_PROCESS)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "	with_databag(ECS.PbDatabagA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b, c):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_8.gd", code));
	}

	ECS::register_system(test_A_system_1, "test_A_system_1");

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_in_phase(ECS.PHASE_CONFIG)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "	with_databag(ECS.PbDatabagA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b, c):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_0.gd", code));
	}

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

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_in_phase(ECS.PHASE_CONFIG)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "	with_databag(ECS.PbDatabagA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b, c):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_7.gd", code));
	}

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_before(ECS.test_A_system_1)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "	with_databag(ECS.PbDatabagA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b, c):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_9.gd", code));
	}

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_after(ECS.test_A_system_13_gd)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_10.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_after(ECS.test_A_system_12_gd)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_11.gd", code));
	}
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

		CHECK(build_and_register_ecs_script("test_A_system_12.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_before(ECS.test_A_system_12_gd)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_13.gd", code));
	}

	ECS::register_system(test_A_system_14, "test_A_system_14")
			.before("test_A_system_9.gd");

	flush_ecs_script_preparation();

	Vector<StringName> system_bundles;

	Vector<StringName> systems;
	// Insert the systems with random order.
	systems.push_back(StringName("test_A_system_14"));
	systems.push_back(StringName("test_A_system_13.gd"));
	systems.push_back(StringName("test_A_system_12.gd"));
	systems.push_back(StringName("test_A_system_11.gd"));
	systems.push_back(StringName("test_A_system_10.gd"));
	systems.push_back(StringName("test_A_system_8.gd"));
	systems.push_back(StringName("test_A_system_6.gd"));
	systems.push_back(StringName("test_A_system_5"));
	systems.push_back(StringName("test_A_system_4.gd"));
	systems.push_back(StringName("test_A_system_3"));
	systems.push_back(StringName("test_A_system_0.gd"));
	systems.push_back(StringName("test_A_system_2.gd"));
	systems.push_back(StringName("test_A_system_1"));
	systems.push_back(StringName("test_A_system_7.gd"));
	systems.push_back(StringName("test_A_system_9.gd"));
	systems.push_back(StringName("test_A_system_15"));

	Pipeline pipeline;
	PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);

	const int stage_test_A_system_14 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_14")));
	const int stage_test_A_system_13 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_13.gd")));
	const int stage_test_A_system_12 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_12.gd")));
	const int stage_test_A_system_11 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_11.gd")));
	const int stage_test_A_system_10 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_10.gd")));
	const int stage_test_A_system_8 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_8.gd")));
	const int stage_test_A_system_6 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_6.gd")));
	const int stage_test_A_system_5 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_5")));
	const int stage_test_A_system_4 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_4.gd")));
	const int stage_test_A_system_3 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_3")));
	const int stage_test_A_system_0 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_0.gd")));
	const int stage_test_A_system_2 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_2.gd")));
	const int stage_test_A_system_1 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_1")));
	const int stage_test_A_system_7 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_7.gd")));
	const int stage_test_A_system_9 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_9.gd")));
	const int stage_test_A_system_15 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_15")));

	// Verify the test_A_system_15 is the first one being executed, and it's not
	// executed in parallel with any other system.
	// It's executed first because it run in the config phase and also thanks to
	// its implicit priority, and implicit dependency given by the mutable Query.
	CHECK(stage_test_A_system_15 < stage_test_A_system_7);

	// The phase config merges with the process phase:

	// test_A_system_14 has an explicit dependency with test_A_system_9, it runs
	// before. However, the test_A_system_7.gd and test_A_system_0.gd is compatible
	// to run with with both in parallel. The optimization algorithm will decide
	// where to put this sysetm.
	CHECK((stage_test_A_system_7 == stage_test_A_system_14 || stage_test_A_system_7 == stage_test_A_system_9));
	CHECK((stage_test_A_system_0 == stage_test_A_system_14 || stage_test_A_system_0 == stage_test_A_system_9));

	// Verify the test_A_system_9.gd is executed before test_A_system_1 and in
	// parallel with test_A_system_0.gd
	CHECK(stage_test_A_system_9 < stage_test_A_system_1);

	// Verify the test_A_system_2.gd runs after test_A_system_1 because they have
	// an implicit dependency and test_A_system_1 is registered before.
	CHECK(stage_test_A_system_1 < stage_test_A_system_2);

	// Verify the test_A_system_3 and test_A_system_4.gd run in parallel
	// and after test_A_system_2.gd
	CHECK(stage_test_A_system_3 == stage_test_A_system_4);
	CHECK(stage_test_A_system_3 > stage_test_A_system_2);

	// Verify the test_A_system_5 and test_A_system_6.gd run in parallel,
	// but after test_A_system_3 and test_A_system_4.gd.
	CHECK(stage_test_A_system_5 == stage_test_A_system_6);
	CHECK(stage_test_A_system_5 > stage_test_A_system_3);

	// Verify the test_A_system_8 run the last one, since it's marked as post
	// process.
	CHECK(stage_test_A_system_8 >= stage_test_A_system_5);

	// Verify explicit dependencies:

	// Verify the test_A_system_13 is executed before test_A_system_12.
	CHECK(stage_test_A_system_13 < stage_test_A_system_12);

	// Verify the test_A_system_11 is executed after test_A_system_12.
	CHECK(stage_test_A_system_11 > stage_test_A_system_12);

	// Verify the test_A_system_10 is executed after test_A_system_13.
	CHECK(stage_test_A_system_10 > stage_test_A_system_13);

	// Verify the test_A_system_14 is executed before test_A_system_9.
	CHECK(stage_test_A_system_14 < stage_test_A_system_9);

	finalize_script_ecs();
}
} // namespace godex_tests

#endif // TEST_ECS_PIPELINE_BUILDER_H
