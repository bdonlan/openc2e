#ifndef REUSEALLOC_H
#define REUSEALLOC_H

#include <boost/pool/pool.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <new>


template <typename element_type>
struct ReusePoolImpl_Wrap {
		typedef ReusePoolImpl_Wrap<element_type> self;
		self *next, *prev;
	protected:
		bool is_free;
		boost::aligned_storage<
			sizeof(element_type),
			boost::alignment_of<element_type>::value
		> storage;

	public:
		element_type *obj() {
			return reinterpret_cast<element_type *>(storage.address());
		}

		const element_type *obj() const {
			return reinterpret_cast<const element_type *>(storage.address());
		}

		bool inuse() const {
			return !is_free;
		}
		
		void activate() {
			assert(!inuse());
			is_free = false;
		}

		void destruct() {
			if (!inuse())
				return;
			obj()->~element_type();
			is_free = true;
		}

		~ReusePoolImpl_Wrap() {
			destruct();
		}

		ReusePoolImpl_Wrap() : next(NULL), prev(NULL), is_free(true) { }
};

template <typename T>
class ReusePool {
	protected:
		typedef T element_type;
		typedef ReusePoolImpl_Wrap<element_type> impl_element;

		boost::pool<> alloc;
		impl_element *alloc_chain;

		element_type *malloc() {
			void *memblock = alloc.malloc();
			impl_element *e = reinterpret_cast<impl_element *>(memblock);
			new(e) impl_element();
			e->prev = NULL;
			e->next = alloc_chain;
			alloc_chain = e;
			if (e->next) {
				assert(!e->next->prev);
				e->next->prev = e;
			}
			return e->obj();
		}

		void free(element_type *e) {
			static const impl_element cmp_elem;
			size_t offset = 
				reinterpret_cast<const unsigned char *>(cmp_elem.obj())
			   		- reinterpret_cast<const unsigned char *>(&cmp_elem);
			unsigned char *p = reinterpret_cast<unsigned char *>(e);
			p -= offset;
			impl_element *iep = reinterpret_cast<impl_element *>(p);
			iep->destruct();

			if (iep->next)
				iep->next->prev = iep->prev;
			if (iep->prev)
				iep->prev->next = iep->next;
			if (alloc_chain == iep) {
				assert(!iep->prev);
				alloc_chain = iep->next;
			}
			alloc.free((void *)iep);
		}
	public:
		void destroy(element_type *e) {
			free(e);
		}

    // Include automatically-generated file for family of template construct()
    //  functions
	//
	//  This is probably a boost implementation detail - can we rely on this
	//  for future boost versions?
#ifndef BOOST_NO_TEMPLATE_CV_REF_OVERLOADS
#   include <boost/pool/detail/pool_construct.inc>
#else
#   include <boost/pool/detail/pool_construct_simple.inc>
#endif

		~ReusePool() {
			for (impl_element *iep = alloc_chain; iep; iep = iep->next) {
				iep->destruct();
			}
		}

		ReusePool() : alloc(sizeof(impl_element)), alloc_chain(NULL) {
		}
};

#endif
