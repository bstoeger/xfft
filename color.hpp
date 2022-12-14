// SPDX-License-Identifier: GPL-2.0
// This file declares color conversion functions to convert from
// complex and real to rgb.
// The functions are implemented in a separate header file as inline
// functions for speed reasons.

#ifndef COLOR_HPP
#define COLOR_HPP

#include "aligned_buf.hpp"

#include <QRgb>

#include <array>
#include <complex>

class QPixmap;
class QIcon;

enum class ColorType {
	RW = 0,
	HSV = 1,
	HSV_WHITE = 2
};

enum class ColorMode {
	LINEAR = 0,
	ROOT = 1,
	LOG = 2
};

class ColorLookup {
public:
	// The lookup table is made up of values in the 0..255 range.
	// A byte representation (unsigned char) takes up less space than
	// an integer representation (unsigned int), but may be slower to access.
	// Therefore, make the type configurable.
	// In a 1024x1024 pixel test on an i3 the difference was not relevant.
	// This seems to be CPU bound.
	using lookup_t = unsigned char;
};

// Look-up table for converting HSV to RGB
class HSVLookup : private ColorLookup {
	std::array<lookup_t, 6*256*2> data;
	unsigned int lookup(double h, unsigned int v);
public:
	QRgb convert(double h, double v);
	QRgb convert_white(double h, double v);
	HSVLookup();
};

// Look-up table for converting RW to RGB
class RWLookup : private ColorLookup {
	std::array<lookup_t, 3*8*256> data;
	unsigned int lookup(double h, unsigned int v);
public:
	QRgb convert(double h, double v);
	RWLookup();
};

template<typename T>
uint32_t
(*get_color_lookup_function(ColorType type, ColorMode mode))
(T v, double factor1, double factor2);

// Generate colorwheel pixmap of given type and size.
// The unused space is either set to black (alpha = false) or transparent (alpha = true).
QPixmap get_color_pixmap(ColorType type, size_t size, bool alpha);

// Convert real in range 0..1 to grayscale without checking
// Does not do any range checking
unsigned char real_to_grayscale_unchecked(double v);

// This function fills out a color wheel in an n*n uint32_t buffer which can be fed to
// QImage with format QImage::Format_RGB32.
// Pixels outside the wheel are not touched so that the caller can decide what they are.
void make_color_wheel(AlignedBuf<uint32_t> &buf, size_t size, double scale, ColorType type);

// To speed up color conversions in tight loops, we calculate up to two constant
// factors, which depend on the image mode.
// For linear mode, we use a single factor used to multiply intensities: scale / max.
// For root mode, there are two factors, viz. 1 / max and 1 / power.
// For logarithmic mode, there are two factors, viz. 1 / max and log(base).
std::pair<double, double> get_color_factors(ColorMode mode, double max, double scale);

extern HSVLookup hsv_lookup;
extern RWLookup rw_lookup;

#include "color_impl.hpp"

#endif
