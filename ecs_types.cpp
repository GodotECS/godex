#include "ecs_types.h"

#include "ecs.h"

void DataAccessor::init(uint32_t p_identifier, DataAccessorTargetType p_type, bool p_mut) {
	target_identifier = p_identifier;
	target_type = p_type;
	mut = p_mut;
}

uint32_t DataAccessor::get_target_identifier() const {
	return target_identifier;
}

DataAccessorTargetType DataAccessor::get_target_type() const {
	return target_type;
}

void DataAccessor::set_target(void *p_target) {
	target = p_target;
}

void *DataAccessor::get_target() {
	return target;
}

const void *DataAccessor::get_target() const {
	return target;
}

bool DataAccessor::is_mutable() const {
	return mut;
}

bool DataAccessor::_setv(const StringName &p_name, const Variant &p_value) {
	ERR_FAIL_COND_V(target == nullptr, false);
	ERR_FAIL_COND_V_MSG(mut == false, false, "This element was taken as not mutable.");

	switch (target_type) {
		case DataAccessorTargetType::Databag:
			return ECS::unsafe_databag_set_by_name(target_identifier, target, p_name, p_value);
		case DataAccessorTargetType::Component:
			return ECS::unsafe_component_set_by_name(target_identifier, target, p_name, p_value);
		case DataAccessorTargetType::Storage:
			return static_cast<StorageBase *>(target)->set(p_name, p_value);
	}

	return false;
}

bool DataAccessor::_getv(const StringName &p_name, Variant &r_ret) const {
	ERR_FAIL_COND_V(target == nullptr, false);

	switch (target_type) {
		case DataAccessorTargetType::Databag:
			return ECS::unsafe_databag_get_by_name(target_identifier, target, p_name, r_ret);
		case DataAccessorTargetType::Component:
			return ECS::unsafe_component_get_by_name(target_identifier, target, p_name, r_ret);
		case DataAccessorTargetType::Storage:
			return static_cast<StorageBase *>(target)->get(p_name, r_ret);
	}

	return false;
}

Variant DataAccessor::call(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	if (String(p_method) == "is_valid") { // TODO convert to a static StringName??
		r_error.error = Callable::CallError::Error::CALL_OK;
		return target != nullptr;
	} else if (String(p_method) == "is_mutable") { // TODO convert to a static StringName??
		r_error.error = Callable::CallError::Error::CALL_OK;
		return mut;
	} else {
		Variant ret;
		ERR_FAIL_COND_V(target == nullptr, ret);
		WARN_PRINT_ONCE("TODO check method mutability here?");
		switch (target_type) {
			case DataAccessorTargetType::Databag:
				ECS::unsafe_databag_call(
						target_identifier,
						target,
						p_method,
						p_args,
						p_argcount,
						&ret,
						r_error);
				break;
			case DataAccessorTargetType::Component:
				ECS::unsafe_component_call(
						target_identifier,
						target,
						p_method,
						p_args,
						p_argcount,
						&ret,
						r_error);
				break;
			case DataAccessorTargetType::Storage:
				static_cast<StorageBase *>(target)->da_call(
						p_method,
						p_args,
						p_argcount,
						&ret,
						r_error);
				break;
		}
		return ret;
	}
}
