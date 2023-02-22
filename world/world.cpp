
#include "world.h"

#include "../ecs.h"
#include "../pipeline/pipeline.h"
#include "../storage/hierarchical_storage.h"

EntityBuilder::EntityBuilder(World *p_world) :
		world(p_world) {
}

const EntityBuilder &EntityBuilder::with(uint32_t p_component_id, const Dictionary &p_data) const {
	world->add_component(entity, p_component_id, p_data);
	return *this;
}

const EntityBuilder &EntityBuilder::with(uint32_t p_component_id, godex::SID p_shared_component) const {
	world->add_shared_component(entity, p_component_id, p_shared_component);
	return *this;
}

void WorldCommands::_bind_methods() {
	add_method("create_entity", &WorldCommands::create_entity);
	add_method("destroy_deferred", &WorldCommands::destroy_deferred);
}

EntityID WorldCommands::create_entity() {
	return entity_register++;
}

void WorldCommands::destroy_deferred(EntityID p_entity) {
	garbage_list.push_back(p_entity);
}

void World::_bind_methods() {
	add_method("get_entity_from_path", &World::get_entity_from_path);
	add_method("get_entity_path", &World::get_entity_path);
}

World::World() :
		World(nullptr) {
}

World::World(WorldECS *p_world_ecs) :
		world_ecs(p_world_ecs) {
	// Add self as databag, so that the `Systems` can obtain it.
	const uint32_t size = MAX(WorldCommands::get_databag_id(), World::get_databag_id()) + 1;

	databags.resize(size);
	for (uint32_t i = 0; i < databags.size(); i += 1) {
		databags[i] = nullptr;
	}

	databags[WorldCommands::get_databag_id()] = &commands;
	databags[World::get_databag_id()] = this;

	create_storage<Child>();
}

World::~World() {
	for (uint32_t i = 0; i < storages.size(); i += 1) {
		if (storages[i]) {
			delete storages[i];
		}
	}
	for (uint32_t i = 0; i < databags.size(); i += 1) {
		if (i == World::get_databag_id() || i == WorldCommands::get_databag_id()) {
			// This is automatic memory.
			continue;
		}
		if (databags[i]) {
			memdelete(databags[i]);
		}
	}
}

EntityID World::create_entity_index() {
	return commands.create_entity();
}

const EntityBuilder &World::create_entity() {
	entity_builder.entity = create_entity_index();
	return entity_builder;
}

void World::assign_nodepath_to_entity(EntityID p_entity, const NodePath &p_path) {
	// TODO consider to convert the nodepath to a more efficient structure?
	// maybe by partizioning each node to a tree structure, it's possible to find
	// the entity much faster?
	// Like, node path: `/root/my/node1/path` & `/root/my/node2/path`
	// Converted to:
	//  root
	//   |- my
	//      |- Node1
	//          |- path = 0
	//      |- Node2
	//          |- path = 1
	entity_paths.insert(p_path, p_entity);
}

void World::destroy_entity(EntityID p_entity) {
	// Removes the components assigned to this entity.
	for (uint32_t i = 0; i < storages.size(); i += 1) {
		if (storages[i] != nullptr && storages[i]->has(p_entity)) {
			storages[i]->remove(p_entity);
		}
	}

	// TODO consider to reuse this ID.
}

EntityID World::get_entity_from_path(const NodePath &p_path) const {
	const EntityID *entity = entity_paths.lookup_ptr(p_path);
	ERR_FAIL_COND_V_MSG(entity == nullptr, EntityID(), "The path `" + p_path + "` is not assigned to any entity.");
	return *entity;
}

NodePath World::get_entity_path(EntityID p_id) const {
	for (OAHashMap<NodePath, EntityID>::Iterator it = entity_paths.iter();
			it.valid;
			it = entity_paths.next_iter(it)) {
		if ((*it.value) == p_id) {
			return *it.key;
		}
	}
	return NodePath();
}

WorldCommands &World::get_commands() {
	return commands;
}

const WorldCommands &World::get_commands() const {
	return commands;
}

void World::flush() {
	// Destroy the `Entities`.
	for (uint32_t i = 0; i < commands.garbage_list.size(); i += 1) {
		destroy_entity(commands.garbage_list[i]);
	}
	commands.garbage_list.clear();
}

void World::add_component(EntityID p_entity, uint32_t p_component_id, const Dictionary &p_data) {
	create_storage(p_component_id);
	StorageBase *storage = get_storage(p_component_id);
	ERR_FAIL_COND(storage == nullptr);
	storage->insert_dynamic(p_entity, p_data);
}

void World::remove_component(EntityID p_entity, uint32_t p_component_id) {
	StorageBase *storage = get_storage(p_component_id);
	ERR_FAIL_COND(storage == nullptr);
	storage->remove(p_entity);
}

bool World::has_component(EntityID p_entity, uint32_t p_component_id) const {
	const StorageBase *storage = get_storage(p_component_id);
	if (unlikely(storage == nullptr)) {
		return false;
	}
	return storage->has(p_entity);
}

godex::SID World::create_shared_component(uint32_t p_component_id, const Dictionary &p_component_data) {
	ERR_FAIL_COND_V_MSG(ECS::is_component_sharable(p_component_id) == false, godex::SID_NONE, "The component " + ECS::get_component_name(p_component_id) + " is not shareable.");
	create_storage(p_component_id);

	SharedStorageBase *storage = get_shared_storage(p_component_id);
	ERR_FAIL_COND_V_MSG(storage == nullptr, godex::SID_NONE, "The storage is not supposed to be `nullptr` at this point.");

	return storage->create_shared_component_dynamic(p_component_data);
}

void World::add_shared_component(EntityID p_entity, uint32_t p_component_id, godex::SID p_shared_component_id) {
	ERR_FAIL_COND_MSG(ECS::is_component_sharable(p_component_id) == false, "The component " + ECS::get_component_name(p_component_id) + " is not shareable.");

	// We can trust this cast, since we know the component is shareable.
	SharedStorageBase *storage = get_shared_storage(p_component_id);
	ERR_FAIL_COND(storage == nullptr);
	ERR_FAIL_COND_MSG(storage->has_shared_component(p_shared_component_id) == false, "The shared component ID: " + itos(p_shared_component_id) + " doesn't exists inside the storage: " + ECS::get_component_name(p_component_id) + ". This seems a bug.");

	storage->insert(p_entity, p_shared_component_id);
}

const StorageBase *World::get_storage(uint32_t p_storage_id) const {
	ERR_FAIL_COND_V_MSG(p_storage_id == UINT32_MAX, nullptr, "The component is not registered.");

	if (unlikely(p_storage_id >= storages.size())) {
		return nullptr;
	}

	return storages[p_storage_id];
}

StorageBase *World::get_storage(uint32_t p_storage_id) {
	if (unlikely(p_storage_id >= storages.size())) {
		return nullptr;
	}

	return storages[p_storage_id];
}

SharedStorageBase *World::get_shared_storage(uint32_t p_storage_id) {
	// Using `dynamic_cast` because the compiler doesn't know how to properly
	// cast this pointer, so we need to do it at runtime.
	return dynamic_cast<SharedStorageBase *>(get_storage(p_storage_id));
}

const SharedStorageBase *World::get_shared_storage(uint32_t p_storage_id) const {
	// Using `dynamic_cast` because the compiler doesn't know how to properly
	// cast this pointer, so we need to do it at runtime.
	return dynamic_cast<const SharedStorageBase *>(get_storage(p_storage_id));
}

void World::create_storage(uint32_t p_component_id) {
	if (is_dispatching_in_progress) {
		// When dispatching is in progress, the storage is already created:
		// so just skip this.
		return;
	}

	ERR_FAIL_COND_MSG(!ECS::verify_component_id(p_component_id), "The component id " + itos(p_component_id) + " is not registered.");

	if (unlikely(p_component_id >= storages.size())) {
		const uint32_t start = storages.size();
		storages.resize(p_component_id + 1);
		for (uint32_t i = start; i < storages.size(); i += 1) {
			storages[i] = nullptr;
		}
	} else if (likely(storages[p_component_id] != nullptr)) {
		// The storage exists, nothing to do.
		return;
	}

	storages[p_component_id] = ECS::create_storage(p_component_id);

	// Automatically set the hierarchy, if this is a HierarchicalStorage.
	HierarchicalStorageBase *hs = dynamic_cast<HierarchicalStorageBase *>(storages[p_component_id]);
	if (hs) {
		// Trust this that `Child` is using the `Hierarchy` storage.
		Hierarchy *hierarchy = static_cast<Hierarchy *>(get_storage<Child>());
		hierarchy->add_sub_storage(hs);
	}

	// Search the config for this storage.
	Dictionary config = storages_config.get(
			ECS::get_component_name(p_component_id),
			storages_config.get(
					String(ECS::get_component_name(p_component_id)),
					Dictionary()));

	if (config.size() == 0) {
		// At this point the config is undefined, so take the default.
		ECS::get_storage_config(p_component_id, config);
	}

	storages[p_component_id]->configure(config);
}

void World::destroy_storage(uint32_t p_component_id) {
	ERR_FAIL_UNSIGNED_INDEX_MSG(p_component_id, storages.size(), "The component storage id " + itos(p_component_id) + " is unknown; so can't be destroyed.");

	if (storages[p_component_id] == nullptr) {
		// Nothing to do.
		return;
	}

	delete storages[p_component_id];
	storages[p_component_id] = nullptr;
}

void World::create_events_storage(godex::event_id p_event_id) {
	if (is_dispatching_in_progress) {
		// When dispatching is in progress, the storage is already created:
		// so just skip this.
		return;
	}

	ERR_FAIL_COND_MSG(ECS::verify_event_id(p_event_id) == false, "The event id " + itos(p_event_id) + " is not registered.");

	if (unlikely(p_event_id >= events_storages.size())) {
		const uint32_t start = events_storages.size();
		events_storages.resize(p_event_id + 1);
		for (uint32_t i = start; i < events_storages.size(); i += 1) {
			events_storages[i] = nullptr;
		}
	} else if (likely(events_storages[p_event_id] != nullptr)) {
		// The storage exists, nothing to do.
		return;
	}

	events_storages[p_event_id] = ECS::create_events_storage(p_event_id);
}

void World::destroy_events_storage(godex::event_id p_event_id) {
	ERR_FAIL_UNSIGNED_INDEX_MSG(p_event_id, events_storages.size(), "The events storage id " + itos(p_event_id) + " is unknown; so can't be destroyed.");

	if (events_storages[p_event_id] == nullptr) {
		// Nothing to do.
		return;
	}

	ECS::destroy_events_storage(p_event_id, events_storages[p_event_id]);
	events_storages[p_event_id] = nullptr;
}

void World::create_databag(godex::databag_id p_id) {
	ERR_FAIL_COND_MSG(ECS::verify_databag_id(p_id) == false, "The databag is not registered.");
	if (unlikely(p_id == WorldCommands::get_databag_id() || p_id == World::get_databag_id())) {
		// Nothing to do.
		return;
	}

	if (p_id >= databags.size()) {
		const uint32_t start = databags.size();
		databags.resize(p_id + 1);
		for (uint32_t i = start; i < databags.size(); i += 1) {
			databags[i] = nullptr;
		}
	}

	if (databags[p_id] == nullptr) {
		databags[p_id] = ECS::create_databag(p_id);
	}
}

void World::remove_databag(godex::databag_id p_id) {
	ERR_FAIL_COND_MSG(ECS::verify_databag_id(p_id) == false, "The databag is not registered.");
	if (unlikely(p_id == WorldCommands::get_databag_id() || p_id == World::get_databag_id())) {
		// Nothing to do.
		return;
	}

	if (unlikely(p_id >= databags.size() || databags[p_id] == nullptr)) {
		// Nothing to do.
		return;
	}

	memdelete(databags[p_id]);
	databags[p_id] = nullptr;
}

godex::Databag *World::get_databag(godex::databag_id p_id) {
	CRASH_COND_MSG(p_id == UINT32_MAX, "The databag is not registered.");

	if (unlikely(p_id >= databags.size())) {
		return nullptr;
	}

	return databags[p_id];
}

const godex::Databag *World::get_databag(godex::databag_id p_id) const {
	CRASH_COND_MSG(p_id == UINT32_MAX, "The databag is not registered.");

	if (unlikely(p_id >= databags.size())) {
		return nullptr;
	}

	return databags[p_id];
}

EventStorageBase *World::get_events_storage(godex::event_id p_id) {
	CRASH_COND_MSG(p_id == UINT32_MAX, "The event is not registered.");

	if (unlikely(p_id >= events_storages.size())) {
		return nullptr;
	}

	return events_storages[p_id];
}

const EventStorageBase *World::get_events_storage(godex::event_id p_id) const {
	CRASH_COND_MSG(p_id == UINT32_MAX, "The event is not registered.");

	if (unlikely(p_id >= events_storages.size())) {
		return nullptr;
	}

	return events_storages[p_id];
}

void World::release_pipelines() {
	for (uint32_t i = 0; i < associated_pipelines.size(); i += 1) {
		Token token = associated_pipelines[i]->get_token(this);
		associated_pipelines[i]->release_world(token);
	}
	associated_pipelines.clear();
}
