#pragma once

/* Author: AndreaCatania */

#include "../components/component.h"
#include "core/templates/local_vector.h"
#include "scene/main/node.h"

class WorldECS;

class Entity : public Node {
	GDCLASS(Entity, Node);

	EntityID entity_id;
	Dictionary components_data;

protected:
	static void _bind_methods();

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;

public:
	Entity();
	virtual ~Entity();

	void _notification(int p_what);

	void set_components_data(Dictionary p_data);
	const Dictionary &get_components_data() const;

	void add_component(const StringName &p_component_name, const Dictionary &p_values);
	void remove_component(const StringName &p_component_name);
	bool has_component(const StringName &p_component_name) const;

	bool set_component_value(StringName p_component_name, StringName p_property_name, const Variant &p_value);
	Variant get_component_value(StringName p_component_name, StringName p_property_name) const;
	bool _get_component_value(StringName p_component_name, StringName p_property_name, Variant &r_ret) const;

private:
	void create_entity();
	void destroy_entity();
	void update_components_data();
};
