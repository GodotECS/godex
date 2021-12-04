#pragma once

#include "entity_list.h"

/// Some stroages support `Entity` nesting, you can get local or global space
/// data, by specifying one or the other.
enum Space {
	LOCAL,
	GLOBAL,
};

struct EntitiesBuffer {
	uint32_t count;
	const EntityID *entities;
	EntitiesBuffer() = default;
	EntitiesBuffer(uint32_t c, const EntityID *e) :
			count(c), entities(e) {}
};

/// Never override this directly. Always override the `Storage`.
class StorageBase {
	LocalVector<EntityList *> changed_listeners;

public:
	/// This function is called each time this storage is initialized.
	/// It's possible to provide configuration by passing a dictionary.
	virtual void configure(const Dictionary &p_config) {}

	virtual ~StorageBase() {}
	virtual String get_type_name() const { return "Overload this function `get_type_name()` please."; }

	/// If a `Storage` set this to true, Godex will notify it once the `System`
	/// has finished using the storage.
	virtual bool notify_release_write() const {
		return false;
	}

	/// A storage return true when the component memory location never changes
	/// once allocated.
	virtual bool is_steady() const {
		return false;
	}

	virtual bool has(EntityID p_entity) const {
		CRASH_NOW_MSG("Override this function.");
		return false;
	}

	virtual void insert_dynamic(EntityID p_entity, const Dictionary &p_data) {
		CRASH_NOW_MSG("Override this function.");
	}

	virtual void *get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) {
		CRASH_NOW_MSG("Override this function.");
		return nullptr;
	}

	virtual const void *get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) const {
		CRASH_NOW_MSG("Override this function.");
		return nullptr;
	}

	/// Immediately removes the `Component` from this index.
	virtual void remove(EntityID p_index) {
		CRASH_NOW_MSG("Override this function.");
	}

	virtual void clear() {
		CRASH_NOW_MSG("Override this function.");
	}

	virtual EntitiesBuffer get_stored_entities() const {
		CRASH_NOW_MSG("Override this function.");
		return { 0, nullptr };
	}

	/// This function is called by the pipeline only at the end of the stage.
	/// It's always called in single thread and the Storage is not used by anyone.
	/// During this stage is also possible to safely operate on other Storages.
	///
	/// This function is called only if the function `notify_release_write`
	/// returns `true`.
	virtual void on_system_release() {}

public:
	void add_change_listener(EntityList *p_changed_listener) {
		if (changed_listeners.find(p_changed_listener) == -1) {
			changed_listeners.push_back(p_changed_listener);
		}
	}

	void remove_change_listener(EntityList *p_changed_listener) {
		const int64_t index = changed_listeners.find(p_changed_listener);
		if (index != -1) {
			changed_listeners.remove_at_unordered(index);
		}
	}

	void notify_changed(EntityID p_entity) {
		for (uint32_t i = 0; i < changed_listeners.size(); i += 1) {
			changed_listeners[i]->insert(p_entity);
		}
	}

	void notify_updated(EntityID p_entity) {
		for (uint32_t i = 0; i < changed_listeners.size(); i += 1) {
			changed_listeners[i]->remove(p_entity);
		}
	}

	void flush_changed() {
		for (uint32_t i = 0; i < changed_listeners.size(); i += 1) {
			changed_listeners[i]->clear();
		}
	}

public:
	/// This method is used by the `DataAccessor` to expose the `Storage` to
	/// GDScript.
	bool set(const StringName &p_name, const Variant &p_value) {
		ERR_FAIL_V_MSG(false, "The storage `set` function does nothing.");
	}

	/// This method is used by the `DataAccessor` to expose the `Storage` to
	/// GDScript.
	bool get(const StringName &p_name, Variant &r_value) const {
		ERR_FAIL_V_MSG(false, "The storage `get` function does nothing.");
	}

	/// This method is used by the `DataAccessor` to expose the `Storage` to
	/// GDScript.
	void da_call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) {
		if (String(p_method) == "insert") { // TODO make this a static StringName to improve the check.
			// Check argument count.
			if (unlikely((p_argcount < 1) || (p_argcount > 2))) {
				r_error.expected = 2;
				r_error.argument = p_argcount;
				r_error.error = p_argcount < 2 ? Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS : Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
				ERR_FAIL_MSG("The function `Storage::insert` expects at least 1 argument.");
			}

#ifdef DEBUG_ENABLED
			// Check argument validity
			if (unlikely(
						p_args[0] == nullptr ||
						p_args[0]->get_type() != Variant::INT ||
						(p_argcount == 2 &&
								(p_args[1] == nullptr ||
										p_args[1]->get_type() != Variant::DICTIONARY)))) {
				r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
				ERR_FAIL_MSG("The `Storage::insert` arguments are: Entity ID (Int); (optional) component values (Dictionary).");
			}
#endif

			insert_dynamic(
					p_args[0]->operator unsigned int(),
					(p_argcount == 2 && p_args[1] != nullptr) ? p_args[1]->operator Dictionary() : Dictionary());

		} else if (String(p_method) == "remove") { // TODO make this a static StringName to improve the check.
			// Check argument count.
			if (unlikely(p_argcount != 1)) {
				r_error.expected = 1;
				r_error.argument = p_argcount;
				r_error.error = p_argcount < 1 ? Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS : Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
				ERR_FAIL_MSG("The function `Storage::remove` expects just the enity ID (int).");
			}

#ifdef DEBUG_ENABLED
			// Check argument validity
			if (unlikely(
						p_args[0] == nullptr ||
						p_args[0]->get_type() != Variant::INT)) {
				r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
				ERR_FAIL_MSG("The `Storage::remove` expects just the Entity ID (Int).");
			}
#endif

			remove(p_args[0]->operator unsigned int());
		} else {
			r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
			ERR_FAIL_MSG("The `Storage` doesn't have the method: " + p_method);
		}

		r_error.error = Callable::CallError::CALL_OK;
	}
};

template <class T>
class Storage : public StorageBase {
public:
	virtual void insert_dynamic(EntityID p_entity, const Dictionary &p_data) override {
		T insert_data;

		// Set the custom data if any.
		for (const Variant *key = p_data.next(); key; key = p_data.next(key)) {
			T::set_by_name((void *)&insert_data, key->operator StringName(), *p_data.getptr(*key));
		}

		insert(p_entity, insert_data);
	}

	virtual void *get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) override final {
		return (void *)get(p_entity, p_mode);
	}

	virtual const void *get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) const override final {
		return (const void *)get(p_entity, p_mode);
	}

public:
	virtual void insert(EntityID p_entity, const T &p_data) {
		CRASH_NOW_MSG("Override this function.");
	}

	virtual T *get(EntityID p_entity, Space p_mode = Space::LOCAL) {
		CRASH_NOW_MSG("Override this function.");
		return nullptr;
	}

	virtual const T *get(EntityID p_entity, Space p_mode = Space::LOCAL) const {
		CRASH_NOW_MSG("Override this function.");
		return nullptr;
	}

	/// Must be overridden if this storage is storing in batch.
	virtual uint32_t get_batch_size(EntityID p_entity) const {
		return 1;
	}
};

class SharedStorageBase {
public:
	virtual ~SharedStorageBase() {}

	virtual godex::SID create_shared_component_dynamic(const Dictionary &p_data) {
		CRASH_NOW_MSG("Please override this function.");
		return UINT32_MAX;
	}

	virtual void free_shared_component(godex::SID p_id) {
		CRASH_NOW_MSG("Please override this function.");
	}

	virtual bool has_shared_component(godex::SID p_id) const {
		CRASH_NOW_MSG("Please override this function.");
		return false;
	}

	virtual void insert(EntityID p_entity, godex::SID p_id) {
		CRASH_NOW_MSG("Please override this function.");
	}
};

/// Base storage for shared components.
template <class T>
class SharedStorage : public SharedStorageBase, public Storage<T> {
public:
	virtual godex::SID create_shared_component(const T &p_data) {
		CRASH_NOW_MSG("Please override this function.");
		return UINT32_MAX;
	}

	virtual T *get_shared_component(godex::SID p_id) {
		CRASH_NOW_MSG("Please override this function.");
		return nullptr;
	}

	virtual const T *get_shared_component(godex::SID p_id) const {
		CRASH_NOW_MSG("Please override this function.");
		return nullptr;
	}

public:
	// Override SharedStorageBase
	virtual godex::SID create_shared_component_dynamic(const Dictionary &p_data) override final {
		// Create a new data inside the storage and take it back.
		const godex::SID sid = create_shared_component(T());
		T *data = get_shared_component(sid);

		// Set the custom data if any.
		// Setting it at this point because the SharedComponents are likely be
		// big storages, so it's better to assigne the data after its creation
		// and so avoid useless copy.
		for (const Variant *key = p_data.next(); key; key = p_data.next(key)) {
			T::set_by_name((void *)data, key->operator StringName(), *p_data.getptr(*key));
		}

		return sid;
	}

public:
	// Override Storage<T>
	virtual void insert(EntityID, const T &) override final {
		ERR_PRINT("This component is stored inside a SharedStorage, so you can't just insert the data using the normal `insert` function. Check the documentation.");
	}
};
