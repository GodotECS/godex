
#include "entity.h"

void Entity3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("__set_components_data", "data"), &Entity3D::set_components_data);
	ClassDB::bind_method(D_METHOD("__get_components_data"), &Entity3D::get_components_data);

	ClassDB::bind_method(D_METHOD("add_component", "component_name", "values"), &Entity3D::add_component, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("remove_component", "component_name"), &Entity3D::remove_component);
	ClassDB::bind_method(D_METHOD("has_component", "component_name"), &Entity3D::has_component);

	ClassDB::bind_method(D_METHOD("set_component_value", "component", "property", "value"), &Entity3D::set_component_value);
	ClassDB::bind_method(D_METHOD("get_component_value", "component", "property"), &Entity3D::get_component_value);

	ClassDB::bind_method(D_METHOD("clone", "world"), &Entity3D::clone);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "__component_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "__set_components_data", "__get_components_data");
}

void Entity2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("__set_components_data", "data"), &Entity2D::set_components_data);
	ClassDB::bind_method(D_METHOD("__get_components_data"), &Entity2D::get_components_data);

	ClassDB::bind_method(D_METHOD("add_component", "component_name", "values"), &Entity2D::add_component, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("remove_component", "component_name"), &Entity2D::remove_component);
	ClassDB::bind_method(D_METHOD("has_component", "component_name"), &Entity2D::has_component);

	ClassDB::bind_method(D_METHOD("set_component_value", "component", "property", "value"), &Entity2D::set_component_value);
	ClassDB::bind_method(D_METHOD("get_component_value", "component", "property"), &Entity2D::get_component_value);

	ClassDB::bind_method(D_METHOD("clone", "world"), &Entity2D::clone);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "__component_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "__set_components_data", "__get_components_data");
}
