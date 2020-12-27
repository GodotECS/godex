#pragma once

/**
	@author AndreaCatania
*/

#include "../ecs_types.h"
#include "../resources/ecs_resource.h"
#include "../storages/storage.h"
#include "core/string/string_name.h"
#include "core/templates/local_vector.h"

class Storage;
class World;

namespace godex {
class Resource;
}

/// Utility that can be used to create an entity with components.
/// You can use it in this way:
/// ```
///	World world;
///
///	world.create_entity()
///			.with(TransformComponent())
///			.with(MeshComponent());
/// ```
class EntityBuilder {
	friend class World;

	EntityID entity;
	World *world;

private:
	EntityBuilder(World *p_world);
	EntityBuilder &operator=(const EntityBuilder &) = delete;
	EntityBuilder &operator=(EntityBuilder) = delete;
	EntityBuilder(const EntityBuilder &) = delete;
	EntityBuilder() = delete;

public:
	template <class C>
	const EntityBuilder &with(const C &p_data) const;
	const EntityBuilder &with(uint32_t p_component_id, const Dictionary &p_data) const;

	operator EntityID() const {
		return entity;
	}
};

// TODO [!important!]
// TODO when access this Resource mutably the system always have to run in
// TODO single thread. This is a special condition.
// TODO [!important!]
class World : public godex::Resource {
	RESOURCE(World)

	LocalVector<Storage *> storages;
	LocalVector<godex::Resource *> resources;
	uint32_t entity_count = 0;
	EntityBuilder entity_builder = EntityBuilder(this);

public:
	World();

	/// Creates a new Entity id. You can add the components using the function
	/// `add_component`.
	EntityID create_entity_index();

	/// Creates a new Entity id and returns an `EntityBuilder`.
	/// You can use the `EntityBuilder` in this way:
	/// ```
	///	World world;
	///
	///	world.create_entity()
	///			.with(TransformComponent())
	///			.with(MeshComponent());
	/// ```
	///
	/// It's possible to get the `EntityID` just by assining the `EntityBuilder`
	/// to an `EntityID`.
	/// ```
	///	EntityID entity = world.create_entity()
	///			.with(MeshComponent());
	/// ```
	///
	/// Note: The `EntityBuilder` reference points to an internal variable.
	/// It's undefined behavior use it in any other way than the above one.
	const EntityBuilder &create_entity();

	/// Remove the entity from this World.
	void destroy_entity(EntityID p_entity);

	/// Returns the last created EntityID or UINT32_MAX.
	EntityID get_last_entity_id() const;

	/// Adds a new component (or sets the default if already exists) to a
	/// specific Entity.
	template <class C>
	void add_component(EntityID p_entity, const C &p_data);

	template <class C>
	void remove_component(EntityID p_entity);

	template <class C>
	bool has_component(EntityID p_entity) const;

	/// Adds a new component using the component id and  a `Dictionary` that
	/// contains the initialization parameters.
	/// Usually this function is used to initialize the script components.
	void add_component(EntityID p_entity, uint32_t p_component_id, const Dictionary &p_data);
	void remove_component(EntityID p_entity, uint32_t p_component_id);
	bool has_component(EntityID p_entity, uint32_t p_component_id) const;

	/// Returns the const storage pointed by the give ID.
	const Storage *get_storage(uint32_t p_storage_id) const;

	/// Returns the storage pointed by the give ID.
	Storage *get_storage(uint32_t p_storage_id);

	/// Returns the constant storage pointer.
	/// If the storage doesn't exist, returns null.
	/// If the type is wrong, this function crashes.
	template <class C>
	const TypedStorage<const C> *get_storage() const;

	/// Returns the storage pointer.
	/// If the storage doesn't exist, returns null.
	/// If the type is wrong, this function crashes.
	template <class C>
	TypedStorage<C> *get_storage();

	/// Adds a new resource or does nothing.
	template <class R>
	R &add_resource();

	void add_resource(godex::resource_id p_id);

	template <class R>
	void remove_resource();

	void remove_resource(godex::resource_id p_id);

	/// Retuns a resource pointer.
	template <class R>
	R *get_resource();

	/// Retuns a resource pointer.
	template <class R>
	const R *get_resource() const;

	/// Retuns a resource pointer.
	godex::Resource *get_resource(godex::resource_id p_id);

	/// Retuns a resource pointer.
	const godex::Resource *get_resource(godex::resource_id p_id) const;

private:
	/// Creates a new component storage into the world, if the storage
	/// already exists, does nothing.
	template <class C>
	void create_storage();
	void create_storage(uint32_t p_component_id);

	/// Destroy a component storage if exists.
	// TODO when this is called?
	template <class C>
	void destroy_storage();
	void destroy_storage(uint32_t p_component_id);
};

template <class C>
const EntityBuilder &EntityBuilder::with(const C &p_data) const {
	world->add_component(entity, p_data);
	return *this;
}

template <class C>
void World::add_component(EntityID p_entity, const C &p_data) {
	create_storage<C>();
	TypedStorage<C> *storage = get_storage<C>();
	ERR_FAIL_COND(storage == nullptr);
	storage->insert(p_entity, p_data);
}

template <class C>
void World::remove_component(EntityID p_entity) {
	remove_component(p_entity, C::get_component_id());
}

template <class C>
bool World::has_component(EntityID p_entity) const {
	return has_component(p_entity, C::get_component_id());
}

template <class C>
const TypedStorage<const C> *World::get_storage() const {
	const uint32_t id = C::get_component_id();
	if (id >= storages.size() || storages[id] == nullptr) {
		return nullptr;
	}

	return static_cast<TypedStorage<const C> *>(storages[id]);
}

template <class C>
TypedStorage<C> *World::get_storage() {
	const uint32_t id = C::get_component_id();
	return static_cast<TypedStorage<C> *>(get_storage(id));
}

template <class R>
R &World::add_resource() {
	const godex::resource_id id = R::get_resource_id();

	add_resource(id);
	// This function is never supposed to fail.
	CRASH_COND_MSG(resources[id] == nullptr, "The function `add_resource` is not supposed to fail, is this resource registered?");

	return *static_cast<R *>(resources[id]);
}

template <class R>
void World::remove_resource() {
	const godex::resource_id id = R::get_resource_id();
	remove_resource(id);
}

template <class C>
void World::create_storage() {
	create_storage(C::get_component_id());
}

template <class C>
void World::destroy_storage() {
	destroy_storage(C::get_component_id());
}

template <class R>
R *World::get_resource() {
	const godex::resource_id id = R::get_resource_id();
	godex::Resource *r = get_resource(id);

	if (unlikely(r == nullptr)) {
		return nullptr;
	}

	return static_cast<R *>(r);
}

template <class R>
const R *World::get_resource() const {
	const godex::resource_id id = R::get_resource_id();
	const godex::Resource *r = get_resource(id);

	if (unlikely(r == nullptr)) {
		return nullptr;
	}

	return static_cast<const R *>(r);
}
