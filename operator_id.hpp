// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_ID_HPP
#define OPERATOR_ID_HPP

// Note: these ids must not change, because they are used in the saved files!
// Every operator must have a public
// inline static constexpr OperatorId id = OperatorId::X;
// member.
enum class OperatorId {
	Conjugate = 20,
	Const = 1,
	Convolution = 2,
	FFT = 3,
	Gauss = 5,
	Inversion = 6,
	Lattice = 7,
	Merge = 8,
	Modulate = 16,
	Mult = 9,
	Pixmap = 10,
	Polygon = 11,
	Pow = 18,
	Powder = 19,
	Split = 12,
	Sum = 13,
	View = 14,
	Wave = 15,
};

#endif
