#pragma once

#include "../components/component.h"
#include "../utils/fetchers.h"
#include "core/string/string_name.h"
#include "core/templates/local_vector.h"

class World;
class ComponentDynamicExposer;

namespace godex {

/// This query is slower compared to `Query` but can be builded at runtime, so
/// that the scripts can still interact with the `World`.
/// Cache this query allow to save the time needed to lookup the components IDs,
/// so it's advised store it and use when needed.
class DynamicQuery : public GodexWorldFetcher {
	GDCLASS(DynamicQuery, GodexWorldFetcher)

	enum FetchMode {
		WITH_MODE,
		MAYBE_MODE,
		CHANGED_MODE,
		WITHOUT_MODE,
	};

	struct DynamicQueryElement {
		godex::component_id id;
		/// Used to get by component name.
		StringName name;
		bool mutability;
		FetchMode mode;
		/// Points to the entity list when the mode is sert to `Inserted`,
		/// `Changed`, `Removed`
		uint32_t entity_list_index;
	};

	bool valid = true;
	bool can_change = true;
	Space space = Space::LOCAL;
	LocalVector<DynamicQueryElement> elements;
	LocalVector<ComponentDynamicExposer> accessors;
	LocalVector<StorageBase *> storages;
	LocalVector<EntityList> entity_lists;

	World *world = nullptr;
	uint32_t iterator_index = 0;
	EntityID current_entity;
	EntitiesBuffer entities = EntitiesBuffer(0, nullptr);

	static void _bind_methods();

public:
	DynamicQuery();

	/// Set the fetch mode of this query.
	void set_space(Space p_space);

	/// Add component.
	void with_component(uint32_t p_component_id, bool p_mutable = false);
	void maybe_component(uint32_t p_component_id, bool p_mutable = false);
	void changed_component(uint32_t p_component_id, bool p_mutable = false);

	/// Excludes this component from the query.
	void not_component(uint32_t p_component_id);

	void _with_component(uint32_t p_component_id, bool p_mutable, FetchMode p_mode);

	/// Returns true if this query is valid.
	bool is_valid() const;

	/// Clear the query so this memory can be reused.
	void reset();

	uint32_t access_count() const;
	/// The returned pointer is valid only for the execution of the query.
	/// If you reset the query, copy it (move the object), this pointer is invalidated.
	Object *get_access_by_index_gd(uint32_t p_index) const;
	ComponentDynamicExposer *get_access_by_index(uint32_t p_index) const;

	virtual void get_system_info(SystemExeInfo *p_info) const override;

	void prepare_world_script(Object *p_world);
	void begin_script(Object *p_world);
	void end_script();

	virtual void prepare_world(World *p_world) override;
	virtual void initiate_process(World *p_world) override;
	virtual void conclude_process(World *p_world) override;
	virtual void release_world(World *p_world) override;
	virtual void set_active(bool p_active) override;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Iterator

	/// Advance entity
	bool next();

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Random Access
	bool script_has(uint32_t p_id) const;
	bool has(EntityID p_id) const;

	void script_fetch(uint32_t p_entity);
	void fetch(EntityID p_entity);

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Utilities
	/// Returns entity id.
	uint32_t script_get_current_entity_id() const;
	EntityID get_current_entity_id() const;
	uint32_t count() const;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Set / Get / Call
	virtual void setvar(const Variant &p_key, const Variant &p_value, bool *r_valid = nullptr) override;
	virtual Variant getvar(const Variant &p_key, bool *r_valid = nullptr) const override;

	int64_t find_element_by_name(const StringName &p_name) const;
};
} // namespace godex
