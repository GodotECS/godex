#pragma once

#include "core/templates/paged_allocator.h"
#include "dense_vector.h"
#include "storage.h"

/// The `SharedSteadyStorage` is the perfect choice when you want to share the
/// same component between entities, and that components memory never changes.
///
/// When dealing with physics engines or audio engines, etc... it's usually
/// needed to have objects that are shared between some other objects: in
/// all those cases, it's possible to use this storage.
template <class T>
class SharedSteadyStorage : public SharedStorage<T> {
	PagedAllocator<T, false> allocator;
	LocalVector<T *> allocated_pointers;
	DenseVector<godex::SID> storage;

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

	virtual godex::SID create_shared_component(const T &p_data) override {
		T *d = allocator.alloc();
		*d = p_data;
		godex::SID id = allocated_pointers.size();
		allocated_pointers.push_back(d);
		return id;
	}

	virtual void free_shared_component(godex::SID p_id) override {
		if (p_id < allocated_pointers.size()) {
			if (allocated_pointers[p_id] != nullptr) {
				allocator.free(allocated_pointers[p_id]);
				allocated_pointers[p_id] = nullptr;
			}
		}
	}

	virtual bool has_shared_component(godex::SID p_id) const override {
		if (p_id < allocated_pointers.size()) {
			return allocated_pointers[p_id] != nullptr;
		}
		return false;
	}

	virtual void insert(EntityID p_entity, godex::SID p_id) override {
		if (p_id < allocated_pointers.size()) {
			if (allocated_pointers[p_id] != nullptr) {
				storage.insert(p_entity, p_id);
				StorageBase::notify_changed(p_entity);
				return;
			}
		}
		ERR_PRINT("The SID is not poiting to any valid object. This is not supposed to happen.");
	}

	virtual T *get_shared_component(godex::SID p_id) override {
		if (p_id < allocated_pointers.size()) {
			if (allocated_pointers[p_id] != nullptr) {
				return allocated_pointers[p_id];
			}
		}
		CRASH_NOW_MSG("This Entity doesn't have anything stored, before get the data you have to use `has()`.");
		return nullptr;
	}

	virtual const T *get_shared_component(godex::SID p_id) const override {
		if (p_id < allocated_pointers.size()) {
			if (allocated_pointers[p_id] != nullptr) {
				return allocated_pointers[p_id];
			}
		}
		CRASH_NOW_MSG("This Entity doesn't have anything stored, before get the data you have to use `has()`.");
		return nullptr;
	}

	virtual bool has(EntityID p_entity) const override {
		bool h = storage.has(p_entity);
		if (h) {
			const godex::SID id = storage.get(p_entity);
			if (id < allocated_pointers.size()) {
				h = allocated_pointers[id] != nullptr;
			} else {
				// Doesn't have.
				h = false;
			}
		}
		return h;
	}

	virtual T *get(EntityID p_entity, Space p_mode = Space::LOCAL) override {
		StorageBase::notify_changed(p_entity);
		return get_shared_component(storage.get(p_entity));
	}

	virtual const T *get(EntityID p_entity, Space p_mode = Space::LOCAL) const override {
		return get_shared_component(storage.get(p_entity));
	}

	virtual void remove(EntityID p_entity) override {
		storage.remove(p_entity);
		// Make sure to remove as changed.
		StorageBase::notify_updated(p_entity);
	}

	virtual void clear() override {
		allocator.reset();
		allocated_pointers.reset();
		storage.clear();
		StorageBase::flush_changed();
	}

	virtual EntitiesBuffer get_stored_entities() const {
		return { storage.get_entities().size(), storage.get_entities().ptr() };
	}
};
