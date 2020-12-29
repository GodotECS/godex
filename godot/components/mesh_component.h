/** @author: AndreaCatania */

#ifndef MESH_COMPONENT_H
#define MESH_COMPONENT_H

#include "../../components/component.h"
#include "scene/resources/mesh.h"

class MeshComponent : public godex::Component {
	COMPONENT(MeshComponent, DenseVector)

	Ref<Mesh> mesh;
	Ref<Material> material_override;
	uint32_t layers;

	RID instance;
	RID mesh_rid;

public:
	MeshComponent();

	static void _bind_properties();

	void set_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_material_override(const Ref<Material> &p_material);
	Ref<Material> get_material_override() const;

	void set_layer_mask(uint32_t p_mask);
	uint32_t get_layer_mask() const;

	void set_instance(const RID &p_instance);
	RID get_instance() const;

	void set_mesh_rid(const RID &p_base);
	RID get_mesh_rid() const;
};

#endif
