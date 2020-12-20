
/* Author: AndreaCatania */

#include "ecs_world.h"

#include "modules/ecs/ecs.h"
#include "modules/ecs/nodes/ecs_utilities.h"
#include "modules/ecs/pipeline/pipeline.h"
#include "modules/ecs/world/world.h"

void PipelineECS::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_pipeline_name", "name"), &PipelineECS::set_pipeline_name);
	ClassDB::bind_method(D_METHOD("get_pipeline_name"), &PipelineECS::get_pipeline_name);
	ClassDB::bind_method(D_METHOD("set_systems_name", "systems_name"), &PipelineECS::set_systems_name);
	ClassDB::bind_method(D_METHOD("get_systems_name"), &PipelineECS::get_systems_name);

	ClassDB::bind_method(D_METHOD("insert_system", "system_name", "position"), &PipelineECS::insert_system, DEFVAL(-1));

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "pipeline_name"), "set_pipeline_name", "get_pipeline_name");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "systems_name"), "set_systems_name", "get_systems_name");
}

PipelineECS::PipelineECS() {}

PipelineECS::~PipelineECS() {
	if (ECS::get_singleton()->get_active_world_pipeline() == pipeline) {
		ECS::get_singleton()->set_active_world_pipeline(nullptr);
	}

	if (pipeline) {
		memdelete(pipeline);
		pipeline = nullptr;
	}
}

void PipelineECS::set_pipeline_name(StringName p_name) {
	pipeline_name = p_name;
	_change_notify("pipeline_name");
}

StringName PipelineECS::get_pipeline_name() const {
	return pipeline_name;
}

void PipelineECS::set_systems_name(Array p_system_names) {
	systems_name = p_system_names;
	_change_notify("systems_name");
}

Array PipelineECS::get_systems_name() const {
	return systems_name;
}

void PipelineECS::insert_system(const StringName &p_system_name, uint32_t p_pos) {
	// Make sure to remove any previously declared link.
	systems_name.erase(p_system_name);

	if (p_pos == UINT32_MAX) {
		// Just push back.
		systems_name.push_back(p_system_name);
	} else {
		ERR_FAIL_INDEX_MSG(int(p_pos), systems_name.size() + 1, "The pipeline is not so big, this system: " + p_system_name + " can't be insert at this position: " + itos(p_pos));
		// Insert the system at given position.
		systems_name.insert(p_pos, p_system_name);
	}

	_change_notify("systems_name");
}

Pipeline *PipelineECS::get_pipeline() {
	// Build the pipeline.

	if (pipeline) {
		return pipeline;
	}

	pipeline = memnew(Pipeline);

	for (int i = 0; i < systems_name.size(); i += 1) {
		const StringName system_name = systems_name[i];
		const godex::system_id id = ECS::find_system_id(system_name);
		ERR_CONTINUE_MSG(id == UINT32_MAX, "The system " + system_name + " was not found.");
		pipeline->add_registered_system(ECS::get_system_info(id));
	}

	return pipeline;
}

void WorldECS::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_pipeline", "pipeline"), &WorldECS::add_pipeline);
	ClassDB::bind_method(D_METHOD("remove_pipeline", "pipeline"), &WorldECS::remove_pipeline);

	ClassDB::bind_method(D_METHOD("set_active_pipeline", "name"), &WorldECS::set_active_pipeline);
	ClassDB::bind_method(D_METHOD("get_active_pipeline"), &WorldECS::get_active_pipeline);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "active_pipeline"), "set_active_pipeline", "get_active_pipeline");
}

bool WorldECS::_set(const StringName &p_name, const Variant &p_value) {
	Vector<String> split = String(p_name).split("/");
	ERR_FAIL_COND_V_MSG(split.size() != 2, false, "This variable name is not recognized: " + p_name);

	const int index = find_pipeline_index(split[1]);

	Ref<PipelineECS> pip = p_value;
	if (pip.is_null()) {
		// Nothing to do.
		return false;
	}

	// Make sure the property name is the same.
	ERR_FAIL_COND_V(pip->get_pipeline_name() != split[1], false);

	if (index == -1) {
		pipelines.push_back(p_value);
	} else {
		pipelines.write[index] = p_value;
	}

	return true;
}

bool WorldECS::_get(const StringName &p_name, Variant &r_ret) const {
	Vector<String> split = String(p_name).split("/");
	ERR_FAIL_COND_V_MSG(split.size() != 2, false, "This variable name is not recognized: " + p_name);

	const int index = find_pipeline_index(split[1]);

	if (index == -1) {
		return false;
	} else {
		r_ret = pipelines[index];
		return true;
	}
}

void WorldECS::_get_property_list(List<PropertyInfo> *p_list) const {
	for (int i = 0; i < pipelines.size(); i += 1) {
		p_list->push_back(PropertyInfo(Variant::OBJECT, "pipelines/" + pipelines[i]->get_pipeline_name(), PROPERTY_HINT_RESOURCE_TYPE, "PipelineECS"));
	}
}

void WorldECS::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
			// TODO this should go in a better place, like at the end of the
			// engine setup: https://github.com/godotengine/godot-proposals/issues/1593
			ScriptECS::register_runtime_scripts();

			add_to_group("_world_ecs");
			if (Engine::get_singleton()->is_editor_hint() == false) {
				active_world();
			}
			break;
		case NOTIFICATION_EXIT_TREE:
			if (Engine::get_singleton()->is_editor_hint() == false) {
				unactive_world();
			}
			remove_from_group("_world_ecs");
			break;
	}
}

WorldECS::WorldECS() {
	world = memnew(World);
}

WorldECS::~WorldECS() {
	memdelete(world);
	world = nullptr;
}

World *WorldECS::get_world() const {
	ERR_FAIL_COND_V_MSG(is_active, nullptr, "This World is active, so you can manipulate the world through `ECS::get_singleton()->get_commands()`.");
	return world;
}

String WorldECS::get_configuration_warning() const {
	String warning = Node::get_configuration_warning();

	if (!is_inside_tree()) {
		return warning;
	}

	List<Node *> nodes;
	get_tree()->get_nodes_in_group("_world_ecs", &nodes);

	if (nodes.size() > 1) {
		if (!warning.empty()) {
			warning += "\n\n";
		}
		warning += TTR("Only one WorldECS is allowed per scene (or set of instanced scenes).");
	}

	return warning;
}

void WorldECS::set_pipelines(Vector<Ref<PipelineECS>> p_pipelines) {
	pipelines = p_pipelines;
}

const Vector<Ref<PipelineECS>> &WorldECS::get_pipelines() const {
	return pipelines;
}

Vector<Ref<PipelineECS>> &WorldECS::get_pipelines() {
	return pipelines;
}

void WorldECS::add_pipeline(Ref<PipelineECS> p_pipeline) {
	pipelines.push_back(p_pipeline);
	_change_notify("pipelines");
}

void WorldECS::remove_pipeline(Ref<PipelineECS> p_pipeline) {
	pipelines.erase(p_pipeline);
	_change_notify("pipelines");
}

Ref<PipelineECS> WorldECS::find_pipeline(StringName p_name) {
	const int index = find_pipeline_index(p_name);
	if (index == -1) {
		return Ref<PipelineECS>();
	} else {
		return pipelines[index];
	}
}

int WorldECS::find_pipeline_index(StringName p_name) const {
	for (int i = 0; i < pipelines.size(); i += 1) {
		if (pipelines[i]->get_pipeline_name() == p_name) {
			return i;
		}
	}
	return -1;
}

void WorldECS::set_active_pipeline(StringName p_name) {
	active_pipeline = p_name;
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (ECS::get_singleton()->get_active_world() == world) {
		Ref<PipelineECS> pip = find_pipeline(p_name);
		if (pip.is_valid()) {
			ECS::get_singleton()->set_active_world_pipeline(pip->get_pipeline());
		} else {
			ECS::get_singleton()->set_active_world_pipeline(nullptr);
		}
	}
}

StringName WorldECS::get_active_pipeline() const {
	return active_pipeline;
}

void WorldECS::active_world() {
	if (ECS::get_singleton()->has_active_world() == false) {
		ECS::get_singleton()->set_active_world(world);
		Ref<PipelineECS> pip = find_pipeline(active_pipeline);
		if (pip.is_valid()) {
			ECS::get_singleton()->set_active_world_pipeline(pip->get_pipeline());
		}
		is_active = true;

		if (ECS::get_singleton()->is_connected(
					"world_unloaded",
					callable_mp(this, &WorldECS::active_world))) {
			// Disconnects to `world_unload`: previously connected because
			// there was already another world connected.
			ECS::get_singleton()->disconnect(
					"world_unloaded",
					callable_mp(this, &WorldECS::active_world));
		}
	} else {
		// Connects to `world_unload`: so when the current one is unloaded
		// this one can be loaded.
		if (ECS::get_singleton()->is_connected(
					"world_unloaded",
					callable_mp(this, &WorldECS::active_world)) == false) {
			ECS::get_singleton()->connect("world_unloaded", callable_mp(this, &WorldECS::active_world));
		}
		ERR_FAIL_MSG("Only one WorldECS is allowed at a time.");
	}
}

void WorldECS::unactive_world() {
	// Disconnects to the eventual connected world_unload.
	if (ECS::get_singleton()->is_connected(
				"world_unloaded",
				callable_mp(this, &WorldECS::active_world)) == false) {
		ECS::get_singleton()->connect("world_unloaded", callable_mp(this, &WorldECS::active_world));
	}

	if (is_active) {
		is_active = false;
		ECS::get_singleton()->set_active_world(nullptr);
	}
}
