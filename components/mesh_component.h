/** @author: AndreaCatania */

#ifndef MESH_COMPONENT_H
#define MESH_COMPONENT_H

#include "component.h"

class MeshComponent : public godex::Component {
	COMPONENT(MeshComponent, DenseVector)

public:
	MeshComponent();

protected:
	static void _bind_params();
};

#endif
