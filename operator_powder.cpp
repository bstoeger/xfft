// SPDX-License-Identifier: GPL-2.0
#include "operator_powder.hpp"
#include "document.hpp"

bool OperatorPowder::input_connection_changed()
{
	// Empty if the input buffer is empty.
	if (input_connectors[0]->is_empty_buffer())
		return make_output_empty(0);

	FFTBuf &buf = input_connectors[0]->get_buffer();
	if (buf.is_complex())
		return make_output_complex(0);
	else
		return make_output_real(0);
}

// Class that keeps track of pixels with the same distance from the origin
template <size_t N>
class Powderizer {
public:
	std::vector<std::vector<size_t>> batches;
	Powderizer();
};

template <size_t N>
Powderizer<N>::Powderizer()
{
	std::array<std::vector<size_t>, N> b;
	for (size_t y = 0; y < N; ++y) {
		size_t act_y = y < N / 2 ? y : N - y;
		for (size_t x = 0; x < N; ++x) {
			size_t act_x = x < N / 2 ? x : N - x;
			size_t dist = static_cast<size_t>(sqrt(act_x*act_x + act_y*act_y));

			b[dist].push_back(y * N + x);
		}
	}

	for(auto &batch: b) {
		if (!batch.empty())
			batches.push_back(std::move(batch));
	}
}

void OperatorPowder::init()
{
	init_simple(icon);
}

template <typename T, size_t N>
static void powderize(FFTBuf &in_buf, FFTBuf &out_buf)
{
	// Generate on demand
	static Powderizer<N> powderizer;
	T *in = in_buf.get_data<T>();
	T *out = out_buf.get_data<T>();
	double max_norm = 0.0;
	for (const std::vector<size_t> &batch: powderizer.batches) {
		T val = T();
		for (size_t i: batch)
			val += in[i];
		val /= batch.size();
		double norm = std::norm(val);
		if (norm > max_norm)
			max_norm = norm;
		for (size_t i: batch)
			out[i] = val;
	}

	out_buf.set_extremes(Extremes(max_norm));
}

template<size_t N>
void OperatorPowder::calculate()
{
	FFTBuf &buf = input_connectors[0]->get_buffer();
	FFTBuf &out = output_buffers[0];

	if (buf.is_complex())
		powderize<std::complex<double>, N>(buf, out);
	else
		powderize<double, N>(buf, out);
}

void OperatorPowder::execute()
{
	if (input_connectors[0]->is_empty_buffer())
		return; // Empty -> nothing to do

	dispatch_calculate(*this);
}
