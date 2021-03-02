#include "mesh_updater_system.h"

#include "../databags/visual_servers_databags.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

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
		// TODO add material override, once the `Changed<>` filter is integrated.
	}
}

void mesh_transform_updater_system(
		RenderingServerDatabag *rs,
		Query<const MeshComponent, Changed<const TransformComponent>> &p_query) {
	ERR_FAIL_COND_MSG(rs == nullptr, "The `RenderingServerDatabag` `Databag` is not part of this world. Add it please.");

	for (auto [mesh, transf] : p_query.space(Space::GLOBAL)) {
		if (mesh->instance != RID()) {
			rs->get_rs()->instance_set_transform(mesh->instance, transf->transform);
		}
	}
}
