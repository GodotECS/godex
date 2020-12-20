#pragma once

/* Author: AndreaCatania */

#include "core/templates/local_vector.h"
#include "modules/ecs/components/component.h"
#include "scene/main/node.h"

class WorldECS;

class Entity : public Node {
	GDCLASS(Entity, Node);

	EntityID entity_id;
	Dictionary components_data;

	WorldECS *ecs_world = nullptr;

protected:
	static void _bind_methods();

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;

public:
	Entity();
	virtual ~Entity();

	void _notification(int p_what);

	void add_component_data(StringName p_component_name);
	void remove_component_data(StringName p_component_name);

	void set_components_data(Dictionary p_data);
	const Dictionary &get_components_data() const;

	void set_component_data_value(StringName p_component_name, StringName p_property_name, const Variant &p_value);
	Variant get_component_data_value(StringName p_component_name, StringName p_property_name) const;

private:
	void create_entity();
	void destroy_entity();
	void update_component_data();
};
