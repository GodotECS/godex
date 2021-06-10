#include "pipeline_builder.h"

#include "../components/child.h"
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
		if (staged_system->is_dispatcher()) {
			// The staged system is a dispatcher, check each sub stages.
			for (const List<StageNode>::Element *disp_stage = staged_system->sub_dispatcher->stages.front(); disp_stage; disp_stage = disp_stage->next()) {
				if (!disp_stage->get().is_compatible(p_system)) {
					// The subdispatcher has a system that is not compatible.
					return false;
				}
			}
		}
		if (p_system->is_dispatcher()) {
			// p_system is a dispatcher check again each sub system.
			for (const List<StageNode>::Element *disp_stage = p_system->sub_dispatcher->stages.front(); disp_stage; disp_stage = disp_stage->next()) {
				if (!disp_stage->get().is_compatible(staged_system)) {
					// The subdispatcher has a system that is not compatible.
					return false;
				}
			}
		}
	}
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
			print_line("|- " + ECS::get_system_name(a->get()->id) + " SystemID: " + itos(a->get()->id));
		}
	}
	print_line("");
}

void ExecutionGraph::print_stages() const {
	ERR_FAIL_COND(dispatchers.has("main") == false);
	Ref<Dispatcher> main_dispatcher = *dispatchers.lookup_ptr("main");

	print_line("Execution Graph, stages:");
	print_line("|");
	uint32_t index = 0;
	print_stages("main", main_dispatcher, 0, index);
	print_line("");
}

void ExecutionGraph::print_stages(StringName p_dispatcher_name, const Ref<Dispatcher> p_dispatcher, uint32_t level, uint32_t &index) const {
	String padding;

	for (uint32_t i = 0; i < level; i += 1) {
		padding += " ";
	}
	print_line(padding + p_dispatcher_name);

	for (const List<StageNode>::Element *a = p_dispatcher->stages.front(); a; a = a->next(), index += 1) {
		print_line(padding + "|- #" + itos(index).lpad(2, "0"));
		for (uint32_t i = 0; i < a->get().systems.size(); i += 1) {
			if (a->get().systems[i]->is_dispatcher()) {
				print_stages(
						ECS::get_system_name(a->get().systems[i]->id),
						a->get().systems[i]->sub_dispatcher,
						level + 1,
						index);
			} else {
				print_line(padding + "  |- " + ECS::get_system_name(a->get().systems[i]->id));
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
	main.instance();
	r_graph->dispatchers.insert("main", main);

	// Initialize the system, by fetching the various dependencies.
	r_graph->systems.resize(ECS::get_systems_count());

	fetch_bundle_info(p_system_bundles, r_graph);
	for (int i = 0; i < p_systems.size(); i += 1) {
		fetch_system_info(p_systems[i], StringName(), -1, LocalVector<Dependency>(), r_graph);
	}

	sort_systems(r_graph);

	// Check if we have a cynclic dependency.
	if (has_cyclick_dependencies(r_graph)) {
		r_graph->print_sorted_systems();
		r_graph->print_stages();

		r_graph->systems_dispatcher.clear();
		r_graph->dispatchers.clear();
		r_graph->systems.clear();
		r_graph->valid = false;
		r_graph->error_msg = "This graph has a cyclick dependency and the pipeline building was discarded.";
		ERR_FAIL_MSG("[FATAL] This graph has a cyclick dependency and the pipeline building was discarded.");
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
	CRASH_COND_MSG(r_pipeline == nullptr, "The pipeline pointer must be valid.");

	r_pipeline->reset();

	// Add the temporary systems.
	{
		r_pipeline->temporary_systems_exe.resize(p_graph.temporary_systems.size());
		uint32_t index = 0;
		for (const List<ExecutionGraph::SystemNode *>::Element *system = p_graph.temporary_systems.front();
				system;
				system = system->next(), index += 1) {
			r_pipeline->temporary_systems_exe[index] = ECS::get_func_temporary_system_exe(system->get()->id);
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
				r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].systems[i].exe = stage->get().systems[i]->info.system_func;

				// Setup the phase.
				if (ECS::is_system_dispatcher(stage->get().systems[i]->id) == false) {
					// Take the events that are generated by this pipeline
					// (no sub pipelines).
					for (const Set<uint32_t>::Element *e = stage->get().systems[i]->info.mutable_components_storage.front(); e; e = e->next()) {
						if (ECS::is_component_events(e->get())) {
							// Make sure it's unique
							if (r_pipeline->dispatchers[dispatcher_index].event_generator.find(e->get()) == -1) {
								r_pipeline->dispatchers[dispatcher_index].event_generator.push_back(e->get());
							}
						}
					}

					// Mark as flush, the storages that need to be flushed at the
					// end of the `System`.
					for (const Set<uint32_t>::Element *e = stage->get().systems[i]->info.mutable_components.front(); e; e = e->next()) {
						if (ECS::storage_notify_release_write(e->get())) {
							r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write.push_back(e->get());
						}
					}
					for (const Set<uint32_t>::Element *e = stage->get().systems[i]->info.mutable_components_storage.front(); e; e = e->next()) {
						if (ECS::storage_notify_release_write(e->get())) {
							if (r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write.find(e->get()) == -1) {
								r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write.push_back(e->get());
							}
						}
					}
				}
			}

			// If set: make sure that the `Child` storage (which is the Hierachy)
			// is flushed for first.
			const int64_t child_index = r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write.find(Child::get_component_id());
			if (child_index != -1) {
				SWAP(
						r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write[child_index],
						r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write[0]);
				CRASH_COND(r_pipeline->dispatchers[dispatcher_index].exec_stages[stage_index].notify_list_release_write[0] != Child::get_component_id());
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
		const LocalVector<Dependency> &p_extra_dependencies,
		ExecutionGraph *r_graph) {
	godex::system_id id = ECS::get_system_id(p_system);
	ERR_FAIL_COND_MSG(id == godex::SYSTEM_NONE, "The system " + p_system + " doesn't exists.");
	ERR_FAIL_COND_MSG(r_graph->systems[id].is_used, "The system " + p_system + " is being used twice. Skip it.");

	SystemExeInfo system_info;
	ECS::get_system_exe_info(id, system_info);
	ERR_FAIL_COND_MSG(system_info.valid == false, "The system " + p_system + " is invalid.");

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
			Ref<ExecutionGraph::Dispatcher> *lookup_dispatcher = r_graph->dispatchers.lookup_ptr(p_system);
			if (lookup_dispatcher != nullptr) {
				dispatcher = *lookup_dispatcher;
			}
			if (dispatcher.is_null()) {
				dispatcher.instance();
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
			r_graph->systems[id].sub_dispatcher.instance();
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

void PipelineBuilder::sort_systems(ExecutionGraph *r_graph) {
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

			dispatcher->sorted_systems.push_back(node);
		}

		// Use inplace, otherwise the sort by dependency fails, since not all element
		// are tested.
		r_graph->print_sorted_systems(); // TODO remove

		dispatcher->sorted_systems.sort_custom_inplace<SortByPriority>();
		dispatcher->sorted_systems.sort_custom_inplace<SortByStage>();

		r_graph->print_sorted_systems();

		// Now adjust the systems according to the explicit dependencies.
		for (List<ExecutionGraph::SystemNode *>::Element *e = dispatcher->sorted_systems.front(); e; e = e->next()) {
			for (int i = 0; i < int(e->get()->execute_after.size()); i += 1) {
				for (List<ExecutionGraph::SystemNode *>::Element *sub = e->next(); sub; sub = sub->next()) {
					if (e->get()->execute_after[i] == sub->get()) {
						// Dependency found, let's move sub, before.
						dispatcher->sorted_systems.move_before(sub, e);
						// Restart the check from sub to make sure we check all.
						e = sub;
						// -1 so we start the `execute_after` for loop for sub.
						i = -1;
						break;
					}
				}
			}
		}

		r_graph->print_sorted_systems();
	}
}

bool PipelineBuilder::has_cyclick_dependencies(const ExecutionGraph *r_graph) {
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
						ERR_PRINT("Detected cylic dependency between: " + ECS::get_system_name(e->get()->id) + " and: " + ECS::get_system_name(sub->get()->id));
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
			if ((*e.value)->dispatched_by == nullptr) {
				String system_names = "";
				for (uint32_t i = 0; i < (*e.value)->systems.size(); i += 1) {
					system_names += String(ECS::get_system_name((*e.value)->systems[i]->id)) + ", ";
				}
				r_graph->warnings.push_back(TTR("The sub dispatcher `") + (*e.key) + TTR("` is not dispatcher by any systems in this pipeline so the following systems are not being executed: ") + "[" + system_names + "]");
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
		Set<GeneratedEventInfo> &generated_events,
		Set<GeneratedEventInfo> &changed_events) {
	for (const List<ExecutionGraph::SystemNode *>::Element *e = dispatcher->sorted_systems.front(); e; e = e->next()) {
		// Detect if this system is generating an event.
		for (Set<godex::component_id>::Element *generated_component = e->get()->info.mutable_components_storage.front(); generated_component; generated_component = generated_component->next()) {
			if (ECS::is_component_events(generated_component->get())) {
				// This system is generating an event.
				generated_events.insert({ generated_component->get(), e->get()->id });
			}

			if (changed_events.has({ generated_component->get(), godex::SYSTEM_NONE })) {
				// A system prior to this one is fetching the changes of this
				// component. Notify it.
				changed_events.erase({ generated_component->get(), godex::SYSTEM_NONE }); // TODO No way to update the existing one instead??
				changed_events.insert({ generated_component->get(), e->get()->id });
			}
		}

		// Detect if this system is fetching the event.
		for (Set<godex::component_id>::Element *fetch_component = e->get()->info.mutable_components.front(); fetch_component; fetch_component = fetch_component->next()) {
			if (ECS::is_component_events(fetch_component->get())) {
				// This system is fetching the event.
				generated_events.erase({ fetch_component->get(), godex::SYSTEM_NONE });
			}

			if (changed_events.has({ fetch_component->get(), godex::SYSTEM_NONE })) {
				// A system prior to this one is fetching the changes of this
				// component. Notify it.
				changed_events.erase({ fetch_component->get(), godex::SYSTEM_NONE }); // TODO No way to update the existing one instead??
				changed_events.insert({ fetch_component->get(), e->get()->id });
			}
		}

		for (Set<godex::component_id>::Element *fetch_component = e->get()->info.immutable_components.front(); fetch_component; fetch_component = fetch_component->next()) {
			if (ECS::is_component_events(fetch_component->get())) {
				// This system is fetching the event.
				generated_events.erase({ fetch_component->get(), godex::SYSTEM_NONE });
			}
		}

		for (Set<godex::component_id>::Element *changed = e->get()->info.need_changed.front(); changed; changed = changed->next()) {
			// Notify each changed component this system is reading.
			changed_events.erase({ changed->get(), godex::SYSTEM_NONE }); // TODO No way to update the existing one instead??
			changed_events.insert({ changed->get(), godex::SYSTEM_NONE });
		}

		// This is a dispatcher, check this before continue.
		if (e->get()->is_dispatcher()) {
			internal_detect_warnings_lost_events(
					e->get()->sub_dispatcher,
					generated_events,
					changed_events);
		}
	}
}

void PipelineBuilder::detect_warnings_lost_events(ExecutionGraph *r_graph) {
	// Detects if this pipeline is generating events that will not catched by
	// anything.

	// This Set is used to detect if the generated events are leaked.
	// It fetches all the sorted systems, and inserts the component ID when a
	// generator is found.
	// When a system reads the event (mutably / immutably) the the ID is removed
	// from this `Set`.
	// At the end, any component inside the list is a leaked event, since not system
	// read it.
	Set<GeneratedEventInfo> generated_events;

	// This list is used to detect if the changed event are leaked.
	// It iterates all the sorted systems, and insert the component ID when the
	// system fetches the changed event.
	// Afterwords, when a system modify it we mark it as `generated_by`:
	// if this event is not marked read again, it reaches the end with the
	// `generated_by` set, so we detected the leak.
	Set<GeneratedEventInfo> changed_events;

	Ref<ExecutionGraph::Dispatcher> dispatcher = *r_graph->dispatchers.lookup_ptr("main");
	internal_detect_warnings_lost_events(
			dispatcher,
			generated_events,
			changed_events);

	for (Set<GeneratedEventInfo>::Element *unfetched_event = generated_events.front(); unfetched_event; unfetched_event = unfetched_event->next()) {
		r_graph->warnings.push_back("The event `" + ECS::get_component_name(unfetched_event->get().component_id) + "` is generated by `" + ECS::get_system_name(unfetched_event->get().generated_by) + "`, but afterward no systems is fetching it before the end of the pipeline, where it's destroyed forever. To fix the problem, and avoid loase events, you should add a system to fetch the event that runs after the system `" + ECS::get_system_name(unfetched_event->get().generated_by) + "`. Or open a bug report.");
	}

	for (Set<GeneratedEventInfo>::Element *changed = changed_events.front(); changed; changed = changed->next()) {
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
