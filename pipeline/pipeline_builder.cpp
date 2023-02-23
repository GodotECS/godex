#include "pipeline_builder.h"

#include "../components/child.h"
#include "../modules/godot/nodes/script_ecs.h"
#include "core/os/os.h"
#include "pipeline.h"

bool ExecutionGraph::StageNode::is_compatible(const SystemNode *p_system) const {
	for (uint32_t i = 0; i < systems.size(); i += 1) {
		const SystemNode *staged_system = systems[i];
		if (!staged_system->is_compatible(p_system)) {
			return false;
		}
	}
	return true;
}

bool ExecutionGraph::SystemNode::is_compatible(const SystemNode *p_system, bool p_skip_explicit_dependency) const {
	if (!p_skip_explicit_dependency) {
		if (p_system->execute_after.find(this) != -1) {
			// The `p_system` must run after this `staged_system`, so we have a
			// dependency and the stage is not compatible.
			return false;
		}
		if (execute_after.find(p_system) != -1) {
			// The `p_system` must run after this `staged_system`, so we have a
			// dependency and the stage is not compatible.
			return false;
		}
	}

	if (!ECS::can_systems_run_in_parallel(p_system->id, id)) {
		// These two systems can't run in parallel. This is an implicit
		// dependency and `p_system` is not compatible with this stage.
		return false;
	}
	if (is_dispatcher()) {
		// The staged system is a dispatcher, check each sub stages.
		for (uint32_t i = 0; i < sub_dispatcher->systems.size(); i += 1) {
			if (!sub_dispatcher->systems[i]->is_compatible(p_system)) {
				// The subdispatcher has a system that is not compatible.
				return false;
			}
		}
	}
	if (p_system->is_dispatcher()) {
		// p_system is a dispatcher check again each sub system.
		for (uint32_t i = 0; i < p_system->sub_dispatcher->systems.size(); i += 1) {
			if (!p_system->sub_dispatcher->systems[i]->is_compatible(this)) {
				// The subdispatcher has a system that is not compatible.
				return false;
			}
		}
	}

	// Compatible
	return true;
}

void ExecutionGraph::prepare_for_optimization() {
	real_t average_systems_per_stage = 0;
	real_t considered_stages = 0;
	for (OAHashMap<StringName, Ref<ExecutionGraph::Dispatcher>>::Iterator d = dispatchers.iter();
			d.valid;
			d = dispatchers.next_iter(d)) {
		for (List<ExecutionGraph::StageNode>::Element *e = (*d.value)->stages.front(); e; e = e->next()) {
			if (e->get().systems.size() <= 1) {
				// Do not consider too little stages.
				continue;
			}
			average_systems_per_stage += e->get().systems.size();
			considered_stages += 1;
		}
	}
	average_systems_per_stage /= considered_stages;

	// The best stage size is taken considering the median stage size of this
	// pipeline, but never excedes the amount of processors this machine has.
	// Note, the stage can still have more systems than the optimal size, if there
	// is no way to balance it.
	best_stage_size = CLAMP(
			average_systems_per_stage,
			real_t(OS::get_singleton()->get_processor_count() / 4.0),
			real_t(OS::get_singleton()->get_processor_count()));

	// Never less than 2.
	best_stage_size = MAX(
			average_systems_per_stage,
			2);
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
	for (OAHashMap<StringName, Ref<ExecutionGraph::Dispatcher>>::Iterator e = dispatchers.iter();
			e.valid;
			e = dispatchers.next_iter(e)) {
		print_line("# Dispatcher: " + (*e.key));
		for (const List<SystemNode *>::Element *a = (*e.value)->sorted_systems.front(); a; a = a->next()) {
			print_line("|- " + ECS::get_system_name(a->get()->id) + " [#" + itos(a->get()->id) + "]");
		}
	}
	print_line("");
}

void ExecutionGraph::print_stages() const {
	ERR_FAIL_COND(dispatchers.has("main") == false);
	Ref<Dispatcher> main_dispatcher = *dispatchers.lookup_ptr("main");

	print_line("Execution Graph, stages:");
	uint32_t index = 0;
	print_line("Main");
	print_stages(main_dispatcher, 0, index);
	print_line("");
}

void ExecutionGraph::print_stages(const Ref<Dispatcher> p_dispatcher, uint32_t level, uint32_t &index) const {
	String padding;

	for (uint32_t i = 0; i < level; i += 1) {
		padding += "|   ";
	}

	for (const List<StageNode>::Element *a = p_dispatcher->stages.front(); a; a = a->next()) {
		print_line(padding + "|- stage@" + itos(index).lpad(2, "0"));
		index += 1;
		for (uint32_t i = 0; i < a->get().systems.size(); i += 1) {
			if (a->get().systems[i]->is_dispatcher()) {
				print_line(padding + "|   |- DISPATCHER#" + ECS::get_system_name(a->get().systems[i]->id));
				print_stages(
						a->get().systems[i]->sub_dispatcher,
						level + 2,
						index);
			} else {
				print_line(padding + "|   |- " + ECS::get_system_name(a->get().systems[i]->id));
			}
		}
	}
}

bool ExecutionGraph::is_valid() const {
	return valid;
}

const String &ExecutionGraph::get_error_msg() const {
	return error_msg;
}

const Vector<String> &ExecutionGraph::get_warnings() const {
	return warnings;
}

const LocalVector<ExecutionGraph::SystemNode> &ExecutionGraph::get_systems() const {
	return systems;
}

const Ref<ExecutionGraph::Dispatcher> ExecutionGraph::get_main_dispatcher() const {
	const Ref<Dispatcher> *main = dispatchers.lookup_ptr("main");
	return main == nullptr ? Ref<Dispatcher>() : (*main);
}

const List<ExecutionGraph::SystemNode *> &ExecutionGraph::get_temporary_systems() const {
	return temporary_systems;
}

real_t ExecutionGraph::get_best_stage_size() const {
	return best_stage_size;
}

PipelineBuilder::PipelineBuilder() {
}

void PipelineBuilder::add_system_bundle(godex::system_bundle_id p_id) {
	systems.push_back(ECS::get_system_bundle_name(p_id));
}

void PipelineBuilder::add_system_bundle(const StringName &p_bundle_name) {
	systems.push_back(p_bundle_name);
}

void PipelineBuilder::add_system(godex::system_id p_id) {
	systems.push_back(ECS::get_system_name(p_id));
}

void PipelineBuilder::add_system(const StringName &p_name) {
	systems.push_back(p_name);
}

void PipelineBuilder::build(Pipeline &r_pipeline) {
	build_pipeline(system_bundles, systems, &r_pipeline);
}

void PipelineBuilder::build_graph(
		const Vector<StringName> &p_system_bundles,
		const Vector<StringName> &p_systems,
		ExecutionGraph *r_graph,
		bool p_skip_warnings) {
	CRASH_COND_MSG(r_graph == nullptr, "The pipeline pointer must be valid.");

	r_graph->valid = false;
	r_graph->error_msg = "";
	r_graph->warnings.clear();
	r_graph->systems.clear();
	r_graph->dispatchers.clear();
	r_graph->systems_dispatcher.clear();

	// Crate the main dispatcher.
	Ref<ExecutionGraph::Dispatcher> main;
	main.instantiate();
	r_graph->dispatchers.insert("main", main);

	// Initialize the system, by fetching the various dependencies.
	r_graph->systems.resize(ECS::get_systems_count());

	fetch_bundle_info(p_system_bundles, r_graph);
	for (int i = 0; i < p_systems.size(); i += 1) {
		fetch_system_info(p_systems[i], StringName(), -1, LocalVector<SystemDependency>(), r_graph);
	}

	String error;
	const bool sort_failed = !sort_systems(r_graph, error);

	// Check if we have a cynclic dependency.
	if (sort_failed || has_cyclick_dependencies(r_graph, error)) {
		r_graph->print_sorted_systems();
		r_graph->print_stages();

		r_graph->systems_dispatcher.clear();
		r_graph->dispatchers.clear();
		r_graph->systems.clear();
		r_graph->valid = false;
		r_graph->error_msg = error;
		r_graph->error_msg += RTR(" Pipeline building is aborted.");

		ERR_FAIL_MSG("[FATAL] " + r_graph->error_msg + " Check the pipeline above ---^");
		return;
	}

	// The validation phase
	if (!p_skip_warnings) {
		detect_warnings_sub_dispatchers_missing(r_graph);
		detect_warnings_lost_events(r_graph);
	}

	// Everything is fine, build the graph and optimize it.
	build_stages(r_graph);
	optimize_stages(r_graph);

	r_graph->valid = true;

	// Done :)
}

void PipelineBuilder::build_pipeline(
		const Vector<StringName> &p_system_bundles,
		const Vector<StringName> &p_systems,
		Pipeline *r_pipeline) {
	CRASH_COND_MSG(r_pipeline == nullptr, "The pipeline pointer must be valid.");
	ExecutionGraph graph;
	build_graph(p_system_bundles, p_systems, &graph, true);
	if (graph.is_valid()) {
		build_pipeline(graph, r_pipeline);
	}
}

void PipelineBuilder::build_pipeline(
		const ExecutionGraph &p_graph,
		Pipeline *r_pipeline) {
	ERR_FAIL_COND_MSG(r_pipeline == nullptr, "The pipeline pointer must be valid.");
	ERR_FAIL_COND_MSG(r_pipeline->can_change() == false, "This pipeline has still some world bind. It's necessary to release those worlds before reusing this pipeline.");

	r_pipeline->reset();

	// Add the temporary systems.
	{
		r_pipeline->temporary_systems.resize(p_graph.temporary_systems.size());
		uint32_t index = 0;
		for (const List<ExecutionGraph::SystemNode *>::Element *system = p_graph.temporary_systems.front();
				system;
				system = system->next(), index += 1) {
			r_pipeline->temporary_systems[index] = system->get()->id;
		}
	}

	// Initialize the dispatchers.
	r_pipeline->dispatchers.resize(ECS::get_dispatchers_count());

	// Add the stages and relative systems; then setup the system.
	for (OAHashMap<StringName, Ref<ExecutionGraph::Dispatcher>>::Iterator d = p_graph.dispatchers.iter();
			d.valid;
			d = p_graph.dispatchers.next_iter(d)) {
		const Ref<ExecutionGraph::Dispatcher> dispatcher = (*d.value);
		int dispatcher_index = 0;
		if ((*d.key) != StringName("main")) {
			if (dispatcher->dispatched_by == nullptr) {
				return;
			} else {
				dispatcher_index = ECS::get_dispatcher_index(dispatcher->dispatched_by->id);
			}
		}

		r_pipeline->dispatchers[dispatcher_index].exec_stages.resize(dispatcher->stages.size());

		uint32_t stage_index = 0;

		for (const List<ExecutionGraph::StageNode>::Element *stage = dispatcher->stages.front();
				stage;
				stage = stage->next(), stage_index += 1) {
			r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].systems.resize(stage->get().systems.size());

			for (uint32_t i = 0; i < stage->get().systems.size(); i += 1) {
#ifdef DEBUG_ENABLED
				CRASH_COND_MSG(stage->get().systems[i]->info.valid == false, "At this point the system is valid because when it's invalid it's discarded by the previous steps. If this is triggered, make sure to build the pipeline using the proper methods.");
				// This is automated by the `add_system` macro or by
				// `ECS::register_system` macro, so is never supposed to happen.
				CRASH_COND_MSG(stage->get().systems[i]->info.system_func == nullptr, "At this point `system_info.system_func` is supposed to be not null. To add a system use the following syntax: `add_system(function_name);` or use the `ECS` class to get the `SystemExeInfo` if it's a registered system.");
#endif

				// Add the exec function to the stage.
				r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].systems[i].id = stage->get().systems[i]->id;
				r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].systems[i].index = UINT32_MAX; // Init just below
				r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].systems[i].exe = stage->get().systems[i]->info.system_func;

				// Setup the phase.
				if (ECS::is_system_dispatcher(stage->get().systems[i]->id) == false) {
					// Mark as flush, the storages that need to be flushed at the
					// end of the `System`.
					for (const RBSet<uint32_t>::Element *e = stage->get().systems[i]->info.mutable_components.front(); e; e = e->next()) {
						if (ECS::storage_notify_release_write(e->get())) {
							r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write.push_back(e->get());
						}
					}
					for (const RBSet<uint32_t>::Element *e = stage->get().systems[i]->info.mutable_components_storage.front(); e; e = e->next()) {
						if (ECS::storage_notify_release_write(e->get())) {
							if (r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write.find(e->get()) == -1) {
								r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write.push_back(e->get());
							}
						}
					}
				}
			}

			// If set: make sure that the `Child` storage (which is the Hierachy)
			// is flushed the first one so the hierachy changes are propagated immediately.
			const int64_t child_index = r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write.find(Child::get_component_id());
			if (child_index != -1) {
				SWAP(
						r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write[child_index],
						r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write[0]);
				CRASH_COND(r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write[0] != Child::get_component_id());
			}
		}
	}

	// Now all the stages for each dispatcher is ready, it's the perfect moment to
	// define the index of each System, in this pipeline.
	{
		uint32_t index = 0;
		for (uint32_t dispatcher_i = 0; dispatcher_i < r_pipeline->dispatchers.size(); dispatcher_i += 1) {
			DispatcherData &dispatcher = r_pipeline->dispatchers[dispatcher_i];

			for (uint32_t stage_i = 0; stage_i < dispatcher.exec_stages.size(); stage_i += 1) {
				for (uint32_t i = 0; i < dispatcher.exec_stages[stage_i].systems.size(); i += 1) {
					dispatcher.exec_stages[stage_i].systems[i].index = index;
					index += 1;
				}
			}
		}
	}

	r_pipeline->ready = true;

	// Build done
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
}

void PipelineBuilder::fetch_system_info(
		const StringName &p_system,
		const StringName &p_bundle_name,
		int p_explicit_priority,
		const LocalVector<SystemDependency> &p_extra_dependencies,
		ExecutionGraph *r_graph) {
	godex::system_id id = ECS::get_system_id(p_system);
	if (id == godex::SYSTEM_NONE) {
		r_graph->warnings.push_back(RTR("The system ") + p_system + RTR(" is invalid and it's excluded from pipeline. Check the log to know how to fix the issue."));
		ERR_FAIL_COND_MSG(id == godex::SYSTEM_NONE, "The system " + p_system + " doesn't exists.");
	}
	if (r_graph->systems[id].is_used) {
		r_graph->warnings.push_back(RTR("The system ") + p_system + RTR(" is being used twice, this is not supposed to happen. The second usage is being dropped."));
		ERR_FAIL_COND_MSG(r_graph->systems[id].is_used, "The system " + p_system + " is being used twice. Skip it.");
	}

	SystemExeInfo system_info;
	ECS::get_system_exe_info(id, system_info);

	if (!system_info.valid) {
		r_graph->warnings.push_back(RTR("The system ") + p_system + RTR(" is invalid and it's excluded from pipeline. Check the log to know how to fix the issue."));
		ERR_FAIL_COND_MSG(system_info.valid == false, "The system " + p_system + " is invalid.");
	}

	r_graph->systems[id].is_used = true;
	r_graph->systems[id].id = id;
	r_graph->systems[id].phase = ECS::get_system_phase(id);
	r_graph->systems[id].explicit_priority = p_explicit_priority;
	r_graph->systems[id].bundle_name = p_bundle_name;
	r_graph->systems[id].info = system_info;

	if (ECS::is_temporary_system(id)) {
		r_graph->temporary_systems.push_back(r_graph->systems.ptr() + id);
	}

	// Put this system inside the dispatcher that has to dispatch it.
	{
		// Put this system inside the proper dispatcher.
		Ref<ExecutionGraph::Dispatcher> dispatcher;

		const StringName dispatcher_name = ECS::get_system_dispatcher(id);
		if (dispatcher_name == StringName()) {
			// This system goes inside the main dispatcher.
			dispatcher = *r_graph->dispatchers.lookup_ptr("main");
		} else {
			Ref<ExecutionGraph::Dispatcher> *lookup_dispatcher = r_graph->dispatchers.lookup_ptr(dispatcher_name);
			if (lookup_dispatcher != nullptr) {
				dispatcher = *lookup_dispatcher;
			}
			if (dispatcher.is_null()) {
				dispatcher.instantiate();
				r_graph->dispatchers.insert(dispatcher_name, dispatcher);
			}
		}

		dispatcher->systems.push_back(r_graph->systems.ptr() + id);
	}

	// Lookup the owning dispatcher.
	if (ECS::is_system_dispatcher(id)) {
		// This system is a dispatcher, lookup it by system name.
		Ref<ExecutionGraph::Dispatcher> *dispatcher = r_graph->dispatchers.lookup_ptr(p_system);

		if (dispatcher == nullptr) {
			// The dispatcher doesn't exists yet, create it.
			r_graph->systems[id].sub_dispatcher.instantiate();
			r_graph->dispatchers.insert(p_system, r_graph->systems[id].sub_dispatcher);
		} else {
			r_graph->systems[id].sub_dispatcher = *dispatcher;
		}

		r_graph->systems[id].sub_dispatcher->dispatched_by = r_graph->systems.ptr() + id;
		r_graph->systems_dispatcher.push_back(r_graph->systems.ptr() + id);
	}

	// Fetches the system dependencies id.
	resolve_dependencies(id, p_extra_dependencies, r_graph);
	resolve_dependencies(id, ECS::get_system_dependencies(id), r_graph);
}

void PipelineBuilder::resolve_dependencies(
		godex::system_id id,
		const LocalVector<SystemDependency> &p_dependencies,
		ExecutionGraph *r_graph) {
	for (uint32_t i = 0; i < p_dependencies.size(); i += 1) {
		const godex::system_id dep_system_id = ECS::get_system_id(p_dependencies[i].system_name);
		ERR_CONTINUE_MSG(dep_system_id == godex::SYSTEM_NONE, "The system " + p_dependencies[i].system_name + " doesn't exists.");
		ERR_CONTINUE_MSG(ECS::get_system_dispatcher(dep_system_id) != ECS::get_system_dispatcher(id), "The system " + p_dependencies[i].system_name + " and the system " + ECS::get_system_name(id) + " are dispatcher by two differnt dispatchers, respectively: " + ECS::get_system_dispatcher(dep_system_id) + ", " + ECS::get_system_dispatcher(id) + ". This dependency is ignored.");
		if (p_dependencies[i].execute_before) {
			// The current system must be executed before the dependency.
			r_graph->systems[dep_system_id].execute_after.push_back(r_graph->systems.ptr() + id);
		} else {
			// The current system must be executed after the dependency.
			r_graph->systems[id].execute_after.push_back(r_graph->systems.ptr() + dep_system_id);
		}
	}
}

/// Check the move_before_if_possible doc, below.
bool move_after_if_possible(
		List<ExecutionGraph::SystemNode *> &r_list,
		List<ExecutionGraph::SystemNode *>::Element *p_val,
		List<ExecutionGraph::SystemNode *>::Element *p_where,
		bool r_recursive_move) {
	if (p_val == p_where) {
		return false;
	}

	// Check if the dependency is already resolved.
	{
		bool found = false;
		for (List<ExecutionGraph::SystemNode *>::Element *next = p_val->next(); next; next = next->next()) {
			if (p_where == next) {
				found = true;
				break;
			}
		}
		if (found == false) {
			// The dependency is already satisfied, nothing to do.
			return true;
		}
	}

	// Can move before?
	for (List<ExecutionGraph::SystemNode *>::Element *next = p_val->next(); next; next = next->next()) {
		// These two system must respect order only if on the same bundle.
		const bool skip_implicit_dependency =
				next->get()->bundle_name == StringName() ||
				next->get()->bundle_name != p_val->get()->bundle_name;

		if (!skip_implicit_dependency) {
			if (!next->get()->is_compatible(p_val->get(), true)) {
				// This system is incompatible no move.
				if (r_recursive_move) {
					// Try to move the system that is blocking first.
					List<ExecutionGraph::SystemNode *>::Element *already_checked = next->prev();
					if (move_after_if_possible(r_list, next, p_where, true)) {
						if (next->prev() != already_checked) {
							// next moved!
							// The blocker was moved before where, now p_val should be able to move
							// before p_where.
							// Try again by setting prev to a value already checked so we can advace
							next = already_checked;
							continue;
						}
					}
				}

				// No way to move the system.
				return false;
			}
		}

		if (p_where == next) {
			// We can move it!
			break;
		}
	}

	r_list.move_before(p_val, p_where->next());
	return true;
}

/// Moves the system in `p_val` before the system `p_where` only if possible.
/// p_val is tested with each system before moving it, if a dependency is found
/// with another system for the same bundle, this function tries to move the
/// colliding system first, and then retries with p_val.
///
/// This recursion is a lot useful when a specific system of a particular bundle
/// runs after/before the system of another bundle, in this case we can move
/// all the needed bundle systems so the bundle order is kept. If the movement
/// breaks the order within the bundle, this function fails and the movement is
/// aborted. This happens in case of a cyclick dependency.
bool move_before_if_possible(
		List<ExecutionGraph::SystemNode *> &r_list,
		List<ExecutionGraph::SystemNode *>::Element *p_val,
		List<ExecutionGraph::SystemNode *>::Element *p_where,
		bool r_recursive_move) {
	if (p_val == p_where) {
		return false;
	}

	// Check if the dependency is already resolved.
	{
		bool found = false;
		for (List<ExecutionGraph::SystemNode *>::Element *prev = p_val->prev(); prev; prev = prev->prev()) {
			if (p_where == prev) {
				found = true;
				break;
			}
		}
		if (found == false) {
			// The dependency is already satisfied, nothing to do.
			return true;
		}
	}

	// Can move before?
	for (List<ExecutionGraph::SystemNode *>::Element *prev = p_val->prev(); prev; prev = prev->prev()) {
		// These two system must respect order only if on the same bundle.
		const bool skip_implicit_dependency =
				prev->get()->bundle_name == StringName() ||
				prev->get()->bundle_name != p_val->get()->bundle_name;

		if (!skip_implicit_dependency) {
			if (!prev->get()->is_compatible(p_val->get(), true)) {
				// This system is incompatible.
				if (r_recursive_move) {
					// Try to move the system that is blocking first.
					List<ExecutionGraph::SystemNode *>::Element *already_checked = prev->next();
					if (move_before_if_possible(r_list, prev, p_where, true)) {
						if (prev->next() != already_checked) {
							// prev moved!
							// The blocker was moved before where, now p_val should be able to move
							// before p_where.
							// Try again by setting prev to a value already checked so we can advace
							prev = already_checked;
							continue;
						}
					}
				}

				// No way to move the system.
				return false;
			}
		}

		if (p_where == prev) {
			break;
		}
	}

	r_list.move_before(p_val, p_where);
	return true;
}

bool PipelineBuilder::sort_systems(ExecutionGraph *r_graph, String &r_error_msg) {
	struct SortByPriority {
		bool operator()(ExecutionGraph::SystemNode *const &p_a, ExecutionGraph::SystemNode *const &p_b) const {
			// Is `p_a` the smallest?
			if (
					p_a->bundle_name != StringName() &&
					p_a->bundle_name == p_b->bundle_name &&
					(p_a->explicit_priority != -1 || p_b->explicit_priority != -1)) {
				return p_a->explicit_priority < p_b->explicit_priority;
			} else {
				return p_a->id < p_b->id;
			}
		}
	};

	struct SortByStage {
		bool operator()(ExecutionGraph::SystemNode *const &p_a, ExecutionGraph::SystemNode *const &p_b) const {
			// Is `p_a` the smallest?

			// First verify the phase.
			if (p_a->phase > p_b->phase) {
				return false;
			} else {
				// `p_a` is the smallest.
				return true;
			}
		}
	};

	for (OAHashMap<StringName, Ref<ExecutionGraph::Dispatcher>>::Iterator d = r_graph->dispatchers.iter();
			d.valid;
			d = r_graph->dispatchers.next_iter(d)) {
		Ref<ExecutionGraph::Dispatcher> dispatcher = (*d.value);

		// Initialize the sorted list.
		for (uint32_t i = 0; i < dispatcher->systems.size(); i += 1) {
			ExecutionGraph::SystemNode *node = dispatcher->systems[i];
			if (node->is_used == false || ECS::is_temporary_system(node->id)) {
				// This system is not included.
				continue;
			}

			node->self_sorted_list = dispatcher->sorted_systems.push_back(node);
		}

		// Use inplace, otherwise the sort by dependency fails, since not all element
		// are tested.

		dispatcher->sorted_systems.sort_custom_inplace<SortByPriority>();
		dispatcher->sorted_systems.sort_custom_inplace<SortByStage>();

		// Now adjust the systems according to the explicit dependencies.
		for (uint32_t s = 0; s < dispatcher->systems.size(); s += 1) {
			for (int i = 0; i < int(dispatcher->systems[s]->execute_after.size()); i += 1) {
				if (dispatcher->systems[s]->execute_after[i]->is_used == false) {
					// System not used, nothing to do.
					continue;
				}
				if (!move_before_if_possible(
							dispatcher->sorted_systems,
							dispatcher->systems[s]->execute_after[i]->self_sorted_list,
							dispatcher->systems[s]->self_sorted_list,
							true)) {
					if (!move_after_if_possible(
								dispatcher->sorted_systems,
								dispatcher->systems[s]->self_sorted_list,
								dispatcher->systems[s]->execute_after[i]->self_sorted_list,
								true)) {
						move_before_if_possible(
								dispatcher->sorted_systems,
								dispatcher->systems[s]->execute_after[i]->self_sorted_list,
								dispatcher->systems[s]->self_sorted_list,
								true);
						move_after_if_possible(
								dispatcher->sorted_systems,
								dispatcher->systems[s]->self_sorted_list,
								dispatcher->systems[s]->execute_after[i]->self_sorted_list,
								true);
						// Possible cyclick dependency, however explicit dependency not respected.
						r_error_msg = RTR("System sorting failed, possible cyclick dependency because is impossible to resolve the following explicit dependency: The system ") + "`" + ECS::get_system_name(dispatcher->systems[s]->id) + RTR(" should be executed after: ") + "`" + ECS::get_system_name(dispatcher->systems[s]->execute_after[i]->id) + "` " + RTR("but for some reason is impossible to do so.");
						return false;
					}
				}
			}
		}
	}
	return true;
}

bool PipelineBuilder::has_cyclick_dependencies(const ExecutionGraph *r_graph, String &r_error) {
	// We can detect cyclic dependencies just by verifiying that the sorted list
	// has all the nodes dependencies correctly resolved. If one node is not
	// correctly resolved, it's a cyclic dependency.
	for (OAHashMap<StringName, Ref<ExecutionGraph::Dispatcher>>::Iterator d = r_graph->dispatchers.iter();
			d.valid;
			d = r_graph->dispatchers.next_iter(d)) {
		Ref<ExecutionGraph::Dispatcher> dispatcher = (*d.value);

		for (const List<ExecutionGraph::SystemNode *>::Element *e = dispatcher->sorted_systems.front(); e; e = e->next()) {
			for (uint32_t i = 0; i < e->get()->execute_after.size(); i += 1) {
				for (const List<ExecutionGraph::SystemNode *>::Element *sub = e->next(); sub; sub = sub->next()) {
					if (e->get()->execute_after[i] == sub->get()) {
						r_error = RTR("Detected cylic dependency between: `") + ECS::get_system_name(e->get()->id) + RTR("` and: `") + ECS::get_system_name(sub->get()->id) + "`.";
						return true;
					}
				}
			}
		}
	}
	return false;
}

void PipelineBuilder::detect_warnings_sub_dispatchers_missing(ExecutionGraph *r_graph) {
	for (OAHashMap<StringName, Ref<ExecutionGraph::Dispatcher>>::Iterator e = r_graph->dispatchers.iter();
			e.valid;
			e = r_graph->dispatchers.next_iter(e)) {
		if (e.value->is_valid()) {
			if ((*e.key) != StringName("main") && (*e.value)->dispatched_by == nullptr) {
				String system_names = "";
				for (uint32_t i = 0; i < (*e.value)->systems.size(); i += 1) {
					system_names += String(ECS::get_system_name((*e.value)->systems[i]->id)) + ", ";
				}
				r_graph->warnings.push_back(RTR("The sub dispatcher `") + (*e.key) + RTR("` is not dispatcher by any systems in this pipeline so the following systems are not being executed: ") + "[" + system_names + "]");
			}
		}
	}
}

struct GeneratedEventInfo {
	godex::component_id component_id;
	godex::system_id generated_by;
	bool operator<(const GeneratedEventInfo &p_other) const {
		return component_id < p_other.component_id;
	}
};

void internal_detect_warnings_lost_events(
		Ref<ExecutionGraph::Dispatcher> dispatcher,
		RBSet<GeneratedEventInfo> &changed_events) {
	// In this moment there aren't warning to detect.
	return;
	/* Just the old code, it's here in case we need to detect some warnings.
	for (const List<ExecutionGraph::SystemNode *>::Element *e = dispatcher->sorted_systems.front(); e; e = e->next()) {
		// This is a dispatcher, check this before continue.
		if (e->get()->is_dispatcher()) {
			internal_detect_warnings_lost_events(
					e->get()->sub_dispatcher,
					changed_events);
		}
	}
	*/
}

void PipelineBuilder::detect_warnings_lost_events(ExecutionGraph *r_graph) {
	// Detects if this pipeline is generating events that will not catched by
	// anything.

	// This list is used to detect if the changed event are leaked.
	// It iterates all the sorted systems, and insert the component ID when the
	// system fetches the changed event.
	// Afterwords, when a system modify it we mark it as `generated_by`:
	// if this event is not marked read again, it reaches the end with the
	// `generated_by` set, so we detected the leak.
	RBSet<GeneratedEventInfo> changed_events;

	Ref<ExecutionGraph::Dispatcher> dispatcher = *r_graph->dispatchers.lookup_ptr("main");
	internal_detect_warnings_lost_events(
			dispatcher,
			changed_events);

	for (RBSet<GeneratedEventInfo>::Element *changed = changed_events.front(); changed; changed = changed->next()) {
		if (changed->get().generated_by != godex::SYSTEM_NONE) {
			// The last time this component was fetched, it was done to write it.
			// In other words we are losing this changed event.
			r_graph->warnings.push_back("The CHANGED event for the component `" + ECS::get_component_name(changed->get().component_id) + "` is generated by the system `" + ECS::get_system_name(changed->get().generated_by) + "`, but afterward no systems is fetching it (using the changed filter) before the end of the pipeline, where it's destroyed forever. To fix the problem, and avoid loase events, you should add a system to fetch the event that runs after the system `" + ECS::get_system_name(changed->get().generated_by) + "`. Or open a bug report.");
		}
	}
}

void internal_build_stages(Ref<ExecutionGraph::Dispatcher> dispatcher) {
	dispatcher->stages.push_back(ExecutionGraph::StageNode());
	for (const List<ExecutionGraph::SystemNode *>::Element *e = dispatcher->sorted_systems.front(); e; e = e->next()) {
		if ((ECS::get_system_flags(e->get()->id) & EXCLUDE_PIPELINE_COMPOSITION) != 0) {
			// Exclude this system from the final pipeline building.
			continue;
		}

		if (e->get()->is_dispatcher()) {
			// This is a dispatcher, so build the stages first.
			internal_build_stages(e->get()->sub_dispatcher);
		}

		ExecutionGraph::StageNode &stage = dispatcher->stages.back()->get();
		if (stage.is_compatible(e->get())) {
			stage.systems.push_back(e->get());
		} else {
			// Incompatible, create a new stage.
			dispatcher->stages.push_back(ExecutionGraph::StageNode());
			dispatcher->stages.back()->get().systems.push_back(e->get());
		}
	}
}

void PipelineBuilder::build_stages(ExecutionGraph *r_graph) {
	Ref<ExecutionGraph::Dispatcher> dispatcher = *r_graph->dispatchers.lookup_ptr("main");
	internal_build_stages(dispatcher);
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

	for (OAHashMap<StringName, Ref<ExecutionGraph::Dispatcher>>::Iterator d = r_graph->dispatchers.iter();
			d.valid;
			d = r_graph->dispatchers.next_iter(d)) {
		Ref<ExecutionGraph::Dispatcher> dispatcher = (*d.value);

		// For each system of each stage:
		for (List<ExecutionGraph::StageNode>::Element *e = dispatcher->stages.front(); e; e = e->next()) {
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
		for (List<ExecutionGraph::StageNode>::Element *e = dispatcher->stages.front(); e; e = e->next()) {
			if (e->get().systems.size() == 0) {
				e->erase();
			}
		}
	}
}
