#include "mesh_component.h"

/* Author: AndreaCatania */

void MeshComponent::_bind_methods() {
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), mesh);
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial,StandardMaterial3D", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_DEFERRED_SET_RESOURCE), material_override);
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::INT, "layers", PROPERTY_HINT_LAYERS_3D_RENDER, ""), layers);
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::BOOL, "visible"), visible);
}
