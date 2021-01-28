#include "mesh_component.h"

/* Author: AndreaCatania */

MeshComponent::MeshComponent() {}

void MeshComponent::_bind_methods() {
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), mesh);
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial,StandardMaterial3D", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_DEFERRED_SET_RESOURCE), material_override);
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::INT, "layers", PROPERTY_HINT_LAYERS_3D_RENDER, ""), layers);
	ECS_BIND_PROPERTY(MeshComponent, PropertyInfo(Variant::BOOL, "visible"), visible);
}

void MeshComponent::set_mesh(const Ref<Mesh> &p_mesh) {
	mesh = p_mesh;
}

Ref<Mesh> MeshComponent::get_mesh() const {
	return mesh;
}

void MeshComponent::set_material_override(const Ref<Material> &p_material) {
	material_override = p_material;
}

Ref<Material> MeshComponent::get_material_override() const {
	return material_override;
}

void MeshComponent::set_layer_mask(uint32_t p_mask) {
	layers = p_mask;
}

uint32_t MeshComponent::get_layer_mask() const {
	return layers;
}

void MeshComponent::set_instance(const RID &p_instance) {
	instance = p_instance;
}

RID MeshComponent::get_instance() const {
	return instance;
}

void MeshComponent::set_mesh_rid(const RID &p_base) {
	mesh_rid = p_base;
}

RID MeshComponent::get_mesh_rid() const {
	return mesh_rid;
}
