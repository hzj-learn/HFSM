#pragma once

#if defined _DEBUG && _MSC_VER
	#include <intrin.h>		// __debugbreak()
#endif

//------------------------------------------------------------------------------

#if defined _DEBUG && _MSC_VER
	#define HSFM_BREAK()			__debugbreak()
	#define HSFM_CHECKED(x)			(!!(x) || (HSFM_BREAK(), 0))
#else
	#define HSFM_BREAK()			((void) 0)
	#define HSFM_CHECKED(x)			x
#endif

#ifdef _DEBUG
	#define HSFM_IF_DEBUG(...)		__VA_ARGS__
#else
	#define HSFM_IF_DEBUG(...)
#endif

#ifndef NDEBUG
	#define HSFM_IF_ASSERT(...)		__VA_ARGS__
#else
	#define HSFM_IF_ASSERT(...)
#endif

//------------------------------------------------------------------------------

namespace hfsm {
namespace detail {

////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline
void
fill(T& a, const char value) {
	memset(&a, (int) value, sizeof(a));
}

////////////////////////////////////////////////////////////////////////////////

template <typename T, unsigned TCount>
inline
unsigned
count(const T(&)[TCount]) {
	return TCount;
}

//------------------------------------------------------------------------------

template <typename T, unsigned TCapacity>
inline
const T*
end(const T(& a)[TCapacity]) {
	return &a[TCapacity];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <typename TReturn, typename T, unsigned TCapacity>
inline
const TReturn*
end(const T(& a)[TCapacity]) {
	return reinterpret_cast<const TReturn*>(&a[TCapacity]);
}

////////////////////////////////////////////////////////////////////////////////

template <int t1, int t2>
struct Min {
	enum {
		Value = t1 < t2 ? t1 : t2
	};
};

//------------------------------------------------------------------------------

template <int t1, int t2>
struct Max {
	enum {
		Value = t1 > t2 ? t1 : t2
	};
};

////////////////////////////////////////////////////////////////////////////////

template <unsigned t>
struct PowerOf2 {
	enum {
		Value = !(t & (t - 1))
	};
};

//------------------------------------------------------------------------------

template <unsigned t>
struct BitCount {
	enum {
		Value =
		t		== 0 ?  0 :
		t >>  1 == 0 ?  1 :
		t >>  2 == 0 ?  2 :
		t >>  3 == 0 ?  3 :
		t >>  4 == 0 ?  4 :
		t >>  5 == 0 ?  5 :
		t >>  6 == 0 ?  6 :
		t >>  7 == 0 ?  7 :

		t >>  8 == 0 ?  8 :
		t >>  9 == 0 ?  9 :
		t >> 10 == 0 ? 10 :
		t >> 11 == 0 ? 11 :
		t >> 12 == 0 ? 12 :
		t >> 13 == 0 ? 13 :
		t >> 14 == 0 ? 14 :
		t >> 15 == 0 ? 15 :

		t >> 16 == 0 ? 16 :
		t >> 17 == 0 ? 17 :
		t >> 18 == 0 ? 18 :
		t >> 19 == 0 ? 19 :
		t >> 20 == 0 ? 20 :
		t >> 21 == 0 ? 21 :
		t >> 22 == 0 ? 22 :
		t >> 23 == 0 ? 23 :

		t >> 24 == 0 ? 24 :
		t >> 25 == 0 ? 25 :
		t >> 26 == 0 ? 26 :
		t >> 27 == 0 ? 27 :
		t >> 28 == 0 ? 28 :
		t >> 29 == 0 ? 29 :
		t >> 30 == 0 ? 30 :
		t >> 31 == 0 ? 31 :
					   32
	};
};

//------------------------------------------------------------------------------

template <unsigned t>
struct NextPowerOf2 {
	enum {
		Value = PowerOf2<t>::Value ?
			t : 1 << BitCount<t>::Value
	};
};

////////////////////////////////////////////////////////////////////////////////

}
}
