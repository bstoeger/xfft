// SPDX-License-Identifier: GPL-2.0
#include "svg_cache.hpp"

SVGCache svg_cache;

const char *SVGCache::names[num_entries] {
	":/icons/move",
	":/icons/arrow_updown",
	":/icons/arrow_leftdown",
};

QSvgRenderer &SVGCache::lookup(Id id,
			       cache_t &cache,
			       const char *suffix)
{
	size_t i = static_cast<size_t>(id);
	if (!cache[i]) {
		QString name = QString(names[i]) + QString(suffix);
		cache[i] = std::make_unique<QSvgRenderer>(name);
	}
	return *cache[i];
}

QSvgRenderer &SVGCache::get(Id id)
{
	return lookup(id, cache, ".svg");
}

QSvgRenderer &SVGCache::get_highlighted(Id id)
{
	return lookup(id, cache_highlighted, "_highlighted.svg");
}
