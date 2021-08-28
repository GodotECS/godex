#pragma once

#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"
#include "core/variant/dictionary.h"

class EventStorageBase {
public:
	virtual ~EventStorageBase() {
	}

	virtual void add_event_emitter(const String &p_emitter) {
		CRASH_NOW_MSG("Override this function.");
	}

	virtual bool has_emitter(const String &p_emitter) {
		CRASH_NOW_MSG("Override this function.");
		return false;
	}

	virtual void add_event_dynamic(const String &p_emitter, const Dictionary &p_data) {
		CRASH_NOW_MSG("Override this function.");
	}

	virtual const void *get_events_ptr(const String &p_emitter) const {
		CRASH_NOW_MSG("Override this function.");
		return nullptr;
	}

	virtual Array get_events_array(const String &p_emitter) const {
		CRASH_NOW_MSG("Override this function.");
		return Array();
	}

	virtual void flush_events() {
		CRASH_NOW_MSG("Override this function.");
	}
};

template <class E>
class EventStorage : public EventStorageBase {
	OAHashMap<String, LocalVector<E>> events_map;

public:
	virtual void add_event_emitter(const String &p_emitter) override {
		ERR_FAIL_COND_MSG(events_map.has(p_emitter), String("The emitter `") + p_emitter + "` for the event `" + E::get_class_static() + "` exists.");
		events_map.insert(p_emitter, LocalVector<E>());
	}

	virtual bool has_emitter(const String &p_emitter) override {
		return events_map.has(p_emitter);
	}

	virtual void add_event_dynamic(const String &p_emitter, const Dictionary &p_data) override {
		E insert_data;

		// Set the custom data if any.
		for (const Variant *key = p_data.next(); key; key = p_data.next(key)) {
			E::set_by_name((void *)&insert_data, key->operator String(), *p_data.getptr(*key));
		}

		add_event(p_emitter, insert_data);
	}

	virtual const void *get_events_ptr(const String &p_emitter) const override {
		return (const void *)get_events(p_emitter);
	}

	virtual Array get_events_array(const String &p_emitter) const override {
		const LocalVector<PropertyInfo> *props = E::get_static_properties();

		Array ret;
		const LocalVector<E> *events = get_events(p_emitter);
		if (events) {
			ret.resize(events->size());
			for (uint32_t i = 0; i < events->size(); i += 1) {
				Dictionary dic;

				for (uint32_t p = 0; p < props->size(); p += 1) {
					Variant v;
					E::get_by_index((void *)(events->ptr() + i), p, v);
					dic[(*props)[p].name] = v;
				}

				ret[i] = dic;
			}
		}
		return ret;
	}

	virtual void flush_events() override {
		for (typename OAHashMap<String, LocalVector<E>>::Iterator it = events_map.iter(); it.valid; it = events_map.next_iter(it)) {
			it.value->clear();
		}
	}

public:
	void add_event(const String &p_emitter, E p_event) {
		LocalVector<E> *emitter = events_map.lookup_ptr(p_emitter);
		ERR_FAIL_COND_MSG(emitter == nullptr, String("The emitter `") + p_emitter + "` for the event `" + E::get_class_static() + "` doesn't exists. No systems are fetching from this emitter.");
		emitter->push_back(p_event);
	}

	const LocalVector<E> *get_events(const String &p_emitter) const {
		return events_map.lookup_ptr(p_emitter);
	}
};
