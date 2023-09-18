#include "entity_editor_plugin.h"
#include "../nodes/ecs_utilities.h"
#include "../nodes/entity.h"
#include "core/io/marshalls.h"
#include "editor/editor_properties.h"
#include "editor/editor_properties_array_dict.h"
#include "editor/editor_settings.h"
#include "editor/editor_undo_redo_manager.h"

void EntityEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("update_editors"), &EntityEditor::update_editors);
}

EntityEditor::EntityEditor(
		EditorInspectorPluginEntity *p_plugin,
		EditorNode *p_editor,
		Node *p_entity) :
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
	const Color section_color = get_theme_color(SNAME("prop_subsection"), "Editor");

	add_component_menu = memnew(MenuButton);
	add_component_menu->set_text(TTR("Add Component"));
	add_component_menu->set_icon(editor->get_theme_base()->get_theme_icon(SNAME("New"), SNAME("EditorIcons")));
	add_component_menu->set_flat(false);
	add_component_menu->get_popup()->connect(SNAME("id_pressed"), callable_mp(this, &EntityEditor::_add_component_pressed));
	add_child(add_component_menu);

	// TODO right now this is not customizable.
	// EditorInspectorCategory *category = memnew(EditorInspectorCategory);
	// add_child(category);

	components_section = memnew(EditorInspectorSection);
	components_section->setup("components", "Components", entity, section_color, false);
	add_child(components_section);
	components_section->unfold();
}

void EntityEditor::update_editors() {
	const Color section_color = get_theme_color(SNAME("prop_subsection"), SNAME("Editor"));

	if (add_component_menu) {
		// Remove all old components.
		add_component_menu->get_popup()->clear();

		const LocalVector<StringName> &components = ECS::get_registered_components();
		for (uint32_t i = 0; i < components.size(); i += 1) {
			add_component_menu->get_popup()->add_item(components[i]);
		}
	}

	if (components_section) {
		// Remove old childs.
		for (int i = components_section->get_vbox()->get_child_count() - 1; i >= 0; i -= 1) {
			components_section->get_vbox()->get_child(i)->queue_free(); // TODO is this enough to also destroy the internally created things?
		}
		components_properties.clear();

		const OAHashMap<StringName, Ref<ComponentDepot>> &components = entity_get_components_data();
		for (OAHashMap<StringName, Ref<ComponentDepot>>::Iterator it = components.iter(); it.valid; it = components.next_iter(it)) {
			// Add the components of this Entity
			EditorInspectorSection *component_section = memnew(EditorInspectorSection);
			component_section->setup("component_" + String(*it.key), String(*it.key), entity, section_color, true);
			component_section->unfold();

			Button *del_btn = memnew(Button);
			del_btn->set_text("Drop");
			del_btn->set_icon(editor->get_theme_base()->get_theme_icon(SNAME("Remove"), SNAME("EditorIcons")));
			del_btn->set_flat(false);
			del_btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
			del_btn->connect(SNAME("pressed"), callable_mp(this, &EntityEditor::_remove_component_pressed).bind(*it.key));
			component_section->get_vbox()->add_child(del_btn);

			create_component_inspector(*it.key, *it.value, component_section->get_vbox());

			components_section->get_vbox()->add_child(component_section);
			update_component_inspector(*it.key);
		}
	}
}

void EntityEditor::create_component_inspector(StringName p_component_name, const Ref<ComponentDepot> &p_depot, VBoxContainer *p_container) {
	ERR_FAIL_COND(!p_depot.is_valid());
	if (ECS::is_component_sharable(ECS::get_component_id(p_component_name))) {
		// The sharable components just have a field that accepts a
		// `SharableComponentResource`.
		EditorPropertyResource *prop_res = memnew(EditorPropertyResource);
		prop_res->setup(entity, entity ? entity->get_path() : NodePath(), "SharedComponentResource");
		prop_res->set_label("Shared Component");
		p_container->add_child(prop_res);

		prop_res->set_object_and_property(entity, String(p_component_name) + "/resource");
		prop_res->connect(SNAME("property_changed"), callable_mp(this, &EntityEditor::_property_changed));

		OAHashMap<StringName, EditorProperty *> editor_properties;
		editor_properties.insert("resource", prop_res);

		components_properties.insert(p_component_name, editor_properties);

	} else {
		const float default_float_step = EDITOR_GET("interface/inspector/default_float_step");

		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND(id == godex::COMPONENT_NONE);
		const bool is_scripted_component = ECS::is_component_dynamic(id);

		List<PropertyInfo> properties;
		p_depot->_get_property_list(&properties);

		OAHashMap<StringName, EditorProperty *> editor_properties;
		for (PropertyInfo &e : properties) {
			EditorProperty *prop = nullptr;

			if (is_scripted_component == false && (e.usage & PROPERTY_USAGE_EDITOR) == 0) {
				// This property is not meant to be displayed on editor.
				continue;
			}

			switch (e.type) {
				case Variant::NIL: {
					prop = memnew(EditorPropertyNil);

				} break;

				case Variant::BOOL: {
					prop = memnew(EditorPropertyCheck);

				} break;
				case Variant::INT: {
					if (e.hint == PROPERTY_HINT_ENUM) {
						EditorPropertyEnum *property_enum = memnew(EditorPropertyEnum);
						Vector<String> options = e.hint_string.split(",");
						property_enum->setup(options);
						prop = property_enum;

					} else if (e.hint == PROPERTY_HINT_FLAGS) {
						EditorPropertyFlags *property_flags = memnew(EditorPropertyFlags);
						Vector<String> options = e.hint_string.split(",");
						property_flags->setup(options);
						prop = property_flags;

					} else if (e.hint == PROPERTY_HINT_LAYERS_2D_PHYSICS || e.hint == PROPERTY_HINT_LAYERS_2D_RENDER || e.hint == PROPERTY_HINT_LAYERS_3D_PHYSICS || e.hint == PROPERTY_HINT_LAYERS_3D_RENDER) {
						EditorPropertyLayers::LayerType lt = EditorPropertyLayers::LAYER_RENDER_2D;
						switch (e.hint) {
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
						EditorPropertyLayers *property_layers = memnew(EditorPropertyLayers);
						property_layers->setup(lt);
						prop = property_layers;

					} else if (e.hint == PROPERTY_HINT_OBJECT_ID) {
						EditorPropertyObjectID *property_object_id = memnew(EditorPropertyObjectID);
						property_object_id->setup("Object");
						prop = property_object_id;

					} else {
						EditorPropertyInteger *property_integer = memnew(EditorPropertyInteger);
						int min = 0, max = 65535, step = 1;
						bool hide_slider = false;
						bool greater = true, lesser = true;

						if (e.hint == PROPERTY_HINT_RANGE && e.hint_string.get_slice_count(",") >= 2) {
							greater = false; // if using ranged, assume false by default
							lesser = false;
							min = e.hint_string.get_slice(",", 0).to_int();
							max = e.hint_string.get_slice(",", 1).to_int();

							if (e.hint_string.get_slice_count(",") >= 3) {
								step = e.hint_string.get_slice(",", 2).to_int();
							}

							for (int i = 2; i < e.hint_string.get_slice_count(","); i++) {
								const String slice = e.hint_string.get_slice(",", i).strip_edges();
								if (slice == "or_greater") {
									greater = true;
								}
								if (slice == "or_lesser") {
									lesser = true;
								}
							}
						}

						property_integer->setup(min, max, step, hide_slider, greater, lesser);
						prop = property_integer;
					}
				} break;
				case Variant::FLOAT: {
					if (e.hint == PROPERTY_HINT_EXP_EASING) {
						EditorPropertyEasing *property_easing = memnew(EditorPropertyEasing);
						bool full = true;
						bool flip = false;
						Vector<String> hints = e.hint_string.split(",");
						for (int i = 0; i < hints.size(); i++) {
							String h = hints[i].strip_edges();
							if (h == "attenuation") {
								flip = true;
							}
							if (h == "inout") {
								full = true;
							}
						}

						property_easing->setup(full, flip);
						prop = property_easing;

					} else {
						EditorPropertyFloat *property_float = memnew(EditorPropertyFloat);
						double min = -65535, max = 65535, step = default_float_step;
						bool hide_slider = true;
						bool exp_range = false;
						bool greater = true, lesser = true;

						if (e.hint == PROPERTY_HINT_RANGE && e.hint_string.get_slice_count(",") >= 2) {
							greater = false; // if using ranged, assume false by default
							lesser = false;
							min = e.hint_string.get_slice(",", 0).to_float();
							max = e.hint_string.get_slice(",", 1).to_float();
							if (e.hint_string.get_slice_count(",") >= 3) {
								step = e.hint_string.get_slice(",", 2).to_float();
							}
							hide_slider = false;
							exp_range = false;
							for (int i = 2; i < e.hint_string.get_slice_count(","); i++) {
								const String slice = e.hint_string.get_slice(",", i).strip_edges();
								if (slice == "or_greater") {
									greater = true;
								}
								if (slice == "or_lesser") {
									lesser = true;
								}
								if (slice == "exp") {
									exp_range = true;
								}
							}
						}

						property_float->setup(min, max, step, hide_slider, exp_range, greater, lesser);
						prop = property_float;
					}

				} break;
				case Variant::STRING: {
					if (e.hint == PROPERTY_HINT_ENUM) {
						EditorPropertyTextEnum *property_text_enum = memnew(EditorPropertyTextEnum);
						Vector<String> options = e.hint_string.split(",");
						property_text_enum->setup(options);
						prop = property_text_enum;

					} else if (e.hint == godex::PROPERTY_HINT_ECS_EVENT_EMITTER) {
						// Show a full list of available event emitters for this event.
						const StringName event_name = e.hint_string;
						godex::event_id event_id = ECS::get_event_id(event_name);
						ERR_CONTINUE_MSG(ECS::verify_event_id(event_id) == false, "The event " + event_name + " doesn't exist.");
						const RBSet<String> &emitters = ECS::get_event_emitters(event_id);

						Vector<String> enum_component_list;
						{
							enum_component_list.resize(emitters.size() + 1);
							String *r = enum_component_list.ptrw();
							r[0] = ""; // Disabled option.
							uint32_t i = 0;
							for (const RBSet<String>::Element *emitter_name = emitters.front(); emitter_name; emitter_name = emitter_name->next()) {
								r[i + 1] = emitter_name->get();
								i += 1;
							}
						}

						EditorPropertyTextEnum *property_text_enum = memnew(EditorPropertyTextEnum);
						property_text_enum->setup(enum_component_list, false);
						prop = property_text_enum;

					} else if (e.hint == godex::PROPERTY_HINT_ECS_SPAWNER) {
						// Show the full list of available spawners for this component.
						const StringName spawner_name = e.hint_string;
						const Vector<StringName> components = ScriptEcs::get_singleton()->spawner_get_components(spawner_name);

						Vector<String> enum_component_list;
						{
							enum_component_list.resize(components.size() + 1);
							String *r = enum_component_list.ptrw();
							r[0] = "";
							for (int i = 0; i < components.size(); i += 1) {
								r[i + 1] = components[i];
							}
						}

						EditorPropertyTextEnum *property_text_enum = memnew(EditorPropertyTextEnum);
						property_text_enum->setup(enum_component_list, false);
						prop = property_text_enum;

					} else if (e.hint == PROPERTY_HINT_MULTILINE_TEXT) {
						EditorPropertyMultilineText *property_multiline_text = memnew(EditorPropertyMultilineText);
						prop = property_multiline_text;

					} else if (e.hint == PROPERTY_HINT_TYPE_STRING) {
						EditorPropertyClassName *property_class_name = memnew(EditorPropertyClassName);
						property_class_name->setup("Object", e.hint_string);
						prop = property_class_name;

					} else if (e.hint == PROPERTY_HINT_DIR || e.hint == PROPERTY_HINT_FILE || e.hint == PROPERTY_HINT_SAVE_FILE || e.hint == PROPERTY_HINT_GLOBAL_DIR || e.hint == PROPERTY_HINT_GLOBAL_FILE) {
						Vector<String> extensions = e.hint_string.split(",");
						bool global = e.hint == PROPERTY_HINT_GLOBAL_DIR || e.hint == PROPERTY_HINT_GLOBAL_FILE;
						bool folder = e.hint == PROPERTY_HINT_DIR || e.hint == PROPERTY_HINT_GLOBAL_DIR;
						bool save = e.hint == PROPERTY_HINT_SAVE_FILE;
						EditorPropertyPath *property_path = memnew(EditorPropertyPath);
						property_path->setup(extensions, folder, global);
						if (save) {
							property_path->set_save_mode();
						}
						prop = property_path;
					} else {
						EditorPropertyText *property_text = memnew(EditorPropertyText);
						if (e.hint == PROPERTY_HINT_PLACEHOLDER_TEXT) {
							property_text->set_placeholder(e.hint_string);
						}
						prop = property_text;
					}
				} break;

#define SETUP_MATH_RANGE(editor, prop_info, type)                                                   \
	type min = -65535, max = 65535;                                                                 \
                                                                                                    \
	if (prop_info.hint == PROPERTY_HINT_RANGE && prop_info.hint_string.get_slice_count(",") >= 2) { \
		min = e.hint_string.get_slice(",", 0).to_float();                                           \
		max = e.hint_string.get_slice(",", 1).to_float();                                           \
	}                                                                                               \
                                                                                                    \
	editor->setup(min, max);

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
					EditorPropertyVector2 *property_vector2 = memnew(EditorPropertyVector2);
					SETUP_MATH_RANGE_WITH_STEP(property_vector2, e, double);
					prop = property_vector2;

				} break;
				case Variant::VECTOR2I: {
					EditorPropertyVector2i *property_vector2i = memnew(EditorPropertyVector2i);
					SETUP_MATH_RANGE(property_vector2i, e, int);
					prop = property_vector2i;

				} break;
				case Variant::RECT2: {
					EditorPropertyRect2 *property_rect2 = memnew(EditorPropertyRect2);
					SETUP_MATH_RANGE_WITH_STEP(property_rect2, e, double);
					prop = property_rect2;

				} break;
				case Variant::RECT2I: {
					EditorPropertyRect2i *property_rect2i = memnew(EditorPropertyRect2i);
					SETUP_MATH_RANGE(property_rect2i, e, int);
					prop = property_rect2i;

				} break;
				case Variant::VECTOR3: {
					EditorPropertyVector3 *property_vector3 = memnew(EditorPropertyVector3);
					SETUP_MATH_RANGE_WITH_STEP(property_vector3, e, double);
					prop = property_vector3;

				} break;
				case Variant::VECTOR3I: {
					EditorPropertyVector3i *property_vector3i = memnew(EditorPropertyVector3i);
					SETUP_MATH_RANGE(property_vector3i, e, int);
					prop = property_vector3i;

				} break;
				case Variant::TRANSFORM2D: {
					EditorPropertyTransform2D *transform_2d = memnew(EditorPropertyTransform2D);
					SETUP_MATH_RANGE_WITH_STEP(transform_2d, e, double);
					prop = transform_2d;

				} break;
				case Variant::PLANE: {
					EditorPropertyPlane *property_plane = memnew(EditorPropertyPlane);
					SETUP_MATH_RANGE_WITH_STEP(property_plane, e, double);
					prop = property_plane;

				} break;
				case Variant::QUATERNION: {
					EditorPropertyQuaternion *property_quaternion = memnew(EditorPropertyQuaternion);
					SETUP_MATH_RANGE_WITH_STEP(property_quaternion, e, double);
					prop = property_quaternion;

				} break;
				case Variant::AABB: {
					EditorPropertyAABB *property_aabb = memnew(EditorPropertyAABB);
					SETUP_MATH_RANGE_WITH_STEP(property_aabb, e, double);
					prop = property_aabb;

				} break;
				case Variant::BASIS: {
					EditorPropertyBasis *property_basis = memnew(EditorPropertyBasis);
					SETUP_MATH_RANGE_WITH_STEP(property_basis, e, double);
					prop = property_basis;

				} break;
				case Variant::TRANSFORM3D: {
					EditorPropertyTransform3D *transform_3d = memnew(EditorPropertyTransform3D);
					SETUP_MATH_RANGE_WITH_STEP(transform_3d, e, double);
					prop = transform_3d;

				} break;

				// misc types
				case Variant::COLOR: {
					EditorPropertyColor *property_color = memnew(EditorPropertyColor);
					property_color->setup(e.hint != PROPERTY_HINT_COLOR_NO_ALPHA);
					prop = property_color;

				} break;
				case Variant::STRING_NAME: {
					if (e.hint == PROPERTY_HINT_ENUM) {
						EditorPropertyTextEnum *property_text_enum = memnew(EditorPropertyTextEnum);
						Vector<String> options = e.hint_string.split(",");
						property_text_enum->setup(options, true);
						prop = property_text_enum;

					} else if (e.hint == godex::PROPERTY_HINT_ECS_SPAWNER) {
						const StringName spawner_name = e.hint_string;
						const Vector<StringName> components = ScriptEcs::get_singleton()->spawner_get_components(spawner_name);

						Vector<String> enum_component_list;
						{
							enum_component_list.resize(components.size() + 1);
							String *r = enum_component_list.ptrw();
							r[0] = "";
							for (int i = 0; i < components.size(); i += 1) {
								r[i + 1] = components[i];
							}
						}

						EditorPropertyTextEnum *property_text_enum = memnew(EditorPropertyTextEnum);
						property_text_enum->setup(enum_component_list, true);
						prop = property_text_enum;

					} else {
						EditorPropertyText *property_text = memnew(EditorPropertyText);
						if (e.hint == PROPERTY_HINT_PLACEHOLDER_TEXT) {
							property_text->set_placeholder(e.hint_string);
						}
						property_text->set_string_name(true);
						prop = property_text;
					}

				} break;
				case Variant::NODE_PATH: {
					EditorPropertyNodePath *property_node_path = memnew(EditorPropertyNodePath);
					const int usage = 0; // TODO how to integrate this? check /modules/godot/property_node_path/editor_properties.cpp::parse_property
					if (e.hint == PROPERTY_HINT_NODE_PATH_TO_EDITED_NODE && e.hint_string != String()) {
						property_node_path->setup(e.hint_string, Vector<StringName>(), (usage & PROPERTY_USAGE_NODE_PATH_FROM_SCENE_ROOT));
					}
					if (e.hint == PROPERTY_HINT_NODE_PATH_VALID_TYPES && e.hint_string != String()) {
						Vector<String> types = e.hint_string.split(",", false);
						Vector<StringName> sn = Variant(types); // convert via variant
						property_node_path->setup(NodePath(), sn, (usage & PROPERTY_USAGE_NODE_PATH_FROM_SCENE_ROOT));
					}
					prop = property_node_path;

				} break;
				case Variant::RID: {
					prop = memnew(EditorPropertyRID);

				} break;
				case Variant::OBJECT: {
					EditorPropertyResource *property_resource = memnew(EditorPropertyResource);
					if (e.hint == PROPERTY_HINT_RESOURCE_TYPE) {
						property_resource->setup(entity, entity ? entity->get_path() : NodePath(), e.hint_string);
						const String open_in_new = EDITOR_GET("interface/inspector/resources_to_open_in_new_inspector");
						for (int i = 0; i < open_in_new.get_slice_count(","); i++) {
							const String type = open_in_new.get_slicec(',', i).strip_edges();
							for (int j = 0; j < e.hint_string.get_slice_count(","); j++) {
								String inherits = e.hint_string.get_slicec(',', j);
								if (ClassDB::is_parent_class(inherits, type)) {
									property_resource->set_use_sub_inspector(false);
								}
							}
						}
					} else {
						property_resource->setup(entity, entity ? entity->get_path() : NodePath(), "Resource");
					}
					prop = property_resource;

				} break;
				case Variant::DICTIONARY: {
					prop = memnew(EditorPropertyDictionary);

				} break;
				case Variant::ARRAY: {
					EditorPropertyArray *property_array = memnew(EditorPropertyArray);
					property_array->setup(Variant::ARRAY, e.hint_string);
					prop = property_array;
				} break;

				// arrays
				case Variant::PACKED_BYTE_ARRAY: {
					EditorPropertyArray *property_array = memnew(EditorPropertyArray);
					property_array->setup(Variant::PACKED_BYTE_ARRAY, e.hint_string);
					prop = property_array;
				} break;
				case Variant::PACKED_INT32_ARRAY: {
					EditorPropertyArray *property_array = memnew(EditorPropertyArray);
					property_array->setup(Variant::PACKED_INT32_ARRAY, e.hint_string);
					prop = property_array;
				} break;
				case Variant::PACKED_FLOAT32_ARRAY: {
					EditorPropertyArray *property_array = memnew(EditorPropertyArray);
					property_array->setup(Variant::PACKED_FLOAT32_ARRAY, e.hint_string);
					prop = property_array;
				} break;
				case Variant::PACKED_INT64_ARRAY: {
					EditorPropertyArray *property_array = memnew(EditorPropertyArray);
					property_array->setup(Variant::PACKED_INT64_ARRAY, e.hint_string);
					prop = property_array;
				} break;
				case Variant::PACKED_FLOAT64_ARRAY: {
					EditorPropertyArray *property_array = memnew(EditorPropertyArray);
					property_array->setup(Variant::PACKED_FLOAT64_ARRAY, e.hint_string);
					prop = property_array;
				} break;
				case Variant::PACKED_STRING_ARRAY: {
					EditorPropertyArray *property_array = memnew(EditorPropertyArray);
					property_array->setup(Variant::PACKED_STRING_ARRAY, e.hint_string);
					prop = property_array;
				} break;
				case Variant::PACKED_VECTOR2_ARRAY: {
					EditorPropertyArray *property_array = memnew(EditorPropertyArray);
					property_array->setup(Variant::PACKED_VECTOR2_ARRAY, e.hint_string);
					prop = property_array;
				} break;
				case Variant::PACKED_VECTOR3_ARRAY: {
					EditorPropertyArray *property_array = memnew(EditorPropertyArray);
					property_array->setup(Variant::PACKED_VECTOR3_ARRAY, e.hint_string);
					prop = property_array;
				} break;
				case Variant::PACKED_COLOR_ARRAY: {
					EditorPropertyArray *property_array = memnew(EditorPropertyArray);
					property_array->setup(Variant::PACKED_COLOR_ARRAY, e.hint_string);
					prop = property_array;
				} break;
				default: {
				}
			}

			if (prop != nullptr) {
				prop->set_label(e.name.capitalize());
				p_container->add_child(prop);

				prop->set_object_and_property(entity, String(p_component_name) + "/" + e.name);
				prop->connect(SNAME("property_changed"), callable_mp(this, &EntityEditor::_property_changed));
				editor_properties.insert(e.name, prop);
			}
		}
		components_properties.insert(p_component_name, editor_properties);
	}
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

	EditorUndoRedoManager::get_singleton()->create_action(TTR("Add component"));
	EditorUndoRedoManager::get_singleton()->add_do_method(entity, SNAME("add_component"), component_name);
	EditorUndoRedoManager::get_singleton()->add_do_method(this, SNAME("update_editors"));
	EditorUndoRedoManager::get_singleton()->add_undo_method(entity, SNAME("remove_component"), component_name);
	EditorUndoRedoManager::get_singleton()->add_undo_method(this, SNAME("update_editors"));
	EditorUndoRedoManager::get_singleton()->commit_action();
}

void EntityEditor::_remove_component_pressed(StringName p_component_name) {
	EditorUndoRedoManager::get_singleton()->create_action(TTR("Drop component"));
	EditorUndoRedoManager::get_singleton()->add_do_method(entity, SNAME("remove_component"), p_component_name);
	EditorUndoRedoManager::get_singleton()->add_do_method(this, SNAME("update_editors"));
	EditorUndoRedoManager::get_singleton()->add_undo_method(entity, SNAME("add_component"), p_component_name, entity_get_component_props_data(p_component_name));
	EditorUndoRedoManager::get_singleton()->add_undo_method(this, SNAME("update_editors"));
	EditorUndoRedoManager::get_singleton()->commit_action();
}

void EntityEditor::_property_changed(const String &p_path, const Variant &p_value, const String &p_name, bool p_changing) {
	if (p_changing) {
		// Nothing to do while chaning.
		return;
	}

	EditorUndoRedoManager::get_singleton()->create_action(TTR("Set component value"));
	EditorUndoRedoManager::get_singleton()->add_do_method(entity, SNAME("set"), p_path, p_value);
	EditorUndoRedoManager::get_singleton()->add_undo_method(entity, SNAME("set"), p_path, entity->get(p_path));
	EditorUndoRedoManager::get_singleton()->add_undo_method(this, SNAME("update_editors"));
	EditorUndoRedoManager::get_singleton()->commit_action();

	if (p_value.get_type() != Variant::STRING) {
		// This is needed because string update is special: If string is updated
		// never reload the editor to avoid losing focus.
		if (!p_changing) {
			update_editors();
		}
	}
}

void EntityEditor::_changed_callback() {
	update_editors();
}

const OAHashMap<StringName, Ref<ComponentDepot>> &EntityEditor::entity_get_components_data() const {
	Entity3D *e = Object::cast_to<Entity3D>(entity);
	if (e) {
		return e->get_components_data();
	} else {
		Entity2D *ee = Object::cast_to<Entity2D>(entity);
		CRASH_COND_MSG(ee == nullptr, "Entity2D and Entity3D are supposed to be assigned.");
		return ee->get_components_data();
	}
}

Dictionary EntityEditor::entity_get_component_props_data(const StringName &p_component) const {
	Entity3D *e = Object::cast_to<Entity3D>(entity);
	if (e) {
		return e->get_component_properties_data(p_component);
	} else {
		Entity2D *ee = Object::cast_to<Entity2D>(entity);
		CRASH_COND_MSG(ee == nullptr, "Entity2D and Entity3D are supposed to be assigned.");
		return ee->get_component_properties_data(p_component);
	}
}

bool EditorInspectorPluginEntity::can_handle(Object *p_object) {
	return Object::cast_to<Entity3D>(p_object) != nullptr ||
		   Object::cast_to<Entity2D>(p_object) != nullptr;
}

void EditorInspectorPluginEntity::parse_begin(Object *p_object) {
	Node *entity = can_handle(p_object) ? static_cast<Node *>(p_object) : nullptr;
	ERR_FAIL_COND(!entity);

	EntityEditor *entity_editor = memnew(EntityEditor(this, editor, entity));
	add_custom_control(entity_editor);
}

EntityEditorPlugin::EntityEditorPlugin(EditorNode *p_node) {
	Ref<EditorInspectorPluginEntity> entity_plugin;
	entity_plugin.instantiate();
	entity_plugin->editor = p_node;

	EditorInspector::add_inspector_plugin(entity_plugin);
}
