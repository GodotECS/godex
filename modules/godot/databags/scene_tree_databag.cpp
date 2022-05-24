#include "scene_tree_databag.h"

#include "../nodes/ecs_world.h"
#include "scene/main/node.h"
#include "scene/main/window.h"

void SceneTreeDatabag::_bind_methods() {
	add_method("get_world_ecs", &SceneTreeDatabag::get_world_ecs_script);
	add_method("get_node", &SceneTreeDatabag::get_node_script);
	add_method("get_node_or_null", &SceneTreeDatabag::get_node_or_null_script);
	add_method("find_node", &SceneTreeDatabag::find_node_script);
}

WorldECS *SceneTreeDatabag::get_world_ecs() {
	return world_ecs;
}

const WorldECS *SceneTreeDatabag::get_world_ecs() const {
	return world_ecs;
}

Node *SceneTreeDatabag::get_world_ecs_script() {
	return get_world_ecs();
}

SceneTree *SceneTreeDatabag::get_scene_tree() {
	return world_ecs->get_tree();
}

const SceneTree *SceneTreeDatabag::get_scene_tree() const {
	return world_ecs->get_tree();
}

Node *SceneTreeDatabag::get_node(const NodePath &p_path) {
	return world_ecs->get_node(p_path);
}

const Node *SceneTreeDatabag::get_node(const NodePath &p_path) const {
	return world_ecs->get_node(p_path);
}

Node *SceneTreeDatabag::get_node_script(const NodePath &p_path) {
	return get_node(p_path);
}

Node *SceneTreeDatabag::get_node_or_null(const NodePath &p_path) {
	return world_ecs->get_node_or_null(p_path);
}

const Node *SceneTreeDatabag::get_node_or_null(const NodePath &p_path) const {
	return world_ecs->get_node_or_null(p_path);
}

Node *SceneTreeDatabag::get_node_or_null_script(const NodePath &p_path) {
	return get_node_or_null(p_path);
}

// TOOD: Update these to utilise the type parameter
Node *SceneTreeDatabag::find_node(const String &p_mask, bool p_recursive, bool p_owner) {
	// Using root to search the node.
	Node *root = world_ecs->get_tree()->get_root();
	return root->find_child(p_mask, p_recursive, p_owner);
}

const Node *SceneTreeDatabag::find_node(const String &p_mask, bool p_recursive, bool p_owner) const {
	// Using root to search the node.
	Node *root = world_ecs->get_tree()->get_root();
	return root->find_child(p_mask, p_recursive, p_owner);
}

Node *SceneTreeDatabag::find_node_script(const String &p_mask, bool p_recursive, bool p_owner) {
	return find_node(p_mask, p_recursive, p_owner);
}

void SceneTreeInfoDatabag::_bind_methods() {
	add_method("is_paused", &SceneTreeInfoDatabag::is_paused);
}

bool SceneTreeInfoDatabag::is_paused() const {
	return world_ecs->get_tree()->is_paused();
}
