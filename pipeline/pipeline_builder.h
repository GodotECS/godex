#pragma once

#include "core/string/string_name.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"

class Pipeline;

class ExecutionGraph {
	friend class PipelineBuilder;

public:
	struct SystemNode {
	};

	struct StageNode {
		LocalVector<SystemNode *> systems;
	};

	struct PhaseNode {
		LocalVector<StageNode> stages;
	};

private:
	LocalVector<SystemNode> systems;
};

class PipelineBuilder {
	const Vector<StringName> &system_bundles;
	const Vector<StringName> &systems;

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
	PipelineBuilder();
};
