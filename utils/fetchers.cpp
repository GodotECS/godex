#include "fetchers.h"

#include "../ecs.h"
#include "core/variant/callable.h"
#include "core/variant/variant.h"

void ComponentDynamicExposer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_mutable"), &ComponentDynamicExposer::is_mutable);
	ClassDB::bind_method(D_METHOD("is_valid"), &ComponentDynamicExposer::is_valid);
}

void ComponentDynamicExposer::init(uint32_t p_identifier, bool p_mut) {
	component_id = p_identifier;
	mut = p_mut;
}

uint32_t ComponentDynamicExposer::get_target_identifier() const {
	return component_id;
}

void ComponentDynamicExposer::set_target(void *p_target) {
	component_ptr = p_target;
}

void *ComponentDynamicExposer::get_target() {
	return component_ptr;
}

const void *ComponentDynamicExposer::get_target() const {
	return component_ptr;
}

bool ComponentDynamicExposer::is_mutable() const {
	return mut;
}

bool ComponentDynamicExposer::is_valid() const {
	return component_ptr != nullptr;
}

bool ComponentDynamicExposer::_set(const StringName &p_name, const Variant &p_value) {
	if (mut) {
		ERR_FAIL_COND_V(component_ptr == nullptr, false);
		return ECS::unsafe_component_set_by_name(component_id, component_ptr, p_name, p_value);
	} else {
		return false;
	}
}

bool ComponentDynamicExposer::_get(const StringName &p_name, Variant &r_ret) const {
	ERR_FAIL_COND_V(component_ptr == nullptr, false);
	return ECS::unsafe_component_get_by_name(component_id, component_ptr, p_name, r_ret);
}

Variant ComponentDynamicExposer::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	Variant ret = Object::callp(p_method, p_args, p_argcount, r_error);
	if (r_error.error == Callable::CallError::CALL_OK) {
		return ret;
	}

	ERR_FAIL_COND_V(component_ptr == nullptr, ret);
	WARN_PRINT_ONCE("TODO check method mutability here?");
	ECS::unsafe_component_call(
			component_id,
			component_ptr,
			p_method,
			p_args,
			p_argcount,
			&ret,
			r_error);
	return ret;
}

void DatabagDynamicFetcher::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_mutable"), &DatabagDynamicFetcher::is_mutable);
	ClassDB::bind_method(D_METHOD("is_valid"), &DatabagDynamicFetcher::is_valid);
}

void DatabagDynamicFetcher::init(uint32_t p_identifier, bool p_mut) {
	databag_id = p_identifier;
	mut = p_mut;
}

uint32_t DatabagDynamicFetcher::get_target_identifier() const {
	return databag_id;
}

void *DatabagDynamicFetcher::get_target() {
	return databag_ptr;
}

const void *DatabagDynamicFetcher::get_target() const {
	return databag_ptr;
}

bool DatabagDynamicFetcher::is_mutable() const {
	return mut;
}

bool DatabagDynamicFetcher::is_valid() const {
	return databag_ptr != nullptr;
}

void DatabagDynamicFetcher::get_system_info(SystemExeInfo *r_info) const {
	if (mut) {
		r_info->mutable_databags.insert(databag_id);
	} else {
		r_info->immutable_databags.insert(databag_id);
	}
}

void DatabagDynamicFetcher::prepare_world(World *p_world) {}

void DatabagDynamicFetcher::initiate_process(World *p_world) {
	databag_ptr = (void *)p_world->get_databag(databag_id);
}

void DatabagDynamicFetcher::conclude_process(World *p_world) {
	databag_ptr = nullptr;
}

void DatabagDynamicFetcher::release_world(World *p_world) {}

void DatabagDynamicFetcher::set_active(bool p_active) {}

bool DatabagDynamicFetcher::_set(const StringName &p_name, const Variant &p_value) {
	if (mut) {
		return ECS::unsafe_databag_set_by_name(databag_id, databag_ptr, p_name, p_value);
	} else {
		return false;
	}
}

bool DatabagDynamicFetcher::_get(const StringName &p_name, Variant &r_ret) const {
	return ECS::unsafe_databag_get_by_name(databag_id, databag_ptr, p_name, r_ret);
}

Variant DatabagDynamicFetcher::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	Variant ret = GodexWorldFetcher::callp(p_method, p_args, p_argcount, r_error);
	if (r_error.error == Callable::CallError::CALL_OK) {
		return ret;
	}

	ERR_FAIL_COND_V(databag_ptr == nullptr, ret);
	WARN_PRINT_ONCE("TODO check method mutability here?");
	ECS::unsafe_databag_call(
			databag_id,
			databag_ptr,
			p_method,
			p_args,
			p_argcount,
			&ret,
			r_error);
	return ret;
}

void StorageDynamicFetcher::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_mutable"), &StorageDynamicFetcher::is_mutable);
	ClassDB::bind_method(D_METHOD("is_valid"), &StorageDynamicFetcher::is_valid);
}

void StorageDynamicFetcher::init(uint32_t p_identifier) {
	storage_component_id = p_identifier;
}

uint32_t StorageDynamicFetcher::get_target_identifier() const {
	return storage_component_id;
}

void *StorageDynamicFetcher::get_target() {
	return storage_ptr;
}

const void *StorageDynamicFetcher::get_target() const {
	return storage_ptr;
}

bool StorageDynamicFetcher::is_mutable() const {
	// Always mutable.
	return true;
}

bool StorageDynamicFetcher::is_valid() const {
	return storage_ptr != nullptr;
}

void StorageDynamicFetcher::get_system_info(SystemExeInfo *r_info) const {
	r_info->mutable_components_storage.insert(storage_component_id);
}

void StorageDynamicFetcher::prepare_world(World *p_world) {}

void StorageDynamicFetcher::initiate_process(World *p_world) {
	storage_ptr = p_world->get_storage(storage_component_id);
}

void StorageDynamicFetcher::conclude_process(World *p_world) {
	storage_ptr = nullptr;
}

void StorageDynamicFetcher::release_world(World *p_world) {}

void StorageDynamicFetcher::set_active(bool p_active) {}

bool StorageDynamicFetcher::_set(const StringName &p_name, const Variant &p_value) {
	return storage_ptr->set(p_name, p_value);
}

bool StorageDynamicFetcher::_get(const StringName &p_name, Variant &r_ret) const {
	return storage_ptr->get(p_name, r_ret);
}

Variant StorageDynamicFetcher::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	Variant ret = GodexWorldFetcher::callp(p_method, p_args, p_argcount, r_error);
	if (r_error.error == Callable::CallError::CALL_OK) {
		return ret;
	}

	ERR_FAIL_COND_V(storage_ptr == nullptr, ret);
	storage_ptr->da_call(
			p_method,
			p_args,
			p_argcount,
			&ret,
			r_error);
	return ret;
}

void EventsEmitterDynamicFetcher::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_mutable"), &EventsEmitterDynamicFetcher::is_mutable);
	ClassDB::bind_method(D_METHOD("is_valid"), &EventsEmitterDynamicFetcher::is_valid);
	ClassDB::bind_method(D_METHOD("emit", "emitter_name", "data"), &EventsEmitterDynamicFetcher::emit, DEFVAL(Variant()));
}

void EventsEmitterDynamicFetcher::init(uint32_t p_identifier) {
	event_id = p_identifier;
}

uint32_t EventsEmitterDynamicFetcher::get_target_identifier() const {
	return event_id;
}

void *EventsEmitterDynamicFetcher::get_target() {
	return event_storage_ptr;
}

const void *EventsEmitterDynamicFetcher::get_target() const {
	return event_storage_ptr;
}

bool EventsEmitterDynamicFetcher::is_mutable() const {
	// Always mutable.
	return true;
}

bool EventsEmitterDynamicFetcher::is_valid() const {
	return event_storage_ptr != nullptr;
}

void EventsEmitterDynamicFetcher::get_system_info(SystemExeInfo *r_info) const {
	r_info->events_emitters.insert(event_id);
}

void EventsEmitterDynamicFetcher::prepare_world(World *p_world) {}

void EventsEmitterDynamicFetcher::initiate_process(World *p_world) {
	event_storage_ptr = p_world->get_events_storage(event_id);
	if (event_storage_ptr) {
		event_storage_ptr->flush_events();
	}
}

void EventsEmitterDynamicFetcher::conclude_process(World *p_world) {
	event_storage_ptr = nullptr;
}

void EventsEmitterDynamicFetcher::release_world(World *p_world) {}

void EventsEmitterDynamicFetcher::set_active(bool p_active) {}

void EventsEmitterDynamicFetcher::emit(const String &p_emitter_name, const Variant &p_data) {
	ERR_FAIL_COND(event_storage_ptr == nullptr);
	event_storage_ptr->add_event_dynamic(p_emitter_name, p_data);
}

void EventsReceiverDynamicFetcher::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_mutable"), &EventsReceiverDynamicFetcher::is_mutable);
	ClassDB::bind_method(D_METHOD("is_valid"), &EventsReceiverDynamicFetcher::is_valid);
	ClassDB::bind_method(D_METHOD("fetch"), &EventsReceiverDynamicFetcher::fetch);
}

void EventsReceiverDynamicFetcher::init(uint32_t p_identifier, const String &p_emitter_name) {
	event_id = p_identifier;
	emitter_name = p_emitter_name;
}

uint32_t EventsReceiverDynamicFetcher::get_target_identifier() const {
	return event_id;
}

void *EventsReceiverDynamicFetcher::get_target() {
	return event_storage_ptr;
}

const void *EventsReceiverDynamicFetcher::get_target() const {
	return event_storage_ptr;
}

bool EventsReceiverDynamicFetcher::is_mutable() const {
	// Always immutable.
	return false;
}

bool EventsReceiverDynamicFetcher::is_valid() const {
	return event_storage_ptr != nullptr;
}

void EventsReceiverDynamicFetcher::get_system_info(SystemExeInfo *r_info) const {
	RBSet<String> *emitters = r_info->events_receivers.lookup_ptr(event_id);
	if (emitters == nullptr) {
		r_info->events_receivers.insert(event_id, RBSet<String>());
		emitters = r_info->events_receivers.lookup_ptr(event_id);
	}
	emitters->insert(emitter_name);
}

void EventsReceiverDynamicFetcher::prepare_world(World *p_world) {}

void EventsReceiverDynamicFetcher::initiate_process(World *p_world) {
	event_storage_ptr = p_world->get_events_storage(event_id);
}

void EventsReceiverDynamicFetcher::conclude_process(World *p_world) {
	event_storage_ptr = nullptr;
}

void EventsReceiverDynamicFetcher::release_world(World *p_world) {}

void EventsReceiverDynamicFetcher::set_active(bool p_active) {}

Array EventsReceiverDynamicFetcher::fetch() {
	ERR_FAIL_COND_V(event_storage_ptr == nullptr, Array());
	return event_storage_ptr->get_events_array(emitter_name);
}
