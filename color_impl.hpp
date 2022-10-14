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

// Map interval [0..1] to [0..1] applying the given mode (linear, sqrt or log)
// For values > 1.0, return 1.0. Caller must not pass negative values!
template <ColorMode MODE>
inline double apply_color_mode(double v, double factor1, double factor2);

template <>
inline double apply_color_mode<ColorMode::LINEAR>(double v, double factor1, double)
{
	return std::min(1.0, v * factor1);
}

template <>
inline double apply_color_mode<ColorMode::ROOT>(double v, double factor1, double factor2)
{
	return pow(v * factor1, factor2);
}

template <>
inline double apply_color_mode<ColorMode::LOG>(double v, double factor1, double factor2)
{
	return v <= std::numeric_limits<double>::epsilon() ? 0.0 : factor2 / (factor2 - log(v * factor1));
}

template<ColorMode MODE>
inline uint32_t complex_to_hsv(std::complex<double> c, double factor1, double factor2)
{
	double h = (std::arg(c) + std::numbers::pi) / 2.0 / std::numbers::pi;
	double v = apply_color_mode<MODE>(std::abs(c), factor1, factor2);
	return hsv_lookup.convert(h, v);
}

template<ColorMode MODE>
inline uint32_t real_to_hsv(double v, double factor1, double factor2)
{
	if (v < 0.0) {
		v = apply_color_mode<MODE>(-v, factor1, factor2);
		unsigned char vi = static_cast<unsigned char>(v * 255.0);
		return qRgb(vi, 0, 0);
	} else {
		v = apply_color_mode<MODE>(v, factor1, factor2);
		unsigned char vi = static_cast<unsigned char>(v * 255.0);
		return qRgb(0, vi, vi);
	}
}

template<ColorMode MODE>
inline uint32_t complex_to_hsv_white(std::complex<double> c, double factor1, double factor2)
{
	double h = (std::arg(c) + std::numbers::pi) / 2.0 / std::numbers::pi;
	double v = apply_color_mode<MODE>(std::abs(c), factor1, factor2);
	return hsv_lookup.convert_white(h, v);
}

template<ColorMode MODE>
inline uint32_t real_to_hsv_white(double v, double factor1, double factor2)
{
	if (v < 0.0) {
		v = apply_color_mode<MODE>(-v, factor1, factor2);
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
		v = apply_color_mode<MODE>(v, factor1, factor2);
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

template<ColorMode MODE>
inline uint32_t complex_to_rw(std::complex<double> c, double factor1, double factor2)
{
	double h = (std::arg(c) + std::numbers::pi) / 2.0 / std::numbers::pi;
	double v = apply_color_mode<MODE>(std::abs(c), factor1, factor2);
	return rw_lookup.convert(h, v);
}

template<ColorMode MODE>
inline uint32_t real_to_rw(double v, double factor1, double factor2)
{
	if (v < 0.0) {
		v = apply_color_mode<MODE>(-v, factor1, factor2);
		unsigned char vi = static_cast<unsigned char>(v * 255.0);
		return qRgb(vi, 0, 0);
	} else {
		v = apply_color_mode<MODE>(v, factor1, factor2);
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
(*get_color_lookup_function<std::complex<double>>(ColorType type, ColorMode mode))
(std::complex<double> c, double factor1, double factor2)
{
	switch (type) {
	case ColorType::RW:
	default:
		switch (mode) {
			case ColorMode::LINEAR:
			default:
				return &complex_to_rw<ColorMode::LINEAR>;
			case ColorMode::ROOT:
				return &complex_to_rw<ColorMode::ROOT>;
			case ColorMode::LOG:
				return &complex_to_rw<ColorMode::LOG>;
		}
	case ColorType::HSV:
		switch (mode) {
			case ColorMode::LINEAR:
			default:
				return &complex_to_hsv<ColorMode::LINEAR>;
			case ColorMode::ROOT:
				return &complex_to_hsv<ColorMode::ROOT>;
			case ColorMode::LOG:
				return &complex_to_hsv<ColorMode::LOG>;
		}
	case ColorType::HSV_WHITE:
		switch (mode) {
			case ColorMode::LINEAR:
			default:
				return &complex_to_hsv_white<ColorMode::LINEAR>;
			case ColorMode::ROOT:
				return &complex_to_hsv_white<ColorMode::ROOT>;
			case ColorMode::LOG:
				return &complex_to_hsv_white<ColorMode::LOG>;
		}
	}
}

template<>
inline uint32_t
(*get_color_lookup_function<double>(ColorType type, ColorMode mode))
(double c, double factor1, double factor2)
{
	switch (type) {
	case ColorType::RW:
	default:
		switch (mode) {
			case ColorMode::LINEAR:
			default:
				return &real_to_rw<ColorMode::LINEAR>;
			case ColorMode::ROOT:
				return &real_to_rw<ColorMode::ROOT>;
			case ColorMode::LOG:
				return &real_to_rw<ColorMode::LOG>;
		}
	case ColorType::HSV:
		switch (mode) {
			case ColorMode::LINEAR:
			default:
				return &real_to_hsv<ColorMode::LINEAR>;
			case ColorMode::ROOT:
				return &real_to_hsv<ColorMode::ROOT>;
			case ColorMode::LOG:
				return &real_to_hsv<ColorMode::LOG>;
		}
	case ColorType::HSV_WHITE:
		switch (mode) {
			case ColorMode::LINEAR:
			default:
				return &real_to_hsv_white<ColorMode::LINEAR>;
			case ColorMode::ROOT:
				return &real_to_hsv_white<ColorMode::ROOT>;
			case ColorMode::LOG:
				return &real_to_hsv_white<ColorMode::LOG>;
		}
	}
}
