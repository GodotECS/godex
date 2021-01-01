#pragma once

#include "../components/component.h"
#include "core/object/object.h"
#include "core/string/string_name.h"
#include "core/templates/local_vector.h"

class World;

namespace godex {

/// This query is slower compared to `Query` but can be builded at runtime, so
/// that the scripts can still interact with the `World`.
/// Cache this query allow to save the time needed to lookup the components IDs,
/// so it's advised store it and use when needed.
class DynamicQuery : public Object {
	GDCLASS(DynamicQuery, Object)

	bool valid = true;
	bool can_change = true;
	LocalVector<uint32_t> component_ids;
	LocalVector<bool> mutability;
	LocalVector<bool> required;
	LocalVector<uint32_t> reject_component_ids;
	LocalVector<DataAccessorScriptInstance<Component>> accessors;
	LocalVector<Object> accessors_obj;
	LocalVector<Storage *> storages;
	LocalVector<Storage *> reject_storages;

	World *world = nullptr;
	uint32_t entity_id = UINT32_MAX;

	static void _bind_methods();

public:
	DynamicQuery();

	/// Add component.
	void with_component(uint32_t p_component_id, bool p_mutable = false);
	void maybe_component(uint32_t p_component_id, bool p_mutable = false);

	/// Excludes this component from the query.
	void without_component(uint32_t p_component_id);

	void _with_component(uint32_t p_component_id, bool p_mutable = false, bool required = true);

	/// Returns true if this query is valid.
	bool is_valid() const;

	/// Build the query, it's not need call this explicitely.
	bool build();

	/// Clear the query so this memory can be reused.
	void reset();

	uint32_t access_count() const;
	/// The returned pointer is valid only for the execution of the query.
	/// If you reset the query, copy it (move the object), this pointer is invalidated.
	Object *get_access(uint32_t p_index);

	/// Start the execution of this query.
	void begin(World *p_world);
	void begin_script(Object *p_world);

	/// Returns `false` if this query can still return the components via `get`.
	bool is_done() const;

	/// Returns entity id.
	EntityID get_current_entity_id() const;
	uint32_t get_current_entity_id_script() const;

	/// Advance entity
	void next();

	/// Ends the query execution.
	void end();

	void get_system_info(SystemExeInfo &p_info) const;

private:
	bool has_entity(EntityID p_id) const;
	void fetch();
};

} // namespace godex
