/* Author: AndreaCatania */

#include "editor_world_ecs.h"

#include "../ecs.h"
#include "../nodes/ecs_utilities.h"
#include "../nodes/ecs_world.h"
#include "core/io/resource_loader.h"
#include "editor/editor_node.h"
#include "editor/editor_scale.h"
#include "scene/gui/color_rect.h"

SystemInfoBox::SystemInfoBox(EditorNode *p_editor, EditorWorldECS *p_editor_world_ecs) :
		editor(p_editor),
		editor_world_ecs(p_editor_world_ecs) {
	add_theme_constant_override("margin_right", 2);
	add_theme_constant_override("margin_top", 2);
	add_theme_constant_override("margin_left", 2);
	add_theme_constant_override("margin_bottom", 2);

	ColorRect *bg = memnew(ColorRect);
	bg->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	bg->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	bg->set_color(Color(0.0, 0.0, 0.0, 0.2));
	add_child(bg);

	MarginContainer *inner_container = memnew(MarginContainer);
	inner_container->add_theme_constant_override("margin_right", 2);
	inner_container->add_theme_constant_override("margin_top", 2);
	inner_container->add_theme_constant_override("margin_left", 10);
	inner_container->add_theme_constant_override("margin_bottom", 2);
	add_child(inner_container);

	HBoxContainer *box = memnew(HBoxContainer);
	box->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	box->set_v_size_flags(0);
	inner_container->add_child(box);

	position_btn = memnew(Button);
	position_btn->set_text("0");
	position_btn->set_h_size_flags(0);
	position_btn->set_v_size_flags(0);
	position_btn->set_flat(true);
	position_btn->connect("pressed", callable_mp(this, &SystemInfoBox::position_btn_pressed));
	box->add_child(position_btn);

	position_input = memnew(SpinBox);
	position_input->set_h_size_flags(0);
	position_input->set_v_size_flags(0);
	position_input->set_visible(false);
	position_input->set_max(10000);
	position_input->set_step(-1); // Invert the spin box direction.
	position_input->connect("value_changed", callable_mp(this, &SystemInfoBox::system_position_changed), Vector<Variant>(), CONNECT_DEFERRED);
	box->add_child(position_input);

	remove_btn = memnew(Button);
	remove_btn->set_icon(editor->get_theme_base()->get_theme_icon("Remove", "EditorIcons"));
	remove_btn->set_h_size_flags(0);
	remove_btn->set_v_size_flags(0);
	remove_btn->set_visible(false);
	remove_btn->connect("pressed", callable_mp(this, &SystemInfoBox::system_remove), Vector<Variant>(), CONNECT_DEFERRED);
	box->add_child(remove_btn);

	system_name_lbl = memnew(Label);
	system_name_lbl->set_h_size_flags(0);
	system_name_lbl->set_v_size_flags(0);
	box->add_child(system_name_lbl);

	system_data_list = memnew(ItemList);
	system_data_list->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	system_data_list->set_v_size_flags(SizeFlags::SIZE_EXPAND);
	system_data_list->set_auto_height(true);
	system_data_list->set_max_columns(0);
	system_data_list->set_fixed_icon_size(Size2(13.0, 13.0));
	system_data_list->add_theme_constant_override("hseparation", 7.0);
	box->add_child(system_data_list);

	dispatcher_pipeline_name = memnew(LineEdit);
	dispatcher_pipeline_name->set_placeholder(TTR("Pipeline name."));
	dispatcher_pipeline_name->set_right_icon(editor->get_theme_base()->get_theme_icon("Rename", "EditorIcons"));
	dispatcher_pipeline_name->set_visible(false);
	dispatcher_pipeline_name->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	dispatcher_pipeline_name->set_v_size_flags(SizeFlags::SIZE_EXPAND);
	dispatcher_pipeline_name->connect("text_changed", callable_mp(this, &SystemInfoBox::dispatcher_pipeline_change));
	box->add_child(dispatcher_pipeline_name);
}

SystemInfoBox::~SystemInfoBox() {
}

void SystemInfoBox::set_position(uint32_t p_position) {
	position_input->set_visible(false);
	position_input->set_value(p_position);
	remove_btn->set_visible(false);
	position_btn->set_text(itos(p_position));
}

void SystemInfoBox::setup_system(const StringName &p_name, SystemMode p_mode) {
	system_name_lbl->set_text(String(p_name) + (p_mode == SYSTEM_INVALID ? " [INVALID]" : ""));
	system_name = p_name;

	StringName icon_name;
	switch (p_mode) {
		case SYSTEM_INVALID:
			icon_name = "FileDeadBigThumb";
			break;
		case SYSTEM_NATIVE:
			icon_name = "ShaderGlobalsOverride";
			break;
		case SYSTEM_DISPATCHER:
			icon_name = "ShaderMaterial";
			system_data_list->set_visible(false);
			dispatcher_pipeline_name->set_visible(true);
			break;
		case SYSTEM_SCRIPT:
			icon_name = "Script";
			break;
	}

	position_btn->set_icon(editor->get_theme_base()->get_theme_icon(icon_name, "EditorIcons"));

	mode = p_mode;
}

void SystemInfoBox::set_pipeline_dispatcher(const StringName &p_current_pipeline_name) {
	// Set the pipeline name before marking this system as dispatcher so
	// we can avoid trigger the name change.
	dispatcher_pipeline_name->set_text(p_current_pipeline_name);
}

void SystemInfoBox::add_system_element(const String &p_name, bool is_write) {
	Ref<Texture2D> icon;
	if (is_write) {
		icon = editor->get_theme_base()->get_theme_icon("Edit", "EditorIcons");
	} else {
		icon = editor->get_theme_base()->get_theme_icon("GuiVisibilityVisible", "EditorIcons");
	}
	system_data_list->add_item(p_name, icon, false);
}

Point2 SystemInfoBox::name_global_transform() const {
	Control *nc = static_cast<Control *>(system_name_lbl->get_parent());
	return nc->get_global_position() + Vector2(0.0, nc->get_size().y / 2.0);
}

void SystemInfoBox::position_btn_pressed() {
	position_input->set_visible(!position_input->is_visible());
	remove_btn->set_visible(!remove_btn->is_visible());
}

void SystemInfoBox::system_position_changed(double p_value) {
	if (position_input->is_visible() == false) {
		// If not visible nothing to do.
		return;
	}
	position_input->set_visible(false);
	remove_btn->set_visible(false);

	const uint32_t new_position = p_value;
	editor_world_ecs->pipeline_item_position_change(system_name, new_position);
}

void SystemInfoBox::system_remove() {
	editor_world_ecs->pipeline_system_remove(system_name);
}

void SystemInfoBox::dispatcher_pipeline_change(const String &p_value) {
	if (mode != SYSTEM_DISPATCHER) {
		// Nothing to do.
		return;
	}

	editor_world_ecs->pipeline_system_dispatcher_set_pipeline(system_name, p_value);
}

DrawLayer::DrawLayer() {
	// This is just a draw layer, we don't need input.
	set_mouse_filter(MOUSE_FILTER_IGNORE);
}

void DrawLayer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (editor->is_pipeline_panel_dirty) {
				editor->is_pipeline_panel_dirty = false;
				update();
			}
		} break;
		case NOTIFICATION_DRAW: {
			editor->draw(this);
		} break;
		case NOTIFICATION_READY: {
			set_process_internal(true);
		} break;
	}
}

EditorWorldECS::EditorWorldECS(EditorNode *p_editor) :
		editor(p_editor) {
	set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	set_anchor(MARGIN_LEFT, 0.0);
	set_anchor(MARGIN_TOP, 0.0);
	set_anchor(MARGIN_RIGHT, 1.0);
	set_anchor(MARGIN_BOTTOM, 1.0);
	set_margin(MARGIN_LEFT, 0.0);
	set_margin(MARGIN_TOP, 0.0);
	set_margin(MARGIN_RIGHT, 0.0);
	set_margin(MARGIN_BOTTOM, 0.0);

	HBoxContainer *main_hb = memnew(HBoxContainer);
	main_hb->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	main_hb->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	main_hb->set_anchor(MARGIN_LEFT, 0.0);
	main_hb->set_anchor(MARGIN_TOP, 0.0);
	main_hb->set_anchor(MARGIN_RIGHT, 1.0);
	main_hb->set_anchor(MARGIN_BOTTOM, 1.0);
	add_child(main_hb);

	DrawLayer *draw_layer = memnew(DrawLayer);
	draw_layer->editor = this;
	draw_layer->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	draw_layer->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	draw_layer->set_anchor(MARGIN_LEFT, 0.0);
	draw_layer->set_anchor(MARGIN_TOP, 0.0);
	draw_layer->set_anchor(MARGIN_RIGHT, 1.0);
	draw_layer->set_anchor(MARGIN_BOTTOM, 1.0);
	draw_layer->set_margin(MARGIN_LEFT, 0.0);
	draw_layer->set_margin(MARGIN_TOP, 0.0);
	draw_layer->set_margin(MARGIN_RIGHT, 0.0);
	draw_layer->set_margin(MARGIN_BOTTOM, 0.0);
	add_child(draw_layer);

	// ~~ Left Panel ~~
	{
		VBoxContainer *main_container = memnew(VBoxContainer);
		main_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		main_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		main_hb->add_child(main_container);

		// ~~ Action box ~~
		{
			HBoxContainer *hori_box = memnew(HBoxContainer);
			hori_box->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			hori_box->set_v_size_flags(0);
			main_container->add_child(hori_box);

			node_name_lbl = memnew(Label);
			node_name_lbl->add_theme_color_override("font_color", Color(0.0, 0.5, 1.0));
			hori_box->add_child(node_name_lbl);

			pipeline_menu = memnew(OptionButton);
			pipeline_menu->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			pipeline_menu->connect("item_selected", callable_mp(this, &EditorWorldECS::pipeline_on_menu_select));
			hori_box->add_child(pipeline_menu);

			Button *new_pipeline_btn = memnew(Button);
			new_pipeline_btn->set_h_size_flags(0);
			new_pipeline_btn->set_icon(editor->get_theme_base()->get_theme_icon("New", "EditorIcons"));
			new_pipeline_btn->set_text(TTR("Add pipeline"));
			new_pipeline_btn->connect("pressed", callable_mp(this, &EditorWorldECS::pipeline_add));
			hori_box->add_child(new_pipeline_btn);

			Button *remove_pipeline_btn = memnew(Button);
			remove_pipeline_btn->set_h_size_flags(0);
			remove_pipeline_btn->set_icon(editor->get_theme_base()->get_theme_icon("Remove", "EditorIcons"));
			remove_pipeline_btn->connect("pressed", callable_mp(this, &EditorWorldECS::pipeline_remove_show_confirmation));
			hori_box->add_child(remove_pipeline_btn);

			pipeline_confirm_remove = memnew(ConfirmationDialog);
			pipeline_confirm_remove->set_min_size(Size2i(200, 80));
			pipeline_confirm_remove->set_title(TTR("Confirm removal"));
			pipeline_confirm_remove->get_label()->set_text(TTR("Do you want to drop the selected pipeline?"));
			pipeline_confirm_remove->get_ok_button()->set_text(TTR("Confirm"));
			pipeline_confirm_remove->connect("confirmed", callable_mp(this, &EditorWorldECS::pipeline_remove));
			add_child(pipeline_confirm_remove);
		}

		// ~~ Resources box ~~
		{
			HSeparator *separator = memnew(HSeparator);
			main_container->add_child(separator);

			HBoxContainer *hori_box = memnew(HBoxContainer);
			hori_box->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			hori_box->set_v_size_flags(0);
			main_container->add_child(hori_box);

			Label *title = memnew(Label);
			title->set_text(TTR("Resources"));
			hori_box->add_child(title);

			Button *add_resource_btn = memnew(Button);
			add_resource_btn->set_h_size_flags(0);
			add_resource_btn->set_icon(editor->get_theme_base()->get_theme_icon("New", "EditorIcons"));
			add_resource_btn->set_text(TTR("Add resource"));
			add_resource_btn->connect("pressed", callable_mp(this, &EditorWorldECS::resource_add_show_menu));
			hori_box->add_child(add_resource_btn);

			resource_list = memnew(ItemList);
			resource_list->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			resource_list->set_v_size_flags(SizeFlags::SIZE_EXPAND);
			resource_list->set_auto_height(true);
			resource_list->set_max_columns(0);
			resource_list->set_fixed_icon_size(Size2(13.0, 13.0));
			resource_list->add_theme_constant_override("hseparation", 7.0);
			main_container->add_child(resource_list);
		}
	}

	VSeparator *separator = memnew(VSeparator);
	main_hb->add_child(separator);

	// ~~ Right Panel ~~
	{
		VBoxContainer *main_container = memnew(VBoxContainer);
		main_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		main_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		main_hb->add_child(main_container);

		HBoxContainer *title_container = memnew(HBoxContainer);
		title_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		title_container->set_v_size_flags(0);
		main_container->add_child(title_container);

		Label *title = memnew(Label);
		title->set_text(TTR("Pipeline"));
		title->set_h_size_flags(0);
		title->set_v_size_flags(SIZE_FILL | SIZE_EXPAND);
		title_container->add_child(title);

		pip_name_ledit = memnew(LineEdit);
		pip_name_ledit->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		pip_name_ledit->set_v_size_flags(0);
		pip_name_ledit->connect("text_changed", callable_mp(this, &EditorWorldECS::pipeline_change_name));
		title_container->add_child(pip_name_ledit);

		Panel *panel = memnew(Panel);
		panel->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		panel->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		panel->set_anchor(MARGIN_LEFT, 0.0);
		panel->set_anchor(MARGIN_TOP, 0.0);
		panel->set_anchor(MARGIN_RIGHT, 1.0);
		panel->set_anchor(MARGIN_BOTTOM, 1.0);
		main_container->add_child(panel);

		ScrollContainer *wrapper = memnew(ScrollContainer);
		wrapper->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		wrapper->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		wrapper->set_anchor(MARGIN_LEFT, 0.0);
		wrapper->set_anchor(MARGIN_TOP, 0.0);
		wrapper->set_anchor(MARGIN_RIGHT, 1.0);
		wrapper->set_anchor(MARGIN_BOTTOM, 1.0);
		wrapper->set_enable_h_scroll(true);
		wrapper->set_enable_v_scroll(false);
		panel->add_child(wrapper);

		pipeline_panel = memnew(VBoxContainer);
		pipeline_panel->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		pipeline_panel->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		wrapper->add_child(pipeline_panel);

		HBoxContainer *button_container = memnew(HBoxContainer);
		button_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		button_container->set_v_size_flags(0);
		main_container->add_child(button_container);

		Button *show_btn_add_sys = memnew(Button);
		show_btn_add_sys->set_text(TTR("Add System"));
		show_btn_add_sys->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		show_btn_add_sys->set_v_size_flags(0);
		show_btn_add_sys->connect("pressed", callable_mp(this, &EditorWorldECS::add_sys_show));
		button_container->add_child(show_btn_add_sys);

		Button *create_sys_btn = memnew(Button);
		create_sys_btn->set_icon(editor->get_theme_base()->get_theme_icon("New", "EditorIcons"));
		create_sys_btn->set_h_size_flags(0.0);
		create_sys_btn->set_v_size_flags(0.0);
		create_sys_btn->connect("pressed", callable_mp(this, &EditorWorldECS::create_sys_show));
		button_container->add_child(create_sys_btn);
	}

	// ~~ Add system window ~~
	{
		add_sys_window = memnew(ConfirmationDialog);
		add_sys_window->set_min_size(Size2i(500, 500));
		add_sys_window->set_title(TTR("Add System"));
		add_sys_window->get_ok_button()->set_text(TTR("Add"));
		add_sys_window->connect("confirmed", callable_mp(this, &EditorWorldECS::add_sys_add));
		add_child(add_sys_window);

		VBoxContainer *vert_container = memnew(VBoxContainer);
		vert_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		vert_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_sys_window->add_child(vert_container);

		add_sys_search = memnew(LineEdit);
		add_sys_search->set_placeholder(TTR("Search"));
		add_sys_search->connect("text_changed", callable_mp(this, &EditorWorldECS::add_sys_update));
		vert_container->add_child(add_sys_search);

		add_sys_tree = memnew(Tree);
		add_sys_tree->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_sys_tree->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_sys_tree->set_hide_root(true);
		add_sys_tree->connect("item_selected", callable_mp(this, &EditorWorldECS::add_sys_update_desc));
		vert_container->add_child(add_sys_tree);

		add_sys_desc = memnew(TextEdit);
		add_sys_desc->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_sys_desc->set_v_size_flags(0);
		add_sys_desc->set_custom_minimum_size(Size2i(0, 50));
		add_sys_desc->set_h_scroll(true);
		add_sys_desc->set_v_scroll(false);
		add_sys_desc->set_context_menu_enabled(false);
		add_sys_desc->set_shortcut_keys_enabled(false);
		add_sys_desc->set_virtual_keyboard_enabled(false);
		add_sys_desc->set_focus_mode(FOCUS_NONE);
		add_sys_desc->set_readonly(true);
		add_sys_desc->add_theme_color_override("font_color_readonly", Color(1.0, 1.0, 1.0));
		vert_container->add_child(add_sys_desc);
	}

	// ~~ Add resource window ~~
	{
		add_res_window = memnew(ConfirmationDialog);
		add_res_window->set_min_size(Size2i(500, 500));
		add_res_window->set_title(TTR("Add Resource"));
		add_res_window->get_ok_button()->set_text(TTR("Add resource"));
		add_res_window->connect("confirmed", callable_mp(this, &EditorWorldECS::resource_add_do));
		add_child(add_res_window);

		VBoxContainer *vert_container = memnew(VBoxContainer);
		vert_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		vert_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_res_window->add_child(vert_container);

		add_res_search = memnew(LineEdit);
		add_res_search->set_placeholder(TTR("Search"));
		add_res_search->connect("text_changed", callable_mp(this, &EditorWorldECS::resource_add_tree_update));
		vert_container->add_child(add_res_search);

		add_res_tree = memnew(Tree);
		add_res_tree->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_res_tree->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_res_tree->set_hide_root(true);
		vert_container->add_child(add_res_tree);
	}

	// ~~ Create script system window ~~
	{
		add_script_window = memnew(ConfirmationDialog);
		add_script_window->set_min_size(Size2i(500, 180));
		add_script_window->set_title(TTR("Add script System / Component / Resource"));
		add_script_window->set_hide_on_ok(false);
		add_script_window->get_ok_button()->set_text(TTR("Create"));
		add_script_window->connect("confirmed", callable_mp(this, &EditorWorldECS::add_script_do));
		add_child(add_script_window);

		VBoxContainer *vert_container = memnew(VBoxContainer);
		vert_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		vert_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_script_window->add_child(vert_container);

		Label *lbl = memnew(Label);
		lbl->set_text("Script path");
		vert_container->add_child(lbl);

		add_script_path = memnew(LineEdit);
		add_script_path->set_placeholder(TTR("Script path"));
		vert_container->add_child(add_script_path);

		add_script_error_lbl = memnew(Label);
		add_script_error_lbl->hide();
		add_script_error_lbl->add_theme_color_override("font_color", Color(1.0, 0.0, 0.0));
		vert_container->add_child(add_script_error_lbl);
	}
}

void EditorWorldECS::_notification(int p_what) {
}

void EditorWorldECS::show_editor() {
	if (world_ecs == nullptr ||
			pipeline.is_null() ||
			// If the given pipeline is not own by the current `WorldECS`.
			world_ecs->get_pipelines().find(pipeline) == -1) {
		// Reset the assigned pipeline.
		if (world_ecs == nullptr || world_ecs->get_pipelines().empty()) {
			set_pipeline(Ref<PipelineECS>());
		} else {
			set_pipeline(world_ecs->get_pipelines()[0]);
		}
	}

	if (world_ecs != nullptr) {
		show();
	} else {
		hide();
	}
	add_sys_hide();
	create_sys_hide();
	pipeline_confirm_remove->set_visible(false);

	pipeline_list_update();
	resource_list_update();
}

void EditorWorldECS::hide_editor() {
	hide();
	add_sys_hide();
	create_sys_hide();
}

void EditorWorldECS::set_world_ecs(WorldECS *p_world) {
	if (world_ecs != nullptr) {
		world_ecs->remove_change_receptor(this);
		node_name_lbl->set_text("");
		set_pipeline(Ref<PipelineECS>());
	}

	world_ecs = p_world;
	pipeline_panel_clear();

	if (world_ecs != nullptr) {
		world_ecs->add_change_receptor(this);
		node_name_lbl->set_text(world_ecs->get_name());
		show();
	}

	pipeline_list_update();
	resource_list_update();
}

void EditorWorldECS::set_pipeline(Ref<PipelineECS> p_pipeline) {
	if (pipeline.is_valid()) {
		pipeline->remove_change_receptor(this);
	}

	pipeline = p_pipeline;

	if (pipeline.is_valid()) {
		pipeline->add_change_receptor(this);
	}

	pipeline_panel_update();
}

void EditorWorldECS::draw(DrawLayer *p_draw_layer) {
	if (pipeline_systems.size() <= 0) {
		return;
	}

	// Setting this value here, so I can avoid to pass this pointer to each func.
	draw_layer = p_draw_layer;

	// TODO now draw the batches using `pipeline_panel_draw_batch();`
	//pipeline_panel_draw_batch(0, 1);
	//pipeline_panel_draw_batch(2, 3);

	draw_layer = nullptr;
}

void EditorWorldECS::pipeline_change_name(const String &p_name) {
	if (pipeline.is_null()) {
		// Nothing to do.
		return;
	}

	editor->get_undo_redo()->create_action(TTR("Change pipeline name"));
	editor->get_undo_redo()->add_do_method(pipeline.ptr(), "set_pipeline_name", p_name);
	editor->get_undo_redo()->add_undo_method(pipeline.ptr(), "set_pipeline_name", pipeline->get_pipeline_name());
	editor->get_undo_redo()->commit_action();
}

void EditorWorldECS::pipeline_list_update() {
	pipeline_menu->clear();

	if (world_ecs == nullptr) {
		pipeline_on_menu_select(-1);
		return;
	}

	const Vector<Ref<PipelineECS>> &pipelines = world_ecs->get_pipelines();
	int select = -1;
	for (int i = 0; i < pipelines.size(); i += 1) {
		pipeline_menu->add_item(pipelines[i]->get_pipeline_name());
		if (pipelines[i] == pipeline) {
			select = i;
		}
	}

	if (select != -1) {
		pipeline_menu->select(select);
		pipeline_on_menu_select(select);
	} else {
		pipeline_on_menu_select(0);
	}
}

void EditorWorldECS::pipeline_on_menu_select(int p_index) {
	if (world_ecs && p_index >= 0 && p_index < world_ecs->get_pipelines().size()) {
		set_pipeline(world_ecs->get_pipelines()[p_index]);
	} else {
		set_pipeline(Ref<PipelineECS>());
	}
	if (pipeline.is_null()) {
		pip_name_ledit->set_text("");
	} else {
		pip_name_ledit->set_text(pipeline->get_pipeline_name());
	}
	// Always position the cursor at the end.
	pip_name_ledit->set_cursor_position(INT32_MAX);
	pipeline_panel_update();
}

void EditorWorldECS::pipeline_add() {
	if (world_ecs == nullptr) {
		// Nothing to do.
		return;
	}

	// Find the proper name for this new pipeline.
	uint32_t default_count = 0;
	Vector<Ref<PipelineECS>> &v = world_ecs->get_pipelines();
	for (int i = 0; i < v.size(); i += 1) {
		if (v[i].is_null()) {
			continue;
		}
		if (String(v[i]->get_pipeline_name()).find("Default") >= 0) {
			default_count += 1;
		}
	}

	StringName name = "Default" + itos(default_count);

	Ref<PipelineECS> pip;
	pip.instance();
	pip->set_pipeline_name(name);
	set_pipeline(pip);

	editor->get_undo_redo()->create_action(TTR("Add pipeline"));
	editor->get_undo_redo()->add_do_method(world_ecs, "add_pipeline", pip);
	editor->get_undo_redo()->add_undo_method(world_ecs, "remove_pipeline", pip);
	editor->get_undo_redo()->commit_action();
}

void EditorWorldECS::pipeline_remove_show_confirmation() {
	const Vector2i modal_pos = (Vector2i(get_viewport_rect().size) - pipeline_confirm_remove->get_size()) / 2.0;
	pipeline_confirm_remove->set_position(modal_pos);
	pipeline_confirm_remove->set_visible(true);
}

void EditorWorldECS::pipeline_remove() {
	if (world_ecs == nullptr || pipeline.is_null()) {
		return;
	}

	editor->get_undo_redo()->create_action(TTR("Pipeline remove"));
	editor->get_undo_redo()->add_do_method(world_ecs, "remove_pipeline", pipeline);
	editor->get_undo_redo()->add_undo_method(world_ecs, "add_pipeline", pipeline);
	editor->get_undo_redo()->commit_action();
}

void EditorWorldECS::pipeline_panel_update() {
	is_pipeline_panel_dirty = true;
	pipeline_panel_clear();

	if (pipeline.is_null()) {
		// Nothing more to do.
		return;
	}

	Array systems = pipeline->get_systems_name();
	for (int i = 0; i < systems.size(); i += 1) {
		SystemInfoBox *info_box = pipeline_panel_add_system();

		const StringName system_name = systems[i];

		if (String(system_name).ends_with(".gd")) {
			// Init a script system.
			info_box->setup_system(system_name, SystemInfoBox::SYSTEM_SCRIPT);
			// TODO add script system components

		} else {
			// Init a native system.
			const uint32_t system_id = ECS::get_system_id(system_name);
			if (system_id == UINT32_MAX) {
				info_box->setup_system(system_name, SystemInfoBox::SYSTEM_INVALID);
			} else {
				SystemExeInfo system_exec_info;
				ECS::get_system_exe_info(system_id, system_exec_info);
				const StringName key_name = ECS::get_system_name(system_id);

				const bool is_dispatcher = ECS::is_system_dispatcher(system_id);
				if (is_dispatcher) {
					// This is a dispatcher system, don't print the dependencies.
					info_box->set_pipeline_dispatcher(world_ecs->get_system_dispatchers_pipeline(key_name));
					info_box->setup_system(key_name, SystemInfoBox::SYSTEM_DISPATCHER);
				} else {
					// Normal native system, add dependencies.
					info_box->setup_system(key_name, SystemInfoBox::SYSTEM_NATIVE);

					// Draw immutable components.
					for (uint32_t u = 0; u < system_exec_info.immutable_components.size(); u += 1) {
						info_box->add_system_element(
								ECS::get_component_name(system_exec_info.immutable_components[u]),
								false);
					}

					// Draw mutable components.
					for (uint32_t u = 0; u < system_exec_info.mutable_components.size(); u += 1) {
						info_box->add_system_element(
								ECS::get_component_name(system_exec_info.mutable_components[u]),
								true);
					}

					// Draw immutable resources.
					for (uint32_t u = 0; u < system_exec_info.immutable_resources.size(); u += 1) {
						info_box->add_system_element(
								String(ECS::get_resource_name(system_exec_info.immutable_resources[u])) + " [res]",
								false);
					}

					// Draw immutable resources.
					for (uint32_t u = 0; u < system_exec_info.mutable_resources.size(); u += 1) {
						info_box->add_system_element(
								String(ECS::get_resource_name(system_exec_info.mutable_resources[u])) + " [res]",
								true);
					}
				}
			}
		}
	}
}

void EditorWorldECS::pipeline_item_position_change(const StringName &p_name, uint32_t p_new_position) {
	if (pipeline.is_null()) {
		return;
	}

	editor->get_undo_redo()->create_action(TTR("Change system position"));
	editor->get_undo_redo()->add_do_method(pipeline.ptr(), "insert_system", p_name, p_new_position);
	// Undo by resetting the `system_names` because the `insert_system` changes
	// the array not trivially.
	editor->get_undo_redo()->add_undo_method(pipeline.ptr(), "set_systems_name", pipeline->get_systems_name().duplicate(true));
	editor->get_undo_redo()->commit_action();
}

void EditorWorldECS::pipeline_system_remove(const StringName &p_name) {
	if (pipeline.is_null()) {
		return;
	}

	editor->get_undo_redo()->create_action(TTR("Remove system"));
	editor->get_undo_redo()->add_do_method(pipeline.ptr(), "remove_system", p_name);
	// Undo by resetting the `system_names` because the `insert_system` changes
	// the array not trivially.
	editor->get_undo_redo()->add_undo_method(pipeline.ptr(), "set_systems_name", pipeline->get_systems_name().duplicate(true));
	editor->get_undo_redo()->commit_action();
}

void EditorWorldECS::pipeline_system_dispatcher_set_pipeline(const StringName &p_system_name, const StringName &p_pipeline_name) {
	if (world_ecs == nullptr) {
		return;
	}

	editor->get_undo_redo()->create_action(TTR("System dispatcher pipeline change."));
	editor->get_undo_redo()->add_do_method(world_ecs, "set_system_dispatchers_pipeline", p_system_name, p_pipeline_name);
	// Undo by resetting the `map` because the `set_system_dispatchers_pipeline`
	// changes the array not trivially.
	editor->get_undo_redo()->add_undo_method(world_ecs, "set_system_dispatchers_map", world_ecs->get_system_dispatchers_map().duplicate(true));
	editor->get_undo_redo()->commit_action();
}

void EditorWorldECS::add_sys_show() {
	// Display the modal window centered.
	const Vector2i modal_pos = (Vector2i(get_viewport_rect().size) - add_sys_window->get_size()) / 2.0;
	add_sys_window->set_position(modal_pos);
	add_sys_window->set_visible(true);
	add_sys_update();
}

void EditorWorldECS::add_sys_hide() {
	add_sys_window->set_visible(false);
}

void EditorWorldECS::add_sys_update(const String &p_search) {
	String search = p_search;
	if (search.empty()) {
		search = add_sys_search->get_text();
	}
	search = search.to_lower();

	add_sys_tree->clear();

	TreeItem *root = add_sys_tree->create_item();
	root->set_text(0, "Systems");
	root->set_selectable(0, false);

	// Native systems
	TreeItem *native_root = nullptr;

	for (uint32_t system_id = 0; system_id < ECS::get_systems_count(); system_id += 1) {
		const StringName key_name = ECS::get_system_name(system_id);
		const String desc = ECS::get_system_desc(system_id);

		const String name(String(key_name).to_lower());
		if (search.empty() == false && name.find(search) != 0) {
			// System filtered.
			continue;
		}

		if (native_root == nullptr) {
			// Add only if needed.
			native_root = add_sys_tree->create_item(root);
			native_root->set_text(0, "Native Systems");
			native_root->set_selectable(0, false);
			native_root->set_custom_color(0, Color(0.0, 0.9, 0.3));
		}

		TreeItem *item = add_sys_tree->create_item(native_root);
		if (ECS::is_system_dispatcher(system_id)) {
			item->set_icon(0, editor->get_theme_base()->get_theme_icon("ShaderMaterial", "EditorIcons"));
		} else {
			item->set_icon(0, editor->get_theme_base()->get_theme_icon("ShaderGlobalsOverride", "EditorIcons"));
		}
		item->set_text(0, key_name);
		item->set_meta("system_name", key_name);
		item->set_meta("desc", desc);
	}

	// Scripts systems
	TreeItem *script_root = nullptr;

	if (ProjectSettings::get_singleton()->has_setting("ECS/System/scripts")) {
		Array sys_scripts = ProjectSettings::get_singleton()->get_setting("ECS/System/scripts");
		for (int i = 0; i < sys_scripts.size(); i += 1) {
			const String sys_script_path = sys_scripts[i];
			const String system_name = sys_script_path.get_file();

			if (search.empty() == false && system_name.to_lower().find(search) != 0) {
				// System filtered.
				continue;
			}

			if (script_root == nullptr) {
				// Add only if needed.
				script_root = add_sys_tree->create_item(root);
				script_root->set_text(0, "Script Systems");
				script_root->set_selectable(0, false);
				script_root->set_custom_color(0, Color(0.0, 0.3, 0.9));
			}

			TreeItem *item = add_sys_tree->create_item(script_root);
			item->set_icon(0, editor->get_theme_base()->get_theme_icon("Script", "EditorIcons"));
			item->set_text(0, system_name);
			item->set_meta("system_name", system_name);
			item->set_meta("desc", "GDScript: " + sys_script_path);
		}
	}

	add_sys_update_desc();
}

void EditorWorldECS::add_sys_update_desc() {
	TreeItem *selected = add_sys_tree->get_selected();
	if (selected == nullptr) {
		// Nothing selected.
		add_sys_desc->set_text("");
	} else {
		const String desc = selected->get_meta("desc");
		add_sys_desc->set_text(desc);
	}
}

void EditorWorldECS::add_sys_add() {
	if (world_ecs == nullptr) {
		// Nothing to do.
		return;
	}

	TreeItem *selected = add_sys_tree->get_selected();
	if (selected == nullptr) {
		// Nothing selected, so nothing to do.
		return;
	}

	if (pipeline.is_null()) {
		if (world_ecs->get_pipelines().empty()) {
			// No pipelines, just create the default one.
			Ref<PipelineECS> def_pip;
			def_pip.instance();
			def_pip->set_pipeline_name("Default");
			world_ecs->get_pipelines().push_back(def_pip);
			world_ecs->_change_notify();
		}
		set_pipeline(world_ecs->get_pipelines()[0]);
		pipeline_list_update();
	}

	editor->get_undo_redo()->create_action(TTR("Add system"));
	editor->get_undo_redo()->add_do_method(pipeline.ptr(), "insert_system", selected->get_meta("system_name"));
	// Undo by resetting the `system_names` because the `insert_system` changes
	// the array not trivially.
	editor->get_undo_redo()->add_undo_method(pipeline.ptr(), "set_systems_name", pipeline->get_systems_name().duplicate(true));
	editor->get_undo_redo()->commit_action();
}

void EditorWorldECS::create_sys_show() {
	// Display the modal window centered.
	const Vector2i modal_pos = (Vector2i(get_viewport_rect().size) - add_script_window->get_size()) / 2.0;
	add_script_window->set_position(modal_pos);
	add_script_window->set_visible(true);
	add_script_error_lbl->hide();
}

void EditorWorldECS::create_sys_hide() {
	add_script_window->set_visible(false);
}

void EditorWorldECS::add_script_do() {
	// Test creating script system list.

	const String script_path = add_script_path->get_text().strip_edges();

	Ref<Script> script = ResourceLoader::load(script_path);
	// Check the script path.
	if (script.is_null()) {
		add_script_error_lbl->set_text(TTR("The script path [") + script_path + TTR("] doesn't point to a script."));
		add_script_error_lbl->show();
		return;
	}

	if (script->is_valid() == false) {
		add_script_error_lbl->set_text(TTR("The script [") + script_path + TTR("] has some errors, fix these."));
		add_script_error_lbl->show();
		return;
	}

	String err = "";
	if ("System" == script->get_instance_base_type()) {
		err = System::validate_script(script);
	} else if ("Component" == script->get_instance_base_type()) {
		err = Component::validate_script(script);
	} else if ("Resource" == script->get_instance_base_type()) {
		err = resource_validate_script(script);
	} else {
		err = TTR("The script must extend a `System` a `Component` or a `Resource`.");
	}

	if (err != "") {
		add_script_error_lbl->set_text(TTR("The script [") + script_path + TTR("] validation failed: ") + err);
		add_script_error_lbl->show();
		return;
	}

	// The script is valid, store it.
	const String script_setting_path = "ECS/" + script->get_instance_base_type() + "/scripts";
	Array scripts;
	if (ProjectSettings::get_singleton()->has_setting(script_setting_path)) {
		scripts = ProjectSettings::get_singleton()->get_setting(script_setting_path);
	}

	// Check if this system already exists.

	if (scripts.find(script_path) >= 0) {
		add_script_error_lbl->set_text(TTR("The") + " " + script->get_instance_base_type() + " [" + script_path + "] " + TTR("is already registered."));
		add_script_error_lbl->show();
		return;
	}

	scripts.push_back(script_path);

	ProjectSettings::get_singleton()->set_setting(script_setting_path, scripts);
	// Make this component available to the system.
	ScriptECS::reload_component(script_path);

	add_script_path->set_text("");
	add_script_window->set_visible(false);
}

void EditorWorldECS::resource_list_update() {
	resource_list->clear();

	if (world_ecs == nullptr) {
		return;
	}

	const Vector<StringName> resources = world_ecs->get_resources();
	const StringName *res_ptr = resources.ptr();
	for (int i = 0; i < resources.size(); i += 1) {
		resource_list->add_item(res_ptr[i], Ref<Texture2D>(), false);
	}
}

void EditorWorldECS::resource_add_show_menu() {
	// Display the modal window centered.
	const Vector2i modal_pos = (Vector2i(get_viewport_rect().size) - add_res_window->get_size()) / 2.0;
	add_res_window->set_position(modal_pos);
	add_res_window->set_visible(true);
	resource_add_tree_update();
}

void EditorWorldECS::resource_add_hide_menu() {
	add_res_window->set_visible(false);
}

void EditorWorldECS::resource_add_tree_update(const String &p_search) {
	String search = p_search;
	if (search.empty()) {
		search = add_res_search->get_text();
	}
	search = search.to_lower();

	add_res_tree->clear();

	TreeItem *root = add_res_tree->create_item();
	root->set_text(0, "Resources");
	root->set_selectable(0, false);

	// Native resources
	TreeItem *native_root = nullptr;

	for (uint32_t i = 0; i < ECS::get_resource_count(); i += 1) {
		StringName key_name = ECS::get_resource_name(i);

		const String name(String(key_name).to_lower());
		if (search.empty() == false && name.find(search) != 0) {
			// System filtered.
			continue;
		}

		if (native_root == nullptr) {
			// Add only if needed.
			native_root = add_res_tree->create_item(root);
			native_root->set_text(0, "Native Resource");
			native_root->set_selectable(0, false);
			native_root->set_custom_color(0, Color(0.0, 0.9, 0.3));
		}

		TreeItem *item = add_res_tree->create_item(native_root);
		item->set_text(0, key_name);
		item->set_meta("res_name", key_name);
	}

	// Scripts resources.
	TreeItem *script_root = nullptr;

	if (ProjectSettings::get_singleton()->has_setting("ECS/Resource/scripts")) {
		Array res_scripts = ProjectSettings::get_singleton()->get_setting("ECS/Resource/scripts");
		for (int i = 0; i < res_scripts.size(); i += 1) {
			const String res_script_path = res_scripts[i];
			const String res_name = res_script_path.get_file();

			if (search.empty() == false && res_name.to_lower().find(search) != 0) {
				// System filtered.
				continue;
			}

			if (script_root == nullptr) {
				// Add only if needed.
				script_root = add_res_tree->create_item(root);
				script_root->set_text(0, "Script Resource");
				script_root->set_selectable(0, false);
				script_root->set_custom_color(0, Color(0.0, 0.3, 0.9));
			}

			TreeItem *item = add_res_tree->create_item(script_root);
			item->set_text(0, res_name);
			item->set_meta("res_name", res_name);
		}
	}
}

void EditorWorldECS::resource_add_do() {
	if (world_ecs == nullptr) {
		// Nothing to do.
		return;
	}

	TreeItem *selected = add_res_tree->get_selected();
	if (selected == nullptr) {
		// Nothing selected, so nothing to do.
		return;
	}

	editor->get_undo_redo()->create_action(TTR("Add resource"));
	editor->get_undo_redo()->add_do_method(world_ecs, "add_resource", selected->get_meta("res_name"));
	editor->get_undo_redo()->add_undo_method(world_ecs, "remove_resource", selected->get_meta("res_name"));
	editor->get_undo_redo()->commit_action();

	resource_list_update();
}

void EditorWorldECS::_changed_callback(Object *p_changed, const char *p_prop) {
	if (p_changed == world_ecs) {
		// The world changed.
		if (String("pipelines") == p_prop) {
			pipeline_list_update();
		}
	} else if (pipeline.is_valid() && pipeline.ptr() == p_changed) {
		// The selected pipeline changes.
		if (String("pipeline_name") == p_prop) {
			pipeline_list_update();
		} else {
			pipeline_panel_update();
		}
	} else {
		// Not sure what changed, at this point.
	}
}

SystemInfoBox *EditorWorldECS::pipeline_panel_add_system() {
	SystemInfoBox *info_box = memnew(SystemInfoBox(editor, this));

	const uint32_t position = pipeline_systems.size();
	info_box->set_position(position);

	pipeline_systems.push_back(info_box);
	pipeline_panel->add_child(info_box);

	return info_box;
}

void EditorWorldECS::pipeline_panel_clear() {
	for (int i = pipeline_panel->get_child_count() - 1; i >= 0; i -= 1) {
		Node *n = pipeline_panel->get_child(i);
		pipeline_panel->get_child(i)->remove_and_skip();
		memdelete(n);
	}
	pipeline_systems.clear();
}

void EditorWorldECS::pipeline_panel_draw_batch(uint32_t p_start_system, uint32_t p_end_system) {
	ERR_FAIL_COND(p_start_system > p_end_system);
	ERR_FAIL_COND(p_end_system >= pipeline_systems.size());

	const Point2 this_pos = draw_layer->get_global_position();
	const Point2 point_offset(-15.0, 0.0);
	const Point2 circle_offset(5.0, 0.0);

	Point2 prev;

	// Draw the points
	for (uint32_t i = p_start_system; i <= p_end_system; i += 1) {
		const Point2 current_point = (pipeline_systems[i]->name_global_transform() - this_pos) + point_offset;

		if (i != p_start_system) {
			draw_layer->draw_line(prev, current_point, Color(1.0, 1.0, 1.0, 0.4), 2.0);
		}

		draw_layer->draw_circle(
				current_point + circle_offset,
				4.0,
				Color(1.0, 1.0, 1.0, 0.4));

		prev = current_point;
	}
}

WorldECSEditorPlugin::WorldECSEditorPlugin(EditorNode *p_node) :
		editor(p_node) {
	ecs_editor = memnew(EditorWorldECS(p_node));
	editor->get_viewport()->add_child(ecs_editor);
	ecs_editor->hide_editor();
}

WorldECSEditorPlugin::~WorldECSEditorPlugin() {
	editor->get_viewport()->remove_child(ecs_editor);
	memdelete(ecs_editor);
	ecs_editor = nullptr;
}

void WorldECSEditorPlugin::edit(Object *p_object) {
	world_ecs = Object::cast_to<WorldECS>(p_object);
	ERR_FAIL_COND_MSG(world_ecs == nullptr, "The object should be of type WorldECS [BUG].");
	ecs_editor->set_world_ecs(world_ecs);
}

bool WorldECSEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<WorldECS>(p_object) != nullptr;
}

void WorldECSEditorPlugin::make_visible(bool p_visible) {
	if (p_visible) {
		ecs_editor->show_editor();
	} else {
		ecs_editor->hide_editor();
		ecs_editor->set_world_ecs(nullptr);
	}
}
