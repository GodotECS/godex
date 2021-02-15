/* Author: AndreaCatania */

#ifndef STORAGE_H
#define STORAGE_H

#include "../ecs_types.h"

namespace godex {
class Component;
}

/// Container used to mark the `Entity` as changed.
/// To mark an `Entity` as changed you can use `notify_updated`.
/// To mark an `Entity` as updated (so no more changed) `notify_updated`.
/// It's possible to use the `for_each([](EntityID p_entity){})` to iterate
/// the change list; while iterating it's safe and fast mark the `Entity`
/// as updated (using `notify_updated`).
class ChangeList {
	/// Sparse vector, used to easily know if an entity changed.
	/// points to the dense_list element.
	LocalVector<uint32_t> entity_to_data;

	/// Used to iterate fast.
	LocalVector<EntityID> dense_list;

	/// Iterator index, used by the mutable for_each` to make sure the update
	/// mechanism is safe even while iterating.
	int64_t iteration_index = -1;

public:
	void notify_changed(EntityID p_entity) {
		if (entity_to_data.size() <= p_entity) {
			const uint32_t initial_size = entity_to_data.size();
			entity_to_data.resize(p_entity + 1);
			for (uint32_t i = initial_size; i < entity_to_data.size(); i += 1) {
				entity_to_data[i] = UINT32_MAX;
			}
		}
		if (entity_to_data[p_entity] == UINT32_MAX) {
			// This entity was not yet notified.
			entity_to_data[p_entity] = dense_list.size();
			dense_list.push_back(p_entity);
		}
	}

	void notify_updated(EntityID p_entity) {
		if (entity_to_data.size() <= p_entity) {
			// Was not changed, Nothing to do.
			return;
		}

		const uint32_t index = entity_to_data[p_entity];

		if (iteration_index >= index) {
			// The current iteration_index is bigger than the index
			// to remove: meaning that we already iterated that.
			// Basing on that, it's possible to perform three copy to
			// kick the index out, in a way that the remainin non
			// processed `EntityID` will be processed.
			// *Note: this mechanism allow to correctly process all the
			//        entities while avoiding copy the entire vector to
			//        keep the vector sort.

			// 1. Copy the current index (already processed) on the index to remove.
			dense_list[index] = dense_list[iteration_index];
			entity_to_data[dense_list[index]] = index;

			// 2. Copy the last element on the current index.
			dense_list[iteration_index] = dense_list[dense_list.size() - 1];
			entity_to_data[dense_list[iteration_index]] = iteration_index;

			// 3. Decrese the current index so to process again this index
			//    since it has a new data now.
			iteration_index -= 1;

			// 4. Just resize the array by -1;
			dense_list.resize(dense_list.size() - 1);

			// 5. Clear the entity_pointer since it was removed.
			entity_to_data[p_entity] = UINT32_MAX;
			return;
		}

		// No iteration in progress or not yet iterated.
		if (index != UINT32_MAX) {
			// Remove the element by replacing it with the last one.
			// Assign the currect entity to remove index to the last one.
			entity_to_data[dense_list[dense_list.size() - 1]] = index;
			entity_to_data[p_entity] = UINT32_MAX;
			dense_list[index] = dense_list[dense_list.size() - 1];
			dense_list.resize(dense_list.size() - 1);

			// This code, make sure to decrease by 1 the iterator index, only
			// if it's iterating, otherwise does nothing.
			iteration_index -= 1;
			iteration_index = MAX(-1, iteration_index);
		}
	}

	bool has(EntityID p_entity) const {
		if (entity_to_data.size() <= p_entity) {
			return false;
		}
		return entity_to_data[p_entity] != UINT32_MAX;
	}

	template <typename F>
	void for_each(F func) {
		for (iteration_index = 0; iteration_index < dense_list.size(); iteration_index += 1) {
			func(dense_list[iteration_index]);
		}
		iteration_index = -1;
	}

	template <typename F>
	void for_each(F func) const {
		for (uint32_t i = 0; i < dense_list.size(); i += 1) {
			func(dense_list[i]);
		}
	}

	bool is_empty() const {
		return dense_list.size() == 0;
	}

	bool size() const {
		return dense_list.size();
	}

	void clear() {
		for (uint32_t i = 0; i < entity_to_data.size(); i += 1) {
			entity_to_data[i] = UINT32_MAX;
		}
		dense_list.clear();
	}
};

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
	bool need_changed = false;
	ChangeList changed;

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
	void set_need_changed(bool p_need_changed) {
		need_changed = p_need_changed;
	}

	bool get_need_changed() const {
		return need_changed;
	}

	void notify_changed(EntityID p_entity) {
		if (need_changed) {
			changed.notify_changed(p_entity);
		}
	}

	void notify_updated(EntityID p_entity) {
		if (need_changed) {
			changed.notify_updated(p_entity);
		}
	}

	bool is_changed(EntityID p_entity) const {
		if (need_changed) {
			return changed.has(p_entity);
		}
		return false;
	}

	void flush_changed() {
		if (need_changed) {
			changed.clear();
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

#endif
