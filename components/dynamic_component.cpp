#include "dynamic_component.h"

DynamicComponentInfo::DynamicComponentInfo() {
}

StorageBase *DynamicComponentInfo::create_storage() {
	switch (storage_type) {
		case StorageType::DENSE_VECTOR:
			// Creates DynamicDenseVector storage.
			switch (properties.size()) {
				case 0:
					return memnew(DynamicDenseVectorStorage<ZeroVariantComponent>(this));
				case 1:
					return memnew(DynamicDenseVectorStorage<VariantComponent<1>>(this));
				case 2:
					return memnew(DynamicDenseVectorStorage<VariantComponent<2>>(this));
				case 3:
					return memnew(DynamicDenseVectorStorage<VariantComponent<3>>(this));
				case 4:
					return memnew(DynamicDenseVectorStorage<VariantComponent<4>>(this));
				case 5:
					return memnew(DynamicDenseVectorStorage<VariantComponent<5>>(this));
				case 6:
					return memnew(DynamicDenseVectorStorage<VariantComponent<6>>(this));
				case 7:
					return memnew(DynamicDenseVectorStorage<VariantComponent<7>>(this));
				case 8:
					return memnew(DynamicDenseVectorStorage<VariantComponent<8>>(this));
				case 9:
					return memnew(DynamicDenseVectorStorage<VariantComponent<9>>(this));
				case 10:
					return memnew(DynamicDenseVectorStorage<VariantComponent<10>>(this));
				case 11:
					return memnew(DynamicDenseVectorStorage<VariantComponent<11>>(this));
				case 12:
					return memnew(DynamicDenseVectorStorage<VariantComponent<12>>(this));
				case 13:
					return memnew(DynamicDenseVectorStorage<VariantComponent<13>>(this));
				case 14:
					return memnew(DynamicDenseVectorStorage<VariantComponent<14>>(this));
				case 15:
					return memnew(DynamicDenseVectorStorage<VariantComponent<15>>(this));
				case 16:
					return memnew(DynamicDenseVectorStorage<VariantComponent<16>>(this));
				case 17:
					return memnew(DynamicDenseVectorStorage<VariantComponent<17>>(this));
				case 18:
					return memnew(DynamicDenseVectorStorage<VariantComponent<18>>(this));
				case 19:
					return memnew(DynamicDenseVectorStorage<VariantComponent<19>>(this));
				case 20:
					return memnew(DynamicDenseVectorStorage<VariantComponent<20>>(this));
			}
			CRASH_NOW_MSG("Components with more than 20 variables are not suppported. Please open an issue https://github.com/GodotECS/godex/issues if you need to support more.");
			break;
		default:
			CRASH_NOW_MSG("This storage type is not supported. This is not expected!");
	}
	return nullptr;
}

/* TODO remove?
void DynamicComponentInfo::static_initialize(void *p_self, DynamicComponentInfo *p_info) {
	CRASH_COND_MSG(p_self == nullptr, "The component can't be nullptr.");
	CRASH_COND_MSG(p_info == nullptr, "The component info can't be nullptr.");
	switch (p_info->properties.size()) {
		case 0: {
			ZeroVariantComponent *self = static_cast<ZeroVariantComponent *>(p_self);
			CRASH_COND_MSG(self, "The passed component is not type ZeroVariantComponent. This is not supposed to happen.");
			self->__initialize(p_info);
		} break;
		case 1: {
			VariantComponent<1> *self = static_cast<VariantComponent<1> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 2: {
			VariantComponent<2> *self = static_cast<VariantComponent<2> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 3: {
			VariantComponent<3> *self = static_cast<VariantComponent<3> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 4: {
			VariantComponent<4> *self = static_cast<VariantComponent<4> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 5: {
			VariantComponent<5> *self = static_cast<VariantComponent<5> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 6: {
			VariantComponent<6> *self = static_cast<VariantComponent<6> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 7: {
			VariantComponent<7> *self = static_cast<VariantComponent<7> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 8: {
			VariantComponent<8> *self = static_cast<VariantComponent<8> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 9: {
			VariantComponent<9> *self = static_cast<VariantComponent<9> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 10: {
			VariantComponent<10> *self = static_cast<VariantComponent<10> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 11: {
			VariantComponent<11> *self = static_cast<VariantComponent<11> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 12: {
			VariantComponent<12> *self = static_cast<VariantComponent<12> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 13: {
			VariantComponent<13> *self = static_cast<VariantComponent<13> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 14: {
			VariantComponent<14> *self = static_cast<VariantComponent<14> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 15: {
			VariantComponent<15> *self = static_cast<VariantComponent<15> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 16: {
			VariantComponent<16> *self = static_cast<VariantComponent<16> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 17: {
			VariantComponent<17> *self = static_cast<VariantComponent<17> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 18: {
			VariantComponent<18> *self = static_cast<VariantComponent<18> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 19: {
			VariantComponent<19> *self = static_cast<VariantComponent<19> *>(p_self);
			self->__initialize(p_info);
		} break;
		case 20: {
			VariantComponent<20> *self = static_cast<VariantComponent<20> *>(p_self);
			self->__initialize(p_info);
		} break;
	}
}
*/

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
