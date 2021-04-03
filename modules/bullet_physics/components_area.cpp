#include "components_area.h"

void BtArea::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtArea, PropertyInfo(Variant::INT, "layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), set_layer, get_layer);
	ECS_BIND_PROPERTY_FUNC(BtArea, PropertyInfo(Variant::INT, "mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), set_mask, get_mask);
}

void BtArea::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 50 areas
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

BtArea::BtArea() {
	// Collision objects with a callback still have collision response
	// with dynamic rigid bodies. In order to use collision objects as
	// trigger (Area), It's necessary to disable the collision response.
	ghost.setCollisionFlags(ghost.getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
}

btGhostObject *BtArea::get_ghost() {
	return &ghost;
}

const btGhostObject *BtArea::get_ghost() const {
	return &ghost;
}

void BtArea::set_layer(uint32_t p_layer) {
	layer = p_layer;
	reload_flags |= RELOAD_FLAGS_BODY;
}

uint32_t BtArea::get_layer() const {
	return layer;
}

void BtArea::set_mask(uint32_t p_mask) {
	mask = p_mask;
	reload_flags |= RELOAD_FLAGS_BODY;
}

uint32_t BtArea::get_mask() const {
	return mask;
}

bool BtArea::need_body_reload() const {
	return reload_flags & RELOAD_FLAGS_BODY;
}

void BtArea::reload_body(BtSpaceIndex p_index) {
	__current_space = p_index;
	reload_flags &= (~RELOAD_FLAGS_BODY);
}

void BtArea::set_shape(btCollisionShape *p_shape) {
	ghost.setCollisionShape(p_shape);
}

btCollisionShape *BtArea::get_shape() {
	return ghost.getCollisionShape();
}

const btCollisionShape *BtArea::get_shape() const {
	return ghost.getCollisionShape();
}
