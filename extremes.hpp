// SPDX-License-Identifier: GPL-2.0
// Class for aggregating maximum norm.
// Since everytime before an element is registered it is multiplied by a factor,
// the division is done on registration. That simplifies code.
// To speed up hot loops, the aggregation function is inlined.
//
// Note: this class used to also keep track of minimum and maximum real and imaginary parts.
// Since this functionality was removed, the whole class lost its purpose and should perhaps
// be eliminated.

#ifndef EXTREMES_HPP
#define EXTREMES_HPP

#include <complex>

#include <boost/operators.hpp>

class Extremes : public boost::operators<Extremes> {
	double	max_norm;
public:
	Extremes();
	Extremes(double max_norm);

	// The input parameter is multiplied by factor.
	// The multiplied value is returned for convenience, so that we can save a few lines of code.
	std::complex<double> reg(std::complex<double> &c, double factor);
	double reg(double &r, double factor);

	double get_max_norm() const;

	Extremes &operator+=(const Extremes &);
	Extremes &operator*=(const Extremes &);
};

#include "extremes_impl.hpp"

#endif
