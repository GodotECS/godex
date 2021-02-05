/* Author: AndreaCatania */

#ifndef STORAGE_H
#define STORAGE_H

#include "../ecs_types.h"

namespace godex {
class Component;
}

class ChangeList {
	LocalVector<EntityID> changed;
	int iteration_index = -1;

public:
	void notify_changed(EntityID p_entity) {
		if (changed.find(p_entity) == -1)
			changed.push_back(p_entity);
	}

	void notify_updated(EntityID p_entity) {
		const int64_t index = changed.find(p_entity);
		if (index != -1) {
			if (iteration_index != -1) {
				// Iteration in progress.

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
					changed[index] = changed[iteration_index];
					// 2. Copy the last element on the current index.
					changed[iteration_index] = changed[changed.size() - 1];
					// 3. Decrese the current index so to process again this index
					//    since it has a new data now.
					iteration_index -= 1;
					// 4. Just resize the array by -1;
					changed.resize(changed.size() - 1);

				} else {
					// This element is not yet fetched, just remove it.
					changed.remove_unordered(index);
					iteration_index -= 1;
				}
			} else {
				// No iteration in progress, just remove it.
				changed.remove_unordered(index);
			}
		}
	}

	template <typename F>
	void for_each(F func) {
		for (iteration_index = 0; iteration_index < int(changed.size()); iteration_index += 1) {
			func(changed[iteration_index]);
		}
		iteration_index = -1;
	}

	template <typename F>
	void for_each(F func) const {
		for (uint32_t i = 0; i < changed.size(); i += 1) {
			func(changed[i]);
		}
	}

	bool is_empty() const {
		return changed.size() == 0;
	}

	void clear() {
		CRASH_COND_MSG(iteration_index != -1, "It's not possible to clear while iterating.");
		changed.clear();
	}
};

/// Some stroages support `Entity` nesting, you can get local or global space
/// data, by specifying one or the other.
enum class Space {
	LOCAL,
	GLOBAL,
};

/// Never override this directly. Always override the `Storage`.
class StorageBase {
public:
	virtual ~StorageBase() {}
	virtual String get_type_name() const { return "Overload this function `get_type_name()` please."; }
	virtual bool has(EntityID p_entity) const {
		CRASH_NOW_MSG("Override this function.");
		return false;
	}

	virtual void insert_dynamic(EntityID p_entity, const Dictionary &p_data) {
		CRASH_NOW_MSG("Override this function.");
	}

	virtual Batch<const godex::Component> get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) const {
		CRASH_NOW_MSG("Override this function.");
		return nullptr;
	}

	virtual Batch<godex::Component> get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) {
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

public:
	// ~~ `DataAccessor` functions.
	bool set(const StringName &p_name, const Variant &p_value) {
		ERR_FAIL_V_MSG(false, "The storage `set` function does nothing.");
	}

	bool get(const StringName &p_name, Variant &r_value) const {
		ERR_FAIL_V_MSG(false, "The storage `get` function does nothing.");
	}

	void call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) {
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
			insert_data.set(StringName(*key), *p_data.getptr(*key));
		}

		insert(p_entity, insert_data);
	}

	virtual Batch<const godex::Component> get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) const override {
		return get(p_entity, p_mode);
	}

	virtual Batch<godex::Component> get_ptr(EntityID p_entity, Space p_mode = Space::LOCAL) override {
		return get(p_entity, p_mode);
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
