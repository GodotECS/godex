#ifndef EDITORWORLDECS_H
#define EDITORWORLDECS_H

#include "editor/editor_plugin.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/text_edit.h"

class EditorNode;
class WorldECS;
class PipelineECS;
class EditorWorldECS;
class SpinBox;
class Tree;
class DispatcherPipelineView;
class ColorRect;

class PipelineElementInfoBox : public MarginContainer {
	GDCLASS(PipelineElementInfoBox, MarginContainer);

public:
	enum SystemMode {
		SYSTEM_BUNDLE,
		SYSTEM_NATIVE,
		SYSTEM_DISPATCHER,
		SYSTEM_SCRIPT,
		SYSTEM_TEMPORARY,
		SYSTEM_INVALID,
	};

private:
	EditorNode *editor = nullptr;
	EditorWorldECS *editor_world_ecs = nullptr;

	Button *remove_btn = nullptr;
	Label *system_name_lbl = nullptr;
	Label *extra_info_lbl = nullptr;
	Button *icon_btn = nullptr;

	StringName name;
	SystemMode mode = SYSTEM_INVALID;
	bool is_bundle = false;

public:
	PipelineElementInfoBox(EditorNode *p_editor, EditorWorldECS *p_editor_world_ecs);
	~PipelineElementInfoBox();

	void setup_system(const StringName &p_name, SystemMode p_mode);
	void set_extra_info(const String &p_desc);
	void set_is_bundle(bool p_bundle);

	Point2 name_global_transform() const;

	void system_remove();
};

class SystemView : public MarginContainer {
	GDCLASS(SystemView, MarginContainer);

	Label *name_lbl = nullptr;
	ColorRect *color_rect = nullptr;

public:
	SystemView();
	~SystemView();

	void set_name(const String &p_name);
	void set_bg_color(const Color &p_color);
};

class StageView : public MarginContainer {
	GDCLASS(StageView, MarginContainer);

	EditorNode *editor = nullptr;
	EditorWorldECS *editor_world_ecs = nullptr;

	Label *name_lbl = nullptr;
	VBoxContainer *box = nullptr;

public:
	StageView(EditorNode *p_editor, EditorWorldECS *p_editor_world_ecs);
	~StageView();

	void setup_stage(uint32_t p_stage_id);
	SystemView *add_system();
	DispatcherPipelineView *add_sub_dispatcher();
};

class DispatcherPipelineView : public MarginContainer {
	GDCLASS(DispatcherPipelineView, MarginContainer);

	EditorNode *editor = nullptr;
	EditorWorldECS *editor_world_ecs = nullptr;

	Ref<StyleBoxFlat> panel_style;
	VBoxContainer *box = nullptr;
	Label *dispatcher_lbl = nullptr;

public:
	DispatcherPipelineView(EditorNode *p_editor, EditorWorldECS *p_editor_world_ecs);
	~DispatcherPipelineView();

	void set_dispatcher_name(const String &p_name);
	void set_bg_color(const Color &p_color);

	StageView *add_stage();
};

class ComponentElement : public HBoxContainer {
	EditorNode *editor = nullptr;

	OptionButton *type = nullptr;
	LineEdit *name = nullptr;
	LineEdit *val = nullptr;

public:
	ComponentElement(EditorNode *p_editor, const String &p_name = "", Variant p_default = false);
	~ComponentElement();

	void init_variable(const String &p_name, Variant p_default);
};

class EditorWorldECS : public PanelContainer {
	GDCLASS(EditorWorldECS, PanelContainer);

	friend class WorldECSEditorPlugin;

	EditorNode *editor = nullptr;
	WorldECS *world_ecs = nullptr;
	Ref<PipelineECS> pipeline;

	HBoxContainer *world_ecs_sub_menu_wrap = nullptr;
	HBoxContainer *workspace_container_hb = nullptr;

	Label *node_name_lbl = nullptr;
	VBoxContainer *pipeline_panel = nullptr;
	OptionButton *pipeline_menu = nullptr;
	ConfirmationDialog *pipeline_window_confirm_remove = nullptr;

	VBoxContainer *main_container_pipeline_view = nullptr;
	VBoxContainer *pipeline_view_panel = nullptr;

	Panel *errors_warnings_panel = nullptr;
	VBoxContainer *errors_warnings_container = nullptr;

	// Rename pipeline
	AcceptDialog *pipeline_window_rename = nullptr;
	LineEdit *pipeline_name_ledit = nullptr;

	// Add system window.
	ConfirmationDialog *add_sys_window = nullptr;
	LineEdit *add_sys_search = nullptr;
	class Tree *add_sys_tree = nullptr;
	class TextEdit *add_sys_desc = nullptr;

	AcceptDialog *components_window = nullptr;
	Tree *components_tree = nullptr;
	LineEdit *component_name_le = nullptr;

public:
	EditorWorldECS(EditorNode *p_editor);

	void _notification(int p_what);
	void _filesystem_changed();

	void show_editor();
	void hide_editor();

	void set_world_ecs(WorldECS *p_world);
	void set_pipeline(Ref<PipelineECS> p_pipeline);

	void pipeline_change_name(const String &p_name);
	void pipeline_list_update();
	void pipeline_on_menu_select(int p_index);
	void pipeline_add();
	void pipeline_rename_show_window();
	void pipeline_remove_show_confirmation();
	void pipeline_remove();
	void pipeline_toggle_pipeline_view();

	void pipeline_features_update();
	void pipeline_errors_warnings_update();
	void pipeline_view_update();

	void pipeline_system_bundle_remove(const StringName &p_name);
	void pipeline_system_remove(const StringName &p_name);

	void add_sys_show();
	void add_sys_hide();
	void add_sys_update(const String &p_search = String());
	void add_sys_update_desc();
	void add_sys_add();

	void components_manage_show();
	void components_manage_on_component_select();

	void add_error(const String &p_msg);
	void add_warning(const String &p_msg);
	void clear_errors_warnings();

protected:
	void _changed_world_callback();
	void _changed_pipeline_callback();

	PipelineElementInfoBox *pipeline_panel_add_entry();
	void pipeline_panel_clear();

	DispatcherPipelineView *pipeline_view_add_dispatcher();
	void pipeline_view_clear();
	static void remove_node_and_reparent_children(Node *p_node);
};

class WorldECSEditorPlugin : public EditorPlugin {
	GDCLASS(WorldECSEditorPlugin, EditorPlugin);

	friend class PipelineElementInfoBox;
	friend class EditorWorldECS;

	EditorNode *editor = nullptr;
	EditorWorldECS *ecs_editor = nullptr;
	WorldECS *world_ecs = nullptr;

public:
	WorldECSEditorPlugin(EditorNode *p_node);
	~WorldECSEditorPlugin();

	virtual String get_name() const override { return "ECS"; }
	virtual bool has_main_screen() const override { return true; }
	virtual void edit(Object *p_object) override;
	virtual bool handles(Object *p_object) const override;
	virtual void make_visible(bool p_visible) override;
};

#endif // EDITORWORLDECS_H
