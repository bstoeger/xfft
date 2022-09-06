// SPDX-License-Identifier: GPL-2.0
#include "extremes.hpp"

#include <algorithm>

Extremes::Extremes()
	: max_norm(0.0)
{
}

Extremes::Extremes(double max_norm_)
	: max_norm(max_norm_)
{
}

Extremes &Extremes::operator+=(const Extremes &e2)
{
	max_norm = max_norm + e2.max_norm + 2*sqrt(max_norm * e2.max_norm);
	return *this;
}

Extremes &Extremes::operator*=(const Extremes &e2)
{
	max_norm *= e2.max_norm;
	return *this;
}

double Extremes::get_max_norm() const
{
	return max_norm;
}
