#include "mesh_component.h"

void MeshComponent::_bind_methods() {
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), mesh);
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::INT, "layers", PROPERTY_HINT_LAYERS_3D_RENDER, ""), layers);
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::BOOL, "visible"), visible);
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::INT, "cast_shadow", PROPERTY_HINT_ENUM, "Off,On,Double-Sided,Shadows Only"), cast_shadow);
}
