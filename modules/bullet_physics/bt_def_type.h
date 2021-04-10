#pragma once

enum BtSpaceIndex : int {
	BT_SPACE_0 = 0,
	BT_SPACE_1 = 1,
	BT_SPACE_2 = 2,
	BT_SPACE_3 = 3,

	BT_SPACE_MAX = 4,
	BT_SPACE_NONE = BT_SPACE_MAX,
};

enum BtBodyType : int {
	TYPE_AREA = 0,
	TYPE_RIGID_BODY,
	TYPE_SOFT_BODY,
	TYPE_KINEMATIC_GHOST_BODY
};
