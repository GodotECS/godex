#include "pipeline_builder.h"

#include "../modules/godot/nodes/script_ecs.h"
#include "pipeline.h"

void PipelineBuilder::build_graph(
		const Vector<StringName> &p_system_bundles,
		const Vector<StringName> &p_systems,
		ExecutionGraph *r_graph) {
	CRASH_COND_MSG(r_graph == nullptr, "The pipeline pointer must be valid.");

	r_graph->systems.clear();
	r_graph->sorted_systems.clear();

	// Initialize the system, by fetching the various dependencies.
	r_graph->systems.resize(ECS::get_systems_count());

	fetch_bundle_info(p_system_bundles, r_graph);
	for (uint32_t i = 0; i < p_systems.size(); i += 1) {
		fetch_system_info(p_systems[i], StringName(), -1, LocalVector<Dependency>(), r_graph);
	}

	system_sorting(r_graph);

	// Count the systems.
	//p_system_bundles

	//p_systems.size();
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

void PipelineBuilder::fetch_bundle_info(
		const Vector<StringName> &p_system_bundles,
		ExecutionGraph *r_graph) {
	const StringName *bundle_names = p_system_bundles.ptr();
	for (int i = 0; i < p_system_bundles.size(); i += 1) {
		const godex::system_bundle_id bundle_id = ECS::get_system_bundle_id(bundle_names[i]);
		ERR_CONTINUE_MSG(bundle_id == godex::SYSTEM_BUNDLE_NONE, "The bundle " + bundle_names[i] + " doesn't exist.");
		const SystemBundleInfo &bundle = ECS::get_system_bundle(bundle_id);

		for (uint32_t s_i = 0; s_i < bundle.systems.size(); s_i += 1) {
			fetch_system_info(
					bundle.systems[s_i],
					bundle_names[i],
					s_i,
					bundle.dependencies,
					r_graph);
		}
	}
	//ScriptEcs::get_singleton()->get_script_system_bundle();
}

void PipelineBuilder::fetch_system_info(
		const StringName &p_system,
		const StringName &p_bundle_name,
		int p_explicit_priority,
		const LocalVector<Dependency> &p_extra_dependencies,
		ExecutionGraph *r_graph) {
	godex::system_id id = ECS::get_system_id(p_system);
	ERR_FAIL_COND_MSG(id == godex::SYSTEM_NONE, "The system " + p_system + " doesn't exists.");
	ERR_FAIL_COND_MSG(r_graph->systems[id].is_used == true, "The system " + p_system + " is being used twice. Skip it.");

	r_graph->systems[id].is_used = true;
	r_graph->systems[id].id = id;
	r_graph->systems[id].phase = ECS::get_system_phase(id);
	r_graph->systems[id].explicit_priority = p_explicit_priority;
	r_graph->systems[id].bundle_name = p_bundle_name;

	resolve_dependencies(id, p_extra_dependencies, r_graph);
	resolve_dependencies(id, ECS::get_system_dependencies(id), r_graph);
}

void PipelineBuilder::resolve_dependencies(
		godex::system_id id,
		const LocalVector<Dependency> &p_dependencies,
		ExecutionGraph *r_graph) {
	for (uint32_t i = 0; i < p_dependencies.size(); i += 1) {
		const godex::system_id dep_system_id = ECS::get_system_id(p_dependencies[i].system_name);
		ERR_CONTINUE_MSG(dep_system_id == godex::SYSTEM_NONE, "The system " + p_dependencies[i].system_name + " doesn't exists.");
		if (p_dependencies[i].execute_before) {
			// The current system must be executed before the dependency.
			r_graph->systems[dep_system_id].execute_after.push_back(r_graph->systems.ptr() + id);
		} else {
			// The current system must be executed after the dependency.
			r_graph->systems[id].execute_after.push_back(r_graph->systems.ptr() + dep_system_id);
		}
	}
}

void PipelineBuilder::system_sorting(ExecutionGraph *r_graph) {
	// TODO detect cyclic reference here?

	for (uint32_t i = 0; i < r_graph->systems.size(); i += 1) {
		ExecutionGraph::SystemNode *node = r_graph->systems.ptr() + i;
		if (node->is_used == false) {
			// This system is not included.
			continue;
		}

		r_graph->systems[i].self_list_element = r_graph->sorted_systems.push_back(r_graph->systems.ptr() + i);
	}
}
