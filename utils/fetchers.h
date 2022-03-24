#pragma once

#include "../ecs_types.h"

class World;
class StorageBase;
class EventStorageBase;

class ComponentDynamicExposer : public Object {
	GDCLASS(ComponentDynamicExposer, Object)

private:
	uint32_t component_id;
	bool mut = false;
	void *component_ptr = nullptr;

	static void _bind_methods();

public:
	void init(uint32_t p_identifier, bool p_mut);

	uint32_t get_target_identifier() const;
	bool is_mutable() const;
	bool is_valid() const;
	void set_target(void *p_target);
	void *get_target();
	const void *get_target() const;

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	virtual Variant callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;
};

class DatabagDynamicFetcher : public GodexWorldFetcher {
	GDCLASS(DatabagDynamicFetcher, GodexWorldFetcher)

private:
	uint32_t databag_id;
	bool mut = false;
	void *databag_ptr = nullptr;

	static void _bind_methods();

public:
	void init(uint32_t p_identifier, bool p_mut);

	uint32_t get_target_identifier() const;
	bool is_mutable() const;
	bool is_valid() const;
	void *get_target();
	const void *get_target() const;

	virtual void get_system_info(SystemExeInfo *r_info) const override;

	virtual void prepare_world(World *p_world) override;
	virtual void initiate_process(World *p_world) override;
	virtual void conclude_process(World *p_world) override;
	virtual void release_world(World *p_world) override;
	virtual void set_active(bool p_active) override;

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	virtual Variant callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;
};

class StorageDynamicFetcher : public GodexWorldFetcher {
	GDCLASS(StorageDynamicFetcher, GodexWorldFetcher)

private:
	uint32_t storage_component_id;
	StorageBase *storage_ptr = nullptr;

	static void _bind_methods();

public:
	void init(uint32_t p_identifier);

	uint32_t get_target_identifier() const;
	bool is_mutable() const;
	bool is_valid() const;
	void *get_target();
	const void *get_target() const;

	virtual void get_system_info(SystemExeInfo *r_info) const override;

	virtual void prepare_world(World *p_world) override;
	virtual void initiate_process(World *p_world) override;
	virtual void conclude_process(World *p_world) override;
	virtual void release_world(World *p_world) override;
	virtual void set_active(bool p_active) override;

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	virtual Variant callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;
};

class EventsEmitterDynamicFetcher : public GodexWorldFetcher {
	GDCLASS(EventsEmitterDynamicFetcher, GodexWorldFetcher)

private:
	uint32_t event_id;
	EventStorageBase *event_storage_ptr = nullptr;

	static void _bind_methods();

public:
	void init(uint32_t p_identifier);

	uint32_t get_target_identifier() const;
	bool is_mutable() const;
	bool is_valid() const;
	void *get_target();
	const void *get_target() const;

	virtual void get_system_info(SystemExeInfo *r_info) const override;

	virtual void prepare_world(World *p_world) override;
	virtual void initiate_process(World *p_world) override;
	virtual void conclude_process(World *p_world) override;
	virtual void release_world(World *p_world) override;
	virtual void set_active(bool p_active) override;

	void emit(const String &p_emitter_name, const Variant &p_data = Variant());
};

class EventsReceiverDynamicFetcher : public GodexWorldFetcher {
	GDCLASS(EventsReceiverDynamicFetcher, GodexWorldFetcher)

private:
	uint32_t event_id;
	String emitter_name;
	EventStorageBase *event_storage_ptr = nullptr;

	static void _bind_methods();

public:
	void init(uint32_t p_identifier, const String &p_emitter_name);

	uint32_t get_target_identifier() const;
	bool is_mutable() const;
	bool is_valid() const;
	void *get_target();
	const void *get_target() const;

	virtual void get_system_info(SystemExeInfo *r_info) const;

	virtual void prepare_world(World *p_world) override;
	virtual void initiate_process(World *p_world) override;
	virtual void conclude_process(World *p_world) override;
	virtual void release_world(World *p_world) override;
	virtual void set_active(bool p_active) override;

	Array fetch();
};

namespace godex {
template <class T>
T *unwrap_component(Object *p_access_databag) {
	ComponentDynamicExposer *comp = dynamic_cast<ComponentDynamicExposer *>(p_access_databag);
	if (unlikely(comp == nullptr || comp->get_target() == nullptr)) {
		return nullptr;
	}
	if (likely(comp->get_target_identifier() == T::get_component_id())) {
		return static_cast<T *>(comp->get_target());
	} else {
		return nullptr;
	}
}

template <class T>
const T *unwrap_component(const Object *p_access_databag) {
	const ComponentDynamicExposer *comp = dynamic_cast<const ComponentDynamicExposer *>(p_access_databag);
	if (unlikely(comp == nullptr || comp->get_target() == nullptr)) {
		return nullptr;
	}
	if (likely(comp->get_target_identifier() == T::get_component_id())) {
		return static_cast<const T *>(comp->get_target());
	} else {
		return nullptr;
	}
}

template <class T>
T *unwrap_databag(Object *p_access_databag) {
	DatabagDynamicFetcher *bag = dynamic_cast<DatabagDynamicFetcher *>(p_access_databag);
	if (unlikely(bag == nullptr || bag->get_target() == nullptr)) {
		return nullptr;
	}
	if (likely(bag->get_target_identifier() == T::get_databag_id())) {
		return static_cast<T *>(bag->get_target());
	} else {
		return nullptr;
	}
}

template <class T>
const T *unwrap_databag(const Object *p_access_databag) {
	const DatabagDynamicFetcher *bag = dynamic_cast<const DatabagDynamicFetcher *>(p_access_databag);
	if (unlikely(bag == nullptr || bag->get_target() == nullptr)) {
		return nullptr;
	}
	if (likely(bag->get_target_identifier() == T::get_databag_id())) {
		return static_cast<const T *>(bag->get_target());
	} else {
		return nullptr;
	}
}
}; // namespace godex
