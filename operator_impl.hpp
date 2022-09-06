// SPDX-License-Identifier: GPL-2.0
#include <stdexcept>
#include <string>

template <typename OperatorType>
void Operator::dispatch_calculate(OperatorType &op)
{
	size_t fft_size = get_fft_size();
	switch (fft_size) {
	case 128:
		return op.template calculate<128>();
	case 256:
		return op.template calculate<256>();
	case 512:
		return op.template calculate<512>();
	case 1024:
		return op.template calculate<1024>();
	default:
		throw std::runtime_error("Unsupported FFT size: " + std::to_string(fft_size));
	}
}
