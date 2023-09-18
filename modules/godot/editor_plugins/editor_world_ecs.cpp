#include "editor_world_ecs.h"

#include "../../../ecs.h"
#include "../../../pipeline/pipeline_builder.h"
#include "../nodes/ecs_world.h"
#include "../nodes/script_ecs.h"
#include "core/io/resource_loader.h"
#include "editor/editor_file_system.h"
#include "editor/editor_node.h"
#include "editor/editor_scale.h"
#include "editor/editor_undo_redo_manager.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/reference_rect.h"
#include "scene/gui/separator.h"
#include "scene/gui/tree.h"

PipelineElementInfoBox::PipelineElementInfoBox(EditorNode *p_editor, EditorWorldECS *p_editor_world_ecs) :
		editor(p_editor),
		editor_world_ecs(p_editor_world_ecs) {
	add_theme_constant_override(SNAME("margin_right"), 2);
	add_theme_constant_override(SNAME("margin_top"), 2);
	add_theme_constant_override(SNAME("margin_left"), 2);
	add_theme_constant_override(SNAME("margin_bottom"), 2);

	ColorRect *bg = memnew(ColorRect);
	bg->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	bg->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	bg->set_color(Color(0.0, 0.0, 0.0, 0.2));
	add_child(bg);

	MarginContainer *inner_container = memnew(MarginContainer);
	inner_container->add_theme_constant_override(SNAME("margin_right"), 2);
	inner_container->add_theme_constant_override(SNAME("margin_top"), 2);
	inner_container->add_theme_constant_override(SNAME("margin_left"), 10);
	inner_container->add_theme_constant_override(SNAME("margin_bottom"), 2);
	add_child(inner_container);

	HBoxContainer *box = memnew(HBoxContainer);
	box->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	box->set_v_size_flags(0);
	inner_container->add_child(box);

	remove_btn = memnew(Button);
	remove_btn->set_icon(editor->get_gui_base()->get_theme_icon(SNAME("Remove"), SNAME("EditorIcons")));
	remove_btn->set_h_size_flags(0);
	remove_btn->set_v_size_flags(0);
	remove_btn->set_flat(true);
	remove_btn->connect(SNAME("pressed"), callable_mp(this, &PipelineElementInfoBox::system_remove), CONNECT_DEFERRED);
	box->add_child(remove_btn);

	system_name_lbl = memnew(Label);
	system_name_lbl->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	system_name_lbl->set_v_size_flags(SizeFlags::SIZE_EXPAND);
	box->add_child(system_name_lbl);

	extra_info_lbl = memnew(Label);
	extra_info_lbl->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	extra_info_lbl->set_v_size_flags(SizeFlags::SIZE_EXPAND);
	extra_info_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	extra_info_lbl->set_visible(false);
	extra_info_lbl->add_theme_color_override(SNAME("font_color"), Color(0.7, 0.7, 0.7));
	box->add_child(extra_info_lbl);

	icon_btn = memnew(Button);
	icon_btn->set_flat(true);
	icon_btn->set_disabled(true);
	icon_btn->set_focus_mode(Button::FOCUS_NONE);
	box->add_child(icon_btn);
}

PipelineElementInfoBox::~PipelineElementInfoBox() {
}

void PipelineElementInfoBox::setup_system(const StringName &p_name, SystemMode p_mode) {
	system_name_lbl->set_text(String(p_name) + (p_mode == SYSTEM_INVALID ? " [INVALID]" : ""));
	name = p_name;

	StringName icon_name;
	switch (p_mode) {
		case SYSTEM_BUNDLE:
			icon_name = SNAME("Load");
			break;
		case SYSTEM_NATIVE:
			icon_name = SNAME("ShaderGlobalsOverride");
			break;
		case SYSTEM_DISPATCHER:
			icon_name = SNAME("ShaderMaterial");
			break;
		case SYSTEM_SCRIPT:
			icon_name = SNAME("Script");
			break;
		case SYSTEM_TEMPORARY:
			icon_name = SNAME("Time");
			break;
		case SYSTEM_INVALID:
			icon_name = SNAME("FileDeadBigThumb");
			break;
	}

	icon_btn->set_icon(editor->get_gui_base()->get_theme_icon(icon_name, SNAME("EditorIcons")));

	mode = p_mode;
}

void PipelineElementInfoBox::set_extra_info(const String &p_desc) {
	extra_info_lbl->set_text(p_desc);
	extra_info_lbl->set_visible(!p_desc.is_empty());
}

void PipelineElementInfoBox::set_is_bundle(bool p_bundle) {
	is_bundle = p_bundle;
}

Point2 PipelineElementInfoBox::name_global_transform() const {
	Control *nc = static_cast<Control *>(system_name_lbl->get_parent());
	return nc->get_global_position() + Vector2(0.0, nc->get_size().y / 2.0);
}

void PipelineElementInfoBox::system_remove() {
	if (is_bundle) {
		editor_world_ecs->pipeline_system_bundle_remove(name);
	} else {
		editor_world_ecs->pipeline_system_remove(name);
	}
}

SystemView::SystemView() {
	add_theme_constant_override(SNAME("margin_right"), 0);
	add_theme_constant_override(SNAME("margin_top"), 0);
	add_theme_constant_override(SNAME("margin_left"), 0);
	add_theme_constant_override(SNAME("margin_bottom"), 0);

	color_rect = memnew(ColorRect);
	color_rect->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	color_rect->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	add_child(color_rect);

	MarginContainer *inner_margin = memnew(MarginContainer);
	inner_margin->add_theme_constant_override(SNAME("margin_right"), 2);
	inner_margin->add_theme_constant_override(SNAME("margin_top"), 2);
	inner_margin->add_theme_constant_override(SNAME("margin_left"), 2);
	inner_margin->add_theme_constant_override(SNAME("margin_bottom"), 2);
	inner_margin->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	inner_margin->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	add_child(inner_margin);

	name_lbl = memnew(Label);
	name_lbl->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	name_lbl->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	inner_margin->add_child(name_lbl);
}

SystemView::~SystemView() {
}

void SystemView::set_name(const String &p_name) {
	name_lbl->set_text(p_name);
}

void SystemView::set_bg_color(const Color &p_color) {
	color_rect->set_color(p_color);
}

StageView::StageView(
		EditorNode *p_editor,
		EditorWorldECS *p_editor_world_ecs) :
		editor(p_editor),
		editor_world_ecs(p_editor_world_ecs) {
	add_theme_constant_override(SNAME("margin_right"), 0);
	add_theme_constant_override(SNAME("margin_top"), 0);
	add_theme_constant_override(SNAME("margin_left"), 0);
	add_theme_constant_override(SNAME("margin_bottom"), 0);

	VBoxContainer *main_container = memnew(VBoxContainer);
	main_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	main_container->set_v_size_flags(0);
	add_child(main_container);

	HBoxContainer *box_titles = memnew(HBoxContainer);
	box_titles->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	box_titles->set_v_size_flags(0);
	main_container->add_child(box_titles);

	name_lbl = memnew(Label);
	name_lbl->add_theme_font_size_override(SNAME("font_size"), 18);
	box_titles->add_child(name_lbl);

	main_container->add_child(memnew(HSeparator));

	MarginContainer *margin = memnew(MarginContainer);
	margin->add_theme_constant_override(SNAME("margin_right"), 10);
	margin->add_theme_constant_override(SNAME("margin_top"), 10);
	margin->add_theme_constant_override(SNAME("margin_left"), 10);
	margin->add_theme_constant_override(SNAME("margin_bottom"), 10);
	margin->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	margin->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	main_container->add_child(margin);

	box = memnew(VBoxContainer);
	box->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	box->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	margin->add_child(box);
}

StageView::~StageView() {
}

void StageView::setup_stage(uint32_t p_stage_id) {
	name_lbl->set_text(TTR("Stage") + ": " + itos(p_stage_id));
}

SystemView *StageView::add_system() {
	SystemView *view = memnew(SystemView);
	box->add_child(view);
	return view;
}

DispatcherPipelineView *StageView::add_sub_dispatcher() {
	DispatcherPipelineView *view = memnew(DispatcherPipelineView(editor, editor_world_ecs));
	box->add_child(view);
	return view;
}

DispatcherPipelineView::DispatcherPipelineView(EditorNode *p_editor, EditorWorldECS *p_editor_world_ecs) :
		editor(p_editor),
		editor_world_ecs(p_editor_world_ecs) {
	add_theme_constant_override(SNAME("margin_right"), 0);
	add_theme_constant_override(SNAME("margin_top"), 0);
	add_theme_constant_override(SNAME("margin_left"), 0);
	add_theme_constant_override(SNAME("margin_bottom"), 0);

	PanelContainer *panel = memnew(PanelContainer);
	panel->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	panel->set_v_size_flags(SizeFlags::SIZE_EXPAND);
	panel->set_anchor(SIDE_LEFT, 0.0);
	panel->set_anchor(SIDE_TOP, 0.0);
	panel->set_anchor(SIDE_RIGHT, 1.0);
	panel->set_anchor(SIDE_BOTTOM, 1.0);
	panel_style.instantiate();
	panel->add_theme_style_override(SNAME("panel"), panel_style);
	add_child(panel);

	box = memnew(VBoxContainer);
	box->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	box->set_v_size_flags(SizeFlags::SIZE_EXPAND);
	panel->add_child(box);

	dispatcher_lbl = memnew(Label);
	box->add_child(dispatcher_lbl);
}

DispatcherPipelineView::~DispatcherPipelineView() {
}

void DispatcherPipelineView::set_dispatcher_name(const String &p_name) {
	dispatcher_lbl->set_text(p_name);
}

void DispatcherPipelineView::set_bg_color(const Color &p_color) {
	panel_style->set_bg_color(p_color);
}

StageView *DispatcherPipelineView::add_stage() {
	StageView *view = memnew(StageView(editor, editor_world_ecs));
	box->add_child(view);
	return view;
}

ComponentElement::ComponentElement(EditorNode *p_editor, const String &p_name, Variant p_default) :
		editor(p_editor) {
	set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);

	type = memnew(OptionButton);
	type->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("bool"), SNAME("EditorIcons")), "Bool");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("int"), SNAME("EditorIcons")), "Int");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("float"), SNAME("EditorIcons")), "Float");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("Vector3"), SNAME("EditorIcons")), "Vector3");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("Vector3i"), SNAME("EditorIcons")), "Vector3i");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("Quat"), SNAME("EditorIcons")), "Quat");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("AABB"), SNAME("EditorIcons")), "Aabb");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("Basis"), SNAME("EditorIcons")), "Basis");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("Transform"), SNAME("EditorIcons")), "Transform3D");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("Vector2"), SNAME("EditorIcons")), "Vector2");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("Vector2i"), SNAME("EditorIcons")), "Vector2i");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("Transform2D"), SNAME("EditorIcons")), "Transform2D");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("Color"), SNAME("EditorIcons")), "Color");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("String"), SNAME("EditorIcons")), "String");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("StringName"), SNAME("EditorIcons")), "StringName");
	type->add_icon_item(p_editor->get_gui_base()->get_theme_icon(SNAME("RID"), SNAME("EditorIcons")), "Rid");
	add_child(type);

	name = memnew(LineEdit);
	add_child(name);

	val = memnew(LineEdit);
	add_child(val);

	init_variable(p_name, p_default);
}

ComponentElement::~ComponentElement() {
}

void ComponentElement::init_variable(const String &p_name, Variant p_default) {
	int c = 0;

	int types[Variant::VARIANT_MAX];
	for (uint32_t i = 0; i < Variant::VARIANT_MAX; i += 1) {
		types[i] = 0;
	}
	types[Variant::BOOL] = c++;
	types[Variant::INT] = c++;
	types[Variant::FLOAT] = c++;
	types[Variant::VECTOR3] = c++;
	types[Variant::VECTOR3I] = c++;
	types[Variant::QUATERNION] = c++;
	types[Variant::AABB] = c++;
	types[Variant::BASIS] = c++;
	types[Variant::TRANSFORM3D] = c++;
	types[Variant::VECTOR2] = c++;
	types[Variant::VECTOR2I] = c++;
	types[Variant::TRANSFORM2D] = c++;
	types[Variant::COLOR] = c++;
	types[Variant::STRING] = c++;
	types[Variant::STRING_NAME] = c++;
	types[Variant::RID] = c++;

	type->select(types[p_default.get_type()]);
	name->set_text(p_name);
	val->set_text(p_default);
}

EditorWorldECS::EditorWorldECS(EditorNode *p_editor) :
		editor(p_editor) {
	set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	set_anchor(SIDE_LEFT, 0.0);
	set_anchor(SIDE_TOP, 0.0);
	set_anchor(SIDE_RIGHT, 1.0);
	set_anchor(SIDE_BOTTOM, 1.0);
	set_offset(SIDE_LEFT, 0.0);
	set_offset(SIDE_TOP, 0.0);
	set_offset(SIDE_RIGHT, 0.0);
	set_offset(SIDE_BOTTOM, 0.0);

	VBoxContainer *main_vb = memnew(VBoxContainer);
	main_vb->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	main_vb->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
	main_vb->set_anchor(SIDE_LEFT, 0.0);
	main_vb->set_anchor(SIDE_TOP, 0.0);
	main_vb->set_anchor(SIDE_RIGHT, 1.0);
	main_vb->set_anchor(SIDE_BOTTOM, 1.0);
	add_child(main_vb);

	// ~~ Main menu ~~
	{
		HBoxContainer *menu_wrapper = memnew(HBoxContainer);
		menu_wrapper->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		main_vb->add_child(menu_wrapper);

		Button *create_comp_btn = memnew(Button);
		create_comp_btn->set_text(TTR("Components"));
		create_comp_btn->set_icon(editor->get_gui_base()->get_theme_icon(SNAME("Load"), SNAME("EditorIcons")));
		create_comp_btn->set_flat(true);
		create_comp_btn->set_h_size_flags(0.0);
		create_comp_btn->set_v_size_flags(0.0);
		create_comp_btn->connect(SNAME("pressed"), callable_mp(this, &EditorWorldECS::components_manage_show));
		menu_wrapper->add_child(create_comp_btn);

		menu_wrapper->add_child(memnew(VSeparator));

		node_name_lbl = memnew(Label);
		menu_wrapper->add_child(node_name_lbl);

		// ~~ Sub menu world ECS ~~
		{
			world_ecs_sub_menu_wrap = memnew(HBoxContainer);
			world_ecs_sub_menu_wrap->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			menu_wrapper->add_child(world_ecs_sub_menu_wrap);

			pipeline_menu = memnew(OptionButton);
			pipeline_menu->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			pipeline_menu->connect(SNAME("item_selected"), callable_mp(this, &EditorWorldECS::pipeline_on_menu_select));
			world_ecs_sub_menu_wrap->add_child(pipeline_menu);

			Button *new_pipeline_btn = memnew(Button);
			new_pipeline_btn->set_h_size_flags(0);
			new_pipeline_btn->set_icon(editor->get_gui_base()->get_theme_icon(SNAME("New"), SNAME("EditorIcons")));
			new_pipeline_btn->set_flat(true);
			new_pipeline_btn->set_text(TTR("Add pipeline"));
			new_pipeline_btn->connect(SNAME("pressed"), callable_mp(this, &EditorWorldECS::pipeline_add));
			world_ecs_sub_menu_wrap->add_child(new_pipeline_btn);

			Button *rename_pipeline_btn = memnew(Button);
			rename_pipeline_btn->set_text(TTR("Rename"));
			rename_pipeline_btn->set_h_size_flags(0);
			rename_pipeline_btn->set_icon(editor->get_gui_base()->get_theme_icon(SNAME("Edit"), SNAME("EditorIcons")));
			rename_pipeline_btn->set_flat(true);
			rename_pipeline_btn->connect(SNAME("pressed"), callable_mp(this, &EditorWorldECS::pipeline_rename_show_window));
			world_ecs_sub_menu_wrap->add_child(rename_pipeline_btn);

			Button *remove_pipeline_btn = memnew(Button);
			remove_pipeline_btn->set_h_size_flags(0);
			remove_pipeline_btn->set_icon(editor->get_gui_base()->get_theme_icon(SNAME("Remove"), SNAME("EditorIcons")));
			remove_pipeline_btn->set_flat(true);
			remove_pipeline_btn->connect(SNAME("pressed"), callable_mp(this, &EditorWorldECS::pipeline_remove_show_confirmation));
			world_ecs_sub_menu_wrap->add_child(remove_pipeline_btn);

			pipeline_window_confirm_remove = memnew(ConfirmationDialog);
			pipeline_window_confirm_remove->set_min_size(Size2i(200, 80));
			pipeline_window_confirm_remove->set_title(TTR("Confirm removal"));
			pipeline_window_confirm_remove->get_label()->set_text(TTR("Do you want to drop the selected pipeline?"));
			pipeline_window_confirm_remove->get_ok_button()->set_text(TTR("Confirm"));
			pipeline_window_confirm_remove->connect(SNAME("confirmed"), callable_mp(this, &EditorWorldECS::pipeline_remove));
			add_child(pipeline_window_confirm_remove);
		}

		menu_wrapper->add_child(memnew(VSeparator));

		Button *show_pipeline_btn = memnew(Button);
		show_pipeline_btn->set_h_size_flags(0);
		show_pipeline_btn->set_text(TTR("Pipeline view"));
		show_pipeline_btn->set_icon(editor->get_gui_base()->get_theme_icon(SNAME("PackedDataContainer"), SNAME("EditorIcons")));
		show_pipeline_btn->set_flat(true);
		show_pipeline_btn->connect(SNAME("pressed"), callable_mp(this, &EditorWorldECS::pipeline_toggle_pipeline_view));
		menu_wrapper->add_child(show_pipeline_btn);
	}

	// ~~ Workspace ~~
	{
		workspace_container_hb = memnew(HBoxContainer);
		workspace_container_hb->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		workspace_container_hb->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		main_vb->add_child(workspace_container_hb);

		// ~~ Features panel ~~
		{
			VBoxContainer *main_container = memnew(VBoxContainer);
			main_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			main_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			workspace_container_hb->add_child(main_container);

			Panel *panel_w = memnew(Panel);
			panel_w->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			panel_w->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			panel_w->set_anchor(SIDE_LEFT, 0.0);
			panel_w->set_anchor(SIDE_TOP, 0.0);
			panel_w->set_anchor(SIDE_RIGHT, 1.0);
			panel_w->set_anchor(SIDE_BOTTOM, 1.0);
			main_container->add_child(panel_w);

			ScrollContainer *wrapper = memnew(ScrollContainer);
			wrapper->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			wrapper->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			wrapper->set_anchor(SIDE_LEFT, 0.0);
			wrapper->set_anchor(SIDE_TOP, 0.0);
			wrapper->set_anchor(SIDE_RIGHT, 1.0);
			wrapper->set_anchor(SIDE_BOTTOM, 1.0);
			wrapper->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
			panel_w->add_child(wrapper);

			PanelContainer *panel = memnew(PanelContainer);
			panel->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			panel->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			panel->set_anchor(SIDE_LEFT, 0.0);
			panel->set_anchor(SIDE_TOP, 0.0);
			panel->set_anchor(SIDE_RIGHT, 1.0);
			panel->set_anchor(SIDE_BOTTOM, 1.0);
			wrapper->add_child(panel);

			pipeline_panel = memnew(VBoxContainer);
			pipeline_panel->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			pipeline_panel->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			panel->add_child(pipeline_panel);

			HBoxContainer *button_container = memnew(HBoxContainer);
			button_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			button_container->set_v_size_flags(0);
			main_container->add_child(button_container);

			Button *show_btn_add_sys = memnew(Button);
			show_btn_add_sys->set_text(TTR("Use feature"));
			show_btn_add_sys->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			show_btn_add_sys->set_v_size_flags(0);
			show_btn_add_sys->connect(SNAME("pressed"), callable_mp(this, &EditorWorldECS::add_sys_show));
			button_container->add_child(show_btn_add_sys);
		}

		// ~~ Pipeline view ~~
		{
			main_container_pipeline_view = memnew(VBoxContainer);
			main_container_pipeline_view->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			main_container_pipeline_view->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			main_container_pipeline_view->set_visible(false);
			workspace_container_hb->add_child(main_container_pipeline_view);

			Panel *panel_w = memnew(Panel);
			panel_w->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			panel_w->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			panel_w->set_anchor(SIDE_LEFT, 0.0);
			panel_w->set_anchor(SIDE_TOP, 0.0);
			panel_w->set_anchor(SIDE_RIGHT, 1.0);
			panel_w->set_anchor(SIDE_BOTTOM, 1.0);
			Ref<StyleBoxFlat> style;
			style.instantiate();
			style->set_bg_color(Color(0.02, 0.04, 0.10));
			panel_w->add_theme_style_override(SNAME("panel"), style);
			main_container_pipeline_view->add_child(panel_w);

			ScrollContainer *wrapper = memnew(ScrollContainer);
			wrapper->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			wrapper->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			wrapper->set_anchor(SIDE_LEFT, 0.0);
			wrapper->set_anchor(SIDE_TOP, 0.0);
			wrapper->set_anchor(SIDE_RIGHT, 1.0);
			wrapper->set_anchor(SIDE_BOTTOM, 1.0);
			wrapper->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
			panel_w->add_child(wrapper);

			MarginContainer *margin = memnew(MarginContainer);
			margin->add_theme_constant_override(SNAME("margin_right"), 10);
			margin->add_theme_constant_override(SNAME("margin_top"), 10);
			margin->add_theme_constant_override(SNAME("margin_left"), 10);
			margin->add_theme_constant_override(SNAME("margin_bottom"), 10);
			margin->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			margin->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			wrapper->add_child(margin);

			pipeline_view_panel = memnew(VBoxContainer);
			pipeline_view_panel->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			pipeline_view_panel->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			margin->add_child(pipeline_view_panel);
		}
	}

	// ~~ Errors & Warnings ~~
	{
		errors_warnings_panel = memnew(Panel);
		errors_warnings_panel->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		errors_warnings_panel->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		errors_warnings_panel->set_anchor(SIDE_LEFT, 0.0);
		errors_warnings_panel->set_anchor(SIDE_TOP, 0.0);
		errors_warnings_panel->set_anchor(SIDE_RIGHT, 1.0);
		errors_warnings_panel->set_anchor(SIDE_BOTTOM, 1.0);
		Ref<StyleBoxFlat> style;
		style.instantiate();
		style->set_bg_color(Color(0.01, 0.01, 0.01));
		errors_warnings_panel->add_theme_style_override(SNAME("panel"), style);
		main_vb->add_child(errors_warnings_panel);

		ScrollContainer *wrapper = memnew(ScrollContainer);
		wrapper->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		// wrapper->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		wrapper->set_anchor(SIDE_LEFT, 0.0);
		wrapper->set_anchor(SIDE_TOP, 0.0);
		wrapper->set_anchor(SIDE_RIGHT, 1.0);
		wrapper->set_anchor(SIDE_BOTTOM, 1.0);
		wrapper->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
		errors_warnings_panel->add_child(wrapper);

		PanelContainer *panel = memnew(PanelContainer);
		panel->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		// panel->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		panel->set_anchor(SIDE_LEFT, 0.0);
		panel->set_anchor(SIDE_TOP, 0.0);
		panel->set_anchor(SIDE_RIGHT, 1.0);
		panel->set_anchor(SIDE_BOTTOM, 1.0);
		wrapper->add_child(panel);

		errors_warnings_container = memnew(VBoxContainer);
		errors_warnings_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		// errors_warnings_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		panel->add_child(errors_warnings_container);
	}

	// ~~ Rename pipeline window ~~
	{
		pipeline_window_rename = memnew(AcceptDialog);
		pipeline_window_rename->set_min_size(Size2i(500, 180));
		pipeline_window_rename->set_title(TTR("Rename pipeline"));
		pipeline_window_rename->set_hide_on_ok(true);
		pipeline_window_rename->get_ok_button()->set_text(TTR("Ok"));
		add_child(pipeline_window_rename);

		VBoxContainer *vert_container = memnew(VBoxContainer);
		vert_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		vert_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		pipeline_window_rename->add_child(vert_container);

		Label *lbl = memnew(Label);
		lbl->set_text("Pipeline name");
		vert_container->add_child(lbl);

		pipeline_name_ledit = memnew(LineEdit);
		pipeline_name_ledit->set_placeholder(TTR("Pipeline name"));
		pipeline_name_ledit->connect(SNAME("text_changed"), callable_mp(this, &EditorWorldECS::pipeline_change_name));
		vert_container->add_child(pipeline_name_ledit);
	}

	// ~~ Add system window ~~
	{
		add_sys_window = memnew(ConfirmationDialog);
		add_sys_window->set_min_size(Size2i(500, 500));
		add_sys_window->set_title(TTR("Add System"));
		add_sys_window->get_ok_button()->set_text(TTR("Add"));
		add_sys_window->connect(SNAME("confirmed"), callable_mp(this, &EditorWorldECS::add_sys_add));
		add_child(add_sys_window);

		VBoxContainer *vert_container = memnew(VBoxContainer);
		vert_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		vert_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_sys_window->add_child(vert_container);

		add_sys_search = memnew(LineEdit);
		add_sys_search->set_placeholder(TTR("Search"));
		add_sys_search->connect(SNAME("text_changed"), callable_mp(this, &EditorWorldECS::add_sys_update));
		vert_container->add_child(add_sys_search);

		add_sys_tree = memnew(Tree);
		add_sys_tree->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_sys_tree->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_sys_tree->set_hide_root(true);
		add_sys_tree->connect(SNAME("item_selected"), callable_mp(this, &EditorWorldECS::add_sys_update_desc));
		vert_container->add_child(add_sys_tree);

		add_sys_desc = memnew(TextEdit);
		add_sys_desc->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		add_sys_desc->set_v_size_flags(0);
		add_sys_desc->set_custom_minimum_size(Size2i(0, 50));
		add_sys_desc->set_h_scroll(false);
		add_sys_desc->set_v_scroll(true);
		add_sys_desc->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
		add_sys_desc->set_context_menu_enabled(false);
		add_sys_desc->set_shortcut_keys_enabled(false);
		add_sys_desc->set_virtual_keyboard_enabled(false);
		add_sys_desc->set_focus_mode(FOCUS_NONE);
		add_sys_desc->set_editable(true);
		add_sys_desc->add_theme_color_override(SNAME("font_color_readonly"), Color(1.0, 1.0, 1.0));
		vert_container->add_child(add_sys_desc);
	}

	// ~~ Component manager ~~
	{
		components_window = memnew(AcceptDialog);
		components_window->set_min_size(Size2i(800, 500));
		components_window->set_title(TTR("Component and Databag manager - [WIP]"));
		components_window->set_hide_on_ok(true);
		components_window->get_ok_button()->set_text(TTR("Done"));
		add_child(components_window);

		HBoxContainer *window_main_hb = memnew(HBoxContainer);
		window_main_hb->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		window_main_hb->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
		components_window->add_child(window_main_hb);

		//  ~~ Left panel ~~
		{
			VBoxContainer *vertical_container = memnew(VBoxContainer);
			vertical_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			vertical_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			window_main_hb->add_child(vertical_container);

			components_tree = memnew(Tree);
			components_tree->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			components_tree->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			components_tree->set_hide_root(true);
			components_tree->connect(SNAME("item_selected"), callable_mp(this, &EditorWorldECS::components_manage_on_component_select));
			vertical_container->add_child(components_tree);

			Button *new_component_btn = memnew(Button);
			new_component_btn->set_icon(editor->get_gui_base()->get_theme_icon(SNAME("New"), SNAME("EditorIcons")));
			new_component_btn->set_text(TTR("New component"));
			new_component_btn->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			new_component_btn->set_v_size_flags(0);
			// new_component_btn->connect(SNAME("pressed"), callable_mp(this, &EditorWorldECS::add_sys_show)); // TODO
			vertical_container->add_child(new_component_btn);
		}

		window_main_hb->add_child(memnew(VSeparator));

		//  ~~ Right panel ~~
		{
			VBoxContainer *vertical_container = memnew(VBoxContainer);
			vertical_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			vertical_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			window_main_hb->add_child(vertical_container);

			component_name_le = memnew(LineEdit);
			component_name_le->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			component_name_le->set_v_size_flags(0);
			component_name_le->set_placeholder(TTR("Component name"));
			vertical_container->add_child(component_name_le);

			OptionButton *component_storage = memnew(OptionButton);
			component_storage->set_text(TTR("Component storage"));
			component_storage->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			component_storage->set_v_size_flags(0);
			component_storage->add_item(TTR("Dense Vector"));
			component_storage->select(0);
			vertical_container->add_child(component_storage);

			// Panel
			{
				Panel *panel = memnew(Panel);
				panel->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
				panel->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
				panel->set_anchor(SIDE_LEFT, 0.0);
				panel->set_anchor(SIDE_TOP, 0.0);
				panel->set_anchor(SIDE_RIGHT, 1.0);
				panel->set_anchor(SIDE_BOTTOM, 1.0);
				vertical_container->add_child(panel);

				ScrollContainer *scroll = memnew(ScrollContainer);
				scroll->set_h_scroll(false);
				scroll->set_v_scroll(true);
				scroll->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
				scroll->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
				scroll->set_anchor(SIDE_LEFT, 0.0);
				scroll->set_anchor(SIDE_TOP, 0.0);
				scroll->set_anchor(SIDE_RIGHT, 1.0);
				scroll->set_anchor(SIDE_BOTTOM, 1.0);
				panel->add_child(scroll);

				PanelContainer *panel_container = memnew(PanelContainer);
				panel_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
				panel_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
				panel_container->set_anchor(SIDE_LEFT, 0.0);
				panel_container->set_anchor(SIDE_TOP, 0.0);
				panel_container->set_anchor(SIDE_RIGHT, 1.0);
				panel_container->set_anchor(SIDE_BOTTOM, 1.0);
				scroll->add_child(panel_container);

				VBoxContainer *component_element_container = memnew(VBoxContainer);
				component_element_container->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
				component_element_container->set_v_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
				panel_container->add_child(component_element_container);

				// TODO list of variables?
				component_element_container->add_child(memnew(ComponentElement(editor, "Var_0", 123)));
				component_element_container->add_child(memnew(ComponentElement(editor, "Var_1", 123.0)));
				component_element_container->add_child(memnew(ComponentElement(editor, "Var_2", true)));
				component_element_container->add_child(memnew(ComponentElement(editor, "Var_3", Vector3(10, 0, 0))));
				component_element_container->add_child(memnew(ComponentElement(editor, "Var_4", Transform2D())));
				component_element_container->add_child(memnew(ComponentElement(editor, "Var_5", Dictionary())));
			}

			Button *add_var_btn = memnew(Button);
			add_var_btn->set_text(TTR("Add variable"));
			add_var_btn->set_icon(editor->get_gui_base()->get_theme_icon(SNAME("Add"), SNAME("EditorIcons")));
			add_var_btn->set_h_size_flags(SizeFlags::SIZE_FILL | SizeFlags::SIZE_EXPAND);
			add_var_btn->set_v_size_flags(0);
			// add_var_btn->connect(SNAME("pressed"), callable_mp(this, &EditorWorldECS::add_sys_show)); // TODO
			vertical_container->add_child(add_var_btn);
		}
	}
}

void EditorWorldECS::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			EditorFileSystem::get_singleton()->connect(SNAME("filesystem_changed"), callable_mp(this, &EditorWorldECS::_filesystem_changed));
		}
	}
}

void EditorWorldECS::_filesystem_changed() {
	if (ScriptEcs::get_singleton()) {
		ScriptEcs::get_singleton()->reload_scripts();
	}
}

void EditorWorldECS::show_editor() {
	if (world_ecs == nullptr ||
			pipeline.is_null() ||
			// If the given pipeline is not own by the current `WorldECS`.
			world_ecs->get_pipelines().find(pipeline) == -1) {
		// Reset the assigned pipeline.
		if (world_ecs == nullptr || world_ecs->get_pipelines().is_empty()) {
			set_pipeline(Ref<PipelineECS>());
		} else {
			set_pipeline(world_ecs->get_pipelines()[0]);
		}
	}

	add_sys_hide();
	pipeline_window_rename->set_visible(false);
	pipeline_window_confirm_remove->set_visible(false);

	// Refresh world ECS show hide.
	set_world_ecs(world_ecs);
	show();
}

void EditorWorldECS::hide_editor() {
	hide();
	add_sys_hide();
}

void EditorWorldECS::set_world_ecs(WorldECS *p_world) {
	if (world_ecs != nullptr) {
		set_pipeline(Ref<PipelineECS>());
		world_ecs->disconnect(SNAME("property_list_changed"), callable_mp(this, &EditorWorldECS::_changed_world_callback));
	}

	node_name_lbl->set_text("No world ECS selected.");
	node_name_lbl->add_theme_color_override(SNAME("font_color"), Color(0.7, 0.7, 0.7));
	world_ecs_sub_menu_wrap->hide();
	workspace_container_hb->hide();

	world_ecs = p_world;
	pipeline_panel_clear();

	if (world_ecs != nullptr) {
		world_ecs->connect(SNAME("property_list_changed"), callable_mp(this, &EditorWorldECS::_changed_world_callback));
		node_name_lbl->set_text(world_ecs->get_name());
		node_name_lbl->add_theme_color_override(SNAME("font_color"), Color(0.0, 0.5, 1.0));
		world_ecs_sub_menu_wrap->show();
		workspace_container_hb->show();
	}

	pipeline_list_update();
}

void EditorWorldECS::set_pipeline(Ref<PipelineECS> p_pipeline) {
	if (pipeline.is_valid()) {
		pipeline->disconnect(SNAME("property_list_changed"), callable_mp(this, &EditorWorldECS::_changed_pipeline_callback));
	}

	pipeline = p_pipeline;

	if (pipeline.is_valid()) {
		pipeline->connect(SNAME("property_list_changed"), callable_mp(this, &EditorWorldECS::_changed_pipeline_callback));
	}

	pipeline_features_update();
	pipeline_errors_warnings_update();
}

void EditorWorldECS::pipeline_change_name(const String &p_name) {
	if (pipeline.is_null()) {
		// Nothing to do.
		return;
	}

	EditorUndoRedoManager::get_singleton()->create_action(TTR("Change pipeline name"));
	EditorUndoRedoManager::get_singleton()->add_do_method(pipeline.ptr(), SNAME("set_pipeline_name"), p_name);
	EditorUndoRedoManager::get_singleton()->add_undo_method(pipeline.ptr(), SNAME("set_pipeline_name"), pipeline->get_pipeline_name());
	EditorUndoRedoManager::get_singleton()->commit_action();
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
		pipeline_name_ledit->set_text("");
	} else {
		pipeline_name_ledit->set_text(pipeline->get_pipeline_name());
	}
	// Always position the cursor at the end.
	pipeline_name_ledit->set_caret_column(INT32_MAX);
	pipeline_features_update();
	pipeline_errors_warnings_update();
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
	pip.instantiate();
	pip->set_pipeline_name(name);
	set_pipeline(pip);

	EditorUndoRedoManager::get_singleton()->create_action(TTR("Add pipeline"));
	EditorUndoRedoManager::get_singleton()->add_do_method(world_ecs, SNAME("add_pipeline"), pip);
	EditorUndoRedoManager::get_singleton()->add_undo_method(world_ecs, SNAME("remove_pipeline"), pip);
	EditorUndoRedoManager::get_singleton()->commit_action();
}

void EditorWorldECS::pipeline_rename_show_window() {
	const Vector2i modal_pos = (Vector2i(get_viewport_rect().size) - pipeline_window_rename->get_size()) / 2.0;
	pipeline_window_rename->set_position(modal_pos);
	pipeline_window_rename->set_visible(true);
}

void EditorWorldECS::pipeline_remove_show_confirmation() {
	const Vector2i modal_pos = (Vector2i(get_viewport_rect().size) - pipeline_window_confirm_remove->get_size()) / 2.0;
	pipeline_window_confirm_remove->set_position(modal_pos);
	pipeline_window_confirm_remove->set_visible(true);
}

void EditorWorldECS::pipeline_remove() {
	if (world_ecs == nullptr || pipeline.is_null()) {
		return;
	}

	EditorUndoRedoManager::get_singleton()->create_action(TTR("Pipeline remove"));
	EditorUndoRedoManager::get_singleton()->add_do_method(world_ecs, SNAME("remove_pipeline"), pipeline);
	EditorUndoRedoManager::get_singleton()->add_undo_method(world_ecs, SNAME("add_pipeline"), pipeline);
	EditorUndoRedoManager::get_singleton()->commit_action();
}

void EditorWorldECS::pipeline_toggle_pipeline_view() {
	main_container_pipeline_view->set_visible(!main_container_pipeline_view->is_visible());
	pipeline_view_update();
}

void EditorWorldECS::pipeline_errors_warnings_update() {
	clear_errors_warnings();
	bool show = false;

	if (pipeline.is_valid()) {
		if (!pipeline->editor_get_execution_graph()->is_valid()) {
			add_error(pipeline->editor_get_execution_graph()->get_error_msg());
			show = true;
		}

		for (int i = 0; i < pipeline->editor_get_execution_graph()->get_warnings().size(); i += 1) {
			add_warning(pipeline->editor_get_execution_graph()->get_warnings()[i]);
			show = true;
		}
	}

	errors_warnings_panel->set_visible(show);
}

void EditorWorldECS::pipeline_features_update() {
	pipeline_view_update();

	pipeline_panel_clear();

	if (pipeline.is_null()) {
		// Nothing more to do.
		return;
	}

	Vector<StringName> systems = pipeline->get_systems_name();
	for (int i = 0; i < systems.size(); i += 1) {
		PipelineElementInfoBox *info_box = pipeline_panel_add_entry();

		const StringName system_name = systems[i];

		// Init a native system.
		const uint32_t system_id = ECS::get_system_id(system_name);
		if (system_id == UINT32_MAX) {
			info_box->setup_system(system_name, PipelineElementInfoBox::SYSTEM_INVALID);
		} else {
			SystemExeInfo system_exec_info;
			if (ECS::is_temporary_system(system_id) == false) {
				ECS::get_system_exe_info(system_id, system_exec_info);
			}

			const StringName key_name = ECS::get_system_name(system_id);

			if (ECS::is_system_dispatcher(system_id)) {
				// This is a dispatcher system, don't print the dependencies.
				info_box->setup_system(key_name, PipelineElementInfoBox::SYSTEM_DISPATCHER);
			} else if (ECS::is_temporary_system(system_id)) {
				// TemporarySystem
				info_box->setup_system(key_name, PipelineElementInfoBox::SYSTEM_TEMPORARY);
			} else if (ECS::is_dynamic_system(system_id)) {
				// Script System
				info_box->setup_system(key_name, PipelineElementInfoBox::SYSTEM_SCRIPT);
			} else {
				// Normal native system, add dependencies.
				info_box->setup_system(key_name, PipelineElementInfoBox::SYSTEM_NATIVE);
			}
		}
	}

	Vector<StringName> bundles = pipeline->get_system_bundles();
	for (int i = 0; i < bundles.size(); i += 1) {
		PipelineElementInfoBox *info_box = pipeline_panel_add_entry();
		info_box->set_is_bundle(true);

		const StringName bundle_name = bundles[i];

		// Init as bundle
		const uint32_t id = ECS::get_system_bundle_id(bundle_name);
		if (id == UINT32_MAX) {
			info_box->setup_system(bundle_name, PipelineElementInfoBox::SYSTEM_INVALID);
		} else {
			info_box->setup_system(bundle_name, PipelineElementInfoBox::SYSTEM_BUNDLE);
			info_box->set_extra_info(TTR("Contains ") + itos(ECS::get_system_bundle_systems_count(id)) + TTR(" systems."));
		}
	}
}

Color get_bg_color_by_deepness(int p_deep) {
	p_deep += 1;
	Color color(0.12, 0.145, 0.192);
	color.set_v(CLAMP(color.get_v() + 0.1 * real_t(p_deep), 0.0, 1.0));
	return color;
}

void pipeline_dispatcher_view_update(DispatcherPipelineView *p_view, Ref<ExecutionGraph::Dispatcher> p_dispatcher, int p_deepness) {
	uint32_t stage_id = 0;
	const List<ExecutionGraph::StageNode> &stages = p_dispatcher->stages;
	for (const List<ExecutionGraph::StageNode>::Element *e = stages.front(); e; e = e->next(), stage_id += 1) {
		StageView *stage_view = p_view->add_stage();
		stage_view->setup_stage(stage_id);
		for (uint32_t i = 0; i < e->get().systems.size(); i += 1) {
			if (e->get().systems[i]->sub_dispatcher.is_valid()) {
				// This system is a sub duspatcher.
				DispatcherPipelineView *sub_view = stage_view->add_sub_dispatcher();
				sub_view->set_dispatcher_name(ECS::get_system_name(e->get().systems[i]->id));
				sub_view->set_bg_color(get_bg_color_by_deepness(p_deepness + 1));
				pipeline_dispatcher_view_update(sub_view, e->get().systems[i]->sub_dispatcher, p_deepness + 1);

			} else {
				// This is a standard system
				SystemView *system_view = stage_view->add_system();
				system_view->set_name(ECS::get_system_name(e->get().systems[i]->id));
				system_view->set_bg_color(get_bg_color_by_deepness(p_deepness + 1));
			}
		}
	}
}

void EditorWorldECS::pipeline_view_update() {
	if (!main_container_pipeline_view->is_visible()) {
		// It's not visible, nothing to do.
		return;
	}

	pipeline_view_clear();
	if (pipeline.is_null()) {
		// Nothing more to do.
		return;
	}

	const ExecutionGraph *graph = pipeline->editor_get_execution_graph();
	Ref<ExecutionGraph::Dispatcher> main_dispatcher = graph->get_main_dispatcher();
	if (main_dispatcher.is_null()) {
		// Nothing to do.
		return;
	}

	int deepness = 0;
	DispatcherPipelineView *view = pipeline_view_add_dispatcher();
	view->set_dispatcher_name("Main");
	view->set_bg_color(get_bg_color_by_deepness(deepness));

	pipeline_dispatcher_view_update(view, main_dispatcher, deepness);
}

void EditorWorldECS::pipeline_system_bundle_remove(const StringName &p_name) {
	if (pipeline.is_null()) {
		return;
	}

	EditorUndoRedoManager::get_singleton()->create_action(TTR("Remove system"));
	EditorUndoRedoManager::get_singleton()->add_do_method(pipeline.ptr(), SNAME("remove_system_bundle"), p_name);
	EditorUndoRedoManager::get_singleton()->add_undo_method(pipeline.ptr(), SNAME("add_system_bundle"), p_name);
	EditorUndoRedoManager::get_singleton()->commit_action();
}

void EditorWorldECS::pipeline_system_remove(const StringName &p_name) {
	if (pipeline.is_null()) {
		return;
	}

	EditorUndoRedoManager::get_singleton()->create_action(TTR("Remove system"));
	EditorUndoRedoManager::get_singleton()->add_do_method(pipeline.ptr(), SNAME("remove_system"), p_name);
	EditorUndoRedoManager::get_singleton()->add_undo_method(pipeline.ptr(), SNAME("insert_system"), p_name);
	EditorUndoRedoManager::get_singleton()->commit_action();
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
	if (search.is_empty()) {
		search = add_sys_search->get_text();
	}
	search = search.to_lower();

	add_sys_tree->clear();

	TreeItem *root = add_sys_tree->create_item();
	root->set_text(0, "Systems");
	root->set_selectable(0, false);

	// System Bundles
	TreeItem *system_bundles_root = nullptr;

	for (uint32_t bundle_id = 0; bundle_id < ECS::get_system_bundle_count(); bundle_id += 1) {
		const StringName key_name = ECS::get_system_bundle_name(bundle_id);
		const String desc = ECS::get_system_bundle_desc(bundle_id);

		const String name(String(key_name).to_lower());
		if (search.is_empty() == false && name.find(search) == -1) {
			// System filtered.
			continue;
		}

		if (system_bundles_root == nullptr) {
			// Add only if needed.
			system_bundles_root = add_sys_tree->create_item(root);
			system_bundles_root->set_text(0, "System bundles");
			system_bundles_root->set_selectable(0, false);
			system_bundles_root->set_custom_color(0, Color(0.0, 0.9, 0.3));
		}

		TreeItem *item = add_sys_tree->create_item(system_bundles_root);
		item->set_icon(0, editor->get_gui_base()->get_theme_icon(SNAME("Load"), SNAME("EditorIcons")));
		item->set_text(0, key_name);
		item->set_meta("system_bundle_name", key_name);
		item->set_meta("desc", desc);
	}

	// Systems
	TreeItem *systems_root = nullptr;

	for (uint32_t system_id = 0; system_id < ECS::get_systems_count(); system_id += 1) {
		const StringName key_name = ECS::get_system_name(system_id);
		const String desc = ECS::get_system_desc(system_id);

		const String name(String(key_name).to_lower());
		if (search.is_empty() == false && name.find(search) == -1) {
			// System filtered.
			continue;
		}

		if (systems_root == nullptr) {
			// Add only if needed.
			systems_root = add_sys_tree->create_item(root);
			systems_root->set_text(0, "Systems");
			systems_root->set_selectable(0, false);
			systems_root->set_custom_color(0, Color(0.0, 0.9, 0.3));
		}

		TreeItem *item = add_sys_tree->create_item(systems_root);
		if (ECS::is_system_dispatcher(system_id)) {
			item->set_icon(0, editor->get_gui_base()->get_theme_icon(SNAME("ShaderMaterial"), SNAME("EditorIcons")));
		} else if (ECS::is_temporary_system(system_id)) {
			item->set_icon(0, editor->get_gui_base()->get_theme_icon(SNAME("Time"), SNAME("EditorIcons")));
		} else if (ECS::is_dynamic_system(system_id)) {
			item->set_icon(0, editor->get_gui_base()->get_theme_icon(SNAME("Script"), SNAME("EditorIcons")));
		} else {
			item->set_icon(0, editor->get_gui_base()->get_theme_icon(SNAME("ShaderGlobalsOverride"), SNAME("EditorIcons")));
		}
		item->set_text(0, key_name);
		item->set_meta("system_name", key_name);
		item->set_meta("desc", desc);
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
		if (world_ecs->get_pipelines().is_empty()) {
			// No pipelines, just create the default one.
			Ref<PipelineECS> def_pip;
			def_pip.instantiate();
			def_pip->set_pipeline_name("Default");
			world_ecs->get_pipelines().push_back(def_pip);
			world_ecs->notify_property_list_changed();
		}
		set_pipeline(world_ecs->get_pipelines()[0]);
		pipeline_list_update();
	}

	if (selected->has_meta("system_name")) {
		EditorUndoRedoManager::get_singleton()->create_action(TTR("Add system"));
		EditorUndoRedoManager::get_singleton()->add_do_method(pipeline.ptr(), SNAME("insert_system"), selected->get_meta("system_name"));
		EditorUndoRedoManager::get_singleton()->add_undo_method(pipeline.ptr(), SNAME("remove_system"), selected->get_meta("system_name"));
	} else {
		EditorUndoRedoManager::get_singleton()->create_action(TTR("Add system bundle"));
		EditorUndoRedoManager::get_singleton()->add_do_method(pipeline.ptr(), SNAME("add_system_bundle"), selected->get_meta("system_bundle_name"));
		EditorUndoRedoManager::get_singleton()->add_undo_method(pipeline.ptr(), SNAME("remove_system_bundle"), selected->get_meta("system_bundle_name"));
	}
	EditorUndoRedoManager::get_singleton()->commit_action();
}

void EditorWorldECS::components_manage_show() {
	// Display the modal window centered.
	const Vector2i modal_pos = (Vector2i(get_viewport_rect().size) - components_window->get_size()) / 2.0;
	components_window->set_position(modal_pos);
	components_window->set_visible(true);
}

void EditorWorldECS::components_manage_on_component_select() {
}

void EditorWorldECS::add_error(const String &p_msg) {
	Label *lbl = memnew(Label);
	lbl->set_text("- [Error] " + p_msg);
	lbl->add_theme_color_override(SNAME("font_color"), Color(0.95, 0.05, 0));
	lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	errors_warnings_container->add_child(lbl);
}

void EditorWorldECS::add_warning(const String &p_msg) {
	Label *lbl = memnew(Label);
	lbl->set_text("- [Warning] " + p_msg);
	lbl->add_theme_color_override(SNAME("font_color"), Color(0.96, 0.9, 0.45));
	lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	errors_warnings_container->add_child(lbl);
}

void EditorWorldECS::remove_node_and_reparent_children(Node *p_node) {
	List<Node *> children;

	while (true) {
		bool clear = true;
		for (int i = 0; i < p_node->get_child_count(false); i++) {
			Node *c_node = p_node->get_child(i, false);
			if (!c_node->get_owner()) {
				continue;
			}

			p_node->remove_child(c_node);
			children.push_back(c_node);
			clear = false;
			break;
		}

		if (clear) {
			break;
		}
	}

	while (!children.is_empty()) {
		Node *c_node = children.front()->get();
		p_node->get_parent()->add_child(c_node);
		children.pop_front();
	}

	p_node->get_parent()->remove_child(p_node);
}

void EditorWorldECS::clear_errors_warnings() {
	for (int i = errors_warnings_container->get_child_count() - 1; i >= 0; i -= 1) {
		Node *n = errors_warnings_container->get_child(i);
		remove_node_and_reparent_children(n);
		memdelete(n);
	}
}

void EditorWorldECS::_changed_world_callback() {
	pipeline_list_update();
}

void EditorWorldECS::_changed_pipeline_callback() {
	pipeline_list_update();
	pipeline_features_update();
	pipeline_errors_warnings_update();
}

PipelineElementInfoBox *EditorWorldECS::pipeline_panel_add_entry() {
	PipelineElementInfoBox *info_box = memnew(PipelineElementInfoBox(editor, this));

	pipeline_panel->add_child(info_box);

	return info_box;
}

void EditorWorldECS::pipeline_panel_clear() {
	for (int i = pipeline_panel->get_child_count() - 1; i >= 0; i -= 1) {
		Node *n = pipeline_panel->get_child(i);
		remove_node_and_reparent_children(n);
		memdelete(n);
	}
}

DispatcherPipelineView *EditorWorldECS::pipeline_view_add_dispatcher() {
	DispatcherPipelineView *info_box = memnew(DispatcherPipelineView(editor, this));
	pipeline_view_panel->add_child(info_box);
	return info_box;
}

void EditorWorldECS::pipeline_view_clear() {
	for (int i = pipeline_view_panel->get_child_count() - 1; i >= 0; i -= 1) {
		Node *n = pipeline_view_panel->get_child(i);
		remove_node_and_reparent_children(n);
		memdelete(n);
	}
}

WorldECSEditorPlugin::WorldECSEditorPlugin(EditorNode *p_node) :
		editor(p_node) {
	ecs_editor = memnew(EditorWorldECS(p_node));
	editor->get_main_screen_control()->add_child(ecs_editor);
	ecs_editor->hide_editor();
}

WorldECSEditorPlugin::~WorldECSEditorPlugin() {
	editor->get_main_screen_control()->remove_child(ecs_editor);
	memdelete(ecs_editor);
	ecs_editor = nullptr;
}

void WorldECSEditorPlugin::edit(Object *p_object) {
	if (p_object == nullptr) {
		return;
	}
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
