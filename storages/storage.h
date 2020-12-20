

/* Author: AndreaCatania */

#ifndef STORAGE_H
#define STORAGE_H

#include "modules/ecs/ecs_types.h"

namespace godex {
class Component;
}

enum class StorageType {
	NONE,
	DENSE_VECTOR,
};

class Storage {
public:
	virtual ~Storage() {}
	virtual StorageType get_type() const { return StorageType::NONE; }
	virtual String get_type_name() const { return "Overload this function `get_type_name()` please."; }
	virtual bool has(EntityID p_entity) const {
		CRASH_NOW_MSG("Override this function.");
		return false;
	}
	virtual void insert_dynamic(EntityID p_entity, const Dictionary &p_data) {
		CRASH_NOW_MSG("Override this function.");
	}
	virtual const godex::Component *get_ptr(EntityID p_entity) const {
		CRASH_NOW_MSG("Override this function.");
		return nullptr;
	}
	virtual godex::Component *get_ptr(EntityID p_entity) {
		CRASH_NOW_MSG("Override this function.");
		return nullptr;
	}
	virtual void remove(EntityID p_index) {}
};

template <class T>
class TypedStorage : public Storage {
	static inline T phantom_data;

public:
	virtual void insert(EntityID p_entity, T p_data) {
		CRASH_NOW_MSG("Override this function.");
	}

	virtual const T &get(EntityID p_entity) const {
		CRASH_NOW_MSG("Override this function.");
		return phantom_data;
	}

	virtual T &get(EntityID p_entity) {
		CRASH_NOW_MSG("Override this function.");
		return phantom_data;
	}
};

#endif
