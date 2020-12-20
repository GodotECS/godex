#pragma once

#include "core/string/string_name.h"
#include "core/templates/local_vector.h"
#include "modules/ecs/components/component.h"

class World;

namespace godex {

/// This class is used to make sure the `Query` mutability is respected.
class AccessComponent : public Object {
	friend class DynamicQuery;

	godex::Component *component = nullptr;
	bool mut = false;

	void set_mutable(bool p_mut);

public:
	AccessComponent();

	virtual bool _setv(const StringName &p_name, const Variant &p_data) override;
	virtual bool _getv(const StringName &p_name, Variant &r_data) const override;

	bool is_mutable() const;
};

/// This query is slower compared to `Query` but can be builded at runtime, so
/// that the scripts can still interact with the `World`.
/// Cache this query allow to save the time needed to lookup the components IDs,
/// so it's advised store it and use when needed.
class DynamicQuery {
	bool valid = true;
	bool can_change = true;
	LocalVector<uint32_t> component_ids;
	LocalVector<bool> mutability;
	LocalVector<AccessComponent> access_components;
	LocalVector<Storage *> storages;

	World *world = nullptr;
	uint32_t entity_id = UINT32_MAX;

public:
	DynamicQuery();

	/// Add component.
	void add_component(uint32_t p_component_id, bool p_mutable = false);

	/// Returns true if this query is valid.
	bool is_valid() const;

	/// Build the query, it's not need call this explicitely.
	bool build();

	/// Clear the query so this memory can be reused.
	void reset();

	uint32_t access_count() const;
	/// The returned pointer is valid only for the execution of the query.
	/// If you reset the query, copy it (move the object), this pointer is invalidated.
	AccessComponent *get_access(uint32_t p_index);

	/// Start the execution of this query.
	void begin(World *p_world);

	/// Returns `false` if this query can still return the components via `get`.
	bool is_done() const;

	/// Returns entity id.
	EntityID get_current_entity_id() const;

	/// Advance entity
	void next_entity();

	/// Ends the query execution.
	void end();

	void get_system_info(SystemInfo &p_info) const;

private:
	bool has_entity(EntityID p_id) const;
	void fetch();
};

} // namespace godex
