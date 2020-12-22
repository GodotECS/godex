#pragma once

#include "component.h"

/// Structure used to create dynamic components.
class DynamicComponentInfo {
	friend class ECS;

	uint32_t component_id = UINT32_MAX;
	// Maps the property to the position
	OAHashMap<StringName, uint32_t> property_map; // TODO make this LocalVector?
	LocalVector<PropertyInfo> properties;
	LocalVector<Variant> defaults;
	StorageType storage_type = StorageType::NONE;

	DynamicComponentInfo();

public:
	Storage *create_storage();

	const LocalVector<PropertyInfo> *get_properties() const {
		return &properties;
	}

	Variant get_property_default(StringName p_name) const {
		const uint32_t *id_ptr = property_map.lookup_ptr(p_name);
		ERR_FAIL_COND_V_MSG(id_ptr == nullptr, Variant(), "The property " + p_name + " doesn't exists on this component " + ECS::get_component_name(component_id));
		return defaults[*id_ptr];
	}

	const LocalVector<Variant> &get_property_defaults() const {
		return defaults;
	}

	uint32_t get_property_id(StringName p_name) const {
		const uint32_t *id_ptr = property_map.lookup_ptr(p_name);
		ERR_FAIL_COND_V_MSG(id_ptr == nullptr, UINT32_MAX, "The property " + p_name + " doesn't exists on this component " + ECS::get_component_name(component_id));
		return *id_ptr;
	}

	bool validate_type(uint32_t p_index, Variant::Type p_type) const {
		return defaults[p_index].get_type() == p_type;
	}
};

/// The `ZeroVariantComponent` is a special type component designed for godot
/// scripts. This component have no variables.
class ZeroVariantComponent : public godex::Component {
	DynamicComponentInfo *info = nullptr;

public:
	ZeroVariantComponent() {}
	void __initialize(DynamicComponentInfo *p_info);

	virtual const LocalVector<PropertyInfo> *get_properties() const override;
	virtual bool set(const StringName &p_name, const Variant &p_data) override;
	virtual bool get(const StringName &p_name, Variant &p_data) const override;

	virtual bool set(const uint32_t p_index, const Variant &p_data) override;
	virtual bool get(const uint32_t p_index, Variant &p_data) const override;
};

/// The `VariantComponent` is a special type component designed for godot
/// scripts. The components are stored consecutively.
template <int SIZE>
class VariantComponent : public godex::Component {
	DynamicComponentInfo *info = nullptr;

	Variant data[SIZE];

public:
	VariantComponent() {}
	void __initialize(DynamicComponentInfo *p_info);

	virtual const LocalVector<PropertyInfo> *get_properties() const override;
	virtual bool set(const StringName &p_name, const Variant &p_data) override;
	virtual bool get(const StringName &p_name, Variant &p_data) const override;

	virtual bool set(const uint32_t p_index, const Variant &p_data) override;
	virtual bool get(const uint32_t p_index, Variant &p_data) const override;
};

template <int SIZE>
void VariantComponent<SIZE>::__initialize(DynamicComponentInfo *p_info) {
	info = p_info;
	CRASH_COND_MSG(p_info == nullptr, "The component info can't be nullptr.");
	CRASH_COND_MSG(info->get_properties()->size() != SIZE, "The VariantComponent(size: " + itos(SIZE) + ") got created with a ScriptComponentInfo that has " + itos(info->get_properties()->size()) + " parameters, this is not supposed to happen.");

	// Set defaults.
	for (uint32_t i = 0; i < SIZE; i += 1) {
		data[i] = info->get_property_defaults()[i];
	}
}

template <int SIZE>
const LocalVector<PropertyInfo> *VariantComponent<SIZE>::get_properties() const {
	return info->get_properties();
}

template <int SIZE>
bool VariantComponent<SIZE>::set(const StringName &p_name, const Variant &p_data) {
#ifdef DEBUG_ENABLED
	CRASH_COND_MSG(info == nullptr, "The component is not initialized. This is not supposed to happen.");
#endif
	const uint32_t index = info->get_property_id(p_name);
	return set(index, p_data);
}

template <int SIZE>
bool VariantComponent<SIZE>::get(const StringName &p_name, Variant &r_data) const {
#ifdef DEBUG_ENABLED
	CRASH_COND_MSG(info == nullptr, "The component is not initialized. This is not supposed to happen.");
#endif
	const uint32_t index = info->get_property_id(p_name);
	return get(index, r_data);
}

template <int SIZE>
bool VariantComponent<SIZE>::set(const uint32_t p_index, const Variant &p_data) {
#ifdef DEBUG_ENABLED
	CRASH_COND_MSG(info == nullptr, "The component is not initialized. This is not supposed to happen.");
#endif
	ERR_FAIL_COND_V(p_index >= SIZE, false);
	ERR_FAIL_COND_V_MSG(info->validate_type(p_index, p_data.get_type()) == false, false, "The component variable " + (*info->get_properties())[p_index].name + " has not the same type of the given value: " + p_data);
	data[p_index] = p_data;
	return true;
}

template <int SIZE>
bool VariantComponent<SIZE>::get(const uint32_t p_index, Variant &r_data) const {
	ERR_FAIL_COND_V(p_index >= SIZE, false);
	r_data = data[p_index];
	return true;
}
