// SPDX-License-Identifier: GPL-2.0
#include "fft_buf.hpp"

#include <fftw3.h>

#include <cassert>
#include <cstring>

FFTBuf::FFTBuf()
	: comp(false)
	, size(0)
	, forwarded_buf(nullptr)
{
}

FFTBuf::FFTBuf(bool comp_, size_t size_)
	: comp(comp_)
	, size(size_)
	, forwarded_buf(nullptr)
{
	size_t n = size * size;
	if (comp)
		complex_data = AlignedBuf<std::complex<double>>(n);
	else
		real_data = AlignedBuf<double>(n);
}

FFTBuf::FFTBuf(FFTBuf &buf)
	: comp(buf.comp)
	, forwarded_buf(&buf)
	, extremes(buf.extremes)
{
}

FFTBuf::FFTBuf(FFTBuf &&buf)
	: comp(buf.comp)
	, size(buf.size)
	, forwarded_buf(buf.forwarded_buf)
	, real_data(std::move(buf.real_data))
	, complex_data(std::move(buf.complex_data))
	, extremes(buf.extremes)
{
	buf.comp = false;
	buf.forwarded_buf = nullptr;
	buf.extremes = Extremes();
}

FFTBuf &FFTBuf::operator=(FFTBuf &&buf)
{
	comp = buf.comp;
	size = buf.size;
	forwarded_buf = buf.forwarded_buf;
	extremes = buf.extremes;
	real_data = std::move(buf.real_data);
	complex_data = std::move(buf.complex_data);

	buf.comp = false;
	buf.forwarded_buf = nullptr;
	buf.extremes = Extremes();
	return *this;
}

FFTBuf::~FFTBuf()
{
}

bool FFTBuf::is_empty() const
{
	if (forwarded_buf)
		return forwarded_buf->is_empty();
	return !real_data && !complex_data;
}

bool FFTBuf::is_complex() const
{
	if (forwarded_buf)
		return forwarded_buf->is_complex();
	return !!complex_data;
}

bool FFTBuf::is_real() const
{
	if (forwarded_buf)
		return forwarded_buf->is_complex();
	return !!real_data;
}

bool FFTBuf::is_forwarded() const
{
	return !!forwarded_buf;
}

size_t FFTBuf::get_size() const
{
	if (forwarded_buf)
		return forwarded_buf->get_size();
	return size;
}

std::complex<double> *FFTBuf::get_complex_data()
{
	if (forwarded_buf)
		return forwarded_buf->get_complex_data();
	assert(complex_data);
	return complex_data.get();
}

double *FFTBuf::get_real_data()
{
	if (forwarded_buf)
		return forwarded_buf->get_real_data();
	assert(real_data);
	return real_data.get();
}

const Extremes &FFTBuf::get_extremes() const
{
	if (forwarded_buf)
		return forwarded_buf->get_extremes();
	return extremes;
}

double FFTBuf::get_max_norm() const
{
	if (forwarded_buf)
		return forwarded_buf->get_max_norm();
	return extremes.get_max_norm();
}

void FFTBuf::set_extremes(const Extremes &extremes_)
{
	if (forwarded_buf)
		return forwarded_buf->set_extremes(extremes_);
	extremes = extremes_;
}

void FFTBuf::clear_data()
{
	if (forwarded_buf) {
		forwarded_buf->clear_data();
		return;
	}

	size_t n = size * size;
	if (complex_data)
		std::fill(complex_data.get(), complex_data.get() + n, 0.0);
	else
		std::fill(real_data.get(), real_data.get() + n, 0.0);
}

void FFTBuf::clear()
{
	if (forwarded_buf) {
		forwarded_buf->clear();
		return;
	}

	clear_data();
	set_extremes(Extremes());
}

template <typename T>
static std::unique_ptr<T []> save_data(const T *src, size_t size)
{
	size_t n = size * size;
	auto dst = std::make_unique_for_overwrite<T []>(n);
	memcpy(&dst[0], src, n*sizeof(T));
	return dst;
}

FFTBuf::SaveState FFTBuf::save() const
{
	if (forwarded_buf)
		return forwarded_buf->save();

	SaveState res;
	if (complex_data)
		res.complex_data = save_data<std::complex<double>>(complex_data.get(), size);
	if (real_data)
		res.real_data = save_data<double>(real_data.get(), size);
	return res;
}

template <typename T>
static void restore_data(T *dst, const T *src, size_t size)
{
	size_t n = size * size;
	memcpy(&*dst, src, n*sizeof(T));
}

void FFTBuf::restore(const SaveState &save)
{
	if (forwarded_buf) {
		forwarded_buf->restore(save);
		return;
	}

	if (save.complex_data) {
		assert(complex_data);
		restore_data<std::complex<double>>(complex_data.get(), &save.complex_data[0], size);
	}
	if (save.real_data) {
		assert(real_data);
		restore_data<double>(real_data.get(), &save.real_data[0], size);
	}
}
