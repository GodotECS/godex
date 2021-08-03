#pragma once

#include "../../components/component.h"
#include "../../storage/steady_storage.h"
#include "bt_def_type.h"
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

struct Overlap {
	// Used to know if this object is Still overlapped or not.
	int detect_frame;
	btCollisionObject *object;
};

struct BtArea {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtArea, SteadyStorage)

	enum ReloadFlags {
		/// Remove and insert the body into the world again.
		RELOAD_FLAGS_BODY = 1 << 1,
	};

private:
	btGhostObject ghost;

	uint32_t layer = 1;
	uint32_t mask = 1;

	uint32_t reload_flags = 0;

public:
	String enter_emitter_name;
	String exit_emitter_name;

	/// List of overlapping objects.
	LocalVector<Overlap> overlaps;

	/// The current space this Area is. Do not modify this.
	BtSpaceIndex __current_space = BT_SPACE_NONE;

	Vector3 area_scale = Vector3(1.0, 1.0, 1.0);

public:
	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

public:
	BtArea();

	btGhostObject *get_ghost();
	const btGhostObject *get_ghost() const;

	const btTransform &get_transform() const;

	void set_layer(uint32_t p_layer);
	uint32_t get_layer() const;

	void set_mask(uint32_t p_mask);
	uint32_t get_mask() const;

	bool need_body_reload() const;
	void reload_body(BtSpaceIndex p_index);

	void set_shape(btCollisionShape *p_shape);
	btCollisionShape *get_shape();
	const btCollisionShape *get_shape() const;

	/// Add new overlap.
	/// You can pass the position at which the Overlap is added in the array.
	/// The position may change
	void add_new_overlap(btCollisionObject *p_object, int p_detect_frame, uint32_t p_index = 0);

	/// Mark the overlap as still overlapping.
	void mark_still_overlapping(uint32_t p_overlap_index, int p_detect_frame);

	/// Find the overlapping object and returns its index.
	/// This function re-organizes the list of overlapping objects so to speedup
	/// the search.
	/// Note: after call this function any previous Index is no more valid.
	uint32_t find_overlapping_object(btCollisionObject *p_coll_obj, uint32_t p_search_from);
};
