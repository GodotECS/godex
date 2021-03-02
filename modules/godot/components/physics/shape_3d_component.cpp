#include "shape_3d_component.h"

void Shape3DComponent::_bind_methods() {
	ECS_BIND_PROPERTY(Shape3DComponent, PropertyInfo(Variant::OBJECT, "shape", PROPERTY_HINT_RESOURCE_TYPE, "Shape3D"), shape);
}
