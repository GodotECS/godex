#pragma once

#include "component.h"

/// Structure used to create dynamic components.
class DynamicComponentInfo {
	friend class ECS;

	uint32_t component_id = UINT32_MAX;
	// Maps the property to the position
	LocalVector<StringName> property_map;
	LocalVector<PropertyInfo> properties;
	LocalVector<Variant> defaults;
	StorageType storage_type = StorageType::NONE;

	DynamicComponentInfo();

public:
	StorageBase *create_storage();

	// TODO move all this to CPP

	const LocalVector<PropertyInfo> *get_properties() const {
		return &properties;
	}

	Variant get_property_default(StringName p_name) const {
		const uint32_t i = get_property_id(p_name);
		ERR_FAIL_COND_V_MSG(i == UINT32_MAX, Variant(), "The property " + p_name + " doesn't exists on this component " + ECS::get_component_name(component_id));
		return defaults[i];
	}

	const LocalVector<Variant> &get_property_defaults() const {
		return defaults;
	}

	uint32_t get_property_id(StringName p_name) const {
		const int64_t i = property_map.find(p_name);
		ERR_FAIL_COND_V_MSG(i == -1, UINT32_MAX, "The property " + p_name + " doesn't exists on this component " + ECS::get_component_name(component_id));
		return i;
	}

	bool validate_type(uint32_t p_index, Variant::Type p_type) const {
		return defaults[p_index].get_type() == p_type;
	}

public:
	// TODO remove?
	//static void static_initialize(void *p_self, DynamicComponentInfo *p_info);

	static bool static_set(void *p_self, const DynamicComponentInfo *p_info, const StringName &p_name, const Variant &p_data);
	static bool static_get(const void *p_self, const DynamicComponentInfo *p_info, const StringName &p_name, Variant &r_data);

	static bool static_set(void *p_self, const DynamicComponentInfo *p_info, const uint32_t p_index, const Variant &p_data);
	static bool static_get(const void *p_self, const DynamicComponentInfo *p_info, const uint32_t p_index, Variant &r_data);

	static Variant *static_get_data(void *p_self, const DynamicComponentInfo *p_info);
	static const Variant *static_get_data(const void *p_self, const DynamicComponentInfo *p_info);
};

/// The `ZeroVariantComponent` is a special type component designed for godot
/// scripts. This component have no variables.
class ZeroVariantComponent : public godex::Component {
	friend class DynamicComponentInfo;
	DynamicComponentInfo *info = nullptr;

public:
	ZeroVariantComponent() {}

	void __initialize(DynamicComponentInfo *p_info) {
		CRASH_COND_MSG(p_info == nullptr, "The component info can't be nullptr.");
		CRASH_COND_MSG(info->get_properties()->size() != 0, "The ZeroVariantComponent can't be initiazlized with `DanamicComponentInfo` that has data for more paramenters.");

		info = p_info;
	}
};

/// The `VariantComponent` is a special type component designed for godot
/// scripts. The components are stored consecutively.
template <int SIZE>
class VariantComponent : public godex::Component {
	friend class DynamicComponentInfo;
	DynamicComponentInfo *info = nullptr;
	Variant data[SIZE];

public:
	VariantComponent() {}

	void __initialize(DynamicComponentInfo *p_info);
};

template <int SIZE>
void VariantComponent<SIZE>::__initialize(DynamicComponentInfo *p_info) {
	CRASH_COND_MSG(p_info == nullptr, "The component info can't be nullptr.");
	CRASH_COND_MSG(info->get_properties()->size() != SIZE, "The VariantComponent(size: " + itos(SIZE) + ") got created with a ScriptComponentInfo that has " + itos(p_info->get_properties()->size()) + " parameters, this is not supposed to happen.");

	info = p_info;

	// Set defaults.
	for (uint32_t i = 0; i < SIZE; i += 1) {
		data[i] = info->get_property_defaults()[i];
	}
}
