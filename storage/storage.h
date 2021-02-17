#pragma once

#include "entity_list.h"

/// Some stroages support `Entity` nesting, you can get local or global space
/// data, by specifying one or the other.
enum Space {
	LOCAL,
	GLOBAL,
};

struct EntitiesBuffer {
	const uint32_t count;
	const EntityID *entities;
};

/// Never override this directly. Always override the `Storage`.
class StorageBase {
	bool tracing_change = false;
	EntityList changed;

public:
	virtual ~StorageBase() {}
	virtual String get_type_name() const { return "Overload this function `get_type_name()` please."; }

	virtual bool notify_release_write() const {
		return false;
	}

	virtual bool has(EntityID p_entity) const {
		CRASH_NOW_MSG("Override this function.");
		return false;
	}

	virtual void insert_dynamic(EntityID p_entity, const Dictionary &p_data) {
		CRASH_NOW_MSG("Override this function.");
	}

	virtual Batch<const void> get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) const {
		CRASH_NOW_MSG("Override this function.");
		return nullptr;
	}

	virtual Batch<void> get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) {
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

	virtual void on_system_release() {}

public:
	void set_tracing_change(bool p_need_changed) {
		tracing_change = p_need_changed;
	}

	bool is_tracing_change() const {
		return tracing_change;
	}

	void notify_changed(EntityID p_entity) {
		if (tracing_change) {
			changed.insert(p_entity);
		}
	}

	void notify_updated(EntityID p_entity) {
		if (tracing_change) {
			changed.remove(p_entity);
		}
	}

	bool is_changed(EntityID p_entity) const {
		if (tracing_change) {
			return changed.has(p_entity);
		}
		return false;
	}

	void flush_changed() {
		if (tracing_change) {
			changed.clear();
		}
	}

	/// Used to hard reset the changed storage.
	void reset_changed() {
		changed.reset();
	}

	EntitiesBuffer get_changed_entities() const {
		return { changed.size(), changed.get_entities().ptr() };
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
			T::set_by_name(&insert_data, StringName(*key), *p_data.getptr(*key));
		}

		insert(p_entity, insert_data);
	}

	virtual Batch<const void> get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) const override {
		const Batch<const std::remove_const_t<T>> b = get(p_entity, p_mode);
		return Batch<const void>(b.get_data(), b.get_size());
	}

	virtual Batch<void> get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) override {
		const Batch<std::remove_const_t<T>> b = get(p_entity, p_mode);
		return Batch<void>(b.get_data(), b.get_size());
	}

public:
	virtual void insert(EntityID p_entity, const T &p_data) {
		CRASH_NOW_MSG("Override this function.");
	}

	// TODO remove `std::remove_const_t` if useless, now.
	virtual Batch<const std::remove_const_t<T>> get(EntityID p_entity, Space p_mode = Space::LOCAL) const {
		CRASH_NOW_MSG("Override this function.");
		return Batch<const std::remove_const_t<T>>();
	}

	// TODO remove `std::remove_const_t` if useless, now.
	virtual Batch<std::remove_const_t<T>> get(EntityID p_entity, Space p_mode = Space::LOCAL) {
		CRASH_NOW_MSG("Override this function.");
		return Batch<std::remove_const_t<T>>();
	}
};
