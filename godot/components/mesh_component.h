/** @author: AndreaCatania */

#ifndef MESH_COMPONENT_H
#define MESH_COMPONENT_H

#include "../../components/component.h"
#include "scene/resources/mesh.h"

class MeshComponent : public godex::Component {
	COMPONENT(MeshComponent, DenseVector)

	Ref<Mesh> mesh;

public:
	MeshComponent();

protected:
	static void _bind_properties();

	void set_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_mesh() const;
};

#endif
