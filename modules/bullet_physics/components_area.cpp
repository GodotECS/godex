#include "components_area.h"

#include "collision_object_bullet.h"

void BtArea::_bind_methods() {
	ECS_BIND_PROPERTY(BtArea, PropertyInfo(Variant::STRING, "enter_emitter_name", (PropertyHint)godex::PROPERTY_HINT_ECS_EVENT_EMITTER, "OverlapStart"), enter_emitter_name);
	ECS_BIND_PROPERTY(BtArea, PropertyInfo(Variant::STRING, "exit_emitter_name", (PropertyHint)godex::PROPERTY_HINT_ECS_EVENT_EMITTER, "OverlapEnd"), exit_emitter_name);
	ECS_BIND_PROPERTY_FUNC(BtArea, PropertyInfo(Variant::INT, "layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), set_layer, get_layer);
	ECS_BIND_PROPERTY_FUNC(BtArea, PropertyInfo(Variant::INT, "mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), set_mask, get_mask);
}

void BtArea::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 50 areas
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

BtArea::BtArea() {
	ghost.setUserPointer(this);

	// Used by `GodotCollisionDispatcher`
	ghost.setUserIndex(CollisionObjectBullet::TYPE_AREA);

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

const btTransform &BtArea::get_transform() const {
	return ghost.getWorldTransform();
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

void BtArea::add_new_overlap(btCollisionObject *p_object, int p_detect_frame, uint32_t p_index) {
	overlaps.push_back({ p_detect_frame, p_object });
	SWAP(overlaps[overlaps.size() - 1], overlaps[p_index]);
}

void BtArea::mark_still_overlapping(uint32_t p_overlap_index, int p_detect_frame) {
	overlaps[p_overlap_index].detect_frame = p_detect_frame;
}

uint32_t BtArea::find_overlapping_object(btCollisionObject *p_col_obj, uint32_t p_search_from) {
	for (uint32_t i = p_search_from; i < overlaps.size(); i += 1) {
		if (overlaps[i].object == p_col_obj) {
			SWAP(overlaps[p_search_from], overlaps[i]);
			return p_search_from;
		}
	}
	return UINT32_MAX;
}
