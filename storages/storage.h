

/* Author: AndreaCatania */

#ifndef STORAGE_H
#define STORAGE_H

#include "../ecs_types.h"

namespace godex {
class Component;
}

class Storage {
public:
	virtual ~Storage() {}
	virtual String get_type_name() const { return "Overload this function `get_type_name()` please."; }
	virtual bool has(EntityID p_entity) const {
		CRASH_NOW_MSG("Override this function.");
		return false;
	}
	virtual void insert_dynamic(EntityID p_entity, const Dictionary &p_data) {
		CRASH_NOW_MSG("Override this function.");
	}
	virtual Batch<const godex::Component> get_ptr(EntityID p_entity) const {
		CRASH_NOW_MSG("Override this function.");
		return nullptr;
	}
	virtual Batch<godex::Component> get_ptr(EntityID p_entity) {
		CRASH_NOW_MSG("Override this function.");
		return nullptr;
	}
	virtual void remove(EntityID p_index) {}
};

template <class T>
class TypedStorage : public Storage {
public:
	virtual void insert(EntityID p_entity, const T &p_data) {
		CRASH_NOW_MSG("Override this function.");
	}

	// TODO remove `std::remove_const_t` if useless, now.
	virtual Batch<const std::remove_const_t<T>> get(EntityID p_entity) const {
		CRASH_NOW_MSG("Override this function.");
		return Batch<const std::remove_const_t<T>>();
	}

	// TODO remove `std::remove_const_t` if useless, now.
	virtual Batch<std::remove_const_t<T>> get(EntityID p_entity) {
		CRASH_NOW_MSG("Override this function.");
		return Batch<std::remove_const_t<T>>();
	}
};

#endif
