#pragma once

#include "../ecs_types.h"

namespace godex {

#define INSTIGATOR(m_class)                            \
	ECSCLASS(m_class)                                  \
	friend class World;                                \
                                                       \
private:                                               \
	/* Components */                                   \
	static inline uint32_t instigator_id = UINT32_MAX; \
                                                       \
public:                                                \
	static uint32_t get_instigator_id() { return instigator_id; }

template <class... Is>
struct ExtractInstigators {
	void extract(LocalVector<godex::instigator_id> &r_instigators) {}
};

template <class I, class... Is>
struct ExtractInstigators<I, Is...> : public ExtractInstigators<Is...> {
	void extract(LocalVector<godex::instigator_id> &r_instigators) {
		r_instigators.push_back(I::get_instigator_id());
		ExtractInstigators<Is...>::extract(r_instigators);
	}
};

template <class... Is>
LocalVector<godex::instigator_id> internal_get_instigators() {
	LocalVector<godex::instigator_id> instigators;
	ExtractInstigators<Is...> e;
	e.extract(instigators);
	return instigators;
}

#define INSTIGATED(...)                                          \
	static LocalVector<godex::instigator_id> get_instigators() { \
		return godex::internal_get_instigators<__VA_ARGS__>();   \
	}
} // namespace godex
