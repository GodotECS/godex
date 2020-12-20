#pragma once

/**
	@author AndreaCatania
*/

#include "modules/ecs/ecs_types.h"
#include "world.h"

/// Via this class is possible to spawn or destroy entities, add or remove
/// components.
// TODO make this also a resource so it can be queried
// TODO make this also a GDScript object so it can be also used to query.
class WorldCommands {
	friend class ECS;

	World *world = nullptr;

public:
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

	/// Adds a new component (or sets the default if already exists) to a
	/// specific Entity.
	template <class C>
	void add_component(EntityID p_entity, const C &p_data);

	/// Adds a new component using the component id and  a `Dictionary` that
	/// contains the initialization parameters.
	/// Usually this function is used to initialize the script components.
	void add_component(EntityID p_entity, uint32_t p_component_id, const Dictionary &p_data);
};

template <class C>
void WorldCommands::add_component(EntityID p_entity, const C &p_data) {
	world->add_component<C>(p_entity, p_data);
}
