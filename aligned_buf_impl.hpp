// SPDX-License-Identifier: GPL-2.0
#include <cstdlib>

template <typename T>
T* assume_aligned(T *d)
{
	return static_cast<T*>(__builtin_assume_aligned(d, AlignedBuf<T>::align));
}

template <typename T>
AlignedBuf<T>::AlignedBuf()
	: buf(nullptr)
{
}

template<typename T>
AlignedBuf<T>::AlignedBuf(size_t n)
// Windows doesn't have the standard(!) aligned_alloc() function.
// Instead it provides an _aligned_malloc() with reversed arguments.
#ifdef _WIN32
	: buf(reinterpret_cast<T*>(_aligned_malloc(n * sizeof(T), align)))
#else
	: buf(reinterpret_cast<T*>(std::aligned_alloc(align, n * sizeof(T))))
#endif
{
}

template<typename T>
AlignedBuf<T>::AlignedBuf(AlignedBuf &&b)
	: buf(b.buf)
{
	b.buf = nullptr;
}

template<typename T>
AlignedBuf<T> &AlignedBuf<T>::operator=(AlignedBuf &&b)
{
	if (buf)
#ifdef _WIN32
		_aligned_free(buf);
#else
		free(buf);
#endif
	buf = b.buf;
	b.buf = nullptr;
	return *this;
}

template<typename T>
AlignedBuf<T>::~AlignedBuf()
{
	if (buf)
#ifdef _WIN32
		_aligned_free(buf);
#else
		free(buf);
#endif
}

template<typename T>
bool AlignedBuf<T>::operator!() const
{
	return !buf;
}

template<typename T>
AlignedBuf<T>::operator bool() const
{
	return !!buf;
}

template<typename T>
T *AlignedBuf<T>::get()
{
	return buf;
}

template<typename T>
const T *AlignedBuf<T>::get() const
{
	return buf;
}
