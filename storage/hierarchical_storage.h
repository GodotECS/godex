#pragma once

#include "../components/child.h"
#include "dense_vector.h"

class Hierarchy;

class HierarchicalStorageBase {
	friend class Hierarchy;
	friend class World;

protected:
	const Hierarchy *hierarchy = nullptr;

public:
	virtual void flush_hierarchy_changes() = 0;
};

class Hierarchy : public Storage<Child> {
	DenseVector<Child> storage;
	ChangeList changed;
	LocalVector<HierarchicalStorageBase *> sub_storages;

public:
	void add_sub_storage(HierarchicalStorageBase *p_storage) {
		CRASH_COND_MSG(p_storage->hierarchy != nullptr, "This hierarchy storage is already added to another `Hierarchy`.");
		sub_storages.push_back(p_storage);
		p_storage->hierarchy = this;
	}

	const ChangeList &get_changed() const {
		return changed;
	}

	void flush() {
		for (uint32_t i = 0; i < sub_storages.size(); i += 1) {
			sub_storages[i]->flush_hierarchy_changes();
		}
		changed.clear();
	}

	virtual String get_type_name() const override {
		return "Hierarchy";
	}

	virtual bool has(EntityID p_entity) const override {
		return storage.has(p_entity);
	}

	virtual void remove(EntityID p_entity) override {
		if (storage.has(p_entity)) {
			Child &child = storage.get(p_entity);

			// 1. Unlink from parent.
			unlink_parent(p_entity, child);

			// 2. Unlink the childs.
			unlink_childs(child);

			// Drop the data.
			storage.remove(p_entity);
		}
	}

	virtual void clear() override {
		storage.clear();
	}

	virtual void insert(EntityID p_entity, const Child &p_data) override {
		bool update;

		if (storage.has(p_entity)) {
			// This is an update.
			update = true;
		} else {
			update = false;
			storage.insert(p_entity, p_data);
		}

		Child &child = storage.get(p_entity);

		if (update) {
			// Update the chain.
			if (child.parent == p_data.parent) {
				// Nothing to do.
				return;
			} else {
				// 1. Unlink p_entity with its current parent.
				unlink_parent(p_entity, child);

				// 2. Set `p_entity` with its new parent.
				child.parent = p_data.parent;
			}
		} else {
			// This is a new insert, so make sure `first_child` and `next` are
			// not set.
			child.first_child = EntityID();
			child.next = EntityID();
		}

		changed.notify_changed(p_entity);

		// Update the parent if any.
		if (child.parent.is_null() == false) {
			if (has(child.parent) == false) {
				// Parent is always root when added in this way.
				insert(child.parent, Child());
			}

			Child &parent = storage.get(child.parent);

			// Add `child` as child of this parent but keep the chain.
			EntityID prev_first_child = parent.first_child;
			parent.first_child = p_entity;
			child.next = prev_first_child;
		}
	}

	virtual Batch<const Child> get(EntityID p_entity) const override {
		return &storage.get(p_entity);
	}

	virtual Batch<Child> get(EntityID p_entity) override {
		CRASH_NOW_MSG("You can't fetch the parenting as mutable. This storage is special and you have to use `insert` to update the structure.");
		return nullptr;
	}

	/// For each child, slow version.
	template <typename F>
	void for_each_child(EntityID p_entity, F func) const {
		const Child &child = storage.get(p_entity);
		for_each_child(child, func);
	}

	/// Iterate over the childs of this `Entity`.
	template <typename F>
	void for_each_child(const Child &this_entity_data, F func) const {
		EntityID next = this_entity_data.first_child;
		while (next.is_null() == false) {
			const Child &next_c = storage.get(next);
			if (func(next, next_c) == false) {
				// Stop here.
				return;
			}
			next = next_c.next;
		}
	}

	/// For each parent, slow version.
	template <typename F>
	void for_each_parent(EntityID p_entity, F func) const {
		const Child &child = storage.get(p_entity);
		for_each_parent(child, func);
	}

	/// For each parent, fast version.
	template <typename F>
	void for_each_parent(const Child &this_entity_data, F func) const {
		EntityID parent = this_entity_data.parent;
		while (parent.is_null() == false) {
			const Child &parent_c = storage.get(parent);
			if (func(parent, parent_c) == false) {
				// Stop here.
				return;
			}
			parent = parent_c.parent;
		}
	}

private:
	/// This is private because it's possible to alter the hierarchy only via:
	/// `insert`, `remove`.
	template <typename F>
	void for_each_child_mutable(Child &this_entity_data, F func) {
		EntityID next = this_entity_data.first_child;
		while (next.is_null() == false) {
			Child &next_c = storage.get(next);
			if (func(next, next_c) == false) {
				// Stop here.
				return;
			}
			next = next_c.next;
		}
	}

	/// Unlink from parent.
	void unlink_parent(EntityID p_entity, Child &this_enity_data) {
		if (this_enity_data.parent.is_null()) {
			return;
		}

		Child &parent = storage.get(this_enity_data.parent);
		if (parent.first_child == p_entity) {
			// Just remove from `first_child`.
			parent.first_child = this_enity_data.next;
		} else {
			for_each_child_mutable(parent, [&](EntityID p_sub_child_id, Child &p_sub_child) -> bool {
				if (p_sub_child.next == p_entity) {
					p_sub_child.next = this_enity_data.next;
					// Interrupt.
					return false;
				}
				return true;
			});
		}
	}

	void unlink_childs(Child &this_entity_data) {
		for_each_child_mutable(this_entity_data, [](EntityID p_entity, Child &p_child) -> bool {
			p_child.parent = EntityID();
			// Keep iterate.
			return true;
		});

		// No more childs.
		this_entity_data.first_child = EntityID();
	}
};

/// Used to store local and global structure.
template <class T>
struct LocalGlobal {
	T local;
	T global;
	bool is_root = true;
	bool global_changed = false;
};

/// Stores the data
template <class T>
class HierarchicalStorage : public Storage<T>, public HierarchicalStorageBase {
	DenseVector<LocalGlobal<T>> internal_storage;
	// List of `Entities` taken mutably, for which we need to flush.
	ChangeList dirty_list;

public:
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
		LocalGlobal<T> d;
		d.local = p_data;
		internal_storage.insert(p_entity, d);
		propagate_change(
				p_entity,
				internal_storage.get(p_entity));
	}

	virtual Batch<const T> get(EntityID p_entity) const override {
		return &internal_storage.get(p_entity).local;
	}

	/// This function returns an internal piece of memory that is possible
	/// to modify. From the outside, it's not possible to detect if the
	/// variable is modified, so we have to assume that it will.
	/// Use the const function suppress this logic.
	/// The data is flushed at the end of each `system`, however you can flush it
	/// manually via storage, if you need the data immediately back.
	virtual Batch<T> get(EntityID p_entity) override {
		dirty_list.notify_changed(p_entity);
		LocalGlobal<T> &data = internal_storage.get(p_entity);
		data.global_changed = false;
		return &data.local;
	}

	const T *get_global(EntityID p_entity) const {
		const LocalGlobal<T> &data = internal_storage.get(p_entity);
		return data.is_root ? &data.local : &data.global;
	}

	/// This function returns an internal piece of memory that is possible
	/// to modify. From the outside, it's not possible to detect if the
	/// variable is modified, so we have to assume that it will.
	/// Use the const function suppress this logic.
	/// The data is flushed at the end of each `system`, however you can flush it
	/// manually via storage, if you need the data immediately back.
	T *get_global(EntityID p_entity) {
		dirty_list.notify_changed(p_entity);
		LocalGlobal<T> &data = internal_storage.get(p_entity);
		data.global_changed = true;
		return data.is_root ? &data.local : &data.global;
	}

	void propagate_change(EntityID p_entity) {
		if (has(p_entity) == false) {
			dirty_list.notify_updated(p_entity);
			return;
		}

		LocalGlobal<T> &data = internal_storage.get(p_entity);
		propagate_change(p_entity, data);
	}

	void propagate_change(EntityID p_entity, LocalGlobal<T> &p_data) {
		p_data.is_root = true;

		if (hierarchy->has(p_entity) == false) {
			// This is not parented, nothing to do.
			dirty_list.notify_updated(p_entity);
			return;
		}
		const Child *child = hierarchy->get(p_entity);
		propagate_change(p_entity, p_data, *child);
	}

	void propagate_change(EntityID p_entity, LocalGlobal<T> &p_data, const Child &p_child) {
#ifdef DEBUG_ENABLED
		CRASH_COND_MSG(hierarchy == nullptr, "Hierarchy is never supposed to be nullptr.");
#endif

		// Update this global first.
		if (p_child.parent.is_null() || has(p_child.parent) == false) {
			// This is root.
			// Nothing to do.
		} else {
			if (p_data.global_changed) {
				// The global got modified.
				// Take it non mutable so to not trigger the change list.
				const T *global_parent_data = const_cast<const HierarchicalStorage<T> *>(this)->get_global(p_child.parent);
				T::combine_inverse(p_data.global, *global_parent_data, p_data.local);
			} else {
				// The local was modified.
				// Take it non mutable so to not trigger the change list.
				const T *global_parent_data = const_cast<const HierarchicalStorage<T> *>(this)->get_global(p_child.parent);
				T::combine(p_data.local, *global_parent_data, p_data.global);
			}
			p_data.is_root = false;
			p_data.global_changed = false;
		}
		dirty_list.notify_updated(p_entity);

		// Now propagate the change to the childs.
		hierarchy->for_each_child(p_child, [&](EntityID p_child_entity, const Child &p_child_data) -> bool {
			if (has(p_child_entity)) {
				propagate_change(
						p_child_entity,
						internal_storage.get(p_child_entity),
						p_child_data);
			}
			return true;
		});
	}

	void flush() {
		dirty_list.for_each([&](EntityID entity) {
			propagate_change(entity);
		});
#ifdef DEBUG_ENABLED
		CRASH_COND_MSG(dirty_list.is_empty() == false, "At this point the flush list must be empty.");
#endif
	}

	virtual void flush_hierarchy_changes() override {
		hierarchy->get_changed().for_each([&](EntityID entity) {
			dirty_list.notify_changed(entity);
		});
		flush();
	}
};
