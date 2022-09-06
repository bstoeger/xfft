// SPDX-License-Identifier: GPL-2.0
// Class that caches SVG renderers for icons.
// Each SVG exists in a normal and a highlighted state.
// Accessed via a the global variable svg_cache.

#ifndef SVG_CACHE_HPP
#define SVG_CACHE_HPP

#include <vector>
#include <memory>
#include <array>
#include <QSvgRenderer>

class SVGCache {
public:
	enum class Id : size_t {
		move,
		arrow_updown,
		arrow_leftdown,
		max
	};
private:
	static constexpr size_t num_entries = static_cast<size_t>(Id::max);
	static const char *names[num_entries];

	using cache_t = std::array<std::unique_ptr<QSvgRenderer>, num_entries>;
	cache_t cache;
	cache_t cache_highlighted;

	QSvgRenderer &lookup(Id id,
			     cache_t &cache,
			     const char *suffix);
public:
	QSvgRenderer &get(Id);
	QSvgRenderer &get_highlighted(Id);
};

#ifndef SVG_CACHE_C
extern
#endif
SVGCache svg_cache;

#endif
