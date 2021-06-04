#include "pipeline_builder.h"

#include "../modules/godot/nodes/script_ecs.h"
#include "core/os/os.h"
#include "pipeline.h"

bool ExecutionGraph::StageNode::is_compatible(const SystemNode *p_system) const {
	for (uint32_t i = 0; i < systems.size(); i += 1) {
		const SystemNode *staged_system = systems[i];
		if (p_system->execute_after.find(staged_system) != -1) {
			// The `p_system` must run after this `staged_system`, so we have a
			// dependency and the stage is not compatible.
			return false;
		}
		if (staged_system->execute_after.find(p_system) != -1) {
			// The `p_system` must run after this `staged_system`, so we have a
			// dependency and the stage is not compatible.
			return false;
		}
		if (!ECS::can_systems_run_in_parallel(p_system->id, staged_system->id)) {
			// These two systems can't run in parallel. This is an implicit
			// dependency and `p_system` is not compatible with this stage.
			return false;
		}
	}
	return true;
}

void ExecutionGraph::prepare_for_optimization() {
	real_t average_systems_per_stage = 0;
	real_t considered_stages = 0;
	for (List<ExecutionGraph::StageNode>::Element *e = stages.front(); e; e = e->next()) {
		if (e->get().systems.size() <= 1) {
			// Do not consider too little stages.
			continue;
		}
		average_systems_per_stage += e->get().systems.size();
		considered_stages += 1;
	}
	average_systems_per_stage /= considered_stages;

	// The best stage size is taken considering the median stage size of this
	// pipeline, but never excedes the amount of processors this machine has.
	// Note, the stage can still have more systems than the optimal size, if there
	// is no way to balance it.
	best_stage_size = MIN(average_systems_per_stage, real_t(OS::get_singleton()->get_processor_count()));
}

real_t ExecutionGraph::compute_effort(uint32_t p_system_count) {
	// The score systems tries to find the best balance for each stage, so that
	// each stage has a constant load.
	const real_t syste_count = p_system_count;
	// The faraway to the best_stage_size, the bigger the effort is.
	return Math::pow(best_stage_size - syste_count, real_t(20.0));
}

void ExecutionGraph::print_sorted_systems() const {
	print_line("Execution Graph, sorted nodes:");
	print_line("|");
	for (const List<SystemNode *>::Element *a = sorted_systems.front(); a; a = a->next()) {
		print_line("|- " + ECS::get_system_name(a->get()->id));
	}
	print_line("");
}

void ExecutionGraph::print_stages() const {
	print_line("Execution Graph, stages:");
	print_line("|");
	for (const List<StageNode>::Element *a = stages.front(); a; a = a->next()) {
		String msg = "|- [";
		for (uint32_t i = 0; i < a->get().systems.size(); i += 1) {
			if (i != 0) {
				msg += ", ";
			}
			msg += ECS::get_system_name(a->get().systems[i]->id);
		}
		msg += "]";
		print_line(msg);
	}
	print_line("");
}

void PipelineBuilder::build_graph(
		const Vector<StringName> &p_system_bundles,
		const Vector<StringName> &p_systems,
		ExecutionGraph *r_graph) {
	CRASH_COND_MSG(r_graph == nullptr, "The pipeline pointer must be valid.");

	r_graph->systems.clear();
	r_graph->sorted_systems.clear();
	r_graph->stages.clear();

	// Initialize the system, by fetching the various dependencies.
	r_graph->systems.resize(ECS::get_systems_count());

	fetch_bundle_info(p_system_bundles, r_graph);
	for (int i = 0; i < p_systems.size(); i += 1) {
		fetch_system_info(p_systems[i], StringName(), -1, LocalVector<Dependency>(), r_graph);
	}

	system_sorting(r_graph);

	// Check if we have a cynclic dependency.
	if (has_cyclick_dependencies(r_graph)) {
		r_graph->sorted_systems.clear();
		r_graph->systems.clear();
		ERR_FAIL_MSG("[FATAL] This graph has a cyclick dependency and the pipeline building was discarded.");
		return;
	}

	// TODO remove this?
	build_stages(r_graph);
	optimize_stages(r_graph);

	// Done :)
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

	r_pipeline->reset();

	/*for (List<ExecutionGraph::StageNode>::Element *node = p_graph.stages.front();
			node;
			node = node->next()) {
		node->get();
	}*/
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
	// Initialize the sorted list.
	for (uint32_t i = 0; i < r_graph->systems.size(); i += 1) {
		ExecutionGraph::SystemNode *node = r_graph->systems.ptr() + i;
		if (node->is_used == false) {
			// This system is not included.
			continue;
		}

		r_graph->systems[i].self_list_element = r_graph->sorted_systems.push_back(node);
	}

	struct SortByPriority {
		bool operator()(ExecutionGraph::SystemNode *const &p_a, ExecutionGraph::SystemNode *const &p_b) const {
			// Is `p_a` the smallest?

			// First verify the phase.
			if (p_a->phase > p_b->phase) {
				return false;
			}

			if (p_a->execute_after.find(p_b) != -1) {
				// `p_a` depends on `p_b` so it's not smallest.
				return false;
			}

			if (
					p_a->bundle_name != StringName() &&
					p_a->bundle_name == p_b->bundle_name &&
					(p_a->explicit_priority != -1 || p_b->explicit_priority != -1)) {
				if (p_a->explicit_priority > p_b->explicit_priority) {
					// `p_a` was explicitely prioritized to run after `p_b`.
					return false;
				}
			}

			if (p_a->id > p_b->id) {
				// `p_a` is implicitely prioritized to run after `p_b`.
				return false;
			}

			// `p_a` is the smallest.
			return true;
		}
	};

	// Use inplace, otherwise the sort by dependency fails, since not all element
	// are tested.
	r_graph->sorted_systems.sort_custom_inplace<SortByPriority>();
}

bool PipelineBuilder::has_cyclick_dependencies(const ExecutionGraph *r_graph) {
	// We can detect cyclic dependencies just by verifiying that the sorted list
	// has all the nodes dependencies correctly resolved. If one node is not
	// correctly resolved, it's a cyclic dependency.
	for (const List<ExecutionGraph::SystemNode *>::Element *e = r_graph->sorted_systems.front(); e; e = e->next()) {
		for (uint32_t i = 0; i < e->get()->execute_after.size(); i += 1) {
			for (const List<ExecutionGraph::SystemNode *>::Element *sub = e->next(); sub; sub = sub->next()) {
				if (e->get()->execute_after[i] == sub->get()) {
					return true;
				}
			}
		}
	}
	return false;
}

void PipelineBuilder::build_stages(ExecutionGraph *r_graph) {
	r_graph->stages.push_back(ExecutionGraph::StageNode());
	for (const List<ExecutionGraph::SystemNode *>::Element *e = r_graph->sorted_systems.front(); e; e = e->next()) {
		ExecutionGraph::StageNode &stage = r_graph->stages.back()->get();
		if (stage.is_compatible(e->get())) {
			stage.systems.push_back(e->get());
		} else {
			// Incompatible, create a new stage.
			r_graph->stages.push_back(ExecutionGraph::StageNode());
			r_graph->stages.back()->get().systems.push_back(e->get());
		}
	}
}

void PipelineBuilder::optimize_stages(ExecutionGraph *r_graph) {
	// The optimization phase works as follow:
	// For each System we find all the **near** and **compatible** Stages, for
	// each stage we enstablish a score depending on the about of Systems in it.
	// Then we take the stage with higher score, and if it beats the one where the
	// system currently is, we move it there.
	// This process is repeated for all the Systems.
	//
	// Thanks to the compatibility check all the various dependencies and priority
	// are still valid.

	r_graph->prepare_for_optimization();

	// For each system of each stage:
	for (List<ExecutionGraph::StageNode>::Element *e = r_graph->stages.front(); e; e = e->next()) {
		for (int i = 0; i < int(e->get().systems.size()); i += 1) {
			ExecutionGraph::SystemNode *system = e->get().systems[i];

			if (system->optimized) {
				// This system was already optimized, nothing more to do here.
				continue;
			}

			// Phase 1. score the current stage.
			real_t lowest_effort = r_graph->compute_effort(e->get().systems.size());
			ExecutionGraph::StageNode *best_stage = &e->get();

			// Phase 2. Try to find a better stage on the previous Stages:
			for (List<ExecutionGraph::StageNode>::Element *prev = e->prev(); prev; prev = prev->prev()) {
				if (prev->get().is_compatible(system)) {
					const real_t eventual_effort = r_graph->compute_effort(prev->get().systems.size() + 1);
					if (eventual_effort < lowest_effort) {
						// If we move the System here we get a more optimized
						// pipeline.
						lowest_effort = eventual_effort;
						best_stage = &prev->get();
					} else {
						// The current stage has an high score, keep searching.
					}
				} else {
					// The System is incompatible with this Stage, so we can't
					// move toward this direcction anymore, otherwise the
					// dependencies would be violated.
					break;
				}
			}

			// Phase 3. Try to find a better stage on the next Stages:
			for (List<ExecutionGraph::StageNode>::Element *next = e->next(); next; next = next->next()) {
				if (next->get().is_compatible(system)) {
					const real_t eventual_effort = r_graph->compute_effort(next->get().systems.size() + 1);
					if (eventual_effort < lowest_effort) {
						// If we move the System here we get a more optimized
						// pipeline.
						lowest_effort = eventual_effort;
						best_stage = &next->get();
					} else {
						// The current stage has an high score, keep searching.
					}
				} else {
					// The System is incompatible with this Stage, so we can't
					// move toward this direcction anymore, otherwise the
					// dependencies would be violated.
					break;
				}
			}

			if ((&e->get()) != best_stage) {
				// We found a better Stage, let's move the System there.
				e->get().systems.erase(system);
				best_stage->systems.push_back(system);
				// Decrease `i` since we removed the system from the stage.
				i -= 1;
			}

			system->optimized = true;
		}
	}

	// Remove the void stages.
	for (List<ExecutionGraph::StageNode>::Element *e = r_graph->stages.front(); e; e = e->next()) {
		if (e->get().systems.size() == 0) {
			e->erase();
		}
	}
}