#include "dynamic_component.h"

#include "../storage/dense_vector_storage.h"

#define godex_new(clazz) new clazz

DynamicComponentInfo::DynamicComponentInfo() {
}

StorageBase *DynamicComponentInfo::create_storage() {
	switch (storage_type) {
		case StorageType::DENSE_VECTOR:
			// Creates DynamicDenseVector storage.
			switch (properties.size()) {
				case 0:
					return godex_new(DynamicDenseVectorStorage<ZeroVariantComponent>(this));
				case 1:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<1>>(this));
				case 2:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<2>>(this));
				case 3:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<3>>(this));
				case 4:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<4>>(this));
				case 5:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<5>>(this));
				case 6:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<6>>(this));
				case 7:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<7>>(this));
				case 8:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<8>>(this));
				case 9:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<9>>(this));
				case 10:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<10>>(this));
				case 11:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<11>>(this));
				case 12:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<12>>(this));
				case 13:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<13>>(this));
				case 14:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<14>>(this));
				case 15:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<15>>(this));
				case 16:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<16>>(this));
				case 17:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<17>>(this));
				case 18:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<18>>(this));
				case 19:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<19>>(this));
				case 20:
					return godex_new(DynamicDenseVectorStorage<VariantComponent<20>>(this));
			}
			CRASH_NOW_MSG("Components with more than 20 variables are not suppported. Please open an issue https://github.com/GodotECS/godex/issues if you need to support more.");
			break;
		default:
			CRASH_NOW_MSG("This storage type is not supported. This is not expected!");
	}
	return nullptr;
}

void DynamicComponentInfo::get_property_list(void *p_self, const DynamicComponentInfo *p_info, List<PropertyInfo> *r_list) {
	for (uint32_t i = 0; i < p_info->properties.size(); ++i) {
		r_list->push_back(p_info->properties[i]);
	}
}

bool DynamicComponentInfo::static_set(void *p_self, const DynamicComponentInfo *p_info, const StringName &p_name, const Variant &p_data) {
	const uint32_t index = p_info->get_property_id(p_name);
	return static_set(p_self, p_info, index, p_data);
}

bool DynamicComponentInfo::static_get(const void *p_self, const DynamicComponentInfo *p_info, const StringName &p_name, Variant &r_data) {
	const uint32_t index = p_info->get_property_id(p_name);
	return static_get(p_self, p_info, index, r_data);
}

bool DynamicComponentInfo::static_set(void *p_self, const DynamicComponentInfo *p_info, const uint32_t p_index, const Variant &p_data) {
	ERR_FAIL_COND_V_MSG(p_index >= p_info->properties.size(), false, "You can't set this data to this VariantComponent.");
	Variant *d = static_get_data(p_self, p_info);
	ERR_FAIL_COND_V_MSG(d[p_index].get_type() != p_data.get_type(), false, "You can't set a variable with different type.");
	d[p_index] = p_data;
	return true;
}

bool DynamicComponentInfo::static_get(const void *p_self, const DynamicComponentInfo *p_info, const uint32_t p_index, Variant &r_data) {
	ERR_FAIL_COND_V_MSG(p_index >= p_info->properties.size(), false, "You can't set this data to this VariantComponent.");
	r_data = static_get_data(p_self, p_info)[p_index];
	return true;
}

Variant *DynamicComponentInfo::static_get_data(void *p_self, const DynamicComponentInfo *p_info) {
	Variant *data = nullptr;
	switch (p_info->properties.size()) {
		case 1: {
			data = static_cast<VariantComponent<1> *>(p_self)->data;
		} break;
		case 2: {
			data = static_cast<VariantComponent<2> *>(p_self)->data;
		} break;
		case 3: {
			data = static_cast<VariantComponent<3> *>(p_self)->data;
		} break;
		case 4: {
			data = static_cast<VariantComponent<4> *>(p_self)->data;
		} break;
		case 5: {
			data = static_cast<VariantComponent<5> *>(p_self)->data;
		} break;
		case 6: {
			data = static_cast<VariantComponent<6> *>(p_self)->data;
		} break;
		case 7: {
			data = static_cast<VariantComponent<7> *>(p_self)->data;
		} break;
		case 8: {
			data = static_cast<VariantComponent<8> *>(p_self)->data;
		} break;
		case 9: {
			data = static_cast<VariantComponent<9> *>(p_self)->data;
		} break;
		case 10: {
			data = static_cast<VariantComponent<10> *>(p_self)->data;
		} break;
		case 11: {
			data = static_cast<VariantComponent<11> *>(p_self)->data;
		} break;
		case 12: {
			data = static_cast<VariantComponent<12> *>(p_self)->data;
		} break;
		case 13: {
			data = static_cast<VariantComponent<13> *>(p_self)->data;
		} break;
		case 14: {
			data = static_cast<VariantComponent<14> *>(p_self)->data;
		} break;
		case 15: {
			data = static_cast<VariantComponent<15> *>(p_self)->data;
		} break;
		case 16: {
			data = static_cast<VariantComponent<16> *>(p_self)->data;
		} break;
		case 17: {
			data = static_cast<VariantComponent<17> *>(p_self)->data;
		} break;
		case 18: {
			data = static_cast<VariantComponent<18> *>(p_self)->data;
		} break;
		case 19: {
			data = static_cast<VariantComponent<19> *>(p_self)->data;
		} break;
		case 20: {
			data = static_cast<VariantComponent<20> *>(p_self)->data;
		} break;
	};
	return data;
}

const Variant *DynamicComponentInfo::static_get_data(const void *p_self, const DynamicComponentInfo *p_info) {
	const Variant *data = nullptr;
	switch (p_info->properties.size()) {
		case 1: {
			data = static_cast<const VariantComponent<1> *>(p_self)->data;
		} break;
		case 2: {
			data = static_cast<const VariantComponent<2> *>(p_self)->data;
		} break;
		case 3: {
			data = static_cast<const VariantComponent<3> *>(p_self)->data;
		} break;
		case 4: {
			data = static_cast<const VariantComponent<4> *>(p_self)->data;
		} break;
		case 5: {
			data = static_cast<const VariantComponent<5> *>(p_self)->data;
		} break;
		case 6: {
			data = static_cast<const VariantComponent<6> *>(p_self)->data;
		} break;
		case 7: {
			data = static_cast<const VariantComponent<7> *>(p_self)->data;
		} break;
		case 8: {
			data = static_cast<const VariantComponent<8> *>(p_self)->data;
		} break;
		case 9: {
			data = static_cast<const VariantComponent<9> *>(p_self)->data;
		} break;
		case 10: {
			data = static_cast<const VariantComponent<10> *>(p_self)->data;
		} break;
		case 11: {
			data = static_cast<const VariantComponent<11> *>(p_self)->data;
		} break;
		case 12: {
			data = static_cast<const VariantComponent<12> *>(p_self)->data;
		} break;
		case 13: {
			data = static_cast<const VariantComponent<13> *>(p_self)->data;
		} break;
		case 14: {
			data = static_cast<const VariantComponent<14> *>(p_self)->data;
		} break;
		case 15: {
			data = static_cast<const VariantComponent<15> *>(p_self)->data;
		} break;
		case 16: {
			data = static_cast<const VariantComponent<16> *>(p_self)->data;
		} break;
		case 17: {
			data = static_cast<const VariantComponent<17> *>(p_self)->data;
		} break;
		case 18: {
			data = static_cast<const VariantComponent<18> *>(p_self)->data;
		} break;
		case 19: {
			data = static_cast<const VariantComponent<19> *>(p_self)->data;
		} break;
		case 20: {
			data = static_cast<const VariantComponent<20> *>(p_self)->data;
		} break;
	};
	return data;
}
