#include "mesh_updater_system.h"

#include "../resources/visual_servers_resource.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

void scenario_manager_system(
		RenderingScenarioResource *p_scenario,
		// Taking this just to make sure this is always performed in single
		// thread, so I can access the `SceneTree` safely.
		World *p_world) {
	ERR_FAIL_COND_MSG(p_scenario == nullptr, "The `RenderingScenarioResource` `Resource` is not part of this world. Add it please.");
	RID main_scenario = SceneTree::get_singleton()->get_root()->get_world_3d()->get_scenario();
	if (p_scenario->get_scenario() != main_scenario) {
		p_scenario->set_scenario(main_scenario);
	}
}

// TODO once the storage even system is implemented, please use the
// `Changed<MeshComponent>` filter to take only the component that really need
// to do something.
// Also, consider that `Changed` should be triggered even when the other
// component of that mesh is different.

void mesh_updater_system(
		const RenderingScenarioResource *p_scenario,
		RenderingServerResource *rs,
		Query<MeshComponent> query) {
	ERR_FAIL_COND_MSG(p_scenario == nullptr, "The `RenderingScenarioResource` `Resource` is not part of this world. Add it please.");
	ERR_FAIL_COND_MSG(rs == nullptr, "The `RenderingServerResource` `Resource` is not part of this world. Add it please.");

	while (query.is_done() == false) {
		MeshComponent *mesh_comp = std::get<MeshComponent *>(query.get());

		if (mesh_comp->get_instance() == RID()) {
			// Instance the Mesh.
			RID instance = rs->get_rs()->instance_create();
			mesh_comp->set_instance(instance);
		} else {
			// Nothing changed.
			// TODO try to never trigger this.
			WARN_PRINT_ONCE("TODO please add the `changed` system so to never enter here.");
		}

		if (mesh_comp->get_mesh_rid() == RID() && mesh_comp->get_mesh().is_valid()) {
			// Set mesh rid, at this point the `Mesh` `RID` should be ready.
			mesh_comp->set_mesh_rid(mesh_comp->get_mesh()->get_rid());
			rs->get_rs()->instance_set_base(mesh_comp->get_instance(), mesh_comp->get_mesh_rid());
			rs->get_rs()->instance_set_visible(mesh_comp->get_instance(), true);
			rs->get_rs()->instance_set_layer_mask(mesh_comp->get_instance(), mesh_comp->get_layer_mask());

		} else if (mesh_comp->get_mesh_rid() != RID() && mesh_comp->get_mesh().is_null()) {
			rs->get_rs()->instance_set_base(mesh_comp->get_instance(), mesh_comp->get_mesh_rid());

		} else {
			// Nothing changed.
			WARN_PRINT_ONCE("TODO please add the `changed` system so to never enter here.");
		}

		// TODO once the is changed system is created this should be done in
		// a separate system.
		if (p_scenario->is_changed()) {
			rs->get_rs()->instance_set_scenario(mesh_comp->get_instance(), p_scenario->get_scenario());
		}

		query.next();
	}
}

void mesh_transform_updater_system(RenderingServerResource *rs, Query<const MeshComponent, const TransformComponent> query) {
	ERR_FAIL_COND_MSG(rs == nullptr, "The `RenderingServerResource` `Resource` is not part of this world. Add it please.");

	while (query.is_done() == false) {
		auto [mesh, transf] = query.get();

		// TODO Please make sure to use the `is_changed`.
		if (mesh->get_instance() != RID() /*&& transf->is_changed()*/) {
			rs->get_rs()->instance_set_transform(mesh->get_instance(), transf->get_transform());
		}

		query.next();
	}
}
