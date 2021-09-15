#pragma once

#include "../../../databags/frame_time.h"
#include "../../../iterators/query.h"
#include "../../../pipeline/pipeline_commands.h"
#include "../../godot/components/interpolated_transform_component.h"
#include "../components/mesh_component.h"
#include "../components/transform_component.h"

// TODO consider create a Bundle system that can be used to initialize
// all these systems in 1 shot.

class RenderingServerDatabag;
class RenderingScenarioDatabag;

/// Make sure to keep track of the main scenario so to properly assign the mesh.
/// This is a compatibility layer.
// TODO Would be cool improve this, maybe strip out completely the godot
// lifecycle and handle everything in full ECS style?
void scenario_manager_system(
		RenderingScenarioDatabag *p_scenario,
		// Taking this just to make sure this is always performed in single
		// thread, so I can access the `SceneTree` safely.
		World *p_world,
		RenderingServerDatabag *rs,
		Query<const MeshComponent> &p_query);

void interpolates_transform(
		const FrameTime *p_frame_time,
		Query<TransformComponent, InterpolatedTransformComponent> &p_query,
		PipelineCommands *p_pipeline_commands);

/// Handles the mesh lifetime. Initializes the mesh, usually this is called
/// before `MeshTransformUpdaterSystem`.
void mesh_updater_system(
		const RenderingScenarioDatabag *p_scenario,
		RenderingServerDatabag *rs,
		Query<Changed<MeshComponent>> &p_query);

/// Updates the `VisualServer` mesh transform.
void mesh_transform_updater_system(
		RenderingServerDatabag *rs,
		Query<const MeshComponent, Changed<const TransformComponent>> &p_query);
