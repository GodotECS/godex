/* Author: AndreaCatania */

#include "entity_editor_plugin.h"
#include "core/io/marshalls.h"
#include "editor/editor_properties.h"
#include "editor/editor_properties_array_dict.h"
#include "modules/ecs/nodes/ecs_utilities.h"
#include "modules/ecs/nodes/entity.h"

void EntityEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("update_editors"), &EntityEditor::update_editors);
}

EntityEditor::EntityEditor(
		EditorInspectorPluginEntity *p_plugin,
		EditorNode *p_editor,
		Entity *p_entity) :
		editor(p_editor),
		editor_plugin(p_plugin),
		entity(p_entity) {
}

EntityEditor::~EntityEditor() {
}

void EntityEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			create_editors();
			update_editors();
			break;
		}
	}
}

void EntityEditor::create_editors() {
	const Color section_color = get_theme_color("prop_subsection", "Editor");

	add_component_menu = memnew(MenuButton);
	add_component_menu->set_text("Add component");
	add_component_menu->set_icon(editor->get_theme_base()->get_theme_icon("New", "EditorIcons"));
	add_component_menu->set_flat(false);
	add_component_menu->get_popup()->connect("id_pressed", callable_mp(this, &EntityEditor::_add_component_pressed));
	add_child(add_component_menu);

	// TODO right now this is not customizable.
	//EditorInspectorCategory *category = memnew(EditorInspectorCategory);
	//add_child(category);

	components_section = memnew(EditorInspectorSection);
	components_section->setup("components", "Components", entity, section_color, false);
	add_child(components_section);
	components_section->unfold();
}

void EntityEditor::update_editors() {
	const Color section_color = get_theme_color("prop_subsection", "Editor");

	if (add_component_menu) {
		// Remove all old components.
		add_component_menu->get_popup()->clear();

		const LocalVector<StringName> &components = ECS::get_registered_components();
		for (uint32_t i = 0; i < components.size(); i += 1) {
			add_component_menu->get_popup()->add_item(components[i]);
		}

		// Make sure to load all the components.
		const LocalVector<Ref<Component>> &scripts = ScriptECS::get_components();
		for (uint32_t i = 0; i < scripts.size(); i += 1) {
			add_component_menu->get_popup()->add_item(scripts[i]->get_name());
		}
	}

	if (components_section) {
		// Remove old childs.
		for (int i = components_section->get_vbox()->get_child_count() - 1; i >= 0; i -= 1) {
			components_section->get_vbox()->get_child(i)->queue_delete(); // TODO is this enough to also destroy the internally created things?
		}
		components_properties.clear();

		const Dictionary &components = entity->get_components_data();
		for (const Variant *key = components.next(nullptr); key != nullptr; key = components.next(key)) {
			// Add the components of this Entity
			EditorInspectorSection *component_section = memnew(EditorInspectorSection);
			component_section->setup("component_" + String(*key), String(*key), entity, section_color, true);
			component_section->unfold();

			Button *del_btn = memnew(Button);
			del_btn->set_text("Drop");
			del_btn->set_icon(editor->get_theme_base()->get_theme_icon("Remove", "EditorIcons"));
			del_btn->set_flat(false);
			del_btn->set_text_align(Button::ALIGN_LEFT);
			del_btn->connect("pressed", callable_mp(this, &EntityEditor::_remove_component_pressed), varray(key->operator StringName()));
			component_section->get_vbox()->add_child(del_btn);

			create_component_inspector(key->operator StringName(), component_section->get_vbox());

			components_section->get_vbox()->add_child(component_section);

			update_component_inspector(key->operator StringName());
		}
	}
}

void EntityEditor::create_component_inspector(StringName p_component_name, VBoxContainer *p_container) {
	List<PropertyInfo> properties;

	if (String(p_component_name).ends_with(".gd")) {
		const uint32_t id = ScriptECS::get_component_id(p_component_name);
		if (id != UINT32_MAX) {
			Ref<Component> component = ScriptECS::get_component(id);
			component->get_component_property_list(&properties);
		}
	} else {
		const LocalVector<PropertyInfo> *props = ECS::get_component_properties(ECS::get_component_id(p_component_name));
		for (uint32_t i = 0; i < props->size(); i += 1) {
			properties.push_back((*props)[i]);
		}
	}

	const float default_float_step = EDITOR_GET("interface/inspector/default_float_step");

	OAHashMap<StringName, EditorProperty *> editor_properties;
	for (List<PropertyInfo>::Element *e = properties.front(); e; e = e->next()) {
		EditorProperty *prop = nullptr;

		switch (e->get().type) {
			case Variant::NIL: {
				prop = memnew(EditorPropertyNil);

			} break;

			case Variant::BOOL: {
				prop = memnew(EditorPropertyCheck);

			} break;
			case Variant::INT: {
				if (e->get().hint == PROPERTY_HINT_ENUM) {
					EditorPropertyEnum *editor = memnew(EditorPropertyEnum);
					Vector<String> options = e->get().hint_string.split(",");
					editor->setup(options);
					prop = editor;

				} else if (e->get().hint == PROPERTY_HINT_FLAGS) {
					EditorPropertyFlags *editor = memnew(EditorPropertyFlags);
					Vector<String> options = e->get().hint_string.split(",");
					editor->setup(options);
					prop = editor;

				} else if (e->get().hint == PROPERTY_HINT_LAYERS_2D_PHYSICS || e->get().hint == PROPERTY_HINT_LAYERS_2D_RENDER || e->get().hint == PROPERTY_HINT_LAYERS_3D_PHYSICS || e->get().hint == PROPERTY_HINT_LAYERS_3D_RENDER) {
					EditorPropertyLayers::LayerType lt = EditorPropertyLayers::LAYER_RENDER_2D;
					switch (e->get().hint) {
						case PROPERTY_HINT_LAYERS_2D_RENDER:
							lt = EditorPropertyLayers::LAYER_RENDER_2D;
							break;
						case PROPERTY_HINT_LAYERS_2D_PHYSICS:
							lt = EditorPropertyLayers::LAYER_PHYSICS_2D;
							break;
						case PROPERTY_HINT_LAYERS_3D_RENDER:
							lt = EditorPropertyLayers::LAYER_RENDER_3D;
							break;
						case PROPERTY_HINT_LAYERS_3D_PHYSICS:
							lt = EditorPropertyLayers::LAYER_PHYSICS_3D;
							break;
						default: {
						}
					}
					EditorPropertyLayers *editor = memnew(EditorPropertyLayers);
					editor->setup(lt);

				} else if (e->get().hint == PROPERTY_HINT_OBJECT_ID) {
					EditorPropertyObjectID *editor = memnew(EditorPropertyObjectID);
					editor->setup("Object");
					prop = editor;

				} else {
					EditorPropertyInteger *editor = memnew(EditorPropertyInteger);
					int min = 0, max = 65535, step = 1;
					bool greater = true, lesser = true;

					if (e->get().hint == PROPERTY_HINT_RANGE && e->get().hint_string.get_slice_count(",") >= 2) {
						greater = false; //if using ranged, assume false by default
						lesser = false;
						min = e->get().hint_string.get_slice(",", 0).to_int();
						max = e->get().hint_string.get_slice(",", 1).to_int();

						if (e->get().hint_string.get_slice_count(",") >= 3) {
							step = e->get().hint_string.get_slice(",", 2).to_int();
						}

						for (int i = 2; i < e->get().hint_string.get_slice_count(","); i++) {
							const String slice = e->get().hint_string.get_slice(",", i).strip_edges();
							if (slice == "or_greater") {
								greater = true;
							}
							if (slice == "or_lesser") {
								lesser = true;
							}
						}
					}

					editor->setup(min, max, step, greater, lesser);
					prop = editor;
				}
			} break;
			case Variant::FLOAT: {
				if (e->get().hint == PROPERTY_HINT_EXP_EASING) {
					EditorPropertyEasing *editor = memnew(EditorPropertyEasing);
					bool full = true;
					bool flip = false;
					Vector<String> hints = e->get().hint_string.split(",");
					for (int i = 0; i < hints.size(); i++) {
						String h = hints[i].strip_edges();
						if (h == "attenuation") {
							flip = true;
						}
						if (h == "inout") {
							full = true;
						}
					}

					editor->setup(full, flip);
					prop = editor;

				} else {
					EditorPropertyFloat *editor = memnew(EditorPropertyFloat);
					double min = -65535, max = 65535, step = default_float_step;
					bool hide_slider = true;
					bool exp_range = false;
					bool greater = true, lesser = true;

					if ((e->get().hint == PROPERTY_HINT_RANGE || e->get().hint == PROPERTY_HINT_EXP_RANGE) && e->get().hint_string.get_slice_count(",") >= 2) {
						greater = false; //if using ranged, assume false by default
						lesser = false;
						min = e->get().hint_string.get_slice(",", 0).to_float();
						max = e->get().hint_string.get_slice(",", 1).to_float();
						if (e->get().hint_string.get_slice_count(",") >= 3) {
							step = e->get().hint_string.get_slice(",", 2).to_float();
						}
						hide_slider = false;
						exp_range = e->get().hint == PROPERTY_HINT_EXP_RANGE;
						for (int i = 2; i < e->get().hint_string.get_slice_count(","); i++) {
							const String slice = e->get().hint_string.get_slice(",", i).strip_edges();
							if (slice == "or_greater") {
								greater = true;
							}
							if (slice == "or_lesser") {
								lesser = true;
							}
						}
					}

					editor->setup(min, max, step, hide_slider, exp_range, greater, lesser);
					prop = editor;
				}

			} break;
			case Variant::STRING: {
				if (e->get().hint == PROPERTY_HINT_ENUM) {
					EditorPropertyTextEnum *editor = memnew(EditorPropertyTextEnum);
					Vector<String> options = e->get().hint_string.split(",");
					editor->setup(options);
					prop = editor;

				} else if (e->get().hint == PROPERTY_HINT_MULTILINE_TEXT) {
					EditorPropertyMultilineText *editor = memnew(EditorPropertyMultilineText);
					prop = editor;

				} else if (e->get().hint == PROPERTY_HINT_TYPE_STRING) {
					EditorPropertyClassName *editor = memnew(EditorPropertyClassName);
					editor->setup("Object", e->get().hint_string);
					prop = editor;

				} else if (e->get().hint == PROPERTY_HINT_DIR || e->get().hint == PROPERTY_HINT_FILE || e->get().hint == PROPERTY_HINT_SAVE_FILE || e->get().hint == PROPERTY_HINT_GLOBAL_DIR || e->get().hint == PROPERTY_HINT_GLOBAL_FILE) {
					Vector<String> extensions = e->get().hint_string.split(",");
					bool global = e->get().hint == PROPERTY_HINT_GLOBAL_DIR || e->get().hint == PROPERTY_HINT_GLOBAL_FILE;
					bool folder = e->get().hint == PROPERTY_HINT_DIR || e->get().hint == PROPERTY_HINT_GLOBAL_DIR;
					bool save = e->get().hint == PROPERTY_HINT_SAVE_FILE;
					EditorPropertyPath *editor = memnew(EditorPropertyPath);
					editor->setup(extensions, folder, global);
					if (save) {
						editor->set_save_mode();
					}
					prop = editor;

				} else if (e->get().hint == PROPERTY_HINT_METHOD_OF_VARIANT_TYPE ||
						   e->get().hint == PROPERTY_HINT_METHOD_OF_BASE_TYPE ||
						   e->get().hint == PROPERTY_HINT_METHOD_OF_INSTANCE ||
						   e->get().hint == PROPERTY_HINT_METHOD_OF_SCRIPT ||
						   e->get().hint == PROPERTY_HINT_PROPERTY_OF_VARIANT_TYPE ||
						   e->get().hint == PROPERTY_HINT_PROPERTY_OF_BASE_TYPE ||
						   e->get().hint == PROPERTY_HINT_PROPERTY_OF_INSTANCE ||
						   e->get().hint == PROPERTY_HINT_PROPERTY_OF_SCRIPT) {
					EditorPropertyMember *editor = memnew(EditorPropertyMember);

					EditorPropertyMember::Type type = EditorPropertyMember::MEMBER_METHOD_OF_BASE_TYPE;
					switch (e->get().hint) {
						case PROPERTY_HINT_METHOD_OF_BASE_TYPE:
							type = EditorPropertyMember::MEMBER_METHOD_OF_BASE_TYPE;
							break;
						case PROPERTY_HINT_METHOD_OF_INSTANCE:
							type = EditorPropertyMember::MEMBER_METHOD_OF_INSTANCE;
							break;
						case PROPERTY_HINT_METHOD_OF_SCRIPT:
							type = EditorPropertyMember::MEMBER_METHOD_OF_SCRIPT;
							break;
						case PROPERTY_HINT_PROPERTY_OF_VARIANT_TYPE:
							type = EditorPropertyMember::MEMBER_PROPERTY_OF_VARIANT_TYPE;
							break;
						case PROPERTY_HINT_PROPERTY_OF_BASE_TYPE:
							type = EditorPropertyMember::MEMBER_PROPERTY_OF_BASE_TYPE;
							break;
						case PROPERTY_HINT_PROPERTY_OF_INSTANCE:
							type = EditorPropertyMember::MEMBER_PROPERTY_OF_INSTANCE;
							break;
						case PROPERTY_HINT_PROPERTY_OF_SCRIPT:
							type = EditorPropertyMember::MEMBER_PROPERTY_OF_SCRIPT;
							break;
						default: {
						}
					}
					editor->setup(type, e->get().hint_string);
					prop = editor;

				} else {
					EditorPropertyText *editor = memnew(EditorPropertyText);
					if (e->get().hint == PROPERTY_HINT_PLACEHOLDER_TEXT) {
						editor->set_placeholder(e->get().hint_string);
					}
					prop = editor;
				}
			} break;

#define SETUP_MATH_RANGE(editor, prop_info, type)                                                   \
	type min = -65535, max = 65535;                                                                 \
	bool hide_slider = true;                                                                        \
																									\
	if (prop_info.hint == PROPERTY_HINT_RANGE && prop_info.hint_string.get_slice_count(",") >= 2) { \
		min = e->get().hint_string.get_slice(",", 0).to_float();                                    \
		max = e->get().hint_string.get_slice(",", 1).to_float();                                    \
		hide_slider = false;                                                                        \
	}                                                                                               \
																									\
	editor->setup(min, max, hide_slider);

#define SETUP_MATH_RANGE_WITH_STEP(editor, prop_info, type)                                         \
	type min = -65535, max = 65535, step = default_float_step;                                      \
	bool hide_slider = true;                                                                        \
																									\
	if (prop_info.hint == PROPERTY_HINT_RANGE && prop_info.hint_string.get_slice_count(",") >= 2) { \
		min = prop_info.hint_string.get_slice(",", 0).to_float();                                   \
		max = prop_info.hint_string.get_slice(",", 1).to_float();                                   \
		if (prop_info.hint_string.get_slice_count(",") >= 3) {                                      \
			step = prop_info.hint_string.get_slice(",", 2).to_float();                              \
		}                                                                                           \
		hide_slider = false;                                                                        \
	}                                                                                               \
																									\
	editor->setup(min, max, step, hide_slider);
			// math types
			case Variant::VECTOR2: {
				EditorPropertyVector2 *editor = memnew(EditorPropertyVector2);
				SETUP_MATH_RANGE_WITH_STEP(editor, e->get(), double);
				prop = editor;

			} break;
			case Variant::VECTOR2I: {
				EditorPropertyVector2i *editor = memnew(EditorPropertyVector2i);
				SETUP_MATH_RANGE(editor, e->get(), int);
				prop = editor;

			} break;
			case Variant::RECT2: {
				EditorPropertyRect2 *editor = memnew(EditorPropertyRect2);
				SETUP_MATH_RANGE_WITH_STEP(editor, e->get(), double);
				prop = editor;

			} break;
			case Variant::RECT2I: {
				EditorPropertyRect2i *editor = memnew(EditorPropertyRect2i);
				SETUP_MATH_RANGE(editor, e->get(), int);
				prop = editor;

			} break;
			case Variant::VECTOR3: {
				EditorPropertyVector3 *editor = memnew(EditorPropertyVector3);
				SETUP_MATH_RANGE_WITH_STEP(editor, e->get(), double);
				prop = editor;

			} break;
			case Variant::VECTOR3I: {
				EditorPropertyVector3i *editor = memnew(EditorPropertyVector3i);
				SETUP_MATH_RANGE(editor, e->get(), int);
				prop = editor;

			} break;
			case Variant::TRANSFORM2D: {
				EditorPropertyTransform2D *editor = memnew(EditorPropertyTransform2D);
				SETUP_MATH_RANGE_WITH_STEP(editor, e->get(), double);
				prop = editor;

			} break;
			case Variant::PLANE: {
				EditorPropertyPlane *editor = memnew(EditorPropertyPlane);
				SETUP_MATH_RANGE_WITH_STEP(editor, e->get(), double);
				prop = editor;

			} break;
			case Variant::QUAT: {
				EditorPropertyQuat *editor = memnew(EditorPropertyQuat);
				SETUP_MATH_RANGE_WITH_STEP(editor, e->get(), double);
				prop = editor;

			} break;
			case Variant::AABB: {
				EditorPropertyAABB *editor = memnew(EditorPropertyAABB);
				SETUP_MATH_RANGE_WITH_STEP(editor, e->get(), double);
				prop = editor;

			} break;
			case Variant::BASIS: {
				EditorPropertyBasis *editor = memnew(EditorPropertyBasis);
				SETUP_MATH_RANGE_WITH_STEP(editor, e->get(), double);
				prop = editor;

			} break;
			case Variant::TRANSFORM: {
				EditorPropertyTransform *editor = memnew(EditorPropertyTransform);
				SETUP_MATH_RANGE_WITH_STEP(editor, e->get(), double);
				prop = editor;

			} break;

			// misc types
			case Variant::COLOR: {
				EditorPropertyColor *editor = memnew(EditorPropertyColor);
				editor->setup(e->get().hint != PROPERTY_HINT_COLOR_NO_ALPHA);
				prop = editor;

			} break;
			case Variant::STRING_NAME: {
				if (e->get().hint == PROPERTY_HINT_ENUM) {
					EditorPropertyTextEnum *editor = memnew(EditorPropertyTextEnum);
					Vector<String> options = e->get().hint_string.split(",");
					editor->setup(options, true);
					prop = editor;
				} else {
					EditorPropertyText *editor = memnew(EditorPropertyText);
					if (e->get().hint == PROPERTY_HINT_PLACEHOLDER_TEXT) {
						editor->set_placeholder(e->get().hint_string);
					}
					editor->set_string_name(true);
					prop = editor;
				}

			} break;
			case Variant::NODE_PATH: {
				EditorPropertyNodePath *editor = memnew(EditorPropertyNodePath);
				const int usage = 0; // TODO how to integrate this? check /godot/editor/editor_properties.cpp::parse_property
				if (e->get().hint == PROPERTY_HINT_NODE_PATH_TO_EDITED_NODE && e->get().hint_string != String()) {
					editor->setup(e->get().hint_string, Vector<StringName>(), (usage & PROPERTY_USAGE_NODE_PATH_FROM_SCENE_ROOT));
				}
				if (e->get().hint == PROPERTY_HINT_NODE_PATH_VALID_TYPES && e->get().hint_string != String()) {
					Vector<String> types = e->get().hint_string.split(",", false);
					Vector<StringName> sn = Variant(types); //convert via variant
					editor->setup(NodePath(), sn, (usage & PROPERTY_USAGE_NODE_PATH_FROM_SCENE_ROOT));
				}
				prop = editor;

			} break;
			case Variant::RID: {
				prop = memnew(EditorPropertyRID);

			} break;
			case Variant::OBJECT: {
				EditorPropertyResource *editor = memnew(EditorPropertyResource);
				if (e->get().hint == PROPERTY_HINT_RESOURCE_TYPE) {
					editor->setup(e->get().hint_string);
					const String open_in_new = EDITOR_GET("interface/inspector/resources_to_open_in_new_inspector");
					for (int i = 0; i < open_in_new.get_slice_count(","); i++) {
						const String type = open_in_new.get_slicec(',', i).strip_edges();
						for (int j = 0; j < e->get().hint_string.get_slice_count(","); j++) {
							String inherits = e->get().hint_string.get_slicec(',', j);
							if (ClassDB::is_parent_class(inherits, type)) {
								editor->set_use_sub_inspector(false);
							}
						}
					}
				} else {
					editor->setup("Resource");
				}
				prop = editor;

			} break;
			case Variant::DICTIONARY: {
				prop = memnew(EditorPropertyDictionary);

			} break;
			case Variant::ARRAY: {
				EditorPropertyArray *editor = memnew(EditorPropertyArray);
				editor->setup(Variant::ARRAY, e->get().hint_string);
				prop = editor;
			} break;

			// arrays
			case Variant::PACKED_BYTE_ARRAY: {
				EditorPropertyArray *editor = memnew(EditorPropertyArray);
				editor->setup(Variant::PACKED_BYTE_ARRAY, e->get().hint_string);
				prop = editor;
			} break;
			case Variant::PACKED_INT32_ARRAY: {
				EditorPropertyArray *editor = memnew(EditorPropertyArray);
				editor->setup(Variant::PACKED_INT32_ARRAY, e->get().hint_string);
				prop = editor;
			} break;
			case Variant::PACKED_FLOAT32_ARRAY: {
				EditorPropertyArray *editor = memnew(EditorPropertyArray);
				editor->setup(Variant::PACKED_FLOAT32_ARRAY, e->get().hint_string);
				prop = editor;
			} break;
			case Variant::PACKED_INT64_ARRAY: {
				EditorPropertyArray *editor = memnew(EditorPropertyArray);
				editor->setup(Variant::PACKED_INT64_ARRAY, e->get().hint_string);
				prop = editor;
			} break;
			case Variant::PACKED_FLOAT64_ARRAY: {
				EditorPropertyArray *editor = memnew(EditorPropertyArray);
				editor->setup(Variant::PACKED_FLOAT64_ARRAY, e->get().hint_string);
				prop = editor;
			} break;
			case Variant::PACKED_STRING_ARRAY: {
				EditorPropertyArray *editor = memnew(EditorPropertyArray);
				editor->setup(Variant::PACKED_STRING_ARRAY, e->get().hint_string);
				prop = editor;
			} break;
			case Variant::PACKED_VECTOR2_ARRAY: {
				EditorPropertyArray *editor = memnew(EditorPropertyArray);
				editor->setup(Variant::PACKED_VECTOR2_ARRAY, e->get().hint_string);
				prop = editor;
			} break;
			case Variant::PACKED_VECTOR3_ARRAY: {
				EditorPropertyArray *editor = memnew(EditorPropertyArray);
				editor->setup(Variant::PACKED_VECTOR3_ARRAY, e->get().hint_string);
				prop = editor;
			} break;
			case Variant::PACKED_COLOR_ARRAY: {
				EditorPropertyArray *editor = memnew(EditorPropertyArray);
				editor->setup(Variant::PACKED_COLOR_ARRAY, e->get().hint_string);
				prop = editor;
			} break;
			default: {
			}
		}

		if (prop != nullptr) {
			prop->set_label(e->get().name.capitalize());
			p_container->add_child(prop);

			prop->set_object_and_property(entity, String(p_component_name) + "/" + e->get().name);
			prop->connect("property_changed", callable_mp(this, &EntityEditor::_property_changed));
			editor_properties.insert(e->get().name, prop);
		}
	}
	components_properties.insert(p_component_name, editor_properties);
}

void EntityEditor::update_component_inspector(StringName p_component_name) {
	OAHashMap<StringName, EditorProperty *> *props = components_properties.lookup_ptr(p_component_name);
	if (props == nullptr) {
		return;
	}

	for (OAHashMap<StringName, EditorProperty *>::Iterator it = props->iter(); it.valid; it = props->next_iter(it)) {
		if ((*it.value) != nullptr) {
			(*it.value)->update_property();
		}
	}
}

void EntityEditor::_add_component_pressed(uint32_t p_index) {
	StringName component_name;
	if (p_index < ECS::get_registered_components().size()) {
		// This is a native component.
		component_name = ECS::get_registered_components()[p_index];
	} else {
		// This is a GDScript component.
		component_name = add_component_menu->get_popup()->get_item_text(p_index);
	}

	editor->get_undo_redo()->create_action(TTR("Add component"));
	editor->get_undo_redo()->add_do_method(entity, "add_component_data", component_name);
	editor->get_undo_redo()->add_do_method(this, "update_editors");
	editor->get_undo_redo()->add_undo_method(entity, "remove_component_data", component_name);
	editor->get_undo_redo()->add_undo_method(this, "update_editors");
	editor->get_undo_redo()->commit_action();
}

void EntityEditor::_remove_component_pressed(StringName p_component_name) {
	editor->get_undo_redo()->create_action(TTR("Drop component"));
	editor->get_undo_redo()->add_do_method(entity, "remove_component_data", p_component_name);
	editor->get_undo_redo()->add_do_method(this, "update_editors");
	// Undo by setting the old component data, so to not lost the parametes.
	editor->get_undo_redo()->add_undo_method(entity, "__set_components_data", entity->get_components_data().duplicate(true));
	editor->get_undo_redo()->add_undo_method(this, "update_editors");
	editor->get_undo_redo()->commit_action();
}

void EntityEditor::_property_changed(const String &p_path, const Variant &p_value, const String &p_name, bool p_changing) {
	if (p_changing) {
		// Nothing to do while chaning.
		// TODO activate this back when this PR is merged: https://github.com/godotengine/godot/pull/44326
		//return;
	}

	editor->get_undo_redo()->create_action(TTR("Set component value"));
	editor->get_undo_redo()->add_do_method(entity, "set", p_path, p_value);
	// Undo by setting the old component data, so to properly reset to previous.
	editor->get_undo_redo()->add_undo_method(entity, "__set_components_data", entity->get_components_data().duplicate(true));
	editor->get_undo_redo()->add_undo_method(this, "update_editors");
	editor->get_undo_redo()->commit_action();
}

bool EditorInspectorPluginEntity::can_handle(Object *p_object) {
	return Object::cast_to<Entity>(p_object) != nullptr;
}

void EditorInspectorPluginEntity::parse_begin(Object *p_object) {
	Entity *entity = Object::cast_to<Entity>(p_object);
	ERR_FAIL_COND(!entity);

	EntityEditor *entity_editor = memnew(EntityEditor(this, editor, entity));
	add_custom_control(entity_editor);
}

EntityEditorPlugin::EntityEditorPlugin(EditorNode *p_node) {
	Ref<EditorInspectorPluginEntity> entity_plugin;
	entity_plugin.instance();
	entity_plugin->editor = p_node;

	EditorInspector::add_inspector_plugin(entity_plugin);
}
