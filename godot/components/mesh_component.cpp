#include "mesh_component.h"

/* Author: AndreaCatania */

MeshComponent::MeshComponent() {}

void MeshComponent::_bind_properties() {
	add_property(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), &MeshComponent::set_mesh, &MeshComponent::get_mesh);
}

void MeshComponent::set_mesh(const Ref<Mesh> &p_mesh) {
	mesh = p_mesh;
}

Ref<Mesh> MeshComponent::get_mesh() const {
	return mesh;
}
