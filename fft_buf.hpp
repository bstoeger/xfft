// SPDX-License-Identifier: GPL-2.0
// Describes an FFT-data buffer
// Small wrapper for fftw_malloc() fftw_free()
// Buffer can be real or complex
// Buffer can be forwarded (managed by another buffer)
// to avoid unmodifying copies
// Buffer can be empty

#ifndef FFT_BUF_HPP
#define FFT_BUF_HPP

#include "extremes.hpp"
#include "aligned_buf.hpp"

#include <complex>
#include <memory>

class FFTBuf {
	bool comp;		// Is complex
	size_t size;		// Size
	FFTBuf *forwarded_buf;	// If forwarded, pointer to forwarded buffer,
				// otherwise nullptr
	AlignedBuf<double> real_data;			// If non-forwarded real buffer
	AlignedBuf<std::complex<double>> complex_data;	// If non-forwarded complex buffer
	Extremes extremes;
protected:
	class SaveState {
		friend FFTBuf;
	protected:
		std::unique_ptr<double []> real_data;
		std::unique_ptr<std::complex<double> []> complex_data;
	};
public:
	FFTBuf();			// Default: forwarded real buffer of all zeros
	FFTBuf(bool comp, size_t size);	// Generate managed buffer
	FFTBuf(FFTBuf &);		// Generate forwarded buffer
	FFTBuf(FFTBuf &&);		// Move buffer, delete old
	FFTBuf &operator=(FFTBuf &&);
	~FFTBuf();

	bool is_empty() const;
	bool is_complex() const;
	bool is_real() const;		// Real, but not empty!
	bool is_forwarded() const;
	size_t get_size() const;
	std::complex<double> *get_complex_data();
	double *get_real_data();
	template <typename T>
	T* get_data();

	void clear();			// Set buffer to zero
	void clear_data();		// Set buffer to zero, but keep extremes

	// Returns maximum real and imaginary values
	const Extremes &get_extremes() const;
	double get_max_norm() const;
	void set_extremes(const Extremes &);

	// Save and restore buffer (because fftw overwrites buffers)
	SaveState save() const;
	void restore(const SaveState &);
};

template <>
inline double *FFTBuf::get_data<double>()
{
	return get_real_data();
}

template <>
inline std::complex<double> *FFTBuf::get_data<std::complex<double>>()
{
	return get_complex_data();
}

#endif
