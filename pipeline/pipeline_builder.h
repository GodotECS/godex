#pragma once

#include "../ecs.h"
#include "core/string/string_name.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"

class Pipeline;

class ExecutionGraph {
	friend class PipelineBuilder;

public:
	struct SystemNode {
		bool is_used = false;
		godex::system_id id = godex::SYSTEM_NONE;
		Phase phase = PHASE_PROCESS;
		// Set by the bundle. When -1 the implicit priority is used instead.
		// Has more importance than the implicit priority.
		int explicit_priority = -1;
		StringName bundle_name;

		// The list of dependencies.
		LocalVector<const SystemNode *> execute_after;

		// List element pointer used to access fast.
		List<SystemNode *>::Element *self_list_element = nullptr;

		// Used by the optimizer to know the visited Nodes.
		bool optimized = false;
	};

	struct StageNode {
		LocalVector<SystemNode *> systems;

		bool is_compatible(const SystemNode *p_system) const;
	};

private:
	bool valid = false;
	String error_msg;

	/// List of all the application systems.
	LocalVector<SystemNode> systems;

	/// List of used systems sorted by implicit and explicit priority.
	List<SystemNode *> sorted_systems;

	/// List of initial temporary systems.
	List<SystemNode *> temporary_systems;

	/// List of stages. The order is important.
	List<StageNode> stages;

	// Used by the optimization phase to enstablish if move or not a System to a
	// new stage.
	real_t best_stage_size = 0;

private:
	void prepare_for_optimization();
	real_t compute_effort(uint32_t p_system_count);

public:
	void print_sorted_systems() const;
	void print_stages() const;

	bool is_valid() const;
	const String &get_error_msg() const;
	const LocalVector<SystemNode> &get_systems() const;
	const List<SystemNode *> &get_sorted_systems() const;
	const List<SystemNode *> &get_temporary_systems() const;
	const List<StageNode> &get_stages() const;
	real_t get_best_stage_size() const;
};

class PipelineBuilder {
	Vector<StringName> system_bundles;
	Vector<StringName> systems;

public:
	PipelineBuilder();
	void add_system_bundle(godex::system_bundle_id p_id);
	void add_system_bundle(const StringName &p_bundle_name);
	void add_system(godex::system_id p_id);
	void add_system(const StringName &p_name);
	void build(Pipeline &r_pipeline);

public:
	/// This method is used to build the pipeline `ExecutionGraph`.
	/// The `ExecutionGraph` is useful to visualize the pipeline composition.
	/// The `Pipeline` is constructed from the `ExecutionGraph`.
	static void build_graph(
			const Vector<StringName> &p_system_bundles,
			const Vector<StringName> &p_systems,
			ExecutionGraph *r_graph);

	/// This method is used to build the `Pipeline`. This method constructs the
	/// `ExecutionGraph` then it cooks it and builds the `Pipeline` from it.
	static void build_pipeline(
			const Vector<StringName> &p_system_bundles,
			const Vector<StringName> &p_systems,
			Pipeline *r_pipeline);

	/// This is method accepts the `ExecutionGraph` and build the pipeline from it.
	static void build_pipeline(
			const ExecutionGraph &p_graph,
			Pipeline *r_pipeline);

private:
	static void fetch_bundle_info(
			const Vector<StringName> &p_system_bundles,
			ExecutionGraph *r_graph);

	static void fetch_system_info(
			const StringName &p_system,
			const StringName &p_bundle_name,
			int p_explicit_priority,
			const LocalVector<Dependency> &p_extra_dependencies,
			ExecutionGraph *r_graph);

	static void resolve_dependencies(
			godex::system_id id,
			const LocalVector<Dependency> &p_dependencies,
			ExecutionGraph *r_graph);

	static void system_sorting(ExecutionGraph *r_graph);
	static bool has_cyclick_dependencies(const ExecutionGraph *r_graph);
	static void build_stages(ExecutionGraph *r_graph);
	static void optimize_stages(ExecutionGraph *r_graph);
};
