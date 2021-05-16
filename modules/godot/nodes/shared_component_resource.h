#ifndef SHARED_COMPONENT_RESOURCE_H
#define SHARED_COMPONENT_RESOURCE_H

#include "../../../ecs.h"
#include "core/io/resource.h"

class StaticComponentDepot;

struct WorldSIDPair {
	void *world;
	godex::SID sid;

	bool operator==(const WorldSIDPair &p_other) {
		return world == p_other.world;
	}
};

class SharedComponentResource : public Resource {
	GDCLASS(SharedComponentResource, Resource)

	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_property);
	bool _get(const StringName &p_name, Variant &r_property) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	StringName component_name;

	/// Component data.
	Ref<StaticComponentDepot> depot;

	/// List of `SID` for each `World`.
	/// The `void *` is the World pointer, and it's here just as index to store
	/// the `SID`. Never use it.
	LocalVector<WorldSIDPair> sids;

public:
	SharedComponentResource();

	/// This function must be called just after a new resource is created.
	/// Note, no need to call this if you are duplicating the resource.
	void init(const StringName &p_component_name);
	bool is_init() const;

	/// Returns the `SID` for this world. Creates one if it has none yet.
	godex::SID get_sid(World *p_world);

	void set_component_name(const StringName &p_component_name);
	StringName get_component_name() const;

	virtual Ref<Resource> duplicate(bool p_subresources = false) const override;
};

#endif // SHARED_COMPONENT_RESOURCE_H
