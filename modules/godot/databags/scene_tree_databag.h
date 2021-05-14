#pragma once

#include "../../../databags/databag.h"

class WorldECS;
class SceneTree;

/// This Databag gives access to the SceneTree and allow to manipulates any
/// node, directly from a system.
/// The Systems that fetch this databag runs always in single thread, so it's
/// safe mutate the nodes.
struct SceneTreeDatabag : public godex::Databag {
	DATABAG(SceneTreeDatabag)
	friend class WorldECS;

private:
	WorldECS *world_ecs = nullptr;

public:
	static void _bind_methods();

	WorldECS *get_world_ecs();
	const WorldECS *get_world_ecs() const;
	Node *get_world_ecs_script();

	SceneTree *get_scene_tree();
	const SceneTree *get_scene_tree() const;

	Node *get_node(const NodePath &p_path);
	const Node *get_node(const NodePath &p_path) const;
	Node *get_node_script(const NodePath &p_path);

	Node *get_node_or_null(const NodePath &p_path);
	const Node *get_node_or_null(const NodePath &p_path) const;
	Node *get_node_or_null_script(const NodePath &p_path);

	Node *find_node(const String &p_mask, bool p_recursive = true, bool p_owner = true);
	const Node *find_node(const String &p_mask, bool p_recursive = true, bool p_owner = true) const;
	Node *find_node_script(const String &p_mask, bool p_recursive = true, bool p_owner = true);
};
