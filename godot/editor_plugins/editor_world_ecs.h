/* Author: AndreaCatania */

#ifndef EDITORWORLDECS_H
#define EDITORWORLDECS_H

#include "editor/editor_plugin.h"

class EditorNode;
class WorldECS;
class PipelineECS;
class EditorWorldECS;
class SpinBox;

class SystemInfoBox : public MarginContainer {
	GDCLASS(SystemInfoBox, MarginContainer);

public:
	enum SystemMode {
		SYSTEM_NATIVE,
		SYSTEM_DISPATCHER,
		SYSTEM_SCRIPT,
		SYSTEM_INVALID,
	};

private:
	EditorNode *editor = nullptr;
	EditorWorldECS *editor_world_ecs = nullptr;
	Button *position_btn = nullptr;
	SpinBox *position_input = nullptr;
	Button *remove_btn = nullptr;
	Label *system_name_lbl = nullptr;
	ItemList *system_data_list = nullptr;
	LineEdit *dispatcher_pipeline_name = nullptr;

	StringName system_name;
	SystemMode mode = SYSTEM_INVALID;

public:
	SystemInfoBox(EditorNode *p_editor, EditorWorldECS *editor_world_ecs);
	~SystemInfoBox();

	void set_position(uint32_t p_position);
	void setup_system(const StringName &p_name, SystemMode p_mode);
	void set_pipeline_dispatcher(const StringName &p_current_pipeline_name);
	void add_system_element(const String &p_name, bool is_write);

	Point2 name_global_transform() const;

	void position_btn_pressed();
	void system_position_changed(double p_value);

	void system_remove();
	void dispatcher_pipeline_change(const String &p_value);
};

class DrawLayer : public Control {
	GDCLASS(DrawLayer, Control);

public:
	EditorWorldECS *editor = nullptr;

public:
	DrawLayer();

	void _notification(int p_what);
};

class EditorWorldECS : public PanelContainer {
	GDCLASS(EditorWorldECS, PanelContainer);

	friend class DrawLayer;
	friend class WorldECSEditorPlugin;

	EditorNode *editor = nullptr;
	WorldECS *world_ecs = nullptr;
	Ref<PipelineECS> pipeline;

	DrawLayer *draw_layer = nullptr;
	Label *node_name_lbl = nullptr;
	LineEdit *pip_name_ledit = nullptr;
	VBoxContainer *pipeline_panel = nullptr;
	OptionButton *pipeline_menu = nullptr;
	ConfirmationDialog *pipeline_confirm_remove = nullptr;

	// Add system window.
	ConfirmationDialog *add_sys_window = nullptr;
	LineEdit *add_sys_search = nullptr;
	class Tree *add_sys_tree = nullptr;
	class TextEdit *add_sys_desc = nullptr;

	// Create script system window.
	ConfirmationDialog *add_script_window = nullptr;
	LineEdit *add_script_path = nullptr;
	Label *add_script_error_lbl = nullptr;

	// Resource page.
	ItemList *resource_list = nullptr;

	// Add Resource menu
	ConfirmationDialog *add_res_window = nullptr;
	LineEdit *add_res_search = nullptr;
	class Tree *add_res_tree = nullptr;

	LocalVector<SystemInfoBox *> pipeline_systems;

	bool is_pipeline_panel_dirty = false;

public:
	EditorWorldECS(EditorNode *p_editor);

	void _notification(int p_what);

	void show_editor();
	void hide_editor();

	void set_world_ecs(WorldECS *p_world);
	void set_pipeline(Ref<PipelineECS> p_pipeline);

	void draw(DrawLayer *p_draw_layer);

	void pipeline_change_name(const String &p_name);
	void pipeline_list_update();
	void pipeline_on_menu_select(int p_index);
	void pipeline_add();
	void pipeline_remove_show_confirmation();
	void pipeline_remove();
	void pipeline_panel_update();

	void pipeline_item_position_change(const StringName &p_name, uint32_t p_new_position);
	void pipeline_system_remove(const StringName &p_name);
	void pipeline_system_dispatcher_set_pipeline(const StringName &p_system_name, const StringName &p_pipeline_name);

	void add_sys_show();
	void add_sys_hide();
	void add_sys_update(const String &p_search = String());
	void add_sys_update_desc();
	void add_sys_add();

	void create_sys_show();
	void create_sys_hide();
	void add_script_do();

	void resource_list_update();

	void resource_add_show_menu();
	void resource_add_hide_menu();
	void resource_add_tree_update(const String &p_search = String());
	void resource_add_do();

protected:
	void _changed_callback(Object *p_changed, const char *p_prop) override;

	SystemInfoBox *pipeline_panel_add_system();
	void pipeline_panel_clear();
	void pipeline_panel_draw_batch(uint32_t p_start_system, uint32_t p_end_system);
};

class WorldECSEditorPlugin : public EditorPlugin {
	GDCLASS(WorldECSEditorPlugin, EditorPlugin);

	friend class SystemInfoBox;
	friend class DrawLayer;
	friend class EditorWorldECS;

	EditorNode *editor = nullptr;
	EditorWorldECS *ecs_editor = nullptr;
	WorldECS *world_ecs = nullptr;

public:
	WorldECSEditorPlugin(EditorNode *p_node);
	~WorldECSEditorPlugin();

	virtual String get_name() const override { return "WorldECS"; }
	virtual bool has_main_screen() const override { return true; }
	virtual void edit(Object *p_object) override;
	virtual bool handles(Object *p_object) const override;
	virtual void make_visible(bool p_visible) override;
};

#endif // EDITORWORLDECS_H
