// SPDX-License-Identifier: GPL-2.0
#include "operator_modulate.hpp"
#include "document.hpp"

void OperatorModulate::init()
{
	init_simple(icon);
}

bool OperatorModulate::input_connection_changed()
{
	if (input_connectors[0]->is_empty_buffer())
		return make_output_empty(0); // Amplitudes empty

	if (input_connectors[1]->is_empty_buffer())
		return make_output_forwarded(0, input_connectors[0]->get_buffer());

	FFTBuf &in_buf1 = input_connectors[0]->get_buffer();
	if (in_buf1.is_complex())
		return make_output_complex(0);
	else
		return make_output_real(0);
}

template<size_t N>
static int mod_coord(int x, double mod)
{
	x += static_cast<int>(mod);
	x %= N;
	return x < 0 ? x+N : x;
}

template<size_t N, typename T>
static void calculate_mod_complex(FFTBuf &basic_buf, FFTBuf &out_buf, FFTBuf &mod_buf)
{
	T *data = basic_buf.get_data<T>();
	T *out = out_buf.get_data<T>();
	std::complex<double> *mod = mod_buf.get_complex_data();
	for (int y = 0; y < static_cast<int>(N); ++y) {
		for (int x = 0; x < static_cast<int>(N); ++x) {
			std::complex<double> delta = *mod++;
			int x_fetch = mod_coord<N>(x, delta.real());
			int y_fetch = mod_coord<N>(y, delta.imag());
			*out++ = data[x_fetch + y_fetch * N];
		}
	}
}

template<size_t N, typename T>
static void calculate_mod_real(FFTBuf &basic_buf, FFTBuf &out_buf, FFTBuf &mod_buf)
{
	T *data = basic_buf.get_data<T>();
	T *out = out_buf.get_data<T>();
	double *mod = mod_buf.get_data<double>();
	for (int y = 0; y < static_cast<int>(N); ++y) {
		for (int x = 0; x < static_cast<int>(N); ++x) {
			int x_fetch = mod_coord<N>(x, *mod++);
			*out++ = data[x_fetch];
		}
		data += N;
	}
}

template<size_t N>
void OperatorModulate::calculate()
{
	FFTBuf &basic_buf = input_connectors[0]->get_buffer();
	FFTBuf &mod_buf = input_connectors[1]->get_buffer();
	FFTBuf &out_buf = output_buffers[0];

	if (mod_buf.is_complex()) {
		if (basic_buf.is_complex())
			calculate_mod_complex<N,std::complex<double>>(basic_buf, out_buf, mod_buf);
		else
			calculate_mod_complex<N,double>(basic_buf, out_buf, mod_buf);
	} else {
		if (basic_buf.is_complex())
			calculate_mod_real<N,std::complex<double>>(basic_buf, out_buf, mod_buf);
		else
			calculate_mod_real<N,double>(basic_buf, out_buf, mod_buf);
	}

	out_buf.set_extremes(basic_buf.get_extremes());
}

void OperatorModulate::execute()
{
	if (input_connectors[0]->is_empty_buffer() ||
	   input_connectors[1]->is_empty_buffer())
		return; // Either buffer is empty -> nothing to do

	dispatch_calculate(*this);
}
