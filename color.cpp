// SPDX-License-Identifier: GPL-2.0
#include "color.hpp"

#include <QIcon>
#include <QPixmap>

HSVLookup hsv_lookup;
RWLookup rw_lookup;

// We start to populate at -1 for the red channel
// We populate double the amount to save one fmod() operation
HSVLookup::HSVLookup()
{
	for (unsigned int i = 0; i < 256; ++i)
		data[i] = 0;
	for (unsigned int i = 256; i < 512; ++i)
		data[i] = i - 256;
	for (unsigned int i = 512; i < 1024; ++i)
		data[i] = 255;
	for (unsigned int i = 1024; i < 1280; ++i)
		data[i] = 255 - (i - 1024);
	for (unsigned int i = 1280; i < 1536; ++i)
		data[i] = 0;
	for (unsigned int i = 0; i < 1536; ++i)
		data[i + 1536] = data[i];
}

// We start to populate at -1
RWLookup::RWLookup()
{
	for (unsigned int i = 0; i < 256; ++i) {
		data[i*3+0] = 255;
		data[i*3+1] = 0;
		data[i*3+2] = i;
	}
	for (unsigned int i = 256; i < 512; ++i) {
		data[i*3+0] = 255 - (i - 256);
		data[i*3+1] = 0;
		data[i*3+2] = 255;
	}
	for (unsigned int i = 512; i < 1024; ++i) {
		data[i*3+0] = (i - 512) / 2;
		data[i*3+1] = (i - 512) / 2;
		data[i*3+2] = 255;
	}
	for (unsigned int i = 1024; i < 1536; ++i) {
		data[i*3+0] = 255 - ((i - 1024) / 2);
		data[i*3+1] = 255;
		data[i*3+2] = 255 - ((i - 1024) / 2);
	}
	for (unsigned int i = 1536; i < 1792; ++i) {
		data[i*3+0] = i - 1536;
		data[i*3+1] = 255;
		data[i*3+2] = 0;
	}
	for (unsigned int i = 1792; i < 2048; ++i) {
		data[i*3+0] = 255;
		data[i*3+1] = 255 - (i - 1792);
		data[i*3+2] = 0;
	}
}

QPixmap get_color_pixmap(ColorType type, size_t size, bool alpha)
{
	AlignedBuf<uint32_t> buf(size * size);
	uint32_t *out = buf.get();
	QRgb background = alpha ? 0 : 0xff000000;
	for (size_t i = 0; i < size * size; ++i) {
		out[i] = background;
	}

	make_color_wheel(buf, size, 1.05, type);

	auto format = alpha ? QImage::Format_ARGB32 : QImage::Format_RGB32;
	return QPixmap::fromImage(
		QImage( reinterpret_cast<unsigned char *>(out),
		  size, size, format )
	);
}

void make_color_wheel(AlignedBuf<uint32_t> &buf, size_t size, double scale, ColorType type)
{
	uint32_t *out = buf.get();
	auto [factor1, factor2] = get_color_factors(ColorMode::LINEAR, 1.0, 1.0);
	auto fun = get_color_lookup_function<std::complex<double>>(type, ColorMode::LINEAR);

	double step = 2.0 * scale / size;
	std::complex<double> act(-scale, scale);
	for (size_t i = 0; i < size; ++i) {
		for (size_t j = 0; j < size; ++j) {
			act += step;
			if (std::norm(act) <= 1.0)
				*out = fun(act, factor1, factor2);
			++out;
		}
		act += std::complex<double>(0.0, -step);
		act.real(-scale);
	}
}

// To speed up color conversions in tight loops, we calculate up to two constant
// factors, which depend on the image mode.
// For linear and sqrt modes, we use a single factor used to multiply intensities: scale / max.
// For logarithmic mode, there are two factors, viz. 1/max and log(base).
std::pair<double, double> get_color_factors(ColorMode mode, double max, double scale)
{
	switch (mode) {
	case ColorMode::LINEAR:
	default:
		return std::make_pair(scale / max, 0.0);
	case ColorMode::ROOT:
		return std::make_pair(1 / max, 1 / scale);
	case ColorMode::LOG:
		return std::make_pair(1.0 / max, log(scale));
	}
}
