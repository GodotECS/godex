#pragma once

#include "core/templates/paged_allocator.h"
#include "dense_vector.h"
#include "storage.h"

/// Storage that is a lot useful when it's necessary to store big components:
/// in those cases, you don't want to reallocate the memory each time a new
/// entity is added or removed.
///
/// This storage is also useful when you are dealing with libraries like
/// the physics engine, audio engine, etc... .
/// With those libraries, it's usually necessary to create objects, so thanks
/// to this storage it's possible to create components that are fixed in memory,
/// so pass the pointer or internal component pointers is safe.
///
/// Note: The allocated memory is contiguous, but fragmented. Internally the
/// storage creates some pages within the components are stored contiguosly.
template <class T>
class SteadyStorage : public Storage<T> {
	PagedAllocator<T, false> allocator;
	DenseVector<T *> storage;

public:
	virtual void configure(const Dictionary &p_config) override {
		clear();
		allocator.configure(p_config.get("page_size", 200));
	}

	virtual String get_type_name() const override {
		return "SteadyStorage[" + String(typeid(T).name()) + "]";
	}

	virtual bool is_steady() const override {
		return true;
	}

	virtual void insert(EntityID p_entity, const T &p_data) override {
		T *d = allocator.alloc();
		*d = p_data;
		storage.insert(p_entity, d);
		StorageBase::notify_changed(p_entity);
	}

	virtual bool has(EntityID p_entity) const override {
		return storage.has(p_entity);
	}

	virtual T *get(EntityID p_entity, Space p_mode = Space::LOCAL) override {
		StorageBase::notify_changed(p_entity);
		return storage.get(p_entity);
	}

	virtual const T *get(EntityID p_entity, Space p_mode = Space::LOCAL) const override {
		return storage.get(p_entity);
	}

	virtual void remove(EntityID p_entity) override {
		ERR_FAIL_COND_MSG(storage.has(p_entity) == false, "No entity: " + itos(p_entity) + " in this storage.");
		T *d = storage.get(p_entity);
		storage.remove(p_entity);
		allocator.free(d);
		// Make sure to remove as changed.
		StorageBase::notify_updated(p_entity);
	}

	virtual void clear() override {
		allocator.reset();
		storage.clear();
		StorageBase::flush_changed();
	}

	virtual EntitiesBuffer get_stored_entities() const {
		return { storage.get_entities().size(), storage.get_entities().ptr() };
	}
};
