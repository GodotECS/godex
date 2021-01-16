#include "dynamic_component.h"

DynamicComponentInfo::DynamicComponentInfo() {
}

Storage *DynamicComponentInfo::create_storage() {
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
		case StorageType::NONE:
		default:
			CRASH_NOW_MSG("This storage type is not supported. This is not expected!");
			return nullptr;
	}
}

void ZeroVariantComponent::__initialize(DynamicComponentInfo *p_info) {
	info = p_info;
	CRASH_COND_MSG(p_info == nullptr, "The component info can't be nullptr.");
	CRASH_COND_MSG(info->get_properties()->size() != 0, "The ZeroVariantComponent(size: " + itos(0) + ") got created with a ScriptComponentInfo that has " + itos(info->get_properties()->size()) + " parameters, this is not supposed to happen.");
}

const LocalVector<PropertyInfo> *ZeroVariantComponent::get_properties() const {
	return info->get_properties();
}

bool ZeroVariantComponent::set(const StringName &p_name, const Variant &p_data) {
	return false;
}

bool ZeroVariantComponent::get(const StringName &p_name, Variant &p_data) const {
	return false;
}

bool ZeroVariantComponent::set(const uint32_t p_index, const Variant &p_data) {
	return false;
}

bool ZeroVariantComponent::get(const uint32_t p_index, Variant &p_data) const {
	return false;
}
