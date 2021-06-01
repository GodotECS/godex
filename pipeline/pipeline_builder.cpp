#include "pipeline_builder.h"

#include "../modules/godot/nodes/script_ecs.h"
#include "pipeline.h"

/*
void fetch_bundle_info(
		const Vector<StringName> &p_system_bundles,
		LocalVector<BundleInfo> &r_bundles) {
	// Fetch the SystemBundle info.
	r_bundles.resize(p_system_bundles.size());

	const StringName *bundle_names = p_system_bundles.ptr();
	for (int i = 0; i < p_system_bundles.size(); i += 1) {
		bundle_names[i];
	}
	//ScriptEcs::get_singleton()->get_script_system_bundle();
}
*/

void PipelineBuilder::build_graph(
		const Vector<StringName> &p_system_bundles,
		const Vector<StringName> &p_systems,
		ExecutionGraph *r_graph) {
	CRASH_COND_MSG(r_graph == nullptr, "The pipeline pointer must be valid.");

	//LocalVector<BundleInfo> bundles;
	//fetch_bundle_info(p_system_bundles, bundles);

	// Count the systems.
	//p_system_bundles

	//p_systems.size();

	//r_graph->systems.resize(10);
}

void PipelineBuilder::build_pipeline(
		const Vector<StringName> &p_system_bundles,
		const Vector<StringName> &p_systems,
		Pipeline *r_pipeline) {
	CRASH_COND_MSG(r_pipeline == nullptr, "The pipeline pointer must be valid.");
	ExecutionGraph graph;
	build_graph(p_system_bundles, p_systems, &graph);
	build_pipeline(graph, r_pipeline);
}

void PipelineBuilder::build_pipeline(
		const ExecutionGraph &p_graph,
		Pipeline *r_pipeline) {
	CRASH_COND_MSG(r_pipeline == nullptr, "The pipeline pointer must be valid.");
	// TODO
}
