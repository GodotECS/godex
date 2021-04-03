#pragma once

#include "../../components/component.h"
#include "../../storage/steady_storage.h"
#include "bt_def_type.h"
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

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
	/// The current space this Area is. Do not modify this.
	BtSpaceIndex __current_space = BT_SPACE_NONE;

public:
	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

public:
	BtArea();

	btGhostObject *get_ghost();
	const btGhostObject *get_ghost() const;

	void set_layer(uint32_t p_layer);
	uint32_t get_layer() const;

	void set_mask(uint32_t p_mask);
	uint32_t get_mask() const;

	bool need_body_reload() const;
	void reload_body(BtSpaceIndex p_index);

	void set_shape(btCollisionShape *p_shape);
	btCollisionShape *get_shape();
	const btCollisionShape *get_shape() const;
};
