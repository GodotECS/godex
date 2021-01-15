
/* Author: AndreaCatania */

#include "ecs_world.h"

#include "../../ecs.h"
#include "../../pipeline/pipeline.h"
#include "../../world/world.h"
#include "ecs_utilities.h"
#include "entity.h"

void PipelineECS::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_pipeline_name", "name"), &PipelineECS::set_pipeline_name);
	ClassDB::bind_method(D_METHOD("get_pipeline_name"), &PipelineECS::get_pipeline_name);
	ClassDB::bind_method(D_METHOD("set_systems_name", "systems_name"), &PipelineECS::set_systems_name);
	ClassDB::bind_method(D_METHOD("get_systems_name"), &PipelineECS::get_systems_name);

	ClassDB::bind_method(D_METHOD("insert_system", "system_name", "position"), &PipelineECS::insert_system, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("remove_system", "system_name"), &PipelineECS::remove_system);

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

	if (p_pos >= uint32_t(systems_name.size())) {
		// Just push back.
		systems_name.push_back(p_system_name);
	} else {
		ERR_FAIL_INDEX_MSG(int(p_pos), systems_name.size() + 1, "The pipeline is not so big, this system: " + p_system_name + " can't be insert at this position: " + itos(p_pos));
		// Insert the system at given position.
		systems_name.insert(p_pos, p_system_name);
	}

	_change_notify("systems_name");
}

void PipelineECS::remove_system(const StringName &p_system_name) {
	systems_name.erase(p_system_name);
	_change_notify("systems_name");
}

void PipelineECS::fetch_used_databags(Set<godex::component_id> &r_databags) const {
	for (int i = 0; i < systems_name.size(); i += 1) {
		const StringName system_name = systems_name[i];
		const godex::system_id id = ECS::get_system_id(system_name);

		SystemExeInfo info;
		ECS::get_system_exe_info(id, info);

		for (uint32_t r = 0; r < info.immutable_databags.size(); r += 1) {
			r_databags.insert(info.immutable_databags[r]);
		}
		for (uint32_t r = 0; r < info.mutable_databags.size(); r += 1) {
			r_databags.insert(info.mutable_databags[r]);
		}
	}
}

Pipeline *PipelineECS::get_pipeline(WorldECS *p_associated_world) {
	// Build the pipeline.

	// TODO change this to `create_pipeline`? Because in this way, this pipeline
	// can't be assigned to two different world, and this is a feature to support.

	if (pipeline) {
		return pipeline;
	}

	// Build the pipeline.

	pipeline = memnew(Pipeline);

	for (int i = 0; i < systems_name.size(); i += 1) {
		const StringName system_name = systems_name[i];
		const godex::system_id id = ECS::get_system_id(system_name);

		ERR_CONTINUE_MSG(ECS::verify_system_id(id) == false, "[FATAL][FATAL][FATAL][PIPELINE-FATAL] The system " + system_name + " was not found.");

		if (ECS::is_system_dispatcher(id)) {
			// Special treatment for systems dispatchers: Init before set.

			const StringName pipeline_name = p_associated_world->get_system_dispatchers_pipeline(system_name);
			Ref<PipelineECS> sub_pipeline = p_associated_world->find_pipeline(pipeline_name);
			ERR_CONTINUE_MSG(sub_pipeline.is_null(), "The pipeline " + pipeline_name + " is not found. It's needed to set it as sub pipeline for the system: " + system_name);
			ECS::set_system_pipeline(id, sub_pipeline->get_pipeline(p_associated_world));
		}

		pipeline->add_registered_system(id);
	}

	pipeline->build();

	return pipeline;
}

void WorldECS::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_pipeline", "pipeline"), &WorldECS::add_pipeline);
	ClassDB::bind_method(D_METHOD("remove_pipeline", "pipeline"), &WorldECS::remove_pipeline);

	ClassDB::bind_method(D_METHOD("set_system_dispatchers_map", "map"), &WorldECS::set_system_dispatchers_map);
	ClassDB::bind_method(D_METHOD("get_system_dispatchers_map"), &WorldECS::get_system_dispatchers_map);
	ClassDB::bind_method(D_METHOD("set_system_dispatchers_pipeline"), &WorldECS::set_system_dispatchers_pipeline);

	ClassDB::bind_method(D_METHOD("set_active_pipeline", "name"), &WorldECS::set_active_pipeline);
	ClassDB::bind_method(D_METHOD("get_active_pipeline"), &WorldECS::get_active_pipeline);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "system_dispatchers_map"), "set_system_dispatchers_map", "get_system_dispatchers_map");
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
			// Make sure to register all scripted components/databags/systems
			// at this point.
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
		if (!warning.is_empty()) {
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

void WorldECS::set_system_dispatchers_map(Dictionary p_map) {
	system_dispatchers_map = p_map;
}

Dictionary WorldECS::get_system_dispatchers_map() const {
	return system_dispatchers_map;
}

void WorldECS::set_system_dispatchers_pipeline(const StringName &p_system_name, const StringName &p_pipeline_name) {
	system_dispatchers_map[p_system_name] = p_pipeline_name;
}

StringName WorldECS::get_system_dispatchers_pipeline(const StringName &p_system_name) {
	return system_dispatchers_map.get(p_system_name, StringName());
}

void WorldECS::set_active_pipeline(StringName p_name) {
	active_pipeline = p_name;
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (ECS::get_singleton()->get_active_world() == world) {
		Ref<PipelineECS> pip = find_pipeline(p_name);
		if (pip.is_valid()) {
			ECS::get_singleton()->set_active_world_pipeline(pip->get_pipeline(this));
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
		// ~~ World activation ~~

		// Set as active world.
		ECS::get_singleton()->set_active_world(world);

		// Make sure all the databags are loaded.
		{
			Set<godex::component_id> databag_ids;

			for (int i = 0; i < pipelines.size(); i += 1) {
				if (pipelines[i].is_null()) {
					continue;
				}
				pipelines[i]->fetch_used_databags(databag_ids);
			}

			for (Set<godex::component_id>::Element *e = databag_ids.front(); e != nullptr; e = e->next()) {
				world->add_databag(e->get());
			}
		}

		// Set the pipeline.
		Ref<PipelineECS> pip = find_pipeline(active_pipeline);
		if (pip.is_valid()) {
			ECS::get_singleton()->set_active_world_pipeline(pip->get_pipeline(this));
		}

		// Mark as active
		is_active = true;

		// Disconnects any previously connected functions.
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

void WorldECSCommands::_bind_methods() {
	ClassDB::bind_method(D_METHOD("create_entity", "world"), &WorldECSCommands::create_entity);

	ClassDB::bind_method(D_METHOD("destroy_entity", "world", "entity_id"), &WorldECSCommands::destroy_entity);

	ClassDB::bind_method(D_METHOD("create_entity_from_prefab", "world", "entity_node"), &WorldECSCommands::create_entity_from_prefab);

	ClassDB::bind_method(D_METHOD("add_component", "world", "component_name", "data"), &WorldECSCommands::add_component);
	ClassDB::bind_method(D_METHOD("add_component_by_id", "world", "component_id", "data"), &WorldECSCommands::add_component_by_id);

	ClassDB::bind_method(D_METHOD("remove_component", "world", "component_name", "data"), &WorldECSCommands::remove_component);
	ClassDB::bind_method(D_METHOD("remove_component_by_id", "world", "component_id", "data"), &WorldECSCommands::remove_component_by_id);

	ClassDB::bind_method(D_METHOD("get_entity_component", "world", "entity_id", "component_name"), &WorldECSCommands::get_entity_component);
	ClassDB::bind_method(D_METHOD("get_entity_component_by_id", "world", "entity_id", "component_id"), &WorldECSCommands::get_entity_component_by_id);

	ClassDB::bind_method(D_METHOD("get_databag", "world", "databag_name"), &WorldECSCommands::get_databag);
	ClassDB::bind_method(D_METHOD("get_databag_by_id", "world", "databag_name"), &WorldECSCommands::get_databag_by_id);
}

WorldECSCommands::WorldECSCommands() {
}

uint32_t WorldECSCommands::create_entity(Object *p_world) {
	World *world = godex::unwrap_databag<World>(p_world);
	ERR_FAIL_COND_V_MSG(world == nullptr, UINT32_MAX, "The passed variable is not a `Databag` of type `World`.");
	return world->create_entity_index();
}

void WorldECSCommands::destroy_entity(Object *p_world, uint32_t p_entity_id) {
	World *world = godex::unwrap_databag<World>(p_world);
	ERR_FAIL_COND_MSG(world == nullptr, "The passed variable is not a `Databag` of type `World`.");
	return world->destroy_entity(p_entity_id);
}

uint32_t WorldECSCommands::create_entity_from_prefab(Object *p_world, Object *p_entity) {
	const Entity *entity = cast_to<Entity>(p_entity);
	ERR_FAIL_COND_V_MSG(entity == nullptr, UINT32_MAX, "The passed object is not an `Entity` `Node`.");
	World *world = godex::unwrap_databag<World>(p_world);
	ERR_FAIL_COND_V_MSG(world == nullptr, UINT32_MAX, "The passed variable is not a `Databag` of type `World`.");

	return entity->_create_entity(world);
}

void WorldECSCommands::add_component(Object *p_world, uint32_t entity_id, const StringName &p_component_name, const Dictionary &p_data) {
	add_component_by_id(p_world, entity_id, ECS::get_component_id(p_component_name), p_data);
}

void WorldECSCommands::add_component_by_id(Object *p_world, uint32_t entity_id, uint32_t p_component_id, const Dictionary &p_data) {
	World *world = godex::unwrap_databag<World>(p_world);
	ERR_FAIL_COND_MSG(world == nullptr, "The passed variable is not a `Databag` of type `World`.");
	ERR_FAIL_COND_MSG(ECS::verify_component_id(p_component_id) == false, "The passed component is not valid.");
	world->add_component(entity_id, p_component_id, p_data);
}

void WorldECSCommands::remove_component(Object *p_world, uint32_t entity_id, const StringName &p_component_name) {
	remove_component_by_id(p_world, entity_id, ECS::get_component_id(p_component_name));
}

void WorldECSCommands::remove_component_by_id(Object *p_world, uint32_t entity_id, uint32_t p_component_id) {
	World *world = godex::unwrap_databag<World>(p_world);
	ERR_FAIL_COND_MSG(world == nullptr, "The passed variable is not a `Databag` of type `World`.");
	ERR_FAIL_COND_MSG(ECS::verify_component_id(p_component_id) == false, "The passed component is not valid.");
	world->remove_component(entity_id, p_component_id);
}

Object *WorldECSCommands::get_entity_component(Object *p_world, uint32_t entity_id, const StringName &p_component_name) {
	return get_entity_component_by_id(p_world, entity_id, ECS::get_component_id(p_component_name));
}

Object *WorldECSCommands::get_entity_component_by_id(Object *p_world, uint32_t entity_id, uint32_t p_component_id) {
	component_accessor.__target = nullptr;

	World *world = godex::unwrap_databag<World>(p_world);
	ERR_FAIL_COND_V_MSG(world == nullptr, &component_accessor, "The passed variable is not a `Databag` of type `World`.");
	ERR_FAIL_COND_V_MSG(ECS::verify_component_id(p_component_id) == false, &component_accessor, "The passed component_name is not valid.");

	component_accessor.__target = world->get_storage(p_component_id)->get_ptr(entity_id);
	component_accessor.__mut = true;

	return &component_accessor;
}

Object *WorldECSCommands::get_databag(Object *p_world, const StringName &p_databag_name) {
	return get_databag_by_id(p_world, ECS::get_databag_id(p_databag_name));
}

Object *WorldECSCommands::get_databag_by_id(Object *p_world, uint32_t p_databag_id) {
	databag_accessor.__target = nullptr;

	World *world = godex::unwrap_databag<World>(p_world);
	ERR_FAIL_COND_V_MSG(world == nullptr, &databag_accessor, "The passed variable is not a `Databag` of type `World`.");
	ERR_FAIL_COND_V_MSG(ECS::verify_databag_id(p_databag_id) == false, &databag_accessor, "The passed `databag_name` is not valid.");

	databag_accessor.__target = world->get_databag(p_databag_id);
	databag_accessor.__mut = true;

	return &databag_accessor;
}
