#ifndef GMS_FASTPODALLOCATOR_H_
#define GMS_FASTPODALLOCATOR_H_

#include <boost/pool/pool.hpp>
#include <cassert>

namespace gms {
	/// Make the allocation/deallocation faster for small NON-POLYMORPHIC objects.
	///
	/// Usage:
	///
	/// class A { };
	/// =>
	/// class A : public gms::FastPODAllocator<A> { };
	///
	/// Note that FastPODAllocator<> is not thread safe.
	/// Use as you wish.
	/// @see http://www.boost.org/
	/// @author Hannu Kankaanp‰‰ (hkankaan@cc.hut.fi)
	template <class T>
	class FastPODAllocator {
	public:
		static void* operator new(size_t size) {
			//just in case someone tries to misuse this class, at least a run-time error is shown
			assert(size == sizeof(T));
			return s_memPool.malloc();
		}

		static void operator delete(void* deletable, size_t size) {
			//don't delete null pointers
			if (deletable)
				s_memPool.free(deletable);
		}

	protected:
		~FastPODAllocator() {}

	private:
		//each FastAllocator specialization has it's own memory pool
		static boost::pool<> s_memPool;
	};

	//the static variable s_memPool is defined here. It's constructor is passed the object size.
	template <class T>
	boost::pool<> FastPODAllocator<T>::s_memPool(sizeof(T));
}

#endif