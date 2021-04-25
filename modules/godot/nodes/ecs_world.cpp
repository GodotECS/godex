
#include "ecs_world.h"

#include "../../../ecs.h"
#include "../../../pipeline/pipeline.h"
#include "../../../world/world.h"
#include "../components/transform_component.h"
#include "../databags/input_databag.h"
#include "core/templates/list.h"
#include "ecs_utilities.h"
#include "entity.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "scene/scene_string_names.h"

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
	notify_property_list_changed();
}

StringName PipelineECS::get_pipeline_name() const {
	return pipeline_name;
}

void PipelineECS::set_systems_name(Array p_system_names) {
	systems_name = p_system_names;
	notify_property_list_changed();
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

	notify_property_list_changed();
}

void PipelineECS::remove_system(const StringName &p_system_name) {
	systems_name.erase(p_system_name);
	notify_property_list_changed();
}

void PipelineECS::fetch_used_databags(Set<godex::component_id> &r_databags) const {
	for (int i = 0; i < systems_name.size(); i += 1) {
		const StringName system_name = systems_name[i];
		const godex::system_id id = ECS::get_system_id(system_name);

		SystemExeInfo info;
		ECS::get_system_exe_info(id, info);

		for (Set<uint32_t>::Element *e = info.immutable_databags.front(); e; e = e->next()) {
			r_databags.insert(e->get());
		}
		for (Set<uint32_t>::Element *e = info.mutable_databags.front(); e; e = e->next()) {
			r_databags.insert(e->get());
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

		if (ECS::is_temporary_system(id)) {
			pipeline->add_registered_temporary_system(id);
		} else {
			if (ECS::is_system_dispatcher(id)) {
				// Special treatment for systems dispatchers: Init before set.

				const StringName pipeline_name = p_associated_world->get_system_dispatchers_pipeline(system_name);
				Ref<PipelineECS> sub_pipeline = p_associated_world->find_pipeline(pipeline_name);
				ERR_CONTINUE_MSG(sub_pipeline.is_null(), "The pipeline " + pipeline_name + " is not found. It's needed to set it as sub pipeline for the system: " + system_name);
				ECS::set_system_pipeline(id, sub_pipeline->get_pipeline(p_associated_world));
			}

			pipeline->add_registered_system(id);
		}
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

	ClassDB::bind_method(D_METHOD("set_storages_config", "config"), &WorldECS::set_storages_config);
	ClassDB::bind_method(D_METHOD("get_storages_config"), &WorldECS::get_storages_config);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "active_pipeline"), "set_active_pipeline", "get_active_pipeline");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "system_dispatchers_map", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_system_dispatchers_map", "get_system_dispatchers_map");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "storages_config", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_storages_config", "get_storages_config");

	// ~~ Runtime API ~~

	ClassDB::bind_method(D_METHOD("create_entity"), &WorldECS::create_entity);

	ClassDB::bind_method(D_METHOD("destroy_entity", "entity_id"), &WorldECS::destroy_entity);

	ClassDB::bind_method(D_METHOD("create_entity_from_prefab", "entity_node"), &WorldECS::create_entity_from_prefab);

	ClassDB::bind_method(D_METHOD("add_component_by_name", "component_name", "data"), &WorldECS::add_component_by_name);
	ClassDB::bind_method(D_METHOD("add_component", "component_id", "data"), &WorldECS::add_component);

	ClassDB::bind_method(D_METHOD("remove_component_by_name", "component_name", "data"), &WorldECS::remove_component_by_name);
	ClassDB::bind_method(D_METHOD("remove_component", "component_id", "data"), &WorldECS::remove_component);

	ClassDB::bind_method(D_METHOD("get_entity_component_by_name", "entity_id", "component_name"), &WorldECS::get_entity_component_by_name);
	ClassDB::bind_method(D_METHOD("get_entity_component", "entity_id", "component_id"), &WorldECS::get_entity_component);

	ClassDB::bind_method(D_METHOD("has_entity_component_by_name", "entity_id", "component_name"), &WorldECS::has_entity_component_by_name);
	ClassDB::bind_method(D_METHOD("has_entity_component", "entity_id", "component_id"), &WorldECS::has_entity_component);

	ClassDB::bind_method(D_METHOD("get_databag_by_name", "databag_name"), &WorldECS::get_databag_by_name);
	ClassDB::bind_method(D_METHOD("get_databag", "databag_name"), &WorldECS::get_databag);
}

bool WorldECS::_set(const StringName &p_name, const Variant &p_value) {
	Vector<String> split = String(p_name).split("/");
	ERR_FAIL_COND_V_MSG(split.size() < 1, false, "This variable name is not recognized: " + p_name);

	if (split[0] == "storages_config") {
		ERR_FAIL_COND_V_MSG(split.size() < 3, false, "This variable name is not recognized: " + p_name);

		const StringName component_name = split[1];
		const String property_name = split[2];

		ERR_FAIL_COND_V_MSG(ECS::verify_component_id(ECS::get_component_id(component_name)) == false, false, "This component " + component_name + " doesn't exist.");

		if (world->storages_config.has(component_name) == false) {
			world->storages_config[component_name] = Dictionary();
		}
		world->storages_config.getptr(component_name)->operator Dictionary()[property_name] = p_value;
		return true;

	} else if (split[0] == "pipelines") {
		ERR_FAIL_COND_V_MSG(split.size() < 2, false, "This variable name is not recognized: " + p_name);

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
	return false;
}

bool WorldECS::_get(const StringName &p_name, Variant &r_ret) const {
	Vector<String> split = String(p_name).split("/");
	ERR_FAIL_COND_V_MSG(split.size() < 1, false, "This variable name is not recognized: " + p_name);

	if (split[0] == "storages_config") {
		ERR_FAIL_COND_V_MSG(split.size() < 3, false, "This variable name is not recognized: " + p_name);

		const StringName component_name = split[1];
		const String property_name = split[2];

		ERR_FAIL_COND_V_MSG(ECS::verify_component_id(ECS::get_component_id(component_name)) == false, false, "This component " + component_name + " doesn't exist.");

		if (world->storages_config.get(component_name, Dictionary()).operator Dictionary().has(property_name)) {
			r_ret = world->storages_config[component_name].operator Dictionary()[property_name];
			return true;
		}

		Dictionary storage_config;
		ECS::get_storage_config(ECS::get_component_id(component_name), storage_config);
		if (storage_config.has(property_name)) {
			r_ret = storage_config[property_name];
			return true;
		}

		return false;

	} else if (split[0] == "pipelines") {
		ERR_FAIL_COND_V_MSG(split.size() < 2, false, "This variable name is not recognized: " + p_name);
		const int index = find_pipeline_index(split[1]);

		if (index == -1) {
			return false;
		} else {
			r_ret = pipelines[index];
			return true;
		}
	}

	return false;
}

void WorldECS::_get_property_list(List<PropertyInfo> *p_list) const {
	// Get storage settings for C++ components.
	for (godex::component_id i = 0; i < ECS::get_components_count(); i += 1) {
		const StringName name = ECS::get_component_name(i);

		Dictionary storage_config;
		ECS::get_storage_config(i, storage_config);
		for (int key_index = 0; key_index < storage_config.size(); key_index += 1) {
			const String prop_name = storage_config.get_key_at_index(key_index);
			const Variant def = storage_config.get_value_at_index(key_index);
			p_list->push_back(PropertyInfo(def.get_type(), "storages_config/" + name + "/" + prop_name));
		}
	}

	for (int i = 0; i < pipelines.size(); i += 1) {
		p_list->push_back(PropertyInfo(Variant::OBJECT, "pipelines/" + pipelines[i]->get_pipeline_name(), PROPERTY_HINT_RESOURCE_TYPE, "PipelineECS", PROPERTY_USAGE_STORAGE));
	}
}

void WorldECS::_notification(int p_what) {
	switch (p_what) {
		case ECS::NOTIFICATION_ECS_WORLD_POST_PROCESS: {
			// The process is done, we can now sync the transform back to the
			// entities.
			sync_3d_transforms();
			sync_2d_transforms();
		} break;
		case NOTIFICATION_READY:
			// Make sure to register all scripted components/databags/systems
			// at this point.
			EditorEcs::register_runtime_scripts();

			add_to_group("_world_ecs");
			if (Engine::get_singleton()->is_editor_hint() == false) {
				active_world();
			}

			get_viewport()->get_base_window()->connect(
					SceneStringNames::get_singleton()->window_input,
					callable_mp(this, &WorldECS::on_input));
			break;
		case ECS::NOTIFICATION_ECS_WORLD_UNLOADED:
			if (want_to_activate) {
				active_world();
			}
			break;
		case NOTIFICATION_EXIT_TREE:
			get_viewport()->get_base_window()->disconnect(
					SceneStringNames::get_singleton()->window_input,
					callable_mp(this, &WorldECS::on_input));

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
	//ERR_FAIL_COND_V_MSG(is_active, nullptr, "This World is active, so you can manipulate the world through `ECS::get_singleton()->get_commands()`.");
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
	notify_property_list_changed();
}

void WorldECS::remove_pipeline(Ref<PipelineECS> p_pipeline) {
	pipelines.erase(p_pipeline);
	notify_property_list_changed();
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

void WorldECS::set_storages_config(Dictionary p_config) {
	world->storages_config = p_config;
}

Dictionary WorldECS::get_storages_config() const {
	return world->storages_config;
}

void WorldECS::active_world() {
	ERR_FAIL_COND_MSG(is_inside_tree() == false, "This WorldECS is not in tree, you can't activate it.");
	if (ECS::get_singleton()->has_active_world() == false) {
		// ~~ World activation ~~

		// Set as active world.
		ECS::get_singleton()->set_active_world(world, this);

		// Set the pipeline.
		Ref<PipelineECS> pip = find_pipeline(active_pipeline);
		if (pip.is_valid()) {
			ECS::get_singleton()->set_active_world_pipeline(pip->get_pipeline(this));
		}

		// Mark as active
		is_active = true;
		want_to_activate = false;

	} else {
		is_active = false;
		want_to_activate = true;
		ERR_FAIL_MSG("Only one WorldECS is allowed at a time.");
	}
}

void WorldECS::unactive_world() {
	if (is_active) {
		is_active = false;
		want_to_activate = false;
		ECS::get_singleton()->set_active_world(nullptr, nullptr);
	}
}

uint32_t WorldECS::create_entity() {
	CRASH_COND_MSG(world == nullptr, "The world is never nullptr.");
	return world->create_entity_index();
}

void WorldECS::destroy_entity(uint32_t p_entity_id) {
	CRASH_COND_MSG(world == nullptr, "The world is never nullptr.");
	return world->destroy_entity(p_entity_id);
}

uint32_t WorldECS::create_entity_from_prefab(Object *p_entity) {
	CRASH_COND_MSG(world == nullptr, "The world is never nullptr.");

	const Entity3D *entity = cast_to<Entity3D>(p_entity);
	ERR_FAIL_COND_V_MSG(entity == nullptr, UINT32_MAX, "The passed object is not an `Entity` `Node`.");

	return entity->_create_entity(world);
}

void WorldECS::add_component_by_name(uint32_t entity_id, const StringName &p_component_name, const Dictionary &p_data) {
	add_component(entity_id, ECS::get_component_id(p_component_name), p_data);
}

void WorldECS::add_component(uint32_t entity_id, uint32_t p_component_id, const Dictionary &p_data) {
	CRASH_COND_MSG(world == nullptr, "The world is never nullptr.");
	ERR_FAIL_COND_MSG(ECS::verify_component_id(p_component_id) == false, "The passed component is not valid.");
	world->add_component(entity_id, p_component_id, p_data);
}

void WorldECS::remove_component_by_name(uint32_t entity_id, const StringName &p_component_name) {
	remove_component(entity_id, ECS::get_component_id(p_component_name));
}

void WorldECS::remove_component(uint32_t entity_id, uint32_t p_component_id) {
	CRASH_COND_MSG(world == nullptr, "The world is never nullptr.");
	ERR_FAIL_COND_MSG(ECS::verify_component_id(p_component_id) == false, "The passed component is not valid.");
	world->remove_component(entity_id, p_component_id);
}

Object *WorldECS::get_entity_component_by_name(uint32_t entity_id, const StringName &p_component_name) {
	return get_entity_component(entity_id, ECS::get_component_id(p_component_name));
}

Object *WorldECS::get_entity_component(uint32_t entity_id, uint32_t p_component_id) {
	if (has_entity_component(entity_id, p_component_id)) {
		component_accessor.init(
				p_component_id,
				DataAccessorTargetType::Component,
				true);
		component_accessor.set_target(world->get_storage(p_component_id)->get_ptr(entity_id));
	} else {
		component_accessor.set_target(nullptr);
	}

	return &component_accessor;
}

bool WorldECS::has_entity_component_by_name(uint32_t entity_id, const StringName &p_component_name) {
	return has_entity_component(entity_id, ECS::get_component_id(p_component_name));
}

bool WorldECS::has_entity_component(uint32_t entity_id, uint32_t p_component_id) {
	CRASH_COND_MSG(world == nullptr, "The world is never nullptr.");
	ERR_FAIL_COND_V_MSG(ECS::verify_component_id(p_component_id) == false, &component_accessor, "The passed component_name is not valid.");
	if (world->get_storage(p_component_id) == nullptr) {
		return false;
	}
	return world->get_storage(p_component_id)->has(entity_id);
}

Object *WorldECS::get_databag_by_name(const StringName &p_databag_name) {
	return get_databag(ECS::get_databag_id(p_databag_name));
}

Object *WorldECS::get_databag(uint32_t p_databag_id) {
	databag_accessor.set_target(nullptr);

	CRASH_COND_MSG(world == nullptr, "The world is never nullptr.");
	ERR_FAIL_COND_V_MSG(ECS::verify_databag_id(p_databag_id) == false, &databag_accessor, "The passed `databag_name` is not valid.");

	databag_accessor.init(
			p_databag_id,
			DataAccessorTargetType::Databag,
			true);
	databag_accessor.set_target(world->get_databag(p_databag_id));

	return &databag_accessor;
}

void WorldECS::on_input(const Ref<InputEvent> &p_ev) {
#ifdef DEBUG_ENABLED
	// This function is called by the OS always at beginning of each frame, before
	// anything else.
	CRASH_COND(ECS::get_singleton()->is_dispatching());
	// The world is never nullptr since it's created in the constructor.
	CRASH_COND(world == nullptr);
#endif

	InputDatabag *input = world->get_databag<InputDatabag>();
	if (likely(input)) {
		if (input_registered_frame != Engine::get_singleton()->get_process_frames()) {
			// This is a new frame.
			input_registered_frame = Engine::get_singleton()->get_process_frames();
			input->clear_input_events();
		}

		input->add_input_event(p_ev);
	}
}

void WorldECS::sync_3d_transforms() {
#ifdef DEBUG_ENABLED
	// The world is never nullptr since it's created in the constructor.
	CRASH_COND(world == nullptr);
#endif

	List<Node *> entities;
	get_tree()->get_nodes_in_group("__sync_transform_3d", &entities);

	const Storage<const TransformComponent> *storage = std::as_const(world)->get_storage<const TransformComponent>();
	if (unlikely(storage == nullptr)) {
		// Nothing to do
		return;
	}

	for (List<Node *>::Element *e = entities.front(); e; e = e->next()) {
		// Thrust this cast <3
		Entity3D *entity_node = static_cast<Entity3D *>(e->get());
		EntityID entity = entity_node->get_entity_id();
		if (entity.is_valid() && storage->has(entity)) {
			const TransformComponent *t = storage->get(entity, Space::GLOBAL);
			entity_node->set_global_transform(t->transform);
		}
	}
}

void WorldECS::sync_2d_transforms() {
	List<Node *> entities;
	get_tree()->get_nodes_in_group("__sync_transform_2d", &entities);
	if (entities.size() > 0) {
		ERR_PRINT("Plese implement the function `WorldECS::sync_2d_transforms`.");
	}
}
