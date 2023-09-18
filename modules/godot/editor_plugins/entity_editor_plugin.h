#ifndef ENTITY_EDITOR_PLUGIN_H
#define ENTITY_EDITOR_PLUGIN_H

#include "core/templates/oa_hash_map.h"
#include "editor/editor_inspector.h"
#include "editor/editor_node.h"
#include "editor/editor_plugin.h"

class EntityEditorPlugin;
class EditorInspectorPluginEntity;
class Entity3D;
class ComponentDepot;

class EntityEditor : public VBoxContainer {
	GDCLASS(EntityEditor, VBoxContainer);

	EditorNode *editor;
	EditorInspectorPluginEntity *editor_plugin;

	Node *entity;
	OAHashMap<StringName, OAHashMap<StringName, EditorProperty *>> components_properties;

	// Add new component HUD objects.
	MenuButton *add_component_menu = nullptr;
	EditorInspectorSection *components_section = nullptr;

	static void _bind_methods();

public:
	EntityEditor(EditorInspectorPluginEntity *p_plugin, EditorNode *p_editor, Node *p_entity);
	~EntityEditor();

	void _notification(int p_what);

	void create_editors();
	void update_editors();
	void create_component_inspector(StringName p_component_name, const Ref<ComponentDepot> &p_depot, VBoxContainer *p_container);
	void update_component_inspector(StringName p_component_name);
	void _add_component_pressed(uint32_t p_index);
	void _remove_component_pressed(StringName p_component_name);
	void _property_changed(const String &p_path, const Variant &p_value, const String &p_name, bool p_changing);

	void _changed_callback();

private:
	const OAHashMap<StringName, Ref<ComponentDepot>> &entity_get_components_data() const;
	Dictionary entity_get_component_props_data(const StringName &p_component) const;
};

class EditorInspectorPluginEntity : public EditorInspectorPlugin {
	GDCLASS(EditorInspectorPluginEntity, EditorInspectorPlugin);

	friend class EntityEditorPlugin;

	EditorNode *editor;

public:
	virtual bool can_handle(Object *p_object) override;
	virtual void parse_begin(Object *p_object) override;
};

class EntityEditorPlugin : public EditorPlugin {
	GDCLASS(EntityEditorPlugin, EditorPlugin);

	EditorNode *editor = nullptr;

public:
	EntityEditorPlugin(EditorNode *p_editor);

	virtual String get_name() const override { return "Entity"; }
};

#endif // ENTITY_EDITOR_PLUGIN_H
