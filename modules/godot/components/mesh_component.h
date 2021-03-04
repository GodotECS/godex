#ifndef MESH_COMPONENT_H
#define MESH_COMPONENT_H

#include "../../../components/component.h"
#include "../../../storage/dense_vector_storage.h"
#include "scene/resources/mesh.h"

struct MeshComponent {
	COMPONENT(MeshComponent, DenseVectorStorage)
	static void _bind_methods();

	RID instance;
	RID mesh_rid;

	Ref<Mesh> mesh;
	Ref<Material> material_override;
	uint32_t layers = 1;
	bool visible = true;
};

#endif
