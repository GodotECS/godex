
/** @author AndreaCatania */

#pragma once

#include "../databags/databag.h"
#include "../iterators/query.h"
#include <type_traits>

// TODO put all this into a CPP or a namespace?

namespace SystemBuilder {

/// Used to fetch the system function arguments:
template <class... Cs>
struct InfoConstructor {
	InfoConstructor(SystemExeInfo &r_info) {}
};

/// Fetches the component stoages: `Storage`s.
/// The component storage can be taken only as mutable (non const) pointer.
/// ```
/// void test_func(Storage<Component> *p_component_storage){}
/// ```
template <class C, class... Cs>
struct InfoConstructor<Storage<C> *, Cs...> : InfoConstructor<Cs...> {
	InfoConstructor(SystemExeInfo &r_info) :
			InfoConstructor<Cs...>(r_info) {
		r_info.mutable_components_storage.push_back(C::get_component_id());
	}
};

/// Fetches the argument `Query`.
/// The query is supposed to be a mutable query reference:
/// ```
/// void test_func(Query<const Component> &query){}
/// ```
template <class... Qcs, class... Cs>
struct InfoConstructor<Query<Qcs...> &, Cs...> : InfoConstructor<Cs...> {
	InfoConstructor(SystemExeInfo &r_info) :
			InfoConstructor<Cs...>(r_info) {
		Query<Qcs...>::get_components(r_info.mutable_components, r_info.immutable_components);
	}
};

/// Fetches the `Databag`.
/// The `Databag` can be taken as mutable or immutable pointer.
/// ```
/// void test_func(const FrameTimeDatabag *p_frame_time){}
/// ```
template <class D, class... Cs>
struct InfoConstructor<D *, Cs...> : InfoConstructor<Cs...> {
	InfoConstructor(SystemExeInfo &r_info) :
			InfoConstructor<Cs...>(r_info) {
		// Databag
		if (std::is_const<D>()) {
			r_info.immutable_databags.push_back(D::get_databag_id());
		} else {
			r_info.mutable_databags.push_back(D::get_databag_id());
		}
	}
};

/// Creates a SystemExeInfo, extracting the information from a system function.
template <class... RCs>
void get_system_info_from_function(SystemExeInfo &r_info, void (*system_func)(RCs...)) {
	InfoConstructor<RCs...> a(r_info);
}

/// `DataFetcher` is used to fetch the data from the world and provide it to the
/// `System`.
template <class C>
struct DataFetcher {};

/// Storage
template <class C>
struct DataFetcher<Storage<C> *> {
	Storage<C> *inner;

	DataFetcher(World *p_world) :
			inner(p_world->get_storage<C>()) {}
};

/// Query
template <class... Cs>
struct DataFetcher<Query<Cs...> &> {
	Query<Cs...> inner;

	DataFetcher(World *p_world) :
			inner(p_world) {}
};

/// Databag
template <class D>
struct DataFetcher<D *> {
	D *inner;

	DataFetcher(World *p_world) {
		inner = p_world->get_databag<D>();
	}
};

#define OBTAIN(name, T, world) auto name = DataFetcher<T>(world);

// ~~~~ system_exec_func definition ~~~~ //

template <class A>
void system_exec_func(World *p_world, void (*p_system)(A)) {
	OBTAIN(a, A, p_world);

	p_system(
			a.inner);
}

template <class A, class B>
void system_exec_func(World *p_world, void (*p_system)(A, B)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);

	p_system(
			a.inner,
			b.inner);
}

template <class A, class B, class C>
void system_exec_func(World *p_world, void (*p_system)(A, B, C)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner);
}

template <class A, class B, class C, class D>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner);
}

template <class A, class B, class C, class D, class E>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner);
}

template <class A, class B, class C, class D, class E, class F>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner);
}

template <class A, class B, class C, class D, class E, class F, class G>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);
	OBTAIN(p, P, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner,
			p.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);
	OBTAIN(p, P, p_world);
	OBTAIN(q, Q, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner,
			p.inner,
			q.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);
	OBTAIN(p, P, p_world);
	OBTAIN(q, Q, p_world);
	OBTAIN(r, R, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner,
			p.inner,
			q.inner,
			r.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);
	OBTAIN(p, P, p_world);
	OBTAIN(q, Q, p_world);
	OBTAIN(r, R, p_world);
	OBTAIN(s, S, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner,
			p.inner,
			q.inner,
			r.inner,
			s.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class T>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);
	OBTAIN(p, P, p_world);
	OBTAIN(q, Q, p_world);
	OBTAIN(r, R, p_world);
	OBTAIN(s, S, p_world);
	OBTAIN(t, T, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner,
			p.inner,
			q.inner,
			r.inner,
			s.inner,
			t.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class T, class U>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);
	OBTAIN(p, P, p_world);
	OBTAIN(q, Q, p_world);
	OBTAIN(r, R, p_world);
	OBTAIN(s, S, p_world);
	OBTAIN(t, T, p_world);
	OBTAIN(u, U, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner,
			p.inner,
			q.inner,
			r.inner,
			s.inner,
			t.inner,
			u.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class T, class U, class V>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);
	OBTAIN(p, P, p_world);
	OBTAIN(q, Q, p_world);
	OBTAIN(r, R, p_world);
	OBTAIN(s, S, p_world);
	OBTAIN(t, T, p_world);
	OBTAIN(u, U, p_world);
	OBTAIN(v, V, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner,
			p.inner,
			q.inner,
			r.inner,
			s.inner,
			t.inner,
			u.inner,
			v.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class T, class U, class V, class W>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);
	OBTAIN(p, P, p_world);
	OBTAIN(q, Q, p_world);
	OBTAIN(r, R, p_world);
	OBTAIN(s, S, p_world);
	OBTAIN(t, T, p_world);
	OBTAIN(u, U, p_world);
	OBTAIN(v, V, p_world);
	OBTAIN(w, W, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner,
			p.inner,
			q.inner,
			r.inner,
			s.inner,
			t.inner,
			u.inner,
			v.inner,
			w.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class T, class U, class V, class W, class X>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);
	OBTAIN(p, P, p_world);
	OBTAIN(q, Q, p_world);
	OBTAIN(r, R, p_world);
	OBTAIN(s, S, p_world);
	OBTAIN(t, T, p_world);
	OBTAIN(u, U, p_world);
	OBTAIN(v, V, p_world);
	OBTAIN(w, W, p_world);
	OBTAIN(x, X, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner,
			p.inner,
			q.inner,
			r.inner,
			s.inner,
			t.inner,
			u.inner,
			v.inner,
			w.inner,
			x.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class T, class U, class V, class W, class X, class Y>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);
	OBTAIN(p, P, p_world);
	OBTAIN(q, Q, p_world);
	OBTAIN(r, R, p_world);
	OBTAIN(s, S, p_world);
	OBTAIN(t, T, p_world);
	OBTAIN(u, U, p_world);
	OBTAIN(v, V, p_world);
	OBTAIN(w, W, p_world);
	OBTAIN(x, X, p_world);
	OBTAIN(y, Y, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner,
			p.inner,
			q.inner,
			r.inner,
			s.inner,
			t.inner,
			u.inner,
			v.inner,
			w.inner,
			x.inner,
			y.inner);
}

template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class T, class U, class V, class W, class X, class Y, class Z>
void system_exec_func(World *p_world, void (*p_system)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z)) {
	OBTAIN(a, A, p_world);
	OBTAIN(b, B, p_world);
	OBTAIN(c, C, p_world);
	OBTAIN(d, D, p_world);
	OBTAIN(e, E, p_world);
	OBTAIN(f, F, p_world);
	OBTAIN(g, G, p_world);
	OBTAIN(h, H, p_world);
	OBTAIN(i, I, p_world);
	OBTAIN(j, J, p_world);
	OBTAIN(k, K, p_world);
	OBTAIN(l, L, p_world);
	OBTAIN(m, M, p_world);
	OBTAIN(n, N, p_world);
	OBTAIN(o, O, p_world);
	OBTAIN(p, P, p_world);
	OBTAIN(q, Q, p_world);
	OBTAIN(r, R, p_world);
	OBTAIN(s, S, p_world);
	OBTAIN(t, T, p_world);
	OBTAIN(u, U, p_world);
	OBTAIN(v, V, p_world);
	OBTAIN(w, W, p_world);
	OBTAIN(x, X, p_world);
	OBTAIN(y, Y, p_world);
	OBTAIN(z, Z, p_world);

	p_system(
			a.inner,
			b.inner,
			c.inner,
			d.inner,
			e.inner,
			f.inner,
			g.inner,
			h.inner,
			i.inner,
			j.inner,
			k.inner,
			l.inner,
			m.inner,
			n.inner,
			o.inner,
			p.inner,
			q.inner,
			r.inner,
			s.inner,
			t.inner,
			u.inner,
			v.inner,
			w.inner,
			x.inner,
			y.inner,
			z.inner);
}

#undef OBTAIN

} // namespace SystemBuilder
