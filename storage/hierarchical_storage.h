#pragma once

#include "../components/child.h"
#include "dense_vector.h"

class Hierarchy : public Storage<Child> {
	DenseVector<Child> storage;

public:
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
	EntityID parent;
	bool is_dirty;
};

/// Stores the data
template <class T>
class HierarchicalStorage : public Storage<T> {
	DenseVector<LocalGlobal<T>> internal_storage;
	ChangeList changed;

public:
	// TODO make this private.
	// TODO do I need this????
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
		LocalGlobal<T> d;
		d.local = p_data;
		d.global = p_data;
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
		changed.notify_changed(p_entity);
		return &internal_storage.get(p_entity).local;
	}

	const T *get_global(EntityID p_entity) const {
		const LocalGlobal<T> &data = internal_storage.get(p_entity);
		return data.parent.is_null() ? &data.local : &data.global;
	}

	T *get_global(EntityID p_entity) {
		changed.notify_changed(p_entity);
		LocalGlobal<T> &data = internal_storage.get(p_entity);
		return data.parent.is_null() ? &data.local : &data.global;
	}

	void update_entity_data(EntityID p_entity) {
#ifdef DEBUG_ENABLED
		CRASH_COND_MSG(hierarchy == nullptr, "Hierarchy is never supposed to be nullptr.");
#endif
		LocalGlobal<T> &data = internal_storage.get(p_entity);

		if (data.is_dirty == false) {
			// Nothing to do.
			return;
		}

		if (data.parent.is_null() || has(data.parent) == false) {
			// This is root, nothing to do.
		} else {
			update_entity_data(data.parent);
			const T *global_parent_data = get_global(data.parent);
			T::combine(data.local, *global_parent_data, data.global);
		}

		changed.notify_updated(p_entity);
	}

	void update_dirty() {
		// TODO propagate changes.
		changed.for_each([](EntityID p_entity) {

		});

		// TODO Update the dirty data.
	}
};
