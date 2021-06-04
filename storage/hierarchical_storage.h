#pragma once

#include "../components/child.h"
#include "dense_vector.h"

class Hierarchy;

class HierarchicalStorageBase {
	friend class Hierarchy;

protected:
	const Hierarchy *hierarchy = nullptr;

public:
	virtual ~HierarchicalStorageBase() {}
	virtual void flush_hierarchy_changes() = 0;
};

/// The Hierarchy storage holds the structure between Entities.
/// It's possible to fetch this in a system that run in multiple threads because
/// the flush happens inside `on_system_release` at the end of the stage:
/// that happens in single thread and now one is using it in any way.
class Hierarchy : public Storage<Child> {
	DenseVector<Child> storage;
	EntityList hierarchy_changed;
	LocalVector<HierarchicalStorageBase *> sub_storages;

public:
	void configure(const Dictionary &p_config) {
		storage.reset();
		storage.configure(p_config.get("pre_allocate", 500));
	}

	void add_sub_storage(HierarchicalStorageBase *p_storage) {
		CRASH_COND_MSG(p_storage->hierarchy != nullptr, "This hierarchy storage is already added to another `Hierarchy`.");
		sub_storages.push_back(p_storage);
		p_storage->hierarchy = this;
	}

	const EntityList &get_changed() const {
		return hierarchy_changed;
	}

	virtual void on_system_release() override {
		flush_hierarchy_changes();
	}

	void flush_hierarchy_changes() {
		for (uint32_t i = 0; i < sub_storages.size(); i += 1) {
			sub_storages[i]->flush_hierarchy_changes();
		}
		hierarchy_changed.clear();
	}

	virtual String get_type_name() const override {
		return "Hierarchy";
	}

	virtual bool notify_release_write() const override {
		return true;
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

			// 3. Drop the data.
			storage.remove(p_entity);

			// 4. Mark this as changed.
			hierarchy_changed.insert(p_entity);
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
		} else if (p_data.parent.is_null() == false) {
			// This is a new insert.
			update = false;
			storage.insert(p_entity, p_data);
		} else {
			// This is a new insert but there is no parent so nothing to do.
			return;
		}

		Child &child = storage.get(p_entity);

		if (update) {
			if (child.parent == p_data.parent) {
				// Same parent, nothing to do.
				return;
			}

			// Has another parent, update the relationships:
			// 1. Unlink p_entity with its current parent.
			unlink_parent(p_entity, child);

			// 2. Set `p_entity` with its new parent.
			child.parent = p_data.parent;

			if (child.parent.is_null() && child.first_child.is_null()) {
				// There are no more relations, so just remove this.
				remove(p_entity);
				return;
			}
		} else {
			// This is a new insert, so make sure `first_child` and `next` are
			// not set.
			child.first_child = EntityID();
			child.next = EntityID();
		}

		hierarchy_changed.insert(p_entity);

		// Update the parent if any.
		if (child.parent.is_null() == false) {
			if (has(child.parent) == false) {
				// Parent is always root when added in this way.
				storage.insert(child.parent, Child());
				hierarchy_changed.insert(child.parent);
			}

			Child &parent = storage.get(child.parent);

			// Add `child` as child of this parent but keep the chain.
			EntityID prev_first_child = parent.first_child;
			parent.first_child = p_entity;
			child.next = prev_first_child;
		}
	}

	virtual Child *get(EntityID p_entity, Space p_mode = Space::LOCAL) override {
		CRASH_NOW_MSG("You can't fetch the parenting as mutable. This storage is special and you have to use `insert` to update the structure.");
		return nullptr;
	}

	virtual const Child *get(EntityID p_entity, Space p_mode = Space::LOCAL) const override {
		return &storage.get(p_entity);
	}

	virtual EntitiesBuffer get_stored_entities() const {
		return { storage.get_entities().size(), storage.get_entities().ptr() };
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
	void unlink_parent(EntityID p_entity, Child &this_entity_data) {
		if (this_entity_data.parent.is_null()) {
			return;
		}

		Child &parent = storage.get(this_entity_data.parent);
		if (parent.first_child == p_entity) {
			// This `Entity` is the first child of its parent.
			parent.first_child = this_entity_data.next;

		} else {
			// This `Entity` is not the first child, so search it and remove.
			for_each_child_mutable(parent, [&](EntityID p_sub_child_id, Child &p_sub_child) -> bool {
				if (p_sub_child.next == p_entity) {
					// This `Entity` is pointeed by this `sub_child` so remove it.
					p_sub_child.next = this_entity_data.next;
					return false;
				}
				return true;
			});
		}

		const EntityID parent_entity = this_entity_data.parent;
		this_entity_data.parent = EntityID();

		if (parent.first_child.is_null() && parent.parent.is_null()) {
			// Since this parent has no more relationships, remove it.
			remove(parent_entity);
		}
	}

	/// Unlink from childs.
	void unlink_childs(Child &this_entity_data) {
		for_each_child_mutable(this_entity_data, [&](EntityID p_entity, Child &p_child) -> bool {
			p_child.parent = EntityID();

			if (p_child.first_child.is_null()) {
				// Since this child has no more relationships, remove it.
				remove(p_entity);
			}

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
	bool has_relationship = false;
	bool global_changed = false;
};

/// Stores the data
template <class T>
class HierarchicalStorage : public Storage<T>, public HierarchicalStorageBase {
	DenseVector<LocalGlobal<T>> internal_storage;
	// List of `Entities` taken mutably, for which we need to flush.
	EntityList relationship_dirty_list;

public:
	void configure(const Dictionary &p_config) {
		internal_storage.reset();
		internal_storage.configure(p_config.get("pre_allocate", 500));
	}

	virtual String get_type_name() const override {
		return "HierarchicalStorage[" + String(typeid(T).name()) + "]";
	}

	virtual bool notify_release_write() const override {
		return true;
	}

	virtual bool has(EntityID p_entity) const override {
		return internal_storage.has(p_entity);
	}

	virtual void remove(EntityID p_index) override {
		internal_storage.remove(p_index);
		StorageBase::notify_updated(p_index);
	}

	virtual void clear() override {
		internal_storage.clear();
		StorageBase::flush_changed();
	}

	virtual void insert(EntityID p_entity, const T &p_data) override {
		LocalGlobal<T> d;
		d.local = p_data;
		internal_storage.insert(p_entity, d);
		StorageBase::notify_changed(p_entity);
		propagate_change(
				p_entity,
				internal_storage.get(p_entity));
	}

	/// This function returns an internal piece of memory that is possible
	/// to modify. From the outside, it's not possible to detect if the
	/// variable is modified, so we have to assume that it will.
	/// Use the const function suppress this logic.
	/// The data is flushed at the end of each `system`, however you can flush it
	/// manually via storage, if you need the data immediately back.
	virtual T *get(EntityID p_entity, Space p_mode = Space::LOCAL) override {
		StorageBase::notify_changed(p_entity);
		LocalGlobal<T> &data = internal_storage.get(p_entity);
		if (data.has_relationship) {
			relationship_dirty_list.insert(p_entity);
			data.global_changed = p_mode == Space::GLOBAL;
		}
		return data.is_root || p_mode == Space::LOCAL ? &data.local : &data.global;
	}

	virtual const T *get(EntityID p_entity, Space p_mode = Space::LOCAL) const override {
		const LocalGlobal<T> &data = internal_storage.get(p_entity);
		return p_mode == Space::LOCAL || data.is_root ? &data.local : &data.global;
	}

	virtual EntitiesBuffer get_stored_entities() const {
		return { internal_storage.get_entities().size(), internal_storage.get_entities().ptr() };
	}

	void propagate_change(EntityID p_entity) {
		if (has(p_entity) == false) {
			relationship_dirty_list.remove(p_entity);
			return;
		}

		LocalGlobal<T> &data = internal_storage.get(p_entity);
		propagate_change(p_entity, data);
	}

	void propagate_change(EntityID p_entity, LocalGlobal<T> &p_data) {
		p_data.is_root = true;

		if (hierarchy->has(p_entity) == false) {
			// This is not parented, nothing more to do.
			relationship_dirty_list.remove(p_entity);
			p_data.has_relationship = false;
			StorageBase::notify_changed(p_entity);
		} else {
			// This is parented, continue
			const Child *child = hierarchy->get(p_entity);
			p_data.has_relationship = true;

			propagate_change(p_entity, p_data, *child);
		}
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
				const T *global_parent_data = const_cast<const HierarchicalStorage<T> *>(this)->get(p_child.parent, Space::GLOBAL);
				T::combine_inverse(p_data.global, *global_parent_data, p_data.local);
			} else {
				// The local was modified.
				// Take it non mutable so to not trigger the change list.
				const T *global_parent_data = const_cast<const HierarchicalStorage<T> *>(this)->get(p_child.parent, Space::GLOBAL);
				T::combine(p_data.local, *global_parent_data, p_data.global);
			}
			p_data.is_root = false;
			p_data.global_changed = false;

			StorageBase::notify_changed(p_entity);
		}

		relationship_dirty_list.remove(p_entity);

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

	virtual void on_system_release() override {
		flush_changes();
	}

	void flush_changes() {
		relationship_dirty_list.for_each([&](EntityID entity) {
			propagate_change(entity);
		});
#ifdef DEBUG_ENABLED
		CRASH_COND_MSG(relationship_dirty_list.is_empty() == false, "At this point the flush list must be empty.");
#endif
	}

	virtual void flush_hierarchy_changes() override {
		hierarchy->get_changed().for_each([&](EntityID entity) {
			relationship_dirty_list.insert(entity);
		});
		flush_changes();
	}
};
