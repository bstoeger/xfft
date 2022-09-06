// SPDX-License-Identifier: GPL-2.0
inline std::complex<double> Extremes::reg(std::complex<double> &c, double factor)
{
	c *= factor;
	double norm = std::norm(c);
	if (norm > max_norm)
		max_norm = norm;
	return c;
}

inline double Extremes::reg(double &r, double factor)
{
	r *= factor;
	double norm = r*r;
	if (norm > max_norm)
		max_norm = norm;
	return r;
}
