#pragma once

#include "../components/child.h"
#include "dense_vector.h"

class Hierarchy : public Storage<Child> {
	DenseVector<Child> childs;

public:
	virtual String get_type_name() const override {
		return "Hierarchy";
	}

	virtual bool has(EntityID p_entity) const override {
		return childs.has(p_entity);
	}

	virtual void remove(EntityID p_index) override {
		childs.remove(p_index);
	}

	virtual void clear() override {
		childs.clear();
	}

	virtual void insert(EntityID p_entity, const Child &p_data) override {
		childs.insert(p_entity, p_data);
	}

	virtual Batch<const Child> get(EntityID p_entity) const override {
		return &childs.get(p_entity);
	}

	virtual Batch<Child> get(EntityID p_entity) override {
		return &childs.get(p_entity);
	}
};

/// Used to store local and global structure.
template <class T>
struct LocalGlobal {
	T local;
	T global;
	EntityID parent;
};

/// Stores the data
template <class T>
class HierarchicalStorage : public Storage<T> {
	DenseVector<LocalGlobal<T>> internal_storage;

public:
	// TODO make this private.
	Hierarchy *hierarchy = nullptr;

	virtual String get_type_name() const override {
		return "HierarchicalStorage[" + String(typeid(T).name()) + "]";
	}

	virtual bool has(EntityID p_entity) const override {
		return internal_storage.has(p_entity);
	}

	virtual void remove(EntityID p_index) override {
		internal_storage.remove(p_index);
	}

	virtual void clear() override {
		internal_storage.clear();
	}

	virtual void insert(EntityID p_entity, const T &p_data) override {
#ifdef DEBUG_ENABLED
		CRASH_COND_MSG(hierarchy == nullptr, "Hierarchy is never supposed to be nullptr.");
#endif

		LocalGlobal<T> d;
		d.local = p_data;
		d.global = p_data;
		if (hierarchy->has(p_entity)) {
			d.parent = hierarchy->get(p_entity)->parent;
		}
		internal_storage.insert(p_entity, d);
	}

	virtual Batch<const T> get(EntityID p_entity) const override {
		return &internal_storage.get(p_entity).local;
	}

	virtual Batch<T> get(EntityID p_entity) override {
		return &internal_storage.get(p_entity).local;
	}

	const T *get_local(EntityID p_entity) const {
		return &internal_storage.get(p_entity).local;
	}

	T *get_local(EntityID p_entity) {
		return &internal_storage.get(p_entity).local;
	}

	const T *get_global(EntityID p_entity) {
		LocalGlobal<T> &data = internal_storage.get(p_entity);

		// Resolve global here if needed.
		if (data.parent.is_null()) {
			// No parent, this is the root, so return local.
			return &data.local;
		}

		// TODO cache?
		// TODO add atomic check here?
		// Ok time to reload the global.

		ERR_FAIL_COND_V_MSG(has(data.parent) == false, &data.local, "The parent [" + itos(data.parent) + "] of this entity " + itos(p_entity) + " doesn't exists.");

		const T *global_parent_data = get_global(data.parent);
		T::combine(data.local, *global_parent_data, data.global);

		return &data.global;
	}

	void trigger_changed(EntityID p_entity) {
		// TODO
	}
};
