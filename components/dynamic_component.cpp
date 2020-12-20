#include "dynamic_component.h"

DynamicComponentInfo::DynamicComponentInfo() {
}

Storage *DynamicComponentInfo::create_storage() {
	switch (storage_type) {
		case StorageType::DENSE_VECTOR:
			// Creates DynamicDenseVector storage.
			switch (properties.size()) {
				case 0:
					return memnew(DynamicDenseVector<ZeroVariantComponent>(this));
				case 1:
					return memnew(DynamicDenseVector<VariantComponent<1>>(this));
				case 2:
					return memnew(DynamicDenseVector<VariantComponent<2>>(this));
				case 3:
					return memnew(DynamicDenseVector<VariantComponent<3>>(this));
				case 4:
					return memnew(DynamicDenseVector<VariantComponent<4>>(this));
				case 5:
					return memnew(DynamicDenseVector<VariantComponent<5>>(this));
				case 6:
					return memnew(DynamicDenseVector<VariantComponent<6>>(this));
				case 7:
					return memnew(DynamicDenseVector<VariantComponent<7>>(this));
				case 8:
					return memnew(DynamicDenseVector<VariantComponent<8>>(this));
				case 9:
					return memnew(DynamicDenseVector<VariantComponent<9>>(this));
				case 10:
					return memnew(DynamicDenseVector<VariantComponent<10>>(this));
				case 11:
					return memnew(DynamicDenseVector<VariantComponent<11>>(this));
				case 12:
					return memnew(DynamicDenseVector<VariantComponent<12>>(this));
				case 13:
					return memnew(DynamicDenseVector<VariantComponent<13>>(this));
				case 14:
					return memnew(DynamicDenseVector<VariantComponent<14>>(this));
				case 15:
					return memnew(DynamicDenseVector<VariantComponent<15>>(this));
				case 16:
					return memnew(DynamicDenseVector<VariantComponent<16>>(this));
				case 17:
					return memnew(DynamicDenseVector<VariantComponent<17>>(this));
				case 18:
					return memnew(DynamicDenseVector<VariantComponent<18>>(this));
				case 19:
					return memnew(DynamicDenseVector<VariantComponent<19>>(this));
				case 20:
					return memnew(DynamicDenseVector<VariantComponent<20>>(this));
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
