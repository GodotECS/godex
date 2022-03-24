#include "mesh_updater_system.h"

#include "../databags/visual_servers_databags.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/world_3d.h"

void scenario_manager_system(
		RenderingScenarioDatabag *p_scenario,
		// Taking this just to make sure this is always performed in single
		// thread, so I can access the `SceneTree` safely.
		World *p_world,
		RenderingServerDatabag *rs,
		Query<const MeshComponent> &p_query) {
	RID main_scenario = SceneTree::get_singleton()->get_root()->get_world_3d()->get_scenario();
	if (p_scenario->scenario != main_scenario) {
		p_scenario->scenario = main_scenario;

		for (auto [mesh_comp] : p_query) {
			rs->get_rs()->instance_set_scenario(mesh_comp->instance, p_scenario->scenario);
		}
	}
}

// TODO use `const &`
/// Interpolates two positions using the hermite interpolation.
/// Returns the interpolated position.
Vector3 hermite_interpolate(
		const real_t t, // Interpolation parameter, goes from 0 to 1.
		const Vector3 &p_position_1,
		const Vector3 &p_position_2,
		const Vector3 &p_velocity_1,
		const Vector3 &p_velocity_2,
		const real_t p_interpolation_delta_time) {
	const real_t t2 = Math::pow(t, real_t(2.0));
	const real_t t3 = Math::pow(t, real_t(3.0));
	const real_t a = 1.0 - 3.0 * t2 + 2.0 * t3;
	const real_t b = t2 * (3.0 - 2.0 * t);
	const real_t c = p_interpolation_delta_time * t * Math::pow(t - real_t(1.0), real_t(2.0));
	const real_t d = p_interpolation_delta_time * t2 * (t - 1.0);
	return (a * p_position_1) + (b * p_position_2) + (c * p_velocity_1) + (d * p_velocity_2);
}

void interpolates_transform(
		const FrameTime *p_frame_time,
		Query<TransformComponent, InterpolatedTransformComponent> &p_query,
		PipelineCommands *p_pipeline_commands) {
	// Make sure this system is disabled so it doesn't receive the changed
	// notifications, otherwise triggered by this system.
	p_pipeline_commands->set_active_system(SNAME("BtTeleportBodies"), false);

	for (auto [transform, interpolated_transform] : p_query) {
		transform->origin = hermite_interpolate(
				p_frame_time->get_physics_interpolation_fraction(),
				interpolated_transform->previous_transform.origin,
				interpolated_transform->current_transform.origin,
				interpolated_transform->previous_linear_velocity,
				interpolated_transform->current_linear_velocity,
				p_frame_time->get_physics_delta());

		// TODO Exist a better way of interpolate two Matrix?
		const Vector3 start_scale = interpolated_transform->previous_transform.basis.get_scale();
		const Quaternion start_rotation = interpolated_transform->previous_transform.basis.get_rotation_quaternion();

		const Vector3 end_scale = interpolated_transform->current_transform.basis.get_scale();
		const Quaternion end_rotation = interpolated_transform->current_transform.basis.get_rotation_quaternion();

		transform->basis.set_quaternion_scale(
				start_rotation.slerp(
						end_rotation,
						p_frame_time->get_physics_interpolation_fraction()),
				start_scale.lerp(
						end_scale,
						p_frame_time->get_physics_interpolation_fraction()));
	}

	// Enable the system again, so it can receive notifications.
	p_pipeline_commands->set_active_system(SNAME("BtTeleportBodies"), true);
}

void mesh_updater_system(
		const RenderingScenarioDatabag *p_scenario,
		RenderingServerDatabag *rs,
		Query<Changed<MeshComponent>> &p_query) {
	ERR_FAIL_COND_MSG(p_scenario == nullptr, "The `RenderingScenarioDatabag` `Databag` is not part of this world. Add it please.");
	ERR_FAIL_COND_MSG(rs == nullptr, "The `RenderingServerDatabag` `Databag` is not part of this world. Add it please.");

	for (auto [mesh_comp] : p_query) {
		if (mesh_comp->instance == RID()) {
			// Instance the Mesh.
			RID instance = rs->get_rs()->instance_create();
			mesh_comp->instance = instance;
			rs->get_rs()->instance_set_scenario(mesh_comp->instance, p_scenario->scenario);
		}

		if (mesh_comp->mesh_rid == RID() && mesh_comp->mesh.is_valid()) {
			// Set mesh rid.
			mesh_comp->mesh_rid = mesh_comp->mesh->get_rid();
			rs->get_rs()->instance_set_base(mesh_comp->instance, mesh_comp->mesh_rid);
		} else if (mesh_comp->mesh_rid != RID() && mesh_comp->mesh.is_null()) {
			// Remove the RID.
			mesh_comp->mesh_rid = RID();
			rs->get_rs()->instance_set_base(mesh_comp->instance, RID());
		}

		rs->get_rs()->instance_set_visible(mesh_comp->instance, mesh_comp->visible);
		rs->get_rs()->instance_set_layer_mask(mesh_comp->instance, mesh_comp->layers);
		rs->get_rs()->instance_geometry_set_cast_shadows_setting(mesh_comp->instance, (RS::ShadowCastingSetting)mesh_comp->cast_shadow);
	}
}

void mesh_transform_updater_system(
		RenderingServerDatabag *rs,
		Query<const MeshComponent, Changed<const TransformComponent>> &p_query) {
	ERR_FAIL_COND_MSG(rs == nullptr, "The `RenderingServerDatabag` `Databag` is not part of this world. Add it please.");

	for (auto [mesh, transf] : p_query.space(Space::GLOBAL)) {
		if (mesh->instance != RID()) {
			rs->get_rs()->instance_set_transform(mesh->instance, *transf);
		}
	}
}
