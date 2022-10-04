// SPDX-License-Identifier: GPL-2.0

#include <numbers>

inline unsigned int HSVLookup::lookup(double h, unsigned int v)
{
	// Check if it faster with or without fmod (more cache needed without?)
	//int x = static_cast<unsigned int>(fmod(h, 1.0)*(6.0*256.0-1.0));
	int x = static_cast<unsigned int>(h*(6.0*256.0-1.0));
	return (data[x] * v) >> 8;
}

inline QRgb HSVLookup::convert(double h, double v)
{
	unsigned int v_int = static_cast<unsigned int>(v * 256.0);
	return qRgb(lookup(h, v_int), lookup(h + 1.0/3.0, v_int), lookup(h + 2.0/3.0, v_int));
}

inline QRgb HSVLookup::convert_white(double h, double v)
{
	if (v > 0.5) {
		v = 1.0 - v;
		h += 0.5;
		unsigned int v_int = static_cast<unsigned int>(v * 2.0 * 256.0);
		return qRgb(255 - lookup(h, v_int), 255 - lookup(h + 1.0/3.0, v_int), 255 - lookup(h + 2.0/3.0, v_int));
	} else {
		unsigned int v_int = static_cast<unsigned int>(v * 2.0 * 256.0);
		return qRgb(lookup(h, v_int), lookup(h + 1.0/3.0, v_int), lookup(h + 2.0/3.0, v_int));
	}
}

inline QRgb RWLookup::convert(double h, double v)
{
	int x = static_cast<unsigned int>(fmod(h, 1.0)*(8.0*256.0-1.0));
	unsigned int v_int = static_cast<unsigned int>(v * 256.0);
	return qRgb(
		(data[x*3+0] * v_int) >> 8,
		(data[x*3+1] * v_int) >> 8,
		(data[x*3+2] * v_int) >> 8);
}

inline uint32_t complex_to_hsv(std::complex<double> c, double factor)
{
	double h = (std::arg(c) + std::numbers::pi) / 2.0 / std::numbers::pi;
	double v = std::abs(c) * factor;
	if (v > 1.0)
		v = 1.0;
	return hsv_lookup.convert(h, v);
}

inline uint32_t real_to_hsv(double v, double factor)
{
	v = v * factor;
	if (v < 0.0) {
		v = -v;
		if (v > 1.0)
			v = 1.0;
		unsigned char vi = static_cast<unsigned char>(v * 255.0);
		return qRgb(vi, 0, 0);
	} else {
		if (v > 1.0)
			v = 1.0;
		unsigned char vi = static_cast<unsigned char>(v * 255.0);
		return qRgb(0, vi, vi);
	}
}

inline uint32_t complex_to_hsv_white(std::complex<double> c, double factor)
{
	double h = (std::arg(c) + std::numbers::pi) / 2.0 / std::numbers::pi;
	double v = std::abs(c) * factor;
	if (v > 1.0)
		v = 1.0;
	return hsv_lookup.convert_white(h, v);
}

inline uint32_t real_to_hsv_white(double v, double factor)
{
	v = v * factor;
	if (v < 0.0) {
		v = -v;
		if (v > 1.0)
			v = 1.0;
		if (v > 0.5) {
			v -= 0.5;
			unsigned char vi = static_cast<unsigned char>(v * 2.0 * 255.0);
			return qRgb(255, vi, vi);
		} else {
			unsigned char vi = static_cast<unsigned char>(v * 2.0 * 255.0);
			return qRgb(vi, 0, 0);
		}
	} else {
		if (v > 1.0)
			v = 1.0;
		if (v > 0.5) {
			v -= 0.5;
			unsigned char vi = static_cast<unsigned char>(v * 2.0 * 255.0);
			return qRgb(vi, 255, 255);
		} else {
			unsigned char vi = static_cast<unsigned char>(v * 2.0 * 255.0);
			return qRgb(0, vi, vi);
		}
	}
}

inline uint32_t complex_to_rw(std::complex<double> c, double factor)
{
	double h = (std::arg(c) + std::numbers::pi) / 2.0 / std::numbers::pi;
	double v = std::abs(c) * factor;
	if (v > 1.0)
		v = 1.0;
	return rw_lookup.convert(h, v);
}

inline uint32_t real_to_rw(double v, double factor)
{
	v = v * factor;
	if (v < 0.0) {
		v = -v;
		if (v > 1.0)
			v = 1.0;
		unsigned char vi = static_cast<unsigned char>(v * 255.0);
		return qRgb(vi, 0, 0);
	} else {
		if (v > 1.0)
			v = 1.0;
		unsigned char vi = static_cast<unsigned char>(v * 255.0);
		return qRgb(vi, vi, vi);
	}
}

inline unsigned char real_to_grayscale_unchecked(double v)
{
	return static_cast<unsigned char>(v * 255.0);
}

template<>
inline uint32_t
(*get_color_lookup_function<std::complex<double>>(ColorType type))
(std::complex<double> c, double factor)
{
	switch (type) {
	case ColorType::RW:
	default:
		return &complex_to_rw;
	case ColorType::HSV:
		return &complex_to_hsv;
	case ColorType::HSV_WHITE:
		return &complex_to_hsv_white;
	}
}

template<>
inline uint32_t
(*get_color_lookup_function<double>(ColorType type))
(double c, double factor)
{
	switch (type) {
	case ColorType::RW:
	default:
		return &real_to_rw;
	case ColorType::HSV:
		return &real_to_hsv;
	case ColorType::HSV_WHITE:
		return &real_to_hsv_white;
	}
}

